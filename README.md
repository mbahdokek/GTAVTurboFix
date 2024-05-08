GTA V Turbo Fix
=====================
- Updated for 3095 (3179 still works)

Forked from ikt32, Check the original for more info

!Please read all of this before downloading!   Downloads are on this right side (search for Releases)  >>>>>>>

Video Showcase (What's New):

[![](https://cdn.discordapp.com/attachments/318715415188733954/1237464824963661855/image0.gif?ex=663bbe6d&is=663a6ced&hm=7203bccc549d7c06da01460dd04d2e5b49d9edfbc0f30af0c25c12c8a053890c&)](http://www.youtube.com/watch?v=kEQCRlDJv-w "Click to play on Youtube.com")

## Changes
- Patches.cpp: Changed the Turbo Limiter NOP patching to \xC7\x43\x7C\x00\x00\x80\x3F\x48\x8B\xCE, now boost can be set higher than 1.0 again
- VehicleExtension.cpp: Fixed the AntiLag inconsistency because GetThrottleP no longer works, now the backfire effect fires as intended (only when throttle get released/not pressed);
  also turned off the version check for old games, so it's not compatible with game versions older than 3095; some addresses updated from SHVDN sdk
- TurboScript.cpp: Added feature of multiple backfire effect 50-50 chance of veh_sanctus and weap_sm_bom,
  and then followed by modded blue/purple veh_backfire (credits to 13stewartc) which separated to the back a bit
  and exp_sht_flame (thanks for the help of JulioNiB for finding Unknown3A0 for particle property) a flame burst fx for epic gritty look. 
  For compatibility with QuantV which has its own backfire effect, you can skip installing the custom ypt file so the script will fallback to the original core/veh_backfire
- Added 2 custom backfire sound kits (credit to [Zendo Revolusound Team](https://www.nexusmods.com/forzahorizon5/mods/86) from Forza Modding Scene, check em' out)
  ZendoDirty for street-modded cars, and ZendoClean for clean supercars

## Install / Usage

2 Version available: The default/latest one (marked green latest) is modded with extra features as listed above. The other one is the original mod with fixes only but no extra features

The modded one is packed with OIV because of custom ypt
(No worries of conflict with core.ypt. It's a standalone custom ypt, thanks to JulioNiB for the tutorial)

Installation details are written inside README, inside the .rar file

## Building

The source code is still the original one, I haven't uploaded it. My version got deleted :) 

Honestly, I've made a very embarrassing mistake using GitHub (yep im dumb) i never used it before and I f*cked up
and before I knew all my files got reverted to the source files 🤦

I probably will upload what is available from my ChatGPT history
