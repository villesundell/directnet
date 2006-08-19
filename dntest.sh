#!/bin/bash
cdown() {
        from=$1
        for (( j = 0; $j < $from; j++ ))
        do
                echo -en $(( from - j ))'     \r'
                sleep 1
        done
        echo 0
}

./src/a.out &
cdown 5

for (( i = 0; $i < 25; i++ ))
do
        ./src/directnet -n $i >& dht_$i.out &
        cdown 30
        kill -10 %1
        cdown 5
        mv -f dht.ps dht$i.ps >& /dev/null
        mv -f dht3.dot dht_$i.dot >& /dev/null
done

kill %1
killall directnet
