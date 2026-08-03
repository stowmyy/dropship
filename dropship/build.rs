fn main() {
    let target_os = std::env::var("CARGO_CFG_TARGET_OS").unwrap_or_default();

    if target_os == "windows" {
        embed_resource::compile("assets/windows/dropship-manifest.rc", embed_resource::NONE)
            .manifest_required()
            .unwrap();
    }
}
