use std::{
    sync::mpsc::{self, Receiver, Sender},
    thread::{self, JoinHandle},
    time::{Duration, Instant},
};

use probe_rs::{MemoryInterface, Session, SessionConfig};

use crate::{
    config::AppConfig,
    types::{Sample, WatchedVariable},
};
#[derive(Debug, Clone, Copy)]
pub enum MemoryType {
    U8,
    U16,
    U32,
    I32,
    F32,
}
#[derive(Debug, Clone, Copy)]
pub enum MemoryValue {
    U8(u8),
    U16(u16),
    U32(u32),
    I32(i32),
    F32(f32),
}

pub enum AcquisitionCommand {
    Start,
    Stop,
    SetVariables(Vec<WatchedVariable>),
    WriteMemory { address: u64, value: MemoryValue },
    Shutdown,
}
pub enum WriteResult {
    Success { address: u64 },
    Failure { address: u64, error: String },
}

pub struct AcquisitionHandle {
    pub receiver: Receiver<Sample>,
    pub write_receiver: Receiver<WriteResult>,
    pub command_tx: Sender<AcquisitionCommand>,
    worker: Option<JoinHandle<()>>,
}
impl MemoryType {
    pub fn from_symbol(name: &str) -> Self {
        if name == "ccr.u" || name == "ccr.v" || name == "ccr.w" {
            Self::U16
        } else if name.ends_with(".control.axis_mode")
            || name.ends_with(".config.pole_pairs")
            || name == "current_sense_state"
        {
            Self::U32
        } else {
            Self::F32
        }
    }
}

impl AcquisitionHandle {
    pub fn start(&self) {
        let _ = self.command_tx.send(AcquisitionCommand::Start);
    }

    pub fn stop(&self) {
        let _ = self.command_tx.send(AcquisitionCommand::Stop);
    }

    pub fn shutdown(&mut self) {
        let _ = self.command_tx.send(AcquisitionCommand::Shutdown);

        if let Some(worker) = self.worker.take() {
            if let Err(error) = worker.join() {
                eprintln!("Acquisition thread panicked: {error:?}");
            }
        }
    }
}
impl AcquisitionHandle {
    pub fn set_variables(&self, variables: Vec<WatchedVariable>) {
        let _ = self
            .command_tx
            .send(AcquisitionCommand::SetVariables(variables));
    }

    pub fn write(
        &self,
        address: u64,
        value: MemoryValue,
    ) -> Result<(), mpsc::SendError<AcquisitionCommand>> {
        self.command_tx
            .send(AcquisitionCommand::WriteMemory { address, value })
    }
}

pub fn new(config: AppConfig) -> AcquisitionHandle {
    let (sample_tx, sample_rx) = mpsc::channel();
    let (command_tx, command_rx) = mpsc::channel();
    let (write_tx, write_rx) = mpsc::channel();

    let worker = thread::spawn(move || {
        let mut session = match Session::auto_attach(config.chip.as_str(), SessionConfig::default())
        {
            Ok(session) => session,

            Err(error) => {
                eprintln!("Failed to attach: {error}");
                eprintln!("Full error: {error:?}");
                return;
            }
        };

        let mut running = false;
        let mut watched_variables = Vec::new();
        let start_time = Instant::now();

        loop {
            while let Ok(command) = command_rx.try_recv() {
                match command {
                    AcquisitionCommand::Start => {
                        running = true;
                    }

                    AcquisitionCommand::Stop => {
                        running = false;
                    }

                    AcquisitionCommand::Shutdown => {
                        return;
                    }

                    AcquisitionCommand::SetVariables(variables) => {
                        // println!("Watch list updated: {} variables", variables.len());
                        //
                        // for variable in &variables {
                        //     println!("  {} @ 0x{:08X}", variable.name, variable.address);
                        // }

                        watched_variables = variables;
                    }
                    AcquisitionCommand::WriteMemory { address, value } => {
                        let result = match session.core(0) {
                            Ok(mut core) => match write_memory(&mut core, address, value) {
                                Ok(()) => WriteResult::Success { address },

                                Err(error) => WriteResult::Failure {
                                    address,
                                    error: error.to_string(),
                                },
                            },

                            Err(error) => WriteResult::Failure {
                                address,
                                error: error.to_string(),
                            },
                        };

                        if let WriteResult::Failure { address, error } = &result {
                            eprintln!("Memory write failed at 0x{address:08X}: {error}");
                        }

                        let _ = write_tx.send(result);
                    }
                }
            }

            if !running {
                thread::sleep(Duration::from_millis(10));
                continue;
            }

            let mut core = match session.core(0) {
                Ok(core) => core,

                Err(error) => {
                    eprintln!("Core error: {error}");
                    thread::sleep(Duration::from_millis(10));
                    continue;
                }
            };

            for variable in &watched_variables {
                let value = match read_memory(&mut core, variable.address, variable.data_type) {
                    Ok(value) => value,

                    Err(error) => {
                        eprintln!(
                            "Failed {} at 0x{:08X}: {error}",
                            variable.name, variable.address,
                        );
                        continue;
                    }
                };

                let sample = Sample {
                    symbol: variable.name.clone(),
                    value,
                    time: start_time.elapsed().as_secs_f64(),
                };

                if sample_tx.send(sample).is_err() {
                    return;
                }
            }

            thread::sleep(config.sample_period);
        }
    });

    AcquisitionHandle {
        receiver: sample_rx,
        command_tx,
        worker: Some(worker),
        write_receiver: write_rx,
    }
}
fn read_memory(
    core: &mut probe_rs::Core<'_>,
    address: u64,
    data_type: MemoryType,
) -> Result<f32, probe_rs::Error> {
    let value = match data_type {
        MemoryType::U8 => {
            let aligned = address & !3;
            let word = core.read_word_32(aligned)?;
            let shift = ((address & 3) * 8) as u32;

            ((word >> shift) & 0xFF) as f32
        }

        MemoryType::U16 => {
            let aligned = address & !3;
            let word = core.read_word_32(aligned)?;
            let shift = ((address & 3) * 8) as u32;

            ((word >> shift) & 0xFFFF) as f32
        }

        MemoryType::U32 => core.read_word_32(address)? as f32,

        MemoryType::I32 => core.read_word_32(address)? as i32 as f32,

        MemoryType::F32 => f32::from_bits(core.read_word_32(address)?),
    };

    Ok(value)
}
fn write_memory(
    core: &mut probe_rs::Core<'_>,
    address: u64,
    value: MemoryValue,
) -> Result<(), probe_rs::Error> {
    match value {
        MemoryValue::U8(value) => {
            core.write_word_8(address, value)?;
        }

        MemoryValue::U16(value) => {
            core.write_word_16(address, value)?;
        }

        MemoryValue::U32(value) => {
            core.write_word_32(address, value)?;
        }

        MemoryValue::I32(value) => {
            core.write_word_32(address, value as u32)?;
        }

        MemoryValue::F32(value) => {
            core.write_word_32(address, value.to_bits())?;
        }
    }

    Ok(())
}
