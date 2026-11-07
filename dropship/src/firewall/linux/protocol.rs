#[derive(serde::Serialize, serde::Deserialize)]
#[serde(tag = "op", rename_all = "snake_case")]
pub(crate) enum Request {
    Hello { dynamic: bool, uid: u32 },
    Apply { blocks: Vec<String>, persist: bool },
    EnsureScope { uid: u32 },
}

#[derive(Default, serde::Serialize, serde::Deserialize)]
pub(crate) struct Response {
    pub ok: bool,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub warning: Option<String>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub error: Option<String>,
}
