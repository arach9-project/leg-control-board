use std::borrow::Cow;

use anyhow::{Result, anyhow};
use gimli::{
    AttributeValue, Dwarf, EndianSlice, LittleEndian, Reader, ReaderOffset, SectionId, Unit,
    UnitOffset,
};
use object::{Object, ObjectSection};

use crate::symbols::{DebugType, DebugVariable};

type ReaderType<'a> = EndianSlice<'a, LittleEndian>;

pub fn populate_children(file: &object::File<'_>, variables: &mut [DebugVariable]) -> Result<()> {
    let dwarf_cow = Dwarf::load(|section_id: SectionId| {
        let data = file
            .section_by_name(section_id.name())
            .and_then(|section| section.uncompressed_data().ok())
            .unwrap_or(Cow::Borrowed(&[]));

        Ok::<_, gimli::Error>(data)
    })?;

    let dwarf = dwarf_cow.borrow(|section| EndianSlice::new(section.as_ref(), LittleEndian));

    let mut units = dwarf.units();

    while let Some(header) = units.next()? {
        let unit = dwarf.unit(header)?;

        for variable in variables.iter_mut() {
            populate_variable(&dwarf, &unit, variable)?;
        }
    }

    Ok(())
}

fn populate_variable<R>(
    dwarf: &Dwarf<R>,
    unit: &Unit<R>,
    variable: &mut DebugVariable,
) -> Result<()>
where
    R: Reader<Offset = usize>,
{
    let mut entries = unit.entries();

    while let Some((_, entry)) = entries.next_dfs()? {
        if entry.tag() != gimli::DW_TAG_variable {
            continue;
        }

        let Some(name) = entry_name(dwarf, unit, entry)? else {
            continue;
        };

        if name != variable.name {
            continue;
        }

        let Some(type_offset) = type_offset(entry)? else {
            continue;
        };

        variable.children = parse_type_children(dwarf, unit, type_offset, variable.address)?;

        variable.ty = resolve_debug_type(dwarf, unit, type_offset)?;

        return Ok(());
    }

    Ok(())
}

fn parse_type_children<R>(
    dwarf: &Dwarf<R>,
    unit: &Unit<R>,
    type_offset: UnitOffset<R::Offset>,
    base_address: u64,
) -> Result<Vec<DebugVariable>>
where
    R: Reader<Offset = usize>,
{
    let type_offset = resolve_type_offset(dwarf, unit, type_offset)?;

    let mut tree = unit.entries_tree(Some(type_offset))?;
    let root = tree.root()?;
    let entry = root.entry();

    match entry.tag() {
        gimli::DW_TAG_structure_type | gimli::DW_TAG_class_type | gimli::DW_TAG_union_type => {
            parse_struct_members(dwarf, unit, root, base_address)
        }

        _ => Ok(Vec::new()),
    }
}

fn parse_struct_members<R>(
    dwarf: &Dwarf<R>,
    unit: &Unit<R>,
    root: gimli::EntriesTreeNode<R>,
    base_address: u64,
) -> Result<Vec<DebugVariable>>
where
    R: Reader<Offset = usize>,
{
    let mut children = root.children();
    let mut result = Vec::new();

    while let Some(child) = children.next()? {
        let entry = child.entry();

        if entry.tag() != gimli::DW_TAG_member {
            continue;
        }

        let Some(name) = entry_name(dwarf, unit, entry)? else {
            continue;
        };

        let offset = member_offset(entry)?.unwrap_or(0);
        let address = base_address + offset;

        let Some(member_type_offset) = type_offset(entry)? else {
            result.push(DebugVariable {
                name,
                address,
                ty: DebugType::Unknown,
                children: Vec::new(),
            });

            continue;
        };

        let ty = resolve_debug_type(dwarf, unit, member_type_offset)?;

        let nested_children = parse_type_children(dwarf, unit, member_type_offset, address)?;

        result.push(DebugVariable {
            name,
            address,
            ty,
            children: nested_children,
        });
    }

    Ok(result)
}

fn resolve_type_offset<R>(
    dwarf: &Dwarf<R>,
    unit: &Unit<R>,
    mut offset: UnitOffset<R::Offset>,
) -> Result<UnitOffset<R::Offset>>
where
    R: Reader<Offset = usize>,
{
    loop {
        let entry = unit.entry(offset)?;

        match entry.tag() {
            gimli::DW_TAG_typedef
            | gimli::DW_TAG_const_type
            | gimli::DW_TAG_volatile_type
            | gimli::DW_TAG_restrict_type => {
                let Some(next) = type_offset(&entry)? else {
                    return Ok(offset);
                };

                offset = next;
            }

            _ => return Ok(offset),
        }
    }
}

fn resolve_debug_type<R>(
    dwarf: &Dwarf<R>,
    unit: &Unit<R>,
    type_offset_value: UnitOffset<R::Offset>,
) -> Result<DebugType>
where
    R: Reader<Offset = usize>,
{
    let offset = resolve_type_offset(dwarf, unit, type_offset_value)?;

    let entry = unit.entry(offset)?;

    match entry.tag() {
        gimli::DW_TAG_structure_type | gimli::DW_TAG_class_type | gimli::DW_TAG_union_type => {
            Ok(DebugType::Struct)
        }

        gimli::DW_TAG_enumeration_type => Ok(DebugType::Enum),

        gimli::DW_TAG_base_type => {
            let name = entry_name(dwarf, unit, &entry)?.unwrap_or_default();

            Ok(match name.as_str() {
                "float" => DebugType::F32,

                "unsigned char" | "uint8_t" | "unsigned __int8" => DebugType::U8,

                "unsigned short" | "uint16_t" => DebugType::U16,

                "unsigned int" | "unsigned long" | "uint32_t" => DebugType::U32,

                "int" | "signed int" | "int32_t" => DebugType::I32,

                "bool" => DebugType::Bool,

                _ => DebugType::Unknown,
            })
        }

        _ => Ok(DebugType::Unknown),
    }
}

fn entry_name<R>(
    dwarf: &Dwarf<R>,
    unit: &Unit<R>,
    entry: &gimli::DebuggingInformationEntry<R>,
) -> Result<Option<String>>
where
    R: Reader<Offset = usize>,
{
    let Some(attr) = entry.attr(gimli::DW_AT_name)? else {
        return Ok(None);
    };

    let reader = dwarf.attr_string(unit, attr.value())?;

    Ok(Some(reader.to_string_lossy()?.into_owned()))
}

fn type_offset<R>(
    entry: &gimli::DebuggingInformationEntry<R>,
) -> Result<Option<UnitOffset<R::Offset>>>
where
    R: Reader<Offset = usize>,
{
    let Some(attr) = entry.attr(gimli::DW_AT_type)? else {
        return Ok(None);
    };

    match attr.value() {
        AttributeValue::UnitRef(offset) => Ok(Some(offset)),
        _ => Ok(None),
    }
}

fn member_offset<R>(entry: &gimli::DebuggingInformationEntry<R>) -> Result<Option<u64>>
where
    R: Reader<Offset = usize>,
{
    let Some(attr) = entry.attr(gimli::DW_AT_data_member_location)? else {
        return Ok(None);
    };

    match attr.value() {
        AttributeValue::Udata(value) => Ok(Some(value)),

        AttributeValue::Data1(value) => Ok(Some(value as u64)),

        AttributeValue::Data2(value) => Ok(Some(value as u64)),

        AttributeValue::Data4(value) => Ok(Some(value as u64)),

        AttributeValue::Data8(value) => Ok(Some(value)),

        _ => Err(anyhow!("Unsupported DW_AT_data_member_location")),
    }
}
