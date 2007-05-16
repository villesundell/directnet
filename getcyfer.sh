#!/bin/sh
cd "`dirname $0`"
cd src/libdirectnet/enc-cyfer

wget --passive-ftp http://software.senko.net/projects/cyfer/sources/cyfer-0.6.0.tar.gz || exit 1
gunzip -c cyfer-0.6.0.tar.gz | tar -xf - || exit 1
mv cyfer-0.6.0 cyfer

cd cyfer
patch -p1 < ../cyfer-0.6.0.dn.diff
