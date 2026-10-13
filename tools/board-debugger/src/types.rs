use eframe::egui::Color32;

use crate::acquisition::MemoryType;

#[derive(Debug, Clone)]
pub struct Sample {
    pub symbol: String,
    pub time: f64,
    pub value: f32,
}

#[derive(Debug, Clone)]
pub struct WatchedVariable {
    pub name: String,
    pub address: u64,
    pub data_type: MemoryType,
}

#[derive(Debug, Clone)]
pub struct PlotVariable {
    pub symbol: String,
    pub label: String,
    pub color: Color32,
}

#[derive(Debug, Clone)]
pub struct PlotOptions {
    pub y_min: Option<f64>,
    pub y_max: Option<f64>,
    pub track_data: bool,
}

impl Default for PlotOptions {
    fn default() -> Self {
        Self {
            y_min: None,
            y_max: None,
            track_data: true,
        }
    }
}

#[derive(Debug, Clone)]
pub struct Chart {
    pub id: String,
    pub title: String,
    pub variables: Vec<PlotVariable>,
    pub options: PlotOptions,
}

impl Chart {
    pub fn new(
        id: impl Into<String>,
        title: impl Into<String>,
        variables: Vec<PlotVariable>,
    ) -> Self {
        Self {
            id: id.into(),
            title: title.into(),
            variables,
            options: PlotOptions::default(),
        }
    }
}
