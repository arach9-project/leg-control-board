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

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u32)]
pub enum AxisMode {
    Disabled = 0,
    Calibration = 1,
    OpenLoop = 2,
    Current = 3,
    Position = 4,
}

impl AxisMode {
    pub const ALL: [Self; 5] = [
        Self::Disabled,
        Self::Calibration,
        Self::OpenLoop,
        Self::Current,
        Self::Position,
    ];

    pub fn label(self) -> &'static str {
        match self {
            Self::Disabled => "Disabled",
            Self::Calibration => "Calibration",
            Self::OpenLoop => "Open Loop",
            Self::Current => "Current",
            Self::Position => "Position",
        }
    }

    pub fn value(self) -> u32 {
        self as u32
    }
}

impl TryFrom<u32> for AxisMode {
    type Error = ();

    fn try_from(value: u32) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Disabled),
            1 => Ok(Self::Calibration),
            2 => Ok(Self::OpenLoop),
            3 => Ok(Self::Current),
            4 => Ok(Self::Position),
            _ => Err(()),
        }
    }
}

#[derive(Debug, Clone)]
pub struct Chart {
    pub id: String,
    pub title: String,
    pub variables: Vec<PlotVariable>,
    pub options: PlotOptions,
    pub visible_modes: Option<Vec<AxisMode>>,
}

impl Chart {
    pub fn new(
        id: impl Into<String>,
        title: impl Into<String>,
        variables: Vec<PlotVariable>,
        options: PlotOptions,
    ) -> Self {
        Self {
            id: id.into(),
            title: title.into(),
            variables,
            options,
            visible_modes: Some(Vec::from(AxisMode::ALL)),
        }
    }
}
