#!/bin/bash

mkdir m4
aclocal -I m4
libtoolize --copy --force --install --automake
aclocal -I m4
autoconf
autoheader
automake --add-missing --copy
