# VGi
A simple taihen plugin that prints useful information about PS Vita games

This can be particularly useful for figuring out back-buffer resolutions, memory usage, etc...

## How to use
* Add VGi.suprx to taihen config.txt (either *ALL or title id of your game), reload
* Open your game
* Press SELECT + LT to open VGi menu

## Controls
* SELECT + LT - Open/Close VGi
* LT/RT - Move between sections
* D-PAD UP/DOWN - Scroll

## Screenshots
![App Info](https://user-images.githubusercontent.com/12598379/46871354-71a29580-ce31-11e8-9b0e-8546328379f6.png)
**Misc info about the game's eboot**
<br>
<br>

![Graphics](https://user-images.githubusercontent.com/12598379/46871397-9991f900-ce31-11e8-9def-fdd35140343e.png)
**P4G uses native 960x544 framebuffer so it can render UI elements at native res.**
<br>
<br>

![Render Targets](https://user-images.githubusercontent.com/12598379/46871419-a6165180-ce31-11e8-8256-14b46baa817f.png)
**Here we can see that P4G uses untraditional 840x476 resolution for its back-buffer(s).**
<br>
<br>

![Color Surfaces](https://user-images.githubusercontent.com/12598379/46871523-fa213600-ce31-11e8-83dd-f96ac4524353.png)
<br>
<br>

![Memory](https://user-images.githubusercontent.com/12598379/46871543-073e2500-ce32-11e8-963c-994e454b8269.png)
**P4G doesn't allocate as much RAM as some other games.**
<br>
<br>

![Input](https://user-images.githubusercontent.com/12598379/46871633-3d7ba480-ce32-11e8-946f-fc433474ab8b.png)
**It also polls for touchscreen input (although it doesn't actually use rear touchpad in game).**
