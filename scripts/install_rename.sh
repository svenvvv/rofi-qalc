#!/bin/sh

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 libdir basename"
    exit 1
fi

if [ -z "$MESON_INSTALL_DESTDIR_PREFIX" ]; then
    echo "MESON_INSTALL_DESTDIR_PREFIX env. var must be set"
    exit 1
fi

libdir="$1"
libname="$2"
libpath="$MESON_INSTALL_DESTDIR_PREFIX/$libdir"

mv "$libpath/lib$libname.so" "$libpath/qalc.so"
