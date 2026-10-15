use eframe::egui::Color32;

use crate::{
    acquisition::MemoryType, app::DebuggerApp, symbols::DebugVariable, types::WatchedVariable,
};

#[derive(Debug, Clone)]
pub struct PlotVariable {
    pub symbol: String,
    pub label: String,
    pub color: Color32,
}

pub fn show(app: &mut DebuggerApp, ui: &mut eframe::egui::Ui) {
    eframe::egui::Panel::right("symbols_panel")
        .resizable(true)
        .default_size(260.0)
        .show(ui, |ui| {
            ui.heading("Symbols");

            ui.add(
                eframe::egui::TextEdit::singleline(&mut app.symbol_search)
                    .hint_text("Search symbols..."),
            );

            ui.separator();

            // Clone the root variables so we don't keep an immutable borrow
            // of app.symbol_table while modifying app below.
            let variables = app
                .symbol_table
                .as_ref()
                .map(|table| table.symbols().into_iter().cloned().collect::<Vec<_>>());

            let Some(variables) = variables else {
                ui.label("No ELF symbol table loaded.");
                return;
            };

            let search = app.symbol_search.to_lowercase();

            eframe::egui::ScrollArea::vertical().show(ui, |ui| {
                for variable in &variables {
                    show_variable(app, ui, variable, "", &search);
                }
            });
        });
}

fn show_variable(
    app: &mut DebuggerApp,
    ui: &mut eframe::egui::Ui,
    variable: &DebugVariable,
    parent_path: &str,
    search: &str,
) {
    let full_name = if parent_path.is_empty() {
        variable.name.clone()
    } else {
        format!("{parent_path}.{}", variable.name)
    };

    // Don't render this branch if neither it nor any descendant matches.
    if !search.is_empty() && !matches_search(variable, &full_name, search) {
        return;
    }

    if !variable.children.is_empty() {
        eframe::egui::CollapsingHeader::new(&variable.name)
            .id_salt(&full_name)
            .show(ui, |ui| {
                for child in &variable.children {
                    show_variable(app, ui, child, &full_name, search);
                }
            });

        return;
    }

    // Leaf variable: selectable/watchable.
    let mut watched = app
        .watched_variables
        .iter()
        .any(|watched| watched.name == full_name);

    ui.horizontal(|ui| {
        if ui.checkbox(&mut watched, "").changed() {
            if watched {
                app.watched_variables.push(WatchedVariable {
                    name: full_name.clone(),
                    address: variable.address,
                    data_type: MemoryType::from_symbol(&full_name),
                });
            } else {
                app.watched_variables
                    .retain(|watched| watched.name != full_name);
            }

            app.acquisition.set_variables(app.watched_variables.clone());

            // if !app.acquisition_running && !app.watched_variables.is_empty() {
            //     app.start_acquisition();
            // }
        }

        ui.label(&variable.name);

        ui.monospace(format!("{:08X}", variable.address));

        if let Some(value) = app.symbol_values.get(&full_name) {
            ui.monospace(format!("{value:.4}"));
        } else {
            ui.label("—");
        }
    });
}

fn matches_search(variable: &DebugVariable, full_name: &str, search: &str) -> bool {
    if full_name.to_lowercase().contains(search) {
        return true;
    }

    variable.children.iter().any(|child| {
        let child_name = format!("{full_name}.{}", child.name);

        matches_search(child, &child_name, search)
    })
}
// pub fn memory_type_for_symbol(name: &str) -> MemoryType {
//     if name.starts_with("ccr.") {
//         MemoryType::U16
//     } else if name.ends_with(".control.axis_mode")
//         || name.ends_with(".config.pole_pairs")
//         || name == "current_sense_state"
//     {
//         MemoryType::U32
//     } else {
//         MemoryType::F32
//     }
// }
