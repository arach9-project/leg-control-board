use crate::{app::DebuggerApp, config::AppConfig, symbols::SymbolTable};

mod acquisition;
mod app;
mod config;
mod dwarf;
mod symbols;
mod types;
mod ui;

fn main() -> eframe::Result {
    let options = eframe::NativeOptions {
        viewport: eframe::egui::ViewportBuilder::default()
            .with_inner_size([1000.0, 650.0])
            .with_min_inner_size([600.0, 400.0]),

        ..Default::default()
    };

    let config = AppConfig::default();
    eframe::run_native(
        "Board Debugger",
        options,
        Box::new(|_| Ok(Box::new(DebuggerApp::new(config)))),
    )
}
