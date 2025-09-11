#!/bin/bash

ignore_obj_size=true
traces_dir=/ltdata/data/oracleReuse
out_dir=/mnt/nfs/lazy_promotions/simulation_results/hashed
task_out=~/task

> $task_out

mkdir -p /mnt/nfs/results/prob
mkdir -p /mnt/nfs/results/delay
mkdir -p /mnt/nfs/results/batch

while IFS= read -r path; do
    if [ -z "$path" ] || [[ "$path" == \#* ]]; then
        continue
    fi

    file="$traces_dir/$path"

    if [ ! -f "$file" ]; then
        echo "File '$file' does not exist."
        continue
    fi

    size=$(stat --format="%s" "$file")
    gb=$(( (size + 1024*1024*1024 - 1) / (1024*1024*1024) ))
    min_dram=$(( gb/4+1 ))
    priority=$(( 100/gb + 1 ))

    echo "shell:$priority:$min_dram:1:cd /mnt/nfs/results/prob  && ~/bob-cachesim/build/bin/cachesim $file oracleGeneral prob -e prob=0.5 0.01 --ignore-obj-size 1" >> $task_out
    echo "shell:$priority:$min_dram:1:cd /mnt/nfs/results/batch && ~/bob-cachesim/build/bin/cachesim $file oracleGeneral batch -e batch-size=0.5 0.01 --ignore-obj-size 1" >> $task_out
    echo "shell:$priority:$min_dram:1:cd /mnt/nfs/results/delay && ~/bob-cachesim/build/bin/cachesim $file oracleGeneral delay -e delay-time=0.5 0.01 --ignore-obj-size 1" >> $task_out

done < ~/FlashMemoryCache/trace/all_datasets.txt
