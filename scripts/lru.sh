#!/bin/bash

relative_cache_sizes=(0.001 0.01 0.1 0.2 0.4)

usage() {
    echo "Usage: bash $0 -r traces_txt -d traces_dir -o out_dir -a add_desc -t task_out"
    exit 1
}

# Parse options
while getopts "d:o:a:t:r:" opt; do
    case $opt in
        d) traces_dir="$OPTARG" ;;
        o) out_dir="$OPTARG" ;;
        t) task_out="$OPTARG" ;;
        r) traces_txt="$OPTARG" ;;
        a) add_desc="$OPTARG" ;;
        *) usage ;;
    esac
done
if [ -n "$add_desc" ]; then
    add_desc=",$add_desc"
fi
if [ -z "$traces_dir" ] || [ -z "$out_dir" ] || [ -z "$traces_txt" ]; then
    usage
fi

echo "" > $task_out

while IFS= read -r link; do
    if [ -z "$link" ] || [[ "$link" == \#* ]]; then
        continue
    fi
    filename=$(basename $link)
    file="$traces_dir/$filename"
    basename="${filename%%.oracleGeneral*}"
    size=$(stat --format="%s" "$file")

    gb=$(( (size + 1024*1024*1024 - 1) / (1024*1024*1024) ))
    min_dram=$(( gb+1 ))

    for cache_size in "${relative_cache_sizes[@]}"; do
        echo "shell:1:$min_dram:1:~/OptimalClockOffline/build/cacheSimulator $file $features -o $out_dir -r $cache_size -i 1 --ignore-obj-size -d ignore_obj_size,lru$add_desc -a lru" >> $task_out
        echo "shell:1:$min_dram:1:~/OptimalClockOffline/build/cacheSimulator $file $features -o $out_dir -r $cache_size -i 1 -d lru$add_desc -a lru" >> $task_out
    done
done < "$traces_txt"
