#!/bin/bash
shopt -s lastpipe
TOTALCLIENT=0
CORRECT=0
CLIENTID_SC=""
SECRET_SC=""
CLIENTID_SS=""
SECRET_SS=""
MIN=0
MAX=0

grep "CLIENT ID" $1 | while read -r line ; do
    CLIENTID_SC=$(echo $line | cut -d" " -f3)
    SECRET_SC=$(echo $line | cut -d" " -f5)
    echo "[statClient]" $line
    TOTALCLIENT=$((TOTALCLIENT+1))
    grep -E "\|\s+[0-9]+\s+\|\s+[a-z0-9]+\s+\|\s+[0-9]+\s+\|" $2 | while read -r line2 ; do
        CLIENTID_SS=$(echo $line2 | cut -d" " -f4)
        SECRET_SS=$(echo $line2 | cut -d" " -f2)
        if [ "$CLIENTID_SC" == "$CLIENTID_SS" ] ; then
            echo "[statSupervisor] CLIENTD ID: $CLIENTID_SS SECRET: $SECRET_SS"
            MIN=$((SECRET_SC-25))
            MAX=$((SECRET_SC+25))
            if [ "$SECRET_SS" -le "$MAX" ] && [ "$SECRET_SS" -ge "$MIN" ] ; then
                echo "CORRECT"
                CORRECT=$((CORRECT+1))
            fi
        fi
    done
done
echo "TOTAL CLIENT: $TOTALCLIENT"
echo "CORRECT: $CORRECT"