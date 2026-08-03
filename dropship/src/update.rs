use std::{
    path::PathBuf,
    sync::{
        Arc,
        atomic::{AtomicU64, Ordering},
    },
};

use crate::dropship::{BINARY_NAME, UPDATE_URI};

#[derive(Debug)]
pub enum UpdateInfo {
    NoUpdate,
    UpdateAvailable(AvailableUpdate),
}

#[derive(Default)]
pub enum UpdatingStatus {
    #[default]
    NotActive,
    Downloading,
    Installed,
    Failed(String),
}

#[derive(Debug)]
pub struct AvailableUpdate {
    pub version: semver::Version,
    pub description: String,
    pub binary: AssetMetadata,
}

#[derive(Debug, serde::Deserialize)]
pub struct AssetMetadata {
    pub browser_download_url: String,

    pub size: u64,
    pub download_count: u64,
    pub updated_at: chrono::DateTime<chrono::Utc>,
}

pub async fn check_for_update() -> Result<UpdateInfo, String> {
    let client = reqwest::Client::builder()
        .user_agent("dropship-update")
        .build()
        .map_err(|e| e.to_string())?;

    let resp = client
        .get(UPDATE_URI)
        .send()
        .await
        .map_err(|e| e.to_string())?
        .json::<serde_json::Value>()
        .await
        .map_err(|e| e.to_string())?;

    let tag = resp["tag_name"]
        .as_str()
        .ok_or("no tag in release")?
        .trim_start_matches("v");

    let remote_version = semver::Version::parse(tag).map_err(|e| e.to_string())?;

    let installed_version =
        semver::Version::parse(env!("CARGO_PKG_VERSION")).map_err(|e| e.to_string())?;

    if remote_version > installed_version {
        let assets = resp["assets"]
            .as_array()
            .ok_or("no assets found in version")?;

        // let (download_uri, size, download_count) = {
        //     let remote_executable = assets
        //         .into_iter()
        //         .find(|a| a["name"].as_str() == Some(BINARY_NAME))
        //         .ok_or("didn't find remote binary in version assets")?;

        //     let download_uri = remote_executable["browser_download_url"]
        //         .as_str()
        //         .ok_or("no version download url in github api")?
        //         .to_string();

        //     let size = remote_executable["size"].as_u64().unwrap_or_default();

        //     let download_count = remote_executable["download_count"]
        //         .as_u64()
        //         .unwrap_or_default();

        //     let updated_at = remote_executable["updated_at"].as_str().ok

        //     (download_uri, size, download_count)
        // };

        let binary = {
            let remote_executable = assets
                .into_iter()
                .find(|a| a["name"].as_str() == Some(BINARY_NAME))
                .ok_or("didn't find remote binary in version assets")?;

            // REVIEW clone
            serde_json::from_value::<AssetMetadata>(remote_executable.clone())
        }
        .map_err(|e| e.to_string())?;

        let description = resp["body"]
            .as_str()
            .ok_or("no version description in github api")?
            .to_string()
            .replace("\n- ", "\n• ");

        Ok(UpdateInfo::UpdateAvailable(AvailableUpdate {
            version: remote_version,
            description,
            binary,
        }))
    } else {
        Ok(UpdateInfo::NoUpdate)
    }
}

pub fn installed_binary_path() -> Result<PathBuf, String> {
    std::env::current_exe().map_err(|e| e.to_string())
}
pub fn graveyard_binary_path() -> Result<PathBuf, String> {
    Ok(installed_binary_path()?.with_added_extension("deleteme"))
}
pub fn downloading_binary_path() -> Result<PathBuf, String> {
    Ok(installed_binary_path()?.with_added_extension("part"))
}

pub async fn update(
    remote_binary: &String,
    (download_total_size, downloaded_size): (Arc<AtomicU64>, Arc<AtomicU64>),
) -> Result<(), String> {
    let client = reqwest::Client::builder()
        .user_agent("dropship-update")
        .build()
        .map_err(|e| e.to_string())?;

    let mut resp = client
        .get(remote_binary)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    if !resp.status().is_success() {
        return Err(format!(
            "failed to download version binary: {}",
            resp.status()
        ));
    }

    // stream
    // let bytes = {
    //     let total_size = resp.content_length();
    //     let mut bytes =
    //         Vec::with_capacity(usize::try_from(total_size.unwrap_or_default()).unwrap_or_default());
    //     let mut processed_size = 0.;

    //     // there is like 128 chunks, they are paged this is not per byte
    //     while let Some(chunk) = resp.chunk().await.map_err(|e| e.to_string())? {
    //         bytes.extend_from_slice(&chunk);
    //         processed_size += chunk.len() as f32;

    //         if let Some(total_size) = total_size {
    //             if let Ok(mut p) = progress.lock() {
    //                 *p = processed_size / total_size as f32;
    //             }
    //         }
    //     }

    //     bytes
    // };

    // NOTE this would directly download without streaming
    // let bytes = resp.bytes().await.map_err(|e| e.to_string())?;

    let installed_binary_path = installed_binary_path()?;
    let graveyard_binary_path = graveyard_binary_path()?;
    let downloading_binary_path = downloading_binary_path()?;

    {
        // clear tmp
        remove_file_if_exists(&downloading_binary_path)
            .await
            .map_err(|e| e.to_string())?;

        // write tmp
        {
            let mut part: tokio::fs::File = tokio::fs::File::create(&downloading_binary_path)
                .await
                .map_err(|e| e.to_string())?;

            if let Some(total_size) = resp.content_length() {
                download_total_size.store(total_size, Ordering::Relaxed);
            }

            downloaded_size.store(0, std::sync::atomic::Ordering::Relaxed);

            // there is like 128 chunks, they are paged this is not per byte
            let mut p = 0;
            while let Some(chunk) = resp.chunk().await.map_err(|e| e.to_string())? {
                tokio::io::AsyncWriteExt::write_all(&mut part, &chunk)
                    .await
                    .map_err(|e| e.to_string())?;

                downloaded_size.fetch_add(chunk.len() as u64, Ordering::Relaxed);

                p += 1;
            }

            dbg!(p);

            // REVIEW this might not be needed, drop() probably works fine.
            // butt.. the docs say "maybe not" so i think this is safer
            // part.flush().await.map_err(|e| e.to_string())?;
            part.sync_all().await.map_err(|e| e.to_string())?;
        }

        // move installed exe to graveyard
        {
            // consecrate
            remove_file_if_exists(&graveyard_binary_path)
                .await
                .map_err(|e| e.to_string())?;

            // gallow installed exe
            tokio::fs::rename(&installed_binary_path, &graveyard_binary_path)
                .await
                .map_err(|e| e.to_string())?;
        }

        // move downloaded exe to installed exe
        {
            tokio::fs::rename(&downloading_binary_path, &installed_binary_path)
                .await
                .map_err(|e| e.to_string())?;
        }

        // NOTE we do this after startup
        // tokio::fs::remove_file(&graveyard_binary_path)
        //     .await
        //     .map_err(|e| e.to_string())?;
    }

    Ok(())
}

// uwu
pub async fn remove_file_if_exists(path: &std::path::PathBuf) -> Result<(), String> {
    match tokio::fs::remove_file(path).await {
        Ok(_) => Ok(()),
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => Ok(()),
        Err(e) => Err(e.to_string()),
    }
}
