use std::{
    sync::{
        Arc,
        atomic::{AtomicBool, Ordering},
        mpsc::{self, Receiver},
    },
    thread,
    time::Instant,
};

use probe_rs::{MemoryInterface, Session, SessionConfig};

use crate::{config::AcquisitionConfig, types::Sample};

pub struct AcquisitionHandle {
    pub receiver: Receiver<Sample>,
    stop_flag: Arc<AtomicBool>,
}

impl AcquisitionHandle {
    pub fn stop(&self) {
        self.stop_flag.store(true, Ordering::Relaxed);
    }
}

pub fn start(config: AcquisitionConfig) -> AcquisitionHandle {
    let (tx, rx) = mpsc::channel();

    let stop_flag = Arc::new(AtomicBool::new(false));
    let worker_stop_flag = Arc::clone(&stop_flag);

    thread::spawn(move || {
        let mut session = match Session::auto_attach((), SessionConfig::default()) {
            Ok(session) => session,

            Err(error) => {
                eprintln!("Failed to attach probe: {error}");
                return;
            }
        };

        let start_time = Instant::now();

        while !worker_stop_flag.load(Ordering::Relaxed) {
            let mut core = match session.core(0) {
                Ok(core) => core,

                Err(error) => {
                    eprintln!("Failed to access core: {error}");
                    break;
                }
            };

            let raw = match core.read_word_32(config.variable_address) {
                Ok(value) => value,

                Err(error) => {
                    eprintln!("Failed to read memory: {error}");
                    break;
                }
            };

            let sample = Sample {
                time: start_time.elapsed().as_secs_f64(),
                value: f32::from_bits(raw),
            };

            if tx.send(sample).is_err() {
                break;
            }

            thread::sleep(config.sample_period);
        }
    });

    AcquisitionHandle {
        receiver: rx,
        stop_flag,
    }
}
