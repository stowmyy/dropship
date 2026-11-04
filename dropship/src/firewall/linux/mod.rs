use std::collections::HashSet;
use std::path::PathBuf;
use std::process::Stdio;
use std::time::Duration;

use tokio::io::{AsyncBufReadExt, AsyncWriteExt, BufReader};
use tokio::process::{Child, ChildStdin, ChildStdout, Command};

use crate::api;

pub(crate) mod helper;
pub(crate) mod launch;
pub(crate) mod netscope;

mod protocol;
mod ruleset;

use protocol::{Request, Response};

const TIMEOUT: Duration = Duration::from_secs(10);

pub(crate) struct Session {
    child: Child,
    stdin: Option<ChildStdin>,
    stdout: BufReader<ChildStdout>,
    dynamic: bool,
    broken: bool,
}

impl Session {
    pub(crate) async fn connect(dynamic: bool) -> Result<Self, String> {
        let executable = std::env::current_exe()
            .map_err(|e| format!("failed to locate the dropship binary: {e}"))?;

        let mut command = if unsafe { libc::geteuid() } == 0 {
            let mut command = Command::new(&executable);
            command.arg("--firewall-helper");
            command
        } else {
            let pkexec = ["/usr/bin/pkexec", "/bin/pkexec"]
                .iter()
                .map(std::path::Path::new)
                .find(|path| path.is_file())
                .ok_or_else(|| {
                    "pkexec was not found; install polkit or run dropship with sudo".to_string()
                })?;

            let mut command = Command::new(pkexec);
            command.arg(&executable).arg("--firewall-helper");
            command
        };

        command
            .stdin(Stdio::piped())
            .stdout(Stdio::piped())
            .stderr(Stdio::inherit());

        let mut child = command
            .spawn()
            .map_err(|e| format!("failed to start the firewall helper: {e}"))?;

        let (Some(stdin), Some(stdout)) = (child.stdin.take(), child.stdout.take()) else {
            let _ = child.kill().await;
            let _ = child.wait().await;
            return Err("failed to connect to the firewall helper".to_string());
        };

        let mut session = Session {
            child,
            stdin: Some(stdin),
            stdout: BufReader::new(stdout),
            dynamic,
            broken: false,
        };

        let ready = match session.read_ready().await {
            Ok(ready) => ready,
            Err(e) => {
                session.shutdown().await;
                return Err(e);
            }
        };

        if !ready.ok {
            let error = ready
                .error
                .unwrap_or_else(|| "firewall helper exited before becoming ready".to_string());
            session.shutdown().await;
            return Err(error);
        }

        if let Some(warning) = ready.warning {
            log::warn!("{}", warning);
        }

        let hello = Request::Hello {
            dynamic,
            uid: unsafe { libc::getuid() },
        };

        match session.round_trip(&hello).await {
            Ok(response) if response.ok => {
                if let Some(warning) = response.warning {
                    log::warn!("{}", warning);
                }
            }
            Ok(response) => {
                let error = response.error.unwrap_or_else(|| {
                    "the firewall helper rejected the hello request".to_string()
                });
                session.shutdown().await;
                return Err(error);
            }
            Err(e) => {
                session.shutdown().await;
                return Err(e);
            }
        }

        Ok(session)
    }

    pub(crate) async fn set_dynamic(&mut self, dynamic: bool) -> Result<(), String> {
        self.dynamic = dynamic;
        Ok(())
    }

    pub(crate) async fn ensure_scope(&mut self) -> Result<(), String> {
        let request = Request::EnsureScope {
            uid: unsafe { libc::getuid() },
        };

        let response = self.round_trip(&request).await?;

        if !response.ok {
            return Err(response.error.unwrap_or_else(|| {
                "the firewall helper could not create the game cgroup".to_string()
            }));
        }

        Ok(())
    }

    pub(crate) async fn apply(
        &mut self,
        blocked: &HashSet<api::KnownServer>,
        _known: &HashSet<PathBuf>,
    ) -> Result<HashSet<PathBuf>, String> {
        let request = Request::Apply {
            blocks: blocked.iter().map(|server| server.block.clone()).collect(),
            persist: !self.dynamic,
        };

        let response = match self.round_trip(&request).await {
            Ok(response) => response,
            Err(_) => {
                self.reconnect().await?;
                self.round_trip(&request).await?
            }
        };

        if !response.ok {
            return Err(response
                .error
                .unwrap_or_else(|| "the firewall helper rejected the request".to_string()));
        }

        if let Some(warning) = response.warning {
            log::warn!("{}", warning);
        }

        Ok(HashSet::new())
    }

    async fn reconnect(&mut self) -> Result<(), String> {
        // closing stdin lets the helper clean up and release the lock before a new one starts
        self.shutdown().await;

        let fresh = Self::connect(self.dynamic).await?;
        let old = std::mem::replace(self, fresh);
        drop(old);

        Ok(())
    }

    async fn shutdown(&mut self) {
        self.stdin.take();

        if tokio::time::timeout(TIMEOUT, self.child.wait())
            .await
            .is_err()
        {
            let _ = self.child.kill().await;
            let _ = self.child.wait().await;
        }
    }

    async fn read_ready(&mut self) -> Result<Response, String> {
        let mut line = String::new();

        match self.stdout.read_line(&mut line).await {
            Ok(0) => Err("firewall helper exited before becoming ready".to_string()),
            Ok(_) => serde_json::from_str(&line).map_err(|e| e.to_string()),
            Err(e) => Err(e.to_string()),
        }
    }

    async fn round_trip(&mut self, request: &Request) -> Result<Response, String> {
        if self.broken {
            return Err("the firewall helper session is not usable".to_string());
        }

        let mut line = serde_json::to_string(request).map_err(|e| e.to_string())?;
        line.push('\n');

        let Some(mut stdin) = self.stdin.take() else {
            self.broken = true;
            return Err("the firewall helper is not connected".to_string());
        };

        let write = tokio::time::timeout(TIMEOUT, async {
            stdin
                .write_all(line.as_bytes())
                .await
                .map_err(|e| e.to_string())?;
            stdin.flush().await.map_err(|e| e.to_string())
        })
        .await;

        self.stdin = Some(stdin);

        match write {
            Ok(Ok(())) => {}
            Ok(Err(e)) => return Err(e),
            Err(_) => {
                self.broken = true;
                return Err("the firewall helper did not accept the request".to_string());
            }
        }

        let mut line = String::new();
        match tokio::time::timeout(TIMEOUT, self.stdout.read_line(&mut line)).await {
            Ok(Ok(0)) => Err("the firewall helper closed the connection".to_string()),
            Ok(Ok(_)) => serde_json::from_str(&line).map_err(|e| e.to_string()),
            Ok(Err(e)) => Err(e.to_string()),
            Err(_) => {
                self.broken = true;
                Err("the firewall helper did not respond".to_string())
            }
        }
    }
}

impl Drop for Session {
    fn drop(&mut self) {
        // the helper sees EOF, removes dynamic rules, and releases the firewall lock
        self.stdin.take();
    }
}

pub(crate) fn persistent_supported() -> bool {
    std::path::Path::new("/run/systemd/system").is_dir()
}
