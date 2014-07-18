#!/bin/sh
bsub -n $[12*1] -m "compute004" -R 'span[ptile=12]' -o /home/trausti12/mimir/syslab-cache/bsub-output.txt '/home/trausti12/mimir/syslab-cache/time_all.sh'
