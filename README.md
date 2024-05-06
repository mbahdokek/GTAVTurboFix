GTA V Turbo Fix
=====================
- Updated for 3095 (3179 still works)

A script for Grand Theft Auto V, that fixes the broken turbo behavior.

Forked from ikt32, Check the original for more info !!!

Showcase:

[![](https://img.youtube.com/vi/kEQCRlDJv-w/hqdefault.jpg)](http://www.youtube.com/watch?v=kEQCRlDJv-w "Click to play on Youtube.com")

## Changes
- Patches.cpp : Changed the Turbo Limiter NOP patching to \xC7\x43\x7C\x00\x00\x80\x3F\x48\x8B\xCE, now boost can be set higher than 1.0 again
- VehicleExtension.cpp : Fixed the AntiLag inconsistency because GetThrottleP no longer works, now the backfire effect fires as intended (only when throttle released/not pressed);
  also turned off version check for old games, so it's not compatible for game version older than 3095; some address updated from SHVDN source file
- TurboScript.cpp : Added feature of multiple backfire effect 50-50 chance of veh_sanctus and weap_sm_bom,
  and then followed by modded blue/purple veh_backfire (credits to 13stewartc) which separated to the back a bit
  and exp_sht_flame (thanks for the help of JulioNiB for finding Unknown3A0 for particle property) a flame burst fx for epic gritty look
  For compatibility with QuantV which has their own backfire effect, you can skip installing the custom ypt file so the script will fallback to original core/veh_backfire
- Added 2 custom backfire sound kits (credit to [Zendo Revolusound Team](https://www.nexusmods.com/forzahorizon5/mods/86) from Forza Modding Scene, check em' out)
  ZendoDirty for street-modded cars, and ZendoClean for clean supercars

## Install / Usage

Will be packed with oiv because of custom ypt
(no worries of conflict it's standalone custom ypt, thanks to JulioNiB for the tutorial)

## Building

Source code is still the original one, i haven't uploaded. In fact my version get deleted :) 

Honestly, i've made a very embarrassing mistake using GitHub (yep im dumb) i never used it before and i f*cked up
and before i knew all my files got reverted back to the source files 🤦

I probably will upload what is available from my ChatGPT history (TurboScript.cpp mostly)
