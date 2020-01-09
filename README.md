# llwm
Small window manager for X11 with Xlib.h i've been working on for fun. Been using [dwm](https://github.com/fanglingsu/dwm) and [tinywm](https://github.com/mackstann/tinywm) to reference for help
You'll need the libx11-dev package\
`sudo apt install libx11-dev`\
then just make an Xsession for the executable. you'll also want to install dmenu to make life easier. or just use a real wm

#### Bindings
`Mod4 + any number` will take you to a new workspace (e.g. Mod4 + 3 to go to workspace 3)\
`Mod4 + Shift + any number` to send a window to another workspace (e.g. Mod4 + Shift + 3 to send to workspace 3)\
`Mod4 + Shift + Q` to close a window\
`Mod4 + F` to maximize a window to the height and width of a screen\
`Mod4 + F1` will raise a window to the stop of the stack\
`Mod4 + Enter` opens your default terminal\
`Mod4 + D` starts dmenu
