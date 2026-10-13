use eframe::egui;

pub(crate) enum ToolbarAction {
    None,
    StartAcquisition,
    StopAcquisition,
    ToggleSymbols,
}

pub(crate) fn show(
    ui: &mut egui::Ui,
    running: bool,
    status: &str,
    show_symbols: bool,
) -> ToolbarAction {
    let mut action = ToolbarAction::None;

    egui::Panel::top("toolbar").show(ui, |ui| {
        ui.horizontal(|ui| {
            ui.heading("Board Debugger");

            ui.separator();

            if ui
                .add_enabled(!running, egui::Button::new("Start Acquisition"))
                .clicked()
            {
                action = ToolbarAction::StartAcquisition;
            }

            if ui
                .add_enabled(running, egui::Button::new("Stop Acquisition"))
                .clicked()
            {
                action = ToolbarAction::StopAcquisition;
            }

            if ui
                .button(if show_symbols {
                    "Hide Symbols"
                } else {
                    "Show Symbols"
                })
                .clicked()
            {
                action = ToolbarAction::ToggleSymbols;
            }

            ui.separator();

            ui.label(format!("Status: {status}"));
        });
    });

    action
}
