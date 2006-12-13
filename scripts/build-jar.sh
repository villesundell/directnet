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

CFLAGS="$CFLAGS -DNESTEDVM" \
CXXFLAGS="$CXXFLAGS -DNESTEDVM" \
./configure --host=mips-unknown-elf --build=`./scripts/config.guess` --enable-ui=dumb || exit 1
make || exit 1

java -cp $1/build:$1/upstream/build/classgen/build org.ibex.nestedvm.Compiler \
  -outfile DirectNet.class -o unixRuntime DirectNet src/directnet || exit 1

if [ ! -e org ]
then
    cp -a $1/build/org .
    cp -a $1/upstream/build/classgen/build/org .
fi

echo 'Manifest-Version: 1.0
Main-Class: DirectNet
Classpath: DirectNet.jar' > DirectNet.manifest
jar -cfm DirectNet.jar DirectNet.manifest DirectNet.class org/ || exit 1

