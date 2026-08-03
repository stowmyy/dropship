use std::{net::IpAddr, str::FromStr};
use tokio::time;

use rand::random;
use surge_ping::{Client, Config, ICMP, IcmpPacket, PingSequence};

pub const PING_TIMEOUT: time::Duration = time::Duration::from_secs(2);
const PING_INTERVAL: time::Duration = time::Duration::from_millis(900);

pub async fn ping_ip(ip: &String) -> Result<f32, String> {
    let ip = IpAddr::from_str(&ip).map_err(|e| e.to_string())?;

    let client = match ip {
        IpAddr::V4(_) => Client::new(&Config::default()).map_err(|e| e.to_string())?,
        IpAddr::V6(_) => {
            Client::new(&Config::builder().kind(ICMP::V6).build()).map_err(|e| e.to_string())?
        }
    };

    let payload = [0; 56];
    let mut pinger = client
        .pinger(ip, surge_ping::PingIdentifier(random::<u16>()))
        .await;

    pinger.timeout(PING_TIMEOUT);

    let mut interval = time::interval(PING_INTERVAL);

    let n_pings = 4;

    // REVIEW can maybe calculate n of hops here with 128 - ttl

    let mut ping = 0.;

    for idx in 0..n_pings {
        interval.tick().await;
        match pinger.ping(PingSequence(idx), &payload).await {
            Ok((IcmpPacket::V4(_packet), dur)) => {
                // println!(
                //     "No.{}: {} bytes from {}: icmp_seq={} ttl={:?} time={:0.2?}",
                //     idx,
                //     packet.get_size(),
                //     packet.get_source(),
                //     packet.get_sequence(),
                //     packet.get_ttl(),
                //     dur
                // )
                ping += dur.as_secs_f32() * 1000.;
                // ping += dur.as_millis_f32();
            }
            Ok((IcmpPacket::V6(_packet), dur)) => {
                // println!(
                //     "No.{}: {} bytes from {}: icmp_seq={} hlim={} time={:0.2?}",
                //     idx,
                //     packet.get_size(),
                //     packet.get_source(),
                //     packet.get_sequence(),
                //     packet.get_max_hop_limit(),
                //     dur
                // )
                ping += dur.as_secs_f32() * 1000.
                // ping += dur.as_millis_f32();
            }
            Err(e) => {
                let e = match e {
                    surge_ping::SurgeError::Timeout { .. } => format!("<{}> pinging failed", ip),
                    _ => e.to_string(),
                };

                // log::warn!("{}", e);

                return Err(e);
            }
        };
    }

    ping /= n_pings as f32;
    // println!("{} {:.2}ms", ip, ping);

    Ok(ping)
}
