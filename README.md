
# dropship v3

<!--
```ruby
OW2 // DROPSHIP
```
-->

<table>
 <tr>
  <td>
   <img src="https://github.com/stowmyy/dropship/assets/120167078/55ba7db6-7d37-4eec-b50f-e1e52192009f" />
  </td>
  <td>
   a portable server selector for overwatch
  </td>
  <td>
    <a href="https://github.com/stowmyy/dropship/releases/latest/download/dropship.exe" target="_blank">
     <p></p>
     <pre>download</pre>
    </a>
  </td>
    <td>
    <a href="https://github.com/stowmyy/dropship/releases" target="_blank">
     <p></p>
     <pre>versions</pre>
    </a>
  </td>
 </tr>
</table>

<!--
## notes
- not affiliated with blizzard
- twitter: [@stormyy_ow](https://twitter.com/stormyy_ow/)
-->
 
## how to use
1. download (using the button above) and open dropship with your game closed
3. **block** the servers you **don't** want to play on
4. done! you can close the app now. blocking servers persists until you unblock them. there is also an option to only have these blocks while the app is open

linux: download dropship-linux-x86_64, chmod +x, run it. requires nftables and polkit; systemd is needed for blocks to survive a reboot. on first run it installs its own desktop entry and icon (`~/.local/share/applications/dropship.desktop`), so the taskbar can show the app icon on wayland. blocks follow the game, not your whole desktop: launch the game through `dropship launch -- %command%` in lutris or steam, or, if [netscope](https://github.com/Qiiks/netscope) is installed, let dropship join its shared scope (`netscope open --consumer dropship ...`) — the game is then attached by name and no launch change is needed.

linux install without touching the launcher: `scripts/install-linux.sh --release` puts the binary in `~/.local/bin` and installs and enables [netscope](https://github.com/Qiiks/netscope) too (`--no-netscope` to skip, `--dry-run` to look first). on arch, `packaging/arch/PKGBUILD` builds a `dropship-bin` package that ships and enables netscoped the same way.

## warnings
> [!IMPORTANT]
> <ul><li>when queuing with other people, the matchmaker may put you on servers you've avoided unless all other members in the party also block them</li><li>practice range does not use the matchmaker. if you'd like to test dropship properly, try custom games</li><li>If you are ever failing to connect to a server, quickly clicking this button will prevent a competitive ban. please report any instance of this on the discord so it can be investigated<br /> <img width="420" height="64" alt="image" src="https://github.com/user-attachments/assets/9045d2f7-d4e0-484b-9f67-25adb8174d60" /></li></ul> 

## faq
question | answer
:-------------------------|:-------------------------
it's not working :( | please write a message in https://discord.gg/QYrF8CVhbC or open an issue here https://github.com/stowmyy/dropship/issues
do i need to keep the app open? | no - blocking servers is permanent (unless you select the option to block only while dropship is open)
how do i uninstall? | click the `disable dropship` button and then delete the `.exe`
does this app install anything? | this app creates a configuration file in `%appdata%/dropship`. the only other changes it makes to your computer are adding an ip filter to windows for blocking game servers, and some temporary files when you update the app. the filter is deleted whenever all servers are unblocked
how do i pin this app to my taskbar? | <img src="https://github.com/user-attachments/assets/a0cf3cf5-4b24-4ee5-b893-b95a73b9e75b" height="90" />
does this app modify the game? | this app does **_not_** modify the game in any way. instead, this app works by adding ip address filters (wfp) just like an ad blocker would
does it only block the game on linux? | yes. the game runs in its own cgroup, so only the game is filtered: launch it through `dropship launch -- %command%` in your launcher, or leave it to netscope when that daemon is installed (dropship then joins its shared scope and the game is attached by name).
i want a feature | suggestions are welcome in the discord. please don't contribute ai generated code.

## media
| expanded | collapsed |
| -- | -- |
| <img width="1511" height="1060" alt="image" src="https://github.com/user-attachments/assets/df7b7ebc-ecc2-472a-b3ab-4dbf0d3cc6d8" /> | <img width="484" height="1063" alt="image" src="https://github.com/user-attachments/assets/9b246ae8-3112-40ee-b0fa-2c1118197f92" /> |


