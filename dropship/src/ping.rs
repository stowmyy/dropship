//! ping over icmp through the windows api (`IcmpSendEcho`, the same thing ping.exe uses):
//! no raw socket, no elevated privileges needed.
//!
//! the ping address a server ships with in `ips.json` sometimes ignores icmp altogether (a lot
//! of the google cloud ones do), so when it stays silent we look for an address that does answer
//! inside the server's own ip blocks (`block`) — the same network the game servers live in —
//! and remember it.

use std::{
    collections::HashMap,
    net::{IpAddr, Ipv4Addr},
    str::FromStr,
    sync::{LazyLock, Mutex},
};
use tokio::time;
use windows::Win32::NetworkManagement::IpHelper::{
    ICMP_ECHO_REPLY, IP_OPTION_INFORMATION, IP_SUCCESS, IcmpCloseHandle, IcmpCreateFile, IcmpSendEcho,
};

pub const PING_TIMEOUT: time::Duration = time::Duration::from_millis(1500);
const PING_INTERVAL: time::Duration = time::Duration::from_millis(700);
const N_PINGS: u16 = 3;

/// the address that turned out to answer, per server (keyed by the api's ping address)
static TARGET: LazyLock<Mutex<HashMap<String, Ipv4Addr>>> = LazyLock::new(|| Mutex::new(HashMap::new()));

/// `ip` is the api's ping address, `block` the server's ip blocks (comma separated cidrs) to
/// search for an alternative when it does not answer
pub async fn ping_server(ip: &String, block: &str) -> Result<f32, String> {
    // whatever answered last time
    let known = TARGET.lock().unwrap().get(ip).copied();
    if let Some(addr) = known {
        if let Some(ms) = echo(addr, N_PINGS).await? {
            return Ok(ms);
        }
    }
    // the api address first, then candidates inside the blocks; the first one that answers wins
    for addr in std::iter::once(IpAddr::from_str(ip).map_err(|e| e.to_string())?)
        .filter_map(v4)
        .chain(candidates(block))
    {
        if Some(addr) == known {
            continue;
        }
        if let Some(ms) = echo(addr, 1).await? {
            TARGET.lock().unwrap().insert(ip.clone(), addr);
            return Ok(ms);
        }
    }
    Err(crate::lang::tf("server.ping_failed", &[ip]))
}

fn v4(a: IpAddr) -> Option<Ipv4Addr> {
    match a {
        IpAddr::V4(a) => Some(a),
        IpAddr::V6(_) => None,
    }
}

/// likely addresses inside the blocks: the first hosts of each block, plus the first host of the
/// next subnet in the wide ones
fn candidates(block: &str) -> impl Iterator<Item = Ipv4Addr> + '_ {
    block
        .split(',')
        .filter_map(|c| ipnet::Ipv4Net::from_str(c.trim()).ok())
        .flat_map(|net| {
            let base = u32::from(net.network());
            let size = 1u32 << (32 - net.prefix_len());
            [1u32, 2, 10, 256 + 1, 65_536 + 1]
                .into_iter()
                .filter(move |o| *o < size.saturating_sub(1))
                .map(move |o| Ipv4Addr::from(base + o))
        })
        .take(24)
}

/// sends `n` echoes and returns the average of the ones that came back, `None` if none did
async fn echo(addr: Ipv4Addr, n: u16) -> Result<Option<f32>, String> {
    let (mut sum, mut replies) = (0f32, 0u16);
    for i in 0..n {
        if i > 0 {
            time::sleep(PING_INTERVAL).await;
        }
        let r = tokio::task::spawn_blocking(move || send_echo(addr))
            .await
            .map_err(|e| e.to_string())??;
        if let Some(ms) = r {
            sum += ms;
            replies += 1;
        }
    }
    Ok((replies > 0).then(|| sum / replies as f32))
}

/// one echo (blocks the thread, hence spawn_blocking)
fn send_echo(addr: Ipv4Addr) -> Result<Option<f32>, String> {
    let data = [0u8; 32];
    // reply + data + room for an error message, as the docs recommend
    let mut reply = vec![0u8; std::mem::size_of::<ICMP_ECHO_REPLY>() + data.len() + 8];
    let opts = IP_OPTION_INFORMATION { Ttl: 128, ..Default::default() };
    // SAFETY: documented win32 calls; the buffers outlive the call and the handle is always closed
    unsafe {
        let handle = IcmpCreateFile().map_err(|e| e.to_string())?;
        let n = IcmpSendEcho(
            handle,
            u32::from_ne_bytes(addr.octets()),
            data.as_ptr().cast(),
            data.len() as u16,
            Some(&opts),
            reply.as_mut_ptr().cast(),
            reply.len() as u32,
            PING_TIMEOUT.as_millis() as u32,
        );
        let _ = IcmpCloseHandle(handle);
        if n == 0 {
            return Ok(None); // timed out or unreachable
        }
        let r: ICMP_ECHO_REPLY = std::ptr::read_unaligned(reply.as_ptr().cast());
        Ok((r.Status == IP_SUCCESS).then_some(r.RoundTripTime as f32))
    }
}
