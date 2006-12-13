#!/bin/bash -x
if [ ! "$1" ]
then
    echo 'Use: scripts/build-jar.sh <path to nestedvm>'
    exit 1
fi

if [ ! -e "$1/env.sh" ]
then
    echo 'You must create an env.sh in the NestedVM directory with `make env.sh`'
    exit 1
fi
. $1/env.sh

#CFLAGS="$CFLAGS -DNESTEDVM" \
#CXXFLAGS="$CXXFLAGS -DNESTEDVM" \
#./configure --host=mips-unknown-elf --build=`./scripts/config.guess` --enable-ui=java || exit 1
#make || exit 1

java -cp $1/build:$1/upstream/build/classgen/build org.ibex.nestedvm.Compiler \
  -outfile src/ui-java/net/imdirect/directnet/DirectNet.class \
  -o unixRuntime,supportCall net.imdirect.directnet.DirectNet src/directnet || exit 1

cd src/ui-java || exit 1
javac net/imdirect/directnet/UI.java || exit 1
cd ../..

if [ ! -e org ]
then
    cp -a $1/build/org .
    cp -a $1/upstream/build/classgen/build/org .
fi

echo 'Manifest-Version: 1.0
Main-Class: net.imdirect.directnet.UI
Classpath: DirectNet.jar' > DirectNet.manifest
jar -cfm DirectNet.jar DirectNet.manifest org/ \
  -C src/ui-java net/imdirect/directnet/DirectNet.class \
  -C src/ui-java net/imdirect/directnet/UI.class \
  -C src/ui-java net/imdirect/directnet/UI\$1.class || exit 1

