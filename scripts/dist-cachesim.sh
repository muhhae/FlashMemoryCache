#!/bin/bash

relative_cache_sizes=(0.005 0.01 0.1 0.25 0.5)

usage() {
    echo "Usage: bash $0 -r t[r]aces_txt -d traces_[d]ir -o [o]ut_dir -a [a]dd_desc -t [t]ask_out -g al[g]orithm -p add_[p]aram -i [i]ignore_obj_size -f [f]orce_replace"
    exit 1
}

ignore_obj_size=false
force_replace=false

while getopts "fid:o:a:t:r:g:p:" opt; do
    case $opt in
        d) traces_dir="$OPTARG" ;;
        o) out_dir="$OPTARG" ;;
        t) task_out="$OPTARG" ;;
        r) traces_txt="$OPTARG" ;;
        a) add_desc="$OPTARG" ;;
        g) algorithm="$OPTARG" ;;
        p) add_param="$OPTARG" ;;
        i) ignore_obj_size=true ;;
        f) force_replace=true ;;
        *) usage ;;
    esac
done

if [ -n "$add_desc" ]; then
    add_desc=",$add_desc"
fi
if [ -z "$traces_dir" ] || [ -z "$out_dir" ] || [ -z "$traces_txt" ] || [ -z "$algorithm" ]; then
    usage
fi

echo "" > $task_out

while IFS= read -r link; do
    if [ -z "$link" ] || [[ "$link" == \#* ]]; then
        continue
    fi

    filename=$(basename $link)
    file="$traces_dir/$filename"

    if [ ! -f "$file" ]; then
        echo "File '$file' does not exist."
        continue
    fi

    basename="${filename%%.oracleGeneral*}"
    size=$(stat --format="%s" "$file")

    gb=$(( (size + 1024*1024*1024 - 1) / (1024*1024*1024) ))
    min_dram=$(( gb*2+1 ))

    for cache_size in "${relative_cache_sizes[@]}"; do
        if $ignore_obj_size; then
            output_path="$out_dir/log/$basename[$cache_size,ignore_obj_size,$algorithm$add_desc].json"
            if [ ! -s "$output_path" ] || $force_replace; then
                echo "$output_path doesn't exist, running!"
                echo "shell:1:$min_dram:1:~/FlashMemoryCache/build/cacheSimulator $file -a $algorithm $add_param -o $out_dir -r $cache_size --ignore-obj-size -d ignore_obj_size,$algorithm$add_desc" >> $task_out
            else
                echo "$output_path exist, skipping!"
            fi
        else
            output_path="$out_dir/log/$basename[$cache_size,$algorithm$add_desc].json"
            if [ ! -s "$output_path" ] || $force_replace; then
                echo "$output_path doesn't exist, running!"
                echo "shell:1:$min_dram:1:~/FlashMemoryCache/build/cacheSimulator $file -a $algorithm $add_param -o $out_dir -r $cache_size -d $algorithm$add_desc" >> $task_out
            else
                echo "$output_path exist, skipping!"
            fi
        fi
    done
done < "$traces_txt"
