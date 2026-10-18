use eframe::egui::{self, Color32};
use std::collections::{HashMap, VecDeque};

use crate::{
    types::{AxisMode, Chart, PlotOptions, PlotVariable, Sample},
    ui::components::{RotaryDial, rotary_dial::DirectQuadrature},
};
pub struct AxisPanel {
    pub charts: Vec<Chart>,
    pub theta_target: f32,
}

#[derive(Debug, Clone, Copy)]
pub enum AxisAction {
    SetMode(u32),
    SetTarget(f32),
    SetIq(f32),
    SetId(f32),
    SetEncoderDirection(f32),
    SetDrvEnabled(bool),
    SetDKp(f32),
    SetDKi(f32),
    SetQKp(f32),
    SetQKi(f32),
}

fn axis_mode_name(value: f32) -> &'static str {
    if !value.is_finite() || value.fract() != 0.0 {
        return "UNKNOWN";
    }

    match value as i32 {
        0 => "DISABLED",
        1 => "CALIBRATION",
        2 => "OPENLOOP",
        3 => "CURRENT",
        4 => "POSITION",
        _ => "UNKNOWN",
    }
}

impl AxisPanel {
    pub fn new(axis: usize) -> Self {
        Self {
            charts: Self::create_charts(axis),
            theta_target: 0.0,
        }
    }

    fn create_charts(axis: usize) -> Vec<Chart> {
        let prefix = format!("axis{axis}");

        let variable = |symbol: &str, label: &str, color: Color32| PlotVariable {
            symbol: symbol.to_owned(),
            label: label.to_owned(),
            color,
        };

        let prefixed = |symbol: &str| format!("{prefix}.{symbol}");

        vec![
            Chart::new(
                format!("{prefix}_phase_currents"),
                "Phase Currents (UVW)",
                vec![
                    variable(&format!("axis{axis}_currents.u"), "U", Color32::BLUE),
                    variable(&format!("axis{axis}_currents.v"), "V", Color32::GREEN),
                    variable(&format!("axis{axis}_currents.w"), "W", Color32::RED),
                ],
                PlotOptions {
                    y_min: Some(-2f64),
                    y_max: Some(2f64),
                    track_data: true,
                },
            ),
            Chart::new(
                format!("{prefix}_velocity"),
                "Velocity",
                vec![variable(
                    &prefixed("omega_mech_"),
                    "velocity",
                    Color32::BLUE,
                )],
                PlotOptions {
                    y_min: Some(-10f64),
                    y_max: Some(10f64),
                    track_data: true,
                },
            ),
            Chart::new(
                format!("{prefix}_ccr"),
                "CCR",
                vec![
                    variable(&format!("axis{axis}_ccr.u"), "U", Color32::BLUE),
                    variable(&format!("axis{axis}_ccr.v"), "V", Color32::GREEN),
                    variable(&format!("axis{axis}_ccr.w"), "W", Color32::RED),
                ],
                PlotOptions {
                    y_min: Some(0f64),
                    y_max: Some(500f64),
                    track_data: true,
                },
            ),
            Chart {
                id: format!("{prefix}_errors"),
                title: "Errors".to_owned(),
                variables: vec![
                    variable(&prefixed("id_error_"), "id_error", Color32::BLUE),
                    variable(&prefixed("iq_error_"), "iq_error", Color32::RED),
                ],
                options: PlotOptions::default(),
                visible_modes: Some(vec![AxisMode::Current, AxisMode::Position]),
            },
        ]
    }
}

impl AxisPanel {
    pub fn show(
        &mut self,
        ui: &mut egui::Ui,
        samples: &VecDeque<Sample>,
        symbol_values: &HashMap<String, f32>,
        axis: usize,
        window_seconds: f64,
    ) -> Option<AxisAction> {
        let axis_prefix = format!("axis{axis}");
        let angles_prefix = format!("{axis_prefix}_angle");
        let mut action = None;

        let prefix = |symbol: &str| -> String { format!("{axis_prefix}.{symbol}") };

        let get = |name: &str, default: f32| -> f32 {
            symbol_values
                .get(name)
                .copied()
                .filter(|value| value.is_finite())
                .unwrap_or(default)
        };

        // Mechanical state

        let theta_mech = get(&prefix("theta_mech_"), 0.0);
        let theta_el = get(&prefix("theta_el_"), 0.0);
        let open_loop_theta_el = get(&prefix("open_loop_theta_el_"), 0.0);
        let current_dq_d = get(&prefix("current_dq_.d"), 0.0);
        let current_dq_q = get(&prefix("current_dq_.q"), 0.0);

        let voltage_dq_d = get(&prefix("voltage_dq_.d"), 0.0);
        let voltage_dq_q = get(&prefix("voltage_dq_.q"), 0.0);

        let pole_pairs = get(&prefix("config.pole_pairs"), 7.0).clamp(1.0, 64.0) as usize;

        let electrical_offset = get(&prefix("electrical_offset_"), 0.0);

        // Axis mode
        let mode: Option<AxisMode> = symbol_values
            .get(&prefix("control.axis_mode"))
            .copied()
            .filter(|v| v.is_finite() && v.fract() == 0.0 && *v >= 0.0 && *v <= 4.0)
            .and_then(|v| AxisMode::try_from(v as u32).ok());

        ui.heading(format!("Axis {axis}"));

        ui.add_space(12.0);

        // ─────────────────────────────────────
        // Motor controls
        // ─────────────────────────────────────

        ui.horizontal(|ui| {
            // Rotary dial
            ui.vertical(|ui| {
                ui.strong("Rotor Position");

                let current_dq = DirectQuadrature {
                    d: current_dq_d,
                    q: current_dq_q,
                };

                let voltage_dq = DirectQuadrature {
                    d: voltage_dq_d,
                    q: voltage_dq_q,
                };

                let response = RotaryDial::new(
                    &mut self.theta_target,
                    theta_mech,
                    theta_el,
                    open_loop_theta_el,
                    current_dq,
                    voltage_dq,
                    pole_pairs,
                    mode.unwrap_or(AxisMode::Disabled),
                )
                .size(240.0)
                .show(ui);

                ui.add_space(8.0);

                ui.label(format!("Mechanical: {:.2}°", theta_mech.to_degrees()));

                // ui.label(format!("Electrical: {:.2}°", theta_electrical.to_degrees()));

                ui.horizontal(|ui| {
                    ui.label("Target:");

                    let edit = ui.add(
                        egui::DragValue::new(&mut self.theta_target)
                            .speed(0.01)
                            .suffix(" rad"),
                    );

                    if edit.lost_focus() {
                        action = Some(AxisAction::SetTarget(self.theta_target));
                    }
                });

                // Commit the position when dragging finishes.
                if response.drag_stopped() {
                    action = Some(AxisAction::SetTarget(self.theta_target));
                }
            });

            ui.add_space(24.0);

            // Telemetry and state machine
            ui.vertical(|ui| {
                ui.strong("Axis Telemetry");

                egui::Grid::new(format!("{axis_prefix}_telemetry"))
                    .num_columns(2)
                    .spacing([16.0, 8.0])
                    .show(ui, |ui| {
                        for (label, symbol) in [("Velocity", prefix("omega_mech_"))] {
                            ui.label(label);

                            ui.monospace(format!("{:.3}", get(&symbol, 0.0),));

                            ui.end_row();
                        }
                    });

                ui.add_space(16.0);
                ui.label("Control Inputs");

                egui::Grid::new(format!("{axis_prefix}_inputs"))
                    .num_columns(2)
                    .spacing([16.0, 8.0])
                    .show(ui, |ui| {
                        for (label, symbol, range) in [
                            ("Iq Reference", prefix("control.iq_ref"), -10.0..=10.0),
                            ("Id Reference", prefix("control.id_ref"), -10.0..=10.0),
                            ("d_axis_kP", prefix("d_axis_loop_.Kp"), -10.0..=10.0),
                            ("d_axis_kI", prefix("d_axis_loop_.Ki"), -10.0..=10.0),
                            ("q_axis_kP", prefix("q_axis_loop_.Kp"), -10.0..=10.0),
                            ("q_axis_kI", prefix("q_axis_loop_.Ki"), -10.0..=10.0),
                        ] {
                            let mut value = get(&symbol, 0.0);

                            ui.label(label);

                            let response = ui.add(
                                egui::DragValue::new(&mut value)
                                    .speed(0.01)
                                    .range(range)
                                    .fixed_decimals(3),
                            );

                            if response.changed() {
                                action = Some(match label {
                                    "Iq Reference" => AxisAction::SetIq(value),
                                    "Id Reference" => AxisAction::SetId(value),
                                    "d_axis_kP" => AxisAction::SetDKp(value),
                                    "d_axis_kI" => AxisAction::SetDKi(value),
                                    "q_axis_kP" => AxisAction::SetQKp(value),
                                    "q_axis_kI" => AxisAction::SetQKi(value),
                                    _ => unreachable!(),
                                });
                            }

                            ui.end_row();
                        }
                    });
                let direction = get(&prefix("encoder_direction_"), 1.0);

                ui.label("Encoder Direction");

                ui.horizontal(|ui| {
                    if ui.selectable_label(direction > 0.0, "Normal").clicked() {
                        action = Some(AxisAction::SetEncoderDirection(1.0));
                    }

                    if ui.selectable_label(direction < 0.0, "Reversed").clicked() {
                        action = Some(AxisAction::SetEncoderDirection(-1.0));
                    }
                });

                let drv_enabled = get("drv8316_on", 0.0) != 0.0;

                ui.label("DRV8316");

                ui.horizontal(|ui| {
                    if ui.selectable_label(drv_enabled, "ON").clicked() {
                        action = Some(AxisAction::SetDrvEnabled(true));
                    }

                    if ui.selectable_label(!drv_enabled, "OFF").clicked() {
                        action = Some(AxisAction::SetDrvEnabled(false));
                    }
                });

                ui.end_row();

                ui.end_row();
            });
        });

        ui.add_space(4.0);

        // ─────────────────────────────────────
        // Mode selection
        // ─────────────────────────────────────
        ui.horizontal_wrapped(|ui| {
            for axis_mode in AxisMode::ALL {
                let selected = mode == Some(axis_mode);

                if ui.selectable_label(selected, axis_mode.label()).clicked() {
                    action = Some(AxisAction::SetMode(axis_mode.value()));
                }
            }
        });

        ui.add_space(16.0);
        ui.separator();

        // ─────────────────────────────────────
        // Live charts
        // ─────────────────────────────────────

        egui::CollapsingHeader::new("Live Charts")
            .id_salt(format!("{axis_prefix}_charts"))
            .default_open(true)
            .show(ui, |ui| {
                let spacing = 12.0;
                let available = ui.available_width();

                let columns = ((available / 300.0).floor() as usize).clamp(1, 3);

                let size =
                    ((available - spacing * (columns - 1) as f32) / columns as f32).max(100.0);

                egui::Grid::new(format!("{axis_prefix}_charts_grid"))
                    .num_columns(columns)
                    .spacing([spacing, spacing])
                    .show(ui, |ui| {
                        for (index, chart) in self.charts.iter_mut().enumerate() {
                            let visible = chart
                                .visible_modes
                                .as_ref()
                                .is_none_or(|modes| mode.is_some_and(|mode| modes.contains(&mode)));
                            if !visible {
                                continue;
                            }
                            ui.vertical(|ui| {
                                egui::CollapsingHeader::new(chart.title.clone())
                                    .id_salt(&chart.id)
                                    .default_open(true)
                                    .show(ui, |ui| {
                                        crate::ui::plot::show(
                                            samples,
                                            ui,
                                            chart,
                                            200f32,
                                            window_seconds,
                                        );
                                    });
                            });

                            if (index + 1) % columns == 0 {
                                ui.end_row();
                            }
                        }
                    });
            });

        action
    }
}
