#!/bin/bash

ignore_obj_size=true
traces_dir=/ltdata/data/oracleReuse
task_out=~/task

> $task_out

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
    for p in 0.1 0.2 0.3 0.4 0.5; do
        echo "shell:$priority:$min_dram:1:cd /mnt/nfs/lazy_promotions/new_results/delay && ~/bobCacheSim/build/bin/cachesim $file oracleGeneral delay -e delay-time=$p 0.01 --ignore-obj-size 1" >> $task_out
    done
    # echo "shell:$priority:$min_dram:1:cd /mnt/nfs/lazy_promotions/new_results/prob && ~/bob-cachesim/build/bin/cachesim $file oracleGeneral prob -e prob=$p 0.01 --ignore-obj-size 1" >> $task_out
    # echo "shell:$priority:$min_dram:1:cd /mnt/nfs/results/batch && ~/bob-cachesim/build/bin/cachesim $file oracleGeneral batch -e batch-size=0.5 0.01 --ignore-obj-size 1" >> $task_out

done < ~/FlashMemoryCache/trace/all_datasets.txt
