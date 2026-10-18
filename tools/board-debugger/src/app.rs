use std::{
    collections::{HashMap, VecDeque},
    time::Duration,
};

use eframe::egui::{self};

use crate::{
    acquisition::MemoryType,
    types::WatchedVariable,
    ui::{
        self,
        axis::{AxisAction, AxisPanel},
        toolbar::ToolbarAction,
    },
};

use crate::{
    acquisition::{self, AcquisitionHandle},
    config::AppConfig,
    symbols::SymbolTable,
    types::Sample,
};

//TODO:
//- [ ] Fix the angle indicators
//- [ ] Show data points in plot

const MAX_SAMPLES_PER_FRAME: usize = 5_000;
const MAX_SAMPLES: usize = 50_000;

// Replace with the actual address of the variable you want to monitor.

pub(crate) struct DebuggerApp {
    pub(crate) samples: VecDeque<Sample>,
    pub(crate) symbol_search: String,
    pub(crate) symbol_values: HashMap<String, f32>,
    pub(crate) status: String,

    pub(crate) acquisition: AcquisitionHandle,
    pub(crate) acquisition_running: bool,
    pub(crate) config: AppConfig,
    pub(crate) symbol_table: Option<SymbolTable>,
    pub(crate) show_symbols: bool,

    pub(crate) watched_variables: Vec<WatchedVariable>,

    pub(crate) axes: [AxisPanel; 3],
    pub(crate) selected_axis: usize,
    pub(crate) plot_window_seconds: f64,
}

impl Default for DebuggerApp {
    fn default() -> Self {
        let config = AppConfig::default().clone();
        let symbol_table = SymbolTable::load(&config.firmware_path).ok();
        let acquisition = acquisition::new(config.clone());

        Self {
            samples: VecDeque::new(),

            symbol_search: String::new(),
            symbol_values: HashMap::new(),

            acquisition,
            acquisition_running: false,
            status: "Idle".to_owned(),
            symbol_table,
            config,

            show_symbols: false,
            watched_variables: Vec::new(),

            axes: [AxisPanel::new(1), AxisPanel::new(2), AxisPanel::new(3)],
            selected_axis: 0,
            plot_window_seconds: 1.0,
        }
    }
}

impl DebuggerApp {
    fn drop(&mut self) {
        self.acquisition.shutdown();
    }
    fn write_symbol(&self, name: &str, value: acquisition::MemoryValue) -> Result<(), String> {
        let table = self
            .symbol_table
            .as_ref()
            .ok_or("Symbol table unavailable")?;

        let variable = table
            .find(name)
            .ok_or_else(|| format!("Symbol not found: {name}"))?;

        self.acquisition
            .write(variable.address, value)
            .map_err(|error| error.to_string())
    }
    pub fn new(config: AppConfig) -> Self {
        let symbol_table = SymbolTable::load(&config.firmware_path).ok();
        let watched_variables = config
            .watched_variables
            .iter()
            .filter_map(|name| {
                let variable = symbol_table.as_ref()?.find(name)?;

                Some(WatchedVariable {
                    name: name.clone(),
                    address: variable.address,
                    data_type: MemoryType::from_symbol(&name),
                })
            })
            .collect::<Vec<_>>();
        let acquisition = acquisition::new(config.clone());

        Self {
            samples: VecDeque::new(),
            symbol_search: String::new(),
            symbol_values: HashMap::new(),
            status: "Idle".to_owned(),

            acquisition,
            acquisition_running: false,

            symbol_table,
            watched_variables,
            config,

            show_symbols: false,

            axes: [AxisPanel::new(1), AxisPanel::new(2), AxisPanel::new(3)],

            selected_axis: 0,
            plot_window_seconds: 5.0,
        }
    }

    fn start_acquisition(&mut self) {
        if self.acquisition_running {
            return;
        }

        self.samples.clear();
        self.symbol_values.clear();

        self.acquisition
            .set_variables(self.watched_variables.clone());

        self.acquisition.start();

        self.acquisition_running = true;
        self.status = "Acquiring".to_owned();
    }

    fn stop_acquisition(&mut self) {
        if !self.acquisition_running {
            return;
        }

        self.acquisition.stop();

        self.acquisition_running = false;
        self.status = "Stopped".to_owned();
    }

    fn receive_samples(&mut self) {
        for _ in 0..MAX_SAMPLES_PER_FRAME {
            let Ok(sample) = self.acquisition.receiver.try_recv() else {
                break;
            };

            self.symbol_values
                .insert(sample.symbol.clone(), sample.value);

            self.samples.push_back(sample);
        }

        if let Some(latest) = self.samples.back() {
            let cutoff = latest.time - self.plot_window_seconds;

            while self
                .samples
                .front()
                .is_some_and(|sample| sample.time < cutoff)
            {
                self.samples.pop_front();
            }
        }

        while self.samples.len() > MAX_SAMPLES {
            self.samples.pop_front();
        }
    }

    fn receive_write_results(&mut self) {
        while let Ok(result) = self.acquisition.write_receiver.try_recv() {
            match result {
                acquisition::WriteResult::Success { address } => {
                    self.status = format!("Memory written: 0x{address:08X}");
                }

                acquisition::WriteResult::Failure { address, error } => {
                    self.status = format!("Write failed at 0x{address:08X}: {error}");
                }
            }
        }
    }
}

impl eframe::App for DebuggerApp {
    fn ui(&mut self, ctx: &mut eframe::egui::Ui, _frame: &mut eframe::Frame) {
        self.receive_samples();
        self.receive_write_results();

        if self.acquisition_running {
            self.status = "Acquiring".to_owned();

            ctx.request_repaint_after(Duration::from_millis(16));
        }

        match crate::ui::toolbar::show(
            ctx,
            self.acquisition_running,
            &self.status,
            self.show_symbols,
        ) {
            ToolbarAction::StartAcquisition => {
                self.start_acquisition();
            }

            ToolbarAction::StopAcquisition => {
                self.stop_acquisition();
            }

            ToolbarAction::ToggleSymbols => {
                self.show_symbols = !self.show_symbols;
            }

            ToolbarAction::None => {}
        }
        if self.show_symbols {
            ui::symbols::show(self, ctx);
        }

        egui::CentralPanel::default()
            .frame(egui::Frame::central_panel(&ctx.style()).inner_margin(6.0))
            .show(ctx, |ui| {
                egui::ScrollArea::vertical().show(ui, |ui| {
                    ui.horizontal(|ui| {
                        for index in 0..3 {
                            ui.selectable_value(
                                &mut self.selected_axis,
                                index,
                                format!("Axis {}", index + 1),
                            );
                        }
                    });

                    ui.separator();

                    let index = self.selected_axis.min(2);

                    let action = self.axes[index].show(
                        ui,
                        &self.samples,
                        &self.symbol_values,
                        index + 1,
                        self.plot_window_seconds,
                    );

                    if let Some(action) = action {
                        let axis = index + 1;

                        let result = match action {
                            AxisAction::SetMode(mode) => self.write_symbol(
                                &format!("axis{axis}.control.axis_mode"),
                                acquisition::MemoryValue::U32(mode),
                            ),

                            AxisAction::SetTarget(theta) => self.write_symbol(
                                &format!("axis{axis}.control.theta_target"),
                                acquisition::MemoryValue::F32(theta),
                            ),

                            AxisAction::SetIq(iq) => self.write_symbol(
                                &format!("axis{axis}.control.iq_ref"),
                                acquisition::MemoryValue::F32(iq),
                            ),
                            AxisAction::SetId(id) => self.write_symbol(
                                &format!("axis{axis}.control.id_ref"),
                                acquisition::MemoryValue::F32(id),
                            ),

                            AxisAction::SetEncoderDirection(direction) => self.write_symbol(
                                &format!("axis{axis}.encoder_direction_"),
                                acquisition::MemoryValue::F32(direction),
                            ),

                            AxisAction::SetDrvEnabled(enabled) => self.write_symbol(
                                &format!("drv8316_{axis}_enable_cmd"),
                                acquisition::MemoryValue::U8(enabled as u8),
                            ),
                            AxisAction::SetDKp(value) => self.write_symbol(
                                &format!("axis{axis}.d_axis_loop_.Kp"),
                                acquisition::MemoryValue::F32(value),
                            ),

                            AxisAction::SetDKi(value) => self.write_symbol(
                                &format!("axis{axis}.d_axis_loop_.Ki"),
                                acquisition::MemoryValue::F32(value),
                            ),

                            AxisAction::SetQKp(value) => self.write_symbol(
                                &format!("axis{axis}.q_axis_loop_.Kp"),
                                acquisition::MemoryValue::F32(value),
                            ),

                            AxisAction::SetQKi(value) => self.write_symbol(
                                &format!("axis{axis}.q_axis_loop_.Ki"),
                                acquisition::MemoryValue::F32(value),
                            ),
                        };

                        if let Err(error) = result {
                            self.status = format!("Write error: {error}");
                        }
                    }
                });
            });
    }
}
