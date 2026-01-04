#!/bin/bash

ignore_obj_size=true
traces_dir=/ltdata/data/oracleReuse
task_out=~/task

rm $task_out
touch $task_out

for p in age delayfr lru fifo clock random arc_prob arc_lru arc_delay arc_batch arc_fr twoq_prob twoq_lru twoq_delay twoq_batch twoq_fr; do
    mkdir -p /mnt/nfs/lazy_promotions/new_results/$p
done

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

    # echo "shell:$priority:$min_dram:1:cd /mnt/nfs/lazy_promotions/new_results/age && ~/bobCacheSim/build/bin/cachesim $file oracleGeneral age -e scaler=1 0.01 --ignore-obj-size 1" >> $task_out
    # for p in 0.1 0.2 0.3 0.4 0.5; do
    #     echo "shell:$priority:$min_dram:1:cd /mnt/nfs/lazy_promotions/new_results/delay && ~/bobCacheSim/build/bin/cachesim $file oracleGeneral lru-delay -e delay-time=$p 0.01 --ignore-obj-size 1" >> $task_out
    # done
    # for p in 0.01 0.05 0.1 0.2 0.4 0.6 0.8 0.9; do
    #     echo "shell:$priority:$min_dram:1:cd /mnt/nfs/lazy_promotions/new_results/prob && ~/bobCacheSim/build/bin/cachesim $file oracleGeneral lru-prob -e prob=$p 0.01 --ignore-obj-size 1" >> $task_out
    # done
    # for p in 0.1 0.2 0.3 0.4 0.6 0.7 0.8 0.9; do
    #     echo "shell:$priority:$min_dram:1:cd /mnt/nfs/lazy_promotions/new_results/batch && ~/bobCacheSim/build/bin/cachesim $file oracleGeneral batch -e batch-size=$p 0.01 --ignore-obj-size 1" >> $task_out
    # done
    # for p in 0.6 0.7 0.8 0.9; do
    #     echo "shell:$priority:$min_dram:1:cd /mnt/nfs/lazy_promotions/new_results/delay && ~/bobCacheSim/build/bin/cachesim $file oracleGeneral lru-delay -e delay-time=$p 0.01 --ignore-obj-size 1" >> $task_out
    # done
    # for p in 1 2 3 4 5 10 15 20; do
    #     echo "shell:$priority:$min_dram:1:cd /mnt/nfs/lazy_promotions/new_results/clock && ~/bobCacheSim/build/bin/cachesim $file oracleGeneral clock -e n-bit-counter=$p 0.01 --ignore-obj-size 1" >> $task_out
    # done
    for p in 1 2 4 8 16 32 64 128; do
        echo "shell:$priority:$min_dram:1:cd /mnt/nfs/lazy_promotions/new_results/random_lru && ~/bobCacheSim/build/bin/cachesim $file oracleGeneral randomlru -e n-samples=$p 0.01 --ignore-obj-size 1" >> $task_out
    done
    #
    # echo "shell:$priority:$min_dram:1:cd /mnt/nfs/lazy_promotions/new_results/arc_prob && ~/bobCacheSim/build/bin/cachesim $file oracleGeneral arc-prob 0.01 --ignore-obj-size 1" >> $task_out
    # echo "shell:$priority:$min_dram:1:cd /mnt/nfs/lazy_promotions/new_results/arc_lru && ~/bobCacheSim/build/bin/cachesim $file oracleGeneral arc-lru 0.01 --ignore-obj-size 1" >> $task_out
    # echo "shell:$priority:$min_dram:1:cd /mnt/nfs/lazy_promotions/new_results/arc_delay && ~/bobCacheSim/build/bin/cachesim $file oracleGeneral arc-delay 0.01 --ignore-obj-size 1" >> $task_out
    # echo "shell:$priority:$min_dram:1:cd /mnt/nfs/lazy_promotions/new_results/arc_batch && ~/bobCacheSim/build/bin/cachesim $file oracleGeneral arc-batch 0.01 --ignore-obj-size 1" >> $task_out
    # echo "shell:$priority:$min_dram:1:cd /mnt/nfs/lazy_promotions/new_results/arc_fr && ~/bobCacheSim/build/bin/cachesim $file oracleGeneral arc-fr 0.01 --ignore-obj-size 1" >> $task_out
    #
    # echo "shell:$priority:$min_dram:1:cd /mnt/nfs/lazy_promotions/new_results/twoq_prob && ~/bobCacheSim/build/bin/cachesim $file oracleGeneral twoq-prob 0.01 --ignore-obj-size 1" >> $task_out
    # echo "shell:$priority:$min_dram:1:cd /mnt/nfs/lazy_promotions/new_results/twoq_lru && ~/bobCacheSim/build/bin/cachesim $file oracleGeneral twoq-lru 0.01 --ignore-obj-size 1" >> $task_out
    # echo "shell:$priority:$min_dram:1:cd /mnt/nfs/lazy_promotions/new_results/twoq_delay && ~/bobCacheSim/build/bin/cachesim $file oracleGeneral twoq-delay 0.01 --ignore-obj-size 1" >> $task_out
    # echo "shell:$priority:$min_dram:1:cd /mnt/nfs/lazy_promotions/new_results/twoq_batch && ~/bobCacheSim/build/bin/cachesim $file oracleGeneral twoq-batch 0.01 --ignore-obj-size 1" >> $task_out
    # echo "shell:$priority:$min_dram:1:cd /mnt/nfs/lazy_promotions/new_results/twoq_fr && ~/bobCacheSim/build/bin/cachesim $file oracleGeneral twoq-fr 0.01 --ignore-obj-size 1" >> $task_out
    #
    # echo "shell:$priority:$min_dram:1:cd /mnt/nfs/lazy_promotions/new_results/fifo && ~/bobCacheSim/build/bin/cachesim $file oracleGeneral fifo 0.01 --ignore-obj-size 1" >> $task_out
    # for p in 0.01 0.05 0.1 0.2 0.4 0.5; do
    #     echo "shell:$priority:$min_dram:1:cd /mnt/nfs/lazy_promotions/new_results/delayfr && ~/bobCacheSim/build/bin/cachesim $file oracleGeneral delayfr -e delay-ratio=$p 0.01 --ignore-obj-size 1" >> $task_out
    # done

    for p in 0.16 0.2 0.25 0.3 0.4 0.5 1 1.5 2 2.5; do
        echo "shell:$priority:$min_dram:1:cd /mnt/nfs/lazy_promotions/new_results/age && ~/bobCacheSim/build/bin/cachesim $file oracleGeneral age -e scaler=$p 0.01 --ignore-obj-size 1" >> $task_out
    done

done < ~/FlashMemoryCache/trace/all_datasets.txt
