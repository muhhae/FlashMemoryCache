#!/bin/bash

usage() {
    echo "Usage: bash $0 -r t[r]aces_txt -d traces_[d]ir -o [o]ut_dir -a [a]dd_desc -t [t]ask_out -g al[g]orithm -p add_[p]aram -i [i]ignore_obj_size -f [f]orce_replace -s cache_[s]ize"
    exit 1
}

ignore_obj_size=false
force_replace=false

while getopts "fid:o:a:t:r:g:p:s:" opt; do
    case $opt in
        d) traces_dir="$OPTARG" ;;
        o) out_dir="$OPTARG" ;;
        t) task_out="$OPTARG" ;;
        r) traces_txt="$OPTARG" ;;
        a) add_desc="$OPTARG" ;;
        g) algorithm="$OPTARG" ;;
        p) add_param="$OPTARG" ;;
        s) cache_size="$OPTARG" ;;
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

while IFS= read -r path; do
    if [ -z "$path" ] || [[ "$path" == \#* ]]; then
        continue
    fi

    file="$traces_dir/$path"

    if [ ! -f "$file" ]; then
        echo "File '$file' does not exist."
        continue
    fi

    base="${path%%.oracleGeneral*}"

    desc_name=${base//\//%2F}
    desc="$add_desc,path=$desc_name"

    basename=$(basename "$base")

    if gdbmtool "cache.gdbm" exists "$file"; then
        echo "fetch size: $size"
        size=$(gdbmtool "cache.gdbm" fetch "$file")
        echo "end"
    else
        size=$(stat --format="%s" "$file")
        echo "store size: $size"
        gdbmtool "cache.gdbm" store "$file" "$size"
        echo "end"
    fi


    gb=$(( (size + 1024*1024*1024 - 1) / (1024*1024*1024) ))
    min_dram=$(( gb/4+1 ))
    priority=$(( 100/gb + 1 ))

    if $ignore_obj_size; then
        output_path="$out_dir/log/$basename[$cache_size,ignore_obj_size,$algorithm$desc].json"
        if [ ! -s "$output_path" ] || $force_replace; then
            echo "shell:$priority:$min_dram:1:~/FlashMemoryCache/build/cacheSimulator $file -r $cache_size -a $algorithm $add_param -o $out_dir --ignore-obj-size -d ignore_obj_size,$algorithm$desc" >> $task_out
        fi
    else
        output_path="$out_dir/log/$basename[$cache_size,$algorithm$desc].json"
        if [ ! -s "$output_path" ] || $force_replace; then
            echo "shell:$priority:$min_dram:1:~/FlashMemoryCache/build/cacheSimulator $file -r $cache_size -a $algorithm $add_param -o $out_dir -d $algorithm$desc" >> $task_out
        fi
    fi
done < "$traces_txt"
