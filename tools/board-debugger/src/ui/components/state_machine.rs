use eframe::egui::{self, Color32, Frame, RichText, Stroke, Ui};

pub struct StateMachine<'a> {
    pub current: &'a str,
    pub states: &'a [&'a str],
    pub transitions: &'a [(&'a str, &'a str)],
}

impl<'a> StateMachine<'a> {
    pub fn new(
        current: &'a str,
        states: &'a [&'a str],
        transitions: &'a [(&'a str, &'a str)],
    ) -> Self {
        Self {
            current,
            states,
            transitions,
        }
    }

    pub fn show(&self, ui: &mut Ui) {
        let active_color = Color32::from_rgb(55, 155, 95);
        let inactive_color = ui.visuals().faint_bg_color;

        ui.vertical(|ui| {
            ui.horizontal(|ui| {
                ui.strong("Axis State");

                ui.separator();

                ui.colored_label(active_color, self.current);
            });

            ui.add_space(8.0);

            ui.horizontal_wrapped(|ui| {
                for &state in self.states {
                    let active = state == self.current;

                    let fill = if active { active_color } else { inactive_color };

                    let stroke = if active {
                        Stroke::new(2.0, active_color)
                    } else {
                        Stroke::new(1.0, ui.visuals().widgets.inactive.bg_stroke.color)
                    };

                    Frame::new()
                        .fill(fill)
                        .stroke(stroke)
                        .corner_radius(6.0)
                        .inner_margin(egui::Margin::symmetric(12, 8))
                        .show(ui, |ui| {
                            ui.label(
                                RichText::new(state)
                                    .color(if active {
                                        Color32::WHITE
                                    } else {
                                        ui.visuals().text_color()
                                    })
                                    .strong(),
                            );
                        });
                }
            });
        });
    }
}
