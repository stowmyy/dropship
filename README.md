

`dropship.exe`             |  why? i dislike playing on certain servers.
:-------------------------:|:-------------------------|
![image](https://github.com/stowmyy/dropship-test/assets/120167078/55ba7db6-7d37-4eec-b50f-e1e52192009f)  | <ul><li>this is a work in progress. this repository is for testing automatic updates and will be replaced when an official version is available</li><li>download: [(latest version)](https://github.com/stowmyy/dropship-test/releases/latest/download/dropship.exe)</li></ul>

<hr />

## what is it
- this is a portable overwatch server selector.
- not affiliated with blizzard.
  - twitter: [@stormyy_ow](https://twitter.com/stormyy_ow/)

## notes
- todo: tidy code, optomize, improve UX, persistent settings
- todo: consider options for this as an in-game overlay
  - (just like discord, obs, overwolf, razer cortex, nvidia geforce, ..)
 
## how to use
1. download and open the app. it's a portable `.exe` file and doesn't install anything.
2. select the servers you wish to block. that's it, read the warnings section.
   - you do not need to keep the app open once servers are blocked.
 
| type | description |
| -- | -- |
| ![image](https://github.com/stowmyy/dropship-test/assets/120167078/f0ce6a64-953b-4ee4-ae5c-5f43af8b99a4) | **allow** - All servers are allowed by default. These are the servers you wish to play on |
| ![image](https://github.com/stowmyy/dropship-test/assets/120167078/db06c377-af4c-4ff8-8e62-c16cfc2d8ee9) | **block** - Set to blocked when you don't want to connect to this server |
| ![image](https://github.com/stowmyy/dropship-test/assets/120167078/078e18bd-e606-4257-95e4-4fb87f821d75) | **gonna block** - Servers can't be blocked while you are playing the game. Please restart your game with the app open |

## media
with game closed             |  with game open
:-------------------------:|:-------------------------:
![image](https://github.com/stowmyy/dropship-test/assets/120167078/8b30e560-4b8d-40f9-a952-8ae295e6ce3d)  |  ![image](https://github.com/stowmyy/dropship-test/assets/120167078/dbb8b4d9-92a9-4893-8369-59735afb8425)

## is it safe?
- don't download random `.exe`s from the internet.
- this program is safe. read the warnings and nothing bad will happen.
- there are no viruses because all the source code is above and the `.exe` is automatically generated [here](https://github.com/stowmyy/dropship-test/actions).
  - you can compile it yourself too, since it's open source.

## acknowledgements
- [foryVERX/Overwatch-Server-Selector](https://github.com/foryVERX/Overwatch-Server-Selector/)
  - please also take a look at this program. it is very similar but is more battle tested and has a community around it.
  - they did all the hard work for finding server ips. i just wanted to make my own version that was more lightweight and portable.

# WARNINGS

#### IMPORTANT INFORMATION
- This app only has TWO functions.
  1. **Add** firewall rules to block individual servers
  2. **Remove** firewall rules to block individual servers
- if you block certain servers, **they will remain blocked forever** until you unblock them
  - this means you do not need to keep the app open once a server is blocked
 
#### WORST CASE SENARIO
- The biggest potential problem is that these servers occasionally change, usually around major patch days
  - You may encounter a situation where you connect to a server that was added before this app was updated to include it.
  - **What this looks like:** Your game gets stuck with the message "ENTERING GAME" covering your whole screen.
  - **How to fix:** *Quickly* open this app and unblock servers. You have about 20 seconds.
  - **How often does this happen**: It's only happened to me once in the past year. It's most likely to happen around patch days

#### HOW TO HELP
  - If this ever happens to you, you can help by letting me know through [discord](https://discord.stormy.gg/) or [twitter](https://twitter.stormy.gg/) and i'll push a hotfix
  - Please ask questions and give suggestions there too


  


