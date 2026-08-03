use eframe::egui;
use tokio::sync::mpsc;

pub struct Logger {
    pub max_level: log::LevelFilter,

    pub tx: mpsc::UnboundedSender<Message>,

    pub ctx: Option<egui::Context>,
}

impl Logger {
    #[allow(dead_code)]
    pub fn set_ctx(&mut self, ctx: Option<egui::Context>) {
        self.ctx = ctx;
    }
}

impl log::Log for Logger {
    fn enabled(&self, metadata: &log::Metadata) -> bool {
        // NOTE this prevents a mutex deadlock i guess
        if metadata.target().starts_with("egui")
            || metadata.target().starts_with("epaint")
            || metadata.target().starts_with("eframe")
        {
            return false;
        }

        metadata.level() <= self.max_level || metadata.target().starts_with("dropship")
    }

    fn log(&self, record: &log::Record) {
        #[cfg(debug_assertions)]
        if record.metadata().target().starts_with("egui")
            || record.metadata().target().starts_with("epaint")
            || record.metadata().target().starts_with("eframe")
        {
            println!("[{}] {}", &record.level(), &record.args().to_string());
        }

        if self.enabled(record.metadata()) {
            let thread_id = std::thread::current().id();

            let _ = self.tx.send(Message {
                level: record.level(),
                message: record.args().to_string(),
                // target: record.target().to_string(),
                time: chrono::Local::now(),
                thread_id,
            });

            // a thing happened so paint next frame just in case
            if let Some(ctx) = &self.ctx {
                ctx.request_repaint();
            }

            #[cfg(debug_assertions)]
            println!(
                "[{}] {:?} {}",
                &record.level(),
                thread_id,
                &record.args().to_string()
            );
        }
    }

    fn flush(&self) {}
}

pub struct Message {
    pub level: log::Level,
    pub message: String,
    // pub target: String,
    pub time: chrono::DateTime<chrono::Local>,
    pub thread_id: std::thread::ThreadId,
}
