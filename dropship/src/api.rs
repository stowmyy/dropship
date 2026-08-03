use std::hash::{Hash, Hasher};

use crate::dropship::DROPSHIP_API_URL;

#[derive(serde::Deserialize, serde::Serialize, Clone, Debug)]
pub struct Notice {
    pub title: String,
    pub date: String,
    pub paragraph: String,
}

#[derive(serde::Deserialize, serde::Serialize, Clone, Debug)]
pub struct ApiServers {
    pub overwatch: Vec<KnownServer>,
}

#[derive(serde::Deserialize, serde::Serialize, Clone, Debug)]
pub struct DropshipApiData {
    pub notices: Vec<Notice>,
    pub servers: ApiServers,
    // test: String,
}

#[derive(serde::Deserialize, serde::Serialize, Clone, Debug)]
pub struct KnownServer {
    pub title: String,
    pub token: String,
    pub block: String,
    pub bit: u8,
    pub ping: String,
}

impl KnownServer {
    #[allow(dead_code)]
    pub fn bit(&self) -> u8 {
        self.bit
    }
}

impl PartialEq for KnownServer {
    fn eq(&self, other: &Self) -> bool {
        self.bit == other.bit
    }
}

impl Eq for KnownServer {}

impl Hash for KnownServer {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.bit.hash(state);
    }
}

pub async fn fetch_dropship_api_data() -> Result<DropshipApiData, String> {
    let client = reqwest::Client::builder()
        .user_agent("dropship")
        .build()
        .unwrap();

    #[cfg(debug_assertions)]
    tokio::time::sleep(std::time::Duration::from_secs(9)).await;

    let data = client
        .get(DROPSHIP_API_URL)
        .send()
        .await
        .map_err(|e| e.to_string())?
        .json::<DropshipApiData>()
        .await
        .map_err(|e| {
            // dbg!(&e);
            e.to_string()
        })?;

    Ok(data)
}
