use std::collections::VecDeque;

use crate::types::{Chart, PlotOptions, PlotVariable, Sample};

pub fn show(
    samples: &VecDeque<Sample>,
    ui: &mut eframe::egui::Ui,
    chart: &mut Chart,
    size: f32,
    window_seconds: f64,
) {
    let plot = egui_plot::Plot::new(&chart.id)
        .width(size)
        .height(size)
        .legend(egui_plot::Legend::default())
        .allow_zoom(true)
        .allow_drag(true)
        .allow_double_click_reset(true);

    let response = plot.show(ui, |plot_ui| {
        for variable in &chart.variables {
            let points: egui_plot::PlotPoints = samples
                .iter()
                .filter(|sample| sample.symbol == variable.symbol)
                .map(|sample| [sample.time, sample.value as f64])
                .collect();

            plot_ui
                .line(egui_plot::Line::new(variable.label.clone(), points).color(variable.color));
        }

        if chart.options.track_data {
            refocus_plot(
                samples,
                &chart.variables,
                &chart.options,
                plot_ui,
                window_seconds,
            );
        } else if let (Some(min), Some(max)) = (chart.options.y_min, chart.options.y_max) {
            if min < max {
                plot_ui.set_plot_bounds_y(min..=max);
            }
        }
    });

    response.response.context_menu(|ui| {
        ui.checkbox(&mut chart.options.track_data, "Track live data");

        if ui.button("Refocus").clicked() {
            chart.options.track_data = true;
            ui.close();
        }
    });
}

fn refocus_plot(
    samples: &VecDeque<Sample>,
    variables: &[PlotVariable],
    options: &PlotOptions,
    plot_ui: &mut egui_plot::PlotUi,
    window_seconds: f64,
) {
    let latest_time = variables
        .iter()
        .filter_map(|variable| {
            samples
                .iter()
                .rev()
                .find(|sample| sample.symbol == variable.symbol)
        })
        .map(|sample| sample.time)
        .reduce(f64::max);

    let Some(latest_time) = latest_time else {
        return;
    };

    plot_ui.set_plot_bounds_x((latest_time - window_seconds).max(0.0)..=latest_time);

    if let (Some(min), Some(max)) = (options.y_min, options.y_max) {
        if min < max {
            plot_ui.set_plot_bounds_y(min..=max);
        }
    } else {
        plot_ui.set_auto_bounds([false, true]);
    }
}
