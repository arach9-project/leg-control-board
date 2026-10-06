use std::{collections::VecDeque, time::Duration};

use egui_plot::{Line, Plot, PlotPoints};

use crate::{
    acquisition::{self, AcquisitionHandle},
    config::AcquisitionConfig,
    types::Sample,
};

const CHIP: &str = "STM32G431CBTx";

// Replace with the actual address of the variable you want to monitor.

pub struct DebuggerApp {
    samples: VecDeque<Sample>,
    status: String,
    acquisition: Option<AcquisitionHandle>,
    config: AcquisitionConfig,
}

impl Default for DebuggerApp {
    fn default() -> Self {
        Self {
            samples: VecDeque::new(),
            acquisition: None,
            status: "Idle".to_owned(),
            config: AcquisitionConfig::default(),
        }
    }
}

impl DebuggerApp {
    fn start_acquisition(&mut self) {
        let config = &self.config
        if self.acquisition.is_some() {
            return;
        }

        self.acquisition = Some(acquisition::start(config));

        self.status = "Acquiring".to_owned();
    }

    fn stop_acquisition(&mut self) {
        if let Some(acquisition) = &self.acquisition {
            acquisition.stop();
        }
        self.acquisition = None;
        self.status = "Stopped".to_owned();
    }

    fn receive_samples(&mut self) {
        let Some(acquisition) = &self.acquisition else {
            return;
        };

        while let Ok(sample) = acquisition.receiver.try_recv() {
            self.samples.push_back(sample);
            while self.samples.len() > self.config.max_samples {
                self.samples.pop_front();
            }
        }
    }
}

impl eframe::App for DebuggerApp {
    fn ui(&mut self, ctx: &mut eframe::egui::Ui, _frame: &mut eframe::Frame) {
        self.receive_samples();

        if let Some(acquisition) = &self.acquisition {
            self.status = "Acquiring".to_owned();

            // Keep refreshing the GUI while data is being acquired.
            ctx.request_repaint_after(Duration::from_millis(16));
        }

        eframe::egui::Panel::top("toolbar").show(ctx, |ui| {
            ui.horizontal(|ui| {
                ui.heading("Board Debugger");

                ui.separator();

                if ui
                    .add_enabled(
                        !self.acquisition.is_none(),
                        eframe::egui::Button::new("Start Acquisition"),
                    )
                    .clicked()
                {
                    self.start_acquisition();
                }

                if ui
                    .add_enabled(
                        self.acquisition.is_none(),
                        eframe::egui::Button::new("Stop Acquisition"),
                    )
                    .clicked()
                {
                    self.stop_acquisition();
                }

                ui.separator();

                ui.label(format!("Status: {}", self.status));
            });
        });

        eframe::egui::CentralPanel::default().show(ctx, |ui| {
            ui.heading("Variable");

            let points: PlotPoints = self
                .samples
                .iter()
                .map(|sample| [sample.time, sample.value as f64])
                .collect();

            let line = Line::new("Value", points);

            Plot::new("variable_plot")
                .height(400.0)
                .x_axis_label("Time (s)")
                .y_axis_label("Value")
                .show(ui, |plot_ui| {
                    plot_ui.line(line);
                });
        });
    }
}
