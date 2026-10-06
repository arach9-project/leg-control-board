use std::time::Duration;

#[derive(Debug, Clone)]
pub struct AcquisitionConfig {
    pub sample_period: Duration,
    pub max_samples: usize,
    pub variable_address: u64,
}

impl Default for AcquisitionConfig {
    fn default() -> Self {
        Self {
            sample_period: Duration::from_millis(10),
            max_samples: 1000,
            variable_address: 0x2000_0000,
        }
    }
}
