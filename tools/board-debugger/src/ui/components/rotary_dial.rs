use std::f32::consts::{PI, TAU};

use eframe::egui::{self, Color32, Pos2, Response, Sense, Shape, Stroke, Ui, Vec2};

pub struct RotaryDial<'a> {
    pub target: &'a mut f32,
    pub mechanical: f32,
    pub electrical: f32,
    pub pole_pairs: usize,
    pub size: f32,
}

impl<'a> RotaryDial<'a> {
    pub fn new(target: &'a mut f32, mechanical: f32, electrical: f32, pole_pairs: usize) -> Self {
        Self {
            target,
            mechanical,
            electrical,
            pole_pairs: pole_pairs.max(1),
            size: 240.0,
        }
    }

    pub fn size(mut self, size: f32) -> Self {
        self.size = size;
        self
    }

    pub fn show(self, ui: &mut Ui) -> Response {
        let (rect, response) =
            ui.allocate_exact_size(Vec2::splat(self.size), Sense::click_and_drag());

        let painter = ui.painter_at(rect);

        let center = rect.center();
        let radius = rect.width().min(rect.height()) * 0.37;

        let north = Color32::from_rgb(195, 75, 75);
        let south = Color32::from_rgb(65, 105, 190);

        let target_color = Color32::WHITE;
        let mechanical_color = Color32::GREEN;
        let electrical_color = Color32::YELLOW;

        // Background.
        painter.circle_filled(center, radius + 8.0, ui.visuals().extreme_bg_color);

        // Magnetic sectors rotate with the rotor.
        let sectors = self.pole_pairs * 2;

        for i in 0..sectors {
            let start = self.mechanical + i as f32 * TAU / sectors as f32;

            let end = self.mechanical + (i + 1) as f32 * TAU / sectors as f32;

            let color = if i % 2 == 0 { north } else { south };

            let mut points = vec![center];

            for step in 0..=12 {
                let t = step as f32 / 12.0;
                let angle = start + (end - start) * t;

                points.push(point(center, radius, angle));
            }

            painter.add(Shape::convex_polygon(points, color, Stroke::NONE));
        }

        // Rotor hub.
        painter.circle_filled(center, radius * 0.28, Color32::from_gray(45));

        painter.circle_stroke(center, radius, Stroke::new(2.0, Color32::GRAY));

        // Mechanical angle indicator.
        indicator(
            &painter,
            center,
            radius * 0.90,
            self.mechanical,
            mechanical_color,
            4.0,
        );

        // Electrical angle indicator.
        indicator(
            &painter,
            center,
            radius * 0.72,
            self.electrical,
            electrical_color,
            3.0,
        );

        // Target indicator extends beyond the rotor.
        indicator(
            &painter,
            center,
            radius * 1.16,
            *self.target,
            target_color,
            3.0,
        );

        painter.circle_filled(center, 5.0, Color32::WHITE);

        // Pointer interaction.
        //
        // Store the previous pointer angle in egui's persistent
        // widget state, so the component does not require
        // external drag-tracking fields.

        let id = response.id.with("drag_angle");

        if response.drag_started() {
            if let Some(pos) = response.interact_pointer_pos() {
                if pos.distance(center) <= radius {
                    let angle = pointer_angle(center, pos);

                    ui.ctx().data_mut(|data| {
                        data.insert_temp(id, angle);
                    });
                }
            }
        }

        if response.dragged() {
            let previous = ui.ctx().data(|data| data.get_temp::<f32>(id));

            // Pointer capture is retained when dragging
            // outside the widget.
            let position = ui.ctx().input(|input| input.pointer.interact_pos());

            if let (Some(previous), Some(position)) = (previous, position) {
                if position.distance(center) > 8.0 {
                    let current = pointer_angle(center, position);

                    let delta = (current - previous + PI).rem_euclid(TAU) - PI;

                    *self.target += delta;

                    ui.ctx().data_mut(|data| {
                        data.insert_temp(id, current);
                    });
                }
            }
        }

        if response.drag_stopped() {
            ui.ctx().data_mut(|data| {
                data.remove::<f32>(id);
            });
        }

        response
    }
}

fn point(center: Pos2, radius: f32, angle: f32) -> Pos2 {
    Pos2::new(
        center.x + radius * angle.cos(),
        center.y - radius * angle.sin(),
    )
}

fn pointer_angle(center: Pos2, position: Pos2) -> f32 {
    (center.y - position.y).atan2(position.x - center.x)
}

fn indicator(
    painter: &egui::Painter,
    center: Pos2,
    length: f32,
    angle: f32,
    color: Color32,
    width: f32,
) {
    let end = point(center, length, angle);

    painter.line_segment([center, end], Stroke::new(width, color));

    painter.circle_filled(end, width * 1.5, color);
}
