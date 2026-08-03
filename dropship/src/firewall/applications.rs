use std::{collections::HashSet, path::PathBuf};

#[derive(Debug)]
pub enum ApplicationType {
    /// battle.net
    Blizzard,

    /// steam
    Valve,

    /// unknown
    Unknown,
}

// #[derive(Debug)]
// pub struct Application {
//     pub path: std::path::PathBuf,
//     pub ty: ApplicationType,
//     pub label: String,
// }

/// gets known overwatch application paths.
/// combines
///    - overwatch application rule paths
///    - dropship rule paths
///    - legacy dropship rule paths
///    - mina paths
///
/// and deduplicates them. filters out application paths that don't exist on the filesystem.
///
/// dropship rules should be created on these paths *before* any legacy rules are deleted
#[cfg(target_os = "windows")]
fn get_known_application_paths_from_firewall() -> windows::core::Result<Vec<std::path::PathBuf>> {
    let matched = [
        super::get_blizzard_overwatch_rules()?.collect(),
        super::get_dropship_rules()?.collect(),
        super::legacy::get_legacy_dropship_rules()?,
        super::legacy::get_mina_rules()?,
    ]
    .into_iter()
    .flatten()
    //
    // has an application path defined
    // .filter_map(|r| r.application_name().clone())
    // .map(std::path::PathBuf::from)
    .filter_map(|r| {
        let n = unsafe { r.ApplicationName().ok() };
        if let Some(n) = n {
            Some(super::win::bstr_to_path(&n))
        } else {
            None
        }
    })
    //
    // exe exists on the file system
    .filter(|path| path.exists())
    //
    // deduplicate
    .collect::<HashSet<_>>()
    .into_iter()
    .collect::<Vec<_>>();

    Ok(matched)
}

#[cfg(target_os = "windows")]
pub fn get_known_valid_application_paths(
    previously_known_paths: &HashSet<PathBuf>,
) -> windows::core::Result<HashSet<PathBuf>> {
    // FIXME do we need this every time?
    let paths_from_firewall = get_known_application_paths_from_firewall()?;

    let mut combined = [
        paths_from_firewall,
        previously_known_paths
            .into_iter()
            .cloned()
            .collect::<Vec<_>>(),
    ]
    .into_iter()
    .flatten()
    .collect::<HashSet<_>>();

    // TODO ensure files exist
    // FIXME
    // FIXME remove rules that are valid?

    combined.retain(|x| x.exists());

    Ok(combined)
}
