#!/bin/sh
cd "`dirname $0`"
cd src/enc-cyfer

wget --passive-ftp ftp://ftp.gnu.org/gnu/gmp/gmp-4.1.4.tar.bz2 || exit 1
bunzip2 -c gmp-4.1.4.tar.bz2 | tar -xf - || exit 1
mv gmp-4.1.4 gmp

wget --passive-ftp http://software.senko.net/projects/cyfer/sources/cyfer-0.6.0.tar.gz || exit 1
gunzip -c cyfer-0.6.0.tar.gz | tar -xf - || exit 1
mv cyfer-0.6.0 cyfer

cd cyfer
patch -p1 < ../cyfer-0.6.0.dn.diff
