use eframe::egui::{self, Color32};
use std::collections::{HashMap, VecDeque};

use crate::{
    types::{Chart, PlotVariable, Sample},
    ui::components::{RotaryDial, StateMachine},
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
}

const STATES: &[&str] = &["DISABLED", "CALIBRATION", "OPENLOOP", "CURRENT", "POSITION"];

const TRANSITIONS: &[(&str, &str)] = &[
    ("DISABLED", "CALIBRATION"),
    ("DISABLED", "OPENLOOP"),
    ("DISABLED", "CURRENT"),
    ("DISABLED", "POSITION"),
    ("CALIBRATION", "DISABLED"),
    ("OPENLOOP", "DISABLED"),
    ("CURRENT", "DISABLED"),
    ("POSITION", "DISABLED"),
    ("CALIBRATION", "OPENLOOP"),
    ("OPENLOOP", "CURRENT"),
    ("CURRENT", "POSITION"),
    ("POSITION", "CURRENT"),
];

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

        let prefix = |symbol: &str| &format!("{prefix}.{symbol}");

        vec![
            Chart::new(
                format!("{prefix}_phase_currents"),
                "Phase Currents (UVW)",
                vec![
                    variable("currents.u", "U", Color32::BLUE),
                    variable("currents.v", "V", Color32::GREEN),
                    variable("currents.w", "W", Color32::RED),
                ],
            ),
            Chart::new(
                format!("{prefix}_phase_duty"),
                "PWM Duty Cycles (UVW)",
                vec![
                    variable(&prefix("output_.duty.u"), "U", Color32::BLUE),
                    variable(&prefix("output_.duty.v"), "V", Color32::GREEN),
                    variable(&prefix("output_.duty.w"), "W", Color32::RED),
                ],
            ),
            Chart::new(
                format!("{prefix}_dq_currents"),
                "DQ Currents",
                vec![
                    variable(&prefix("current_dq_.d"), "Id", Color32::BLUE),
                    variable(&prefix("current_dq_.q"), "Iq", Color32::RED),
                ],
            ),
            Chart::new(
                format!("{prefix}_dq_voltages"),
                "DQ Voltages",
                vec![
                    variable(&prefix("voltage_dq_.d"), "Vd", Color32::BLUE),
                    variable(&prefix("voltage_dq_.q"), "Vq", Color32::RED),
                ],
            ),
            Chart::new(
                format!("{prefix}_errors"),
                "Errors",
                vec![
                    variable(&prefix("id_error_"), "id_error", Color32::BLUE),
                    variable(&prefix("iq_error_"), "iq_error", Color32::RED),
                ],
            ),
            Chart::new(
                format!("{prefix}_velocity"),
                "Velocity",
                vec![
                    variable(&prefix("omega_mech_"), "velocity", Color32::BLUE),
                    variable(&prefix("omega_raw_"), "raw", Color32::RED),
                ],
            ),
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

        let pole_pairs = get(&prefix("config.pole_pairs"), 7.0).clamp(1.0, 64.0) as usize;

        let electrical_offset = get(&prefix("electrical_offset_"), 0.0);
        let encoder_direction = get(&prefix("encoder_direction_"), 1.0);

        // Axis mode
        let mode = symbol_values
            .get(&prefix("control.axis_mode"))
            .copied()
            .filter(|value| value.is_finite());

        ui.heading(format!("Axis {axis}"));

        ui.add_space(12.0);

        // ─────────────────────────────────────
        // Motor controls
        // ─────────────────────────────────────

        ui.horizontal(|ui| {
            // Rotary dial
            ui.vertical(|ui| {
                ui.strong("Rotor Position");

                let response =
                    RotaryDial::new(&mut self.theta_target, theta_mech, theta_el, pole_pairs)
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
                        for (label, symbol) in [
                            ("Iq", "current_dq_.q"),
                            ("Id", "current_dq_.d"),
                            ("Vq", "voltage_dq_.q"),
                            ("Vd", "voltage_dq_.d"),
                            ("Velocity", "omega_mech_"),
                        ] {
                            ui.label(label);

                            ui.monospace(format!("{:.3}", get(symbol, 0.0),));

                            ui.end_row();
                        }
                    });

                ui.add_space(16.0);

                // Firmware state visualization
                if let Some(mode) = mode {
                    let current_state = axis_mode_name(mode);

                    StateMachine::new(current_state, STATES, TRANSITIONS).show(ui);

                    ui.weak(format!("Firmware mode: {}", mode as i32,));
                } else {
                    ui.strong("Axis State");
                    ui.weak("Waiting for state data...");
                }
            });
        });

        ui.add_space(16.0);

        // ─────────────────────────────────────
        // Mode selection
        // ─────────────────────────────────────

        ui.strong("Control Mode");

        ui.horizontal_wrapped(|ui| {
            for (label, value) in [
                ("Disabled", 0_u32),
                ("Calibration", 1),
                ("Open Loop", 2),
                ("Current", 3),
                ("Position", 4),
            ] {
                let selected = mode.is_some_and(|current| current == value as f32);

                if ui.selectable_label(selected, label).clicked() {
                    action = Some(AxisAction::SetMode(value));
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
