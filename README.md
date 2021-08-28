# lolo-wm
Small window manager for X11 with Xlib.h. Check out [dwm](https://github.com/fanglingsu/dwm), [tinywm](https://github.com/mackstann/tinywm), and [Xlib](https://www.x.org/releases/X11R7.7/doc/libX11/libX11/libX11.html) as these are what helped me get started.

You'll need the libx11-dev package\
`sudo apt-get install libx11-dev`\
Afterwards make an Xsession that uses the path to your executable. Make sure to install dmenu to make life easier. Or just use a real wm.

I am aware of how to make the event calls O(1), and it's not hard as seen in the dwm source code. So go check that out!

#### Bindings
`Mod4 + any number` will take you to a new workspace (e.g. Mod4 + 3 to go to workspace 3)\
`Mod4 + Shift + any number` to send a window to another workspace (e.g. Mod4 + Shift + 3 to send to workspace 3)\
`Mod4 + Shift + Q` to close a window\
`Mod4 + F` to maximize a window to the height and width of a screen\
`Mod4 + F1` will raise a window to the stop of the stack\
`Mod4 + Enter` opens your default terminal\
`Mod4 + D` starts dmenu
