# Sharing a game process with netscope (ExitLink)

Dropship's Linux block and netscope's routing cannot each own the game's cgroup.
Linux gives a process exactly one, and both tools move the game into their own:

| Tool | Where the game lands |
|---|---|
| Dropship | `/sys/fs/cgroup/user.slice/user-<uid>.slice/user@<uid>.service/dropship-game` (`helper.rs:20`, `cgroup_rel_path`) |
| netscope | `/sys/fs/cgroup/netscope/<profile>` (`cgroup.rs::attach_pid`) |

Dropship's rule is `socket cgroupv2 level 4 "dropship-game"`
(`cgroup_level` → 4, `ruleset.rs::generate`). A process moved to
`netscope/<profile>` matches neither that level nor that name, so the block
rules stay loaded and match nothing: **the game connects to blocked servers
with no error anywhere**. netscope's route breaks the same way if Dropship
attaches second, since the route's `socket cgroupv2` selector is level 2.

The fix is not to make the cgroups coexist — it is to stop having two.
netscope (>= 0.2.0) exposes a shared scope: one cgroup per process, one route
policy, any number of filter policies, all in one nftables table. Dropship
becomes the filter consumer; ExitLink is the route consumer. Filter rules run
before the route mark, so a Dropship drop is terminal for that packet and a
destination Dropship allows still gets routed.

## What Dropship must send

Dropship blocks specific game-server ranges and allows everything else, which
is `--filter allow --deny-cidr`, not `--filter drop --allow-cidr` (that one
would also drop Battle.net auth and every other destination).

```sh
# create or join the scope; same (target, uid) as ExitLink means one profile
sudo netscope --uid "$UID" open --consumer dropship --process Overwatch.exe \
  --filter allow --deny-cidr 64.224.26.0/23 --deny-cidr 5.42.0.0/16

# the server list changed: replace just our policy, atomically
sudo netscope policy overwatch --consumer dropship --filter allow \
  --deny-cidr 64.224.26.0/23

# app exit / all servers unblocked
sudo netscope close overwatch --consumer dropship
```

- `open` is idempotent for `(target, target_uid, consumer)`, so Dropship can
  call it on every apply without tracking state.
- Use `resolve --process <game>` to find the profile ExitLink already created;
  do not create a second profile for the same game (two profiles would fight
  over the process).
- `policy` is a full replacement, so send the whole current deny list each
  time. That is the live-update path for the server list.
- Only one consumer may hold a route; Dropship never asks for one.

## Ordering

1. Open the netscope claim before the game starts. A `--process` target holds
   the Wine/Proton loader pre-exec, so the game inherits `netscope/<profile>`
   from its first packet.
2. Do not move the process into `dropship-game` when a netscope claim exists.
   That move is what silently disables both tools.

## Fallback

If the daemon is unreachable, keep the current path exactly as it is: wrapper
moves the process into `dropship-game`, `inet dropship` carries the block. Never
do both. Detect netscope with `netscope doctor` (exit 0, daemon PASS) or by
connecting to the daemon socket, not by probing the binary alone.

## Verify

```sh
netscope status                     # one profile, consumers: dropship,exitlink
nft list table inet netscope_overwatch   # filter chain: daddr <blocked> drop
```

Then block a server Dropship can reach (practice range is not matchmaker
traffic) and watch that rule's counter increment while ExitLink still shows the
game routed. If the counter stays 0 and the game reaches the server, the
process is not in `netscope/<profile>` — check `cat /proc/<pid>/cgroup`.

## Limits to carry over from netscope

- A name target attaches every process with that name under the target UID.
- Loopback is always accepted by a drop filter; an allow filter's deny list is
  applied literally, so never put loopback or the LAN in it.
- `filter` and `route` are the only policy kinds — no per-destination
  proxy/TCP layer. Dropship's block is destination filtering only, which fits.
- The block applies to the whole profile cgroup, which is the game's process
  tree. It no longer applies to unrelated programs running as the user, which
  is what the `dropship-game` cgroup bought and what README still claims.
