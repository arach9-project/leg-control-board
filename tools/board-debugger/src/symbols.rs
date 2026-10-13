use std::{fs, path::Path};

use object::{Object, ObjectSymbol, SymbolKind};

use crate::dwarf;

#[derive(Debug, Clone, PartialEq)]
pub enum DebugType {
    Unknown,
    F32,
    U8,
    U16,
    U32,
    I32,
    Bool,
    Struct,
    Enum,
}

#[derive(Debug, Clone)]
pub struct DebugVariable {
    pub name: String,
    pub address: u64,
    pub ty: DebugType,
    pub children: Vec<DebugVariable>,
}

pub struct SymbolTable {
    symbols: Vec<DebugVariable>,
}

impl SymbolTable {
    pub fn symbols(&self) -> Vec<&DebugVariable> {
        let mut symbols = self.symbols.iter().collect::<Vec<_>>();

        symbols.sort_by_key(|symbol| symbol.address);

        symbols
    }

    pub fn load(path: impl AsRef<Path>) -> anyhow::Result<Self> {
        let data = fs::read(path)?;
        let file = object::File::parse(&*data)?;

        let mut symbols = Vec::new();

        for symbol in file.symbols() {
            let Ok(name) = symbol.name() else {
                continue;
            };

            if symbol.kind() != SymbolKind::Data {
                continue;
            }

            let address = symbol.address();

            // Keep only RAM objects for now.
            if !(0x2000_0000..0x3000_0000).contains(&address) {
                continue;
            }

            symbols.push(DebugVariable {
                name: name.to_owned(),
                address,
                ty: DebugType::Unknown,
                children: Vec::new(),
            });
        }

        dwarf::populate_children(&file, symbols.as_mut_slice())?;

        Ok(Self { symbols })
    }

    pub fn find(&self, path: &str) -> Option<&DebugVariable> {
        let mut parts = path.split('.');

        let root_name = parts.next()?;

        let mut variable = self
            .symbols
            .iter()
            .find(|variable| variable.name == root_name)?;

        for part in parts {
            variable = variable.children.iter().find(|child| child.name == part)?;
        }

        Some(variable)
    }
}
