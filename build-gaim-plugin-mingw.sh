#!/bin/sh
# Put the Gaim version here:
export GVER=1.2.0



if [ ! -e ../win32-dev ] 
then
	echo 'You must have the win32-dev directory immediately above this one, as from a standard Gaim Windows build.'
	exit 1
fi

if [ ! -e ../gaim-$GVER ]
then
	echo 'You must have a gaim-'$GVER' directory immediately above this one.'
	exit 1
fi

PKG_CONFIG_PATH=$PWD/scripts CFLAGS="-I$PWD/../gaim-1.2.0/src -I$PWD/../gaim-1.2.0/src/win32 -I$PWD/../win32-dev/gtk_2_0/include/glib-2.0 -I$PWD/../win32-dev/gtk_2_0/include/atk-1.0 -I$PWD/../win32-dev/gtk_2_0/include/pango-1.0 -I$PWD/../win32-dev/gtk_2_0/include/gtk-2.0 -I$PWD/../win32-dev/gtk_2_0/lib/glib-2.0/include -I$PWD/../win32-dev/gtk_2_0/lib/gtk-2.0/include" ./configure --enable-gaim-plugin --host=i686-pc-mingw32 --build=i686-pc-linux-gnu || exit 1
make all || exit 1
cd src/.libs
mkdir ext
cd ext
i686-pc-mingw32-ar x ../libdirectnet_gaim.a
i686-pc-mingw32-gcc -shared * -o ../../../libdirectnet_gaim.dll -lwsock32 -lpthread ../../../../gaim-1.2.0/win32-install-dir/gaim.dll ../../../../win32-dev/gtk_2_0/lib/libglib-2.0.dll.a || exit 1

echo 'Finisehd: the file is libdirectnet_gaim.dll'

