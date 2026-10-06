use crate::app::DebuggerApp;

mod acquisition;
mod app;
mod config;
mod types;

fn main() -> eframe::Result {
    let options = eframe::NativeOptions {
        viewport: eframe::egui::ViewportBuilder::default()
            .with_inner_size([1000.0, 650.0])
            .with_min_inner_size([600.0, 400.0]),

        ..Default::default()
    };

    eframe::run_native(
        "Board Debugger",
        options,
        Box::new(|_| Ok(Box::new(DebuggerApp::default()))),
    )
}
