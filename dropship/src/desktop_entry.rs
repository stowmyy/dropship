//! Installs the desktop entry and icon for the running binary.
//!
//! Wayland has no protocol for a window to carry its own icon, so compositors
//! resolve the window's app id (`dropship`) against a `.desktop` file in the
//! XDG data directories. Without the entry the taskbar shows a generic icon.

use std::fs;
use std::path::{Path, PathBuf};

const ICON: &[u8] = include_bytes!("../assets/linux/dropship.png");

pub(crate) fn ensure() {
    if let Err(e) = install() {
        // the logger is installed after this point, so stderr is all we have
        eprintln!("warning: could not install the desktop entry: {e}");
    }
}

fn install() -> Result<(), String> {
    let home = std::env::var_os("HOME").ok_or("HOME is not set")?;
    let data = std::env::var_os("XDG_DATA_HOME")
        .map(PathBuf::from)
        .unwrap_or_else(|| PathBuf::from(home).join(".local/share"));

    let icon = data.join("icons/hicolor/256x256/apps/dropship.png");
    let entry = data.join("applications/dropship.desktop");
    let exec = std::env::current_exe().map_err(|e| e.to_string())?;

    write_if_changed(&icon, ICON)?;
    write_if_changed(&entry, desktop_file(&exec, &icon).as_bytes())
}

fn write_if_changed(path: &Path, bytes: &[u8]) -> Result<(), String> {
    if fs::read(path).is_ok_and(|current| current == bytes) {
        return Ok(());
    }

    if let Some(dir) = path.parent() {
        fs::create_dir_all(dir).map_err(|e| e.to_string())?;
    }

    fs::write(path, bytes).map_err(|e| e.to_string())
}

fn desktop_file(exec: &Path, icon: &Path) -> String {
    // Exec follows the desktop entry quoting rules; `%` is a field code.
    let raw = exec
        .display()
        .to_string()
        .replace('%', "%%")
        .replace('\\', "\\\\");
    let exec = if raw.contains([' ', '"']) {
        format!("\"{}\"", raw.replace('"', "\\\""))
    } else {
        raw
    };

    // shortcut: Icon takes no quoting, so a HOME containing spaces breaks the icon
    format!(
        "[Desktop Entry]\n\
         Type=Application\n\
         Version=1.0\n\
         Name=Dropship\n\
         GenericName=Overwatch Server Selector\n\
         Comment=Block distant Overwatch servers at the firewall\n\
         Exec={exec}\n\
         Icon={}\n\
         Terminal=false\n\
         Categories=Network;\n\
         StartupNotify=true\n\
         StartupWMClass=dropship\n",
        icon.display()
    )
}

#[cfg(test)]
mod tests {
    use super::desktop_file;
    use std::path::Path;

    #[test]
    fn quotes_exec_paths_with_spaces_and_escapes_field_codes() {
        let file = desktop_file(
            Path::new("/home/me/my games/dropship 100%"),
            Path::new("/home/me/.local/share/icons/dropship.png"),
        );

        assert!(file.contains("Exec=\"/home/me/my games/dropship 100%%\""));
        assert!(file.contains("Icon=/home/me/.local/share/icons/dropship.png"));
        assert!(file.contains("StartupWMClass=dropship"));
    }

    #[test]
    fn leaves_plain_exec_paths_unquoted() {
        let file = desktop_file(Path::new("/usr/bin/dropship"), Path::new("/usr/icon.png"));
        assert!(file.contains("Exec=/usr/bin/dropship\n"));
    }
}
