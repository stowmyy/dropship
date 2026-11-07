use std::time::Duration;

mod commands;
mod events;

pub use commands::{Command, start_processing_commands};
pub use events::{Event, process_events};

mod startup_dispatch;
pub use startup_dispatch::startup_dispatch;

// version checks are performed this often
pub const INTERVAL_VERSION_CHECK: Duration = Duration::from_secs(9000);
pub const INTERVAL_API_CHECK: Duration = Duration::from_secs(900);
pub const INTERVAL_PROCESS_CHECK: Duration = Duration::from_millis(900);

// pub const DROPSHIP_API_URL: &str = "http://127.0.0.1:9999/dropship.json";
pub const DROPSHIP_API_URL: &str = "https://stowmyy.github.io/dropship/ips.json";

pub const UPDATE_URI: &str = match option_env!("DROPSHIP_UPDATE_URI") {
    Some(uri) => uri,
    None => "https://api.github.com/repos/stowmyy/dropship/releases/latest",
};
pub const GITHUB_URI: &str = match option_env!("DROPSHIP_SOURCE_URI") {
    Some(uri) => uri,
    None => "https://github.com/stowmyy/dropship",
};
#[cfg(target_os = "windows")]
pub const BINARY_NAME: &str = "dropship.exe";
#[cfg(all(target_os = "linux", target_arch = "x86_64"))]
pub const BINARY_NAME: &str = "dropship-linux-x86_64";
#[cfg(all(target_os = "linux", not(target_arch = "x86_64")))]
pub const BINARY_NAME: &str = "dropship-linux-unsupported-arch";

pub const DISCORD_INVITE_LINK: &str = "https://discord.gg/QYrF8CVhbC";
