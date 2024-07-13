# rofi-qalc

Run calculations using libqalculate from a rofi menu.

Compared to [rofi-calc](https://github.com/svenstaro/rofi-calc), which
calls the libqalculate CLI, `rofi-qalc` interfaces with the qalculate
library.

This provides a number of benefits:

* Plotting works --- a single GNUplot window is updated in the background;
* Variables can be used --- but see the note at the end of this section;
* Marginally faster --- while not a goal of mine, maybe it matters to someone else.

Not very well tested as of yet, but give it a try if you missed any of the same
features I did.
Uses the same history file as `rofi-calc`, so if you have anything important there 
then back it up just-in-case.

**A note on variables**: since rofi doesn't provide a way to clear the filter textbox
contents then using variables is a bit iffy. 
To use variables you first must type the expression, e.g. `ten = 10` and then
clear the textbox by pressing Ctrl-U.

When you clear the textbox by deleting the text gradually then rofi will at some
point call the `rofi-qalc` mode with `ten =`, which is parsed by libqalculate as
`ten = 0`, zeroing out the variable.

Variables are not persistent throughout multiple sessions.

## Building

First install `libqalculate` and `rofi`, as this project depends on them.

```sh
# Generate project files
meson setup build \
    --buildtype=release \
    --optimization=3 \
    -Dstrip=true \
    -Dlibdir=lib/rofi \
    -Dprefix=/usr

# Compile the project
meson compile -C build
```

Arch Linux users can make use of the provided `PKGBUILD` file and 
install the resulting package.

## Running

To try out `rofi-qalc` you can provide the plugin search path as a 
command line argument, e.g.:
```sh
cd /path/to/rofi-qalc
# configure, build, etc..
rofi -plugin-path ./build -show "qalc"
```

For development purposes it might be worthwhile to enable logging
for the `rofi-qalc` module, e.g.:
```sh
G_MESSAGES_DEBUG=rq rofi -show "qalc"
```

