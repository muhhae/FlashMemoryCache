relative_cache_sizes=(0.001 0.01 0.1 0.2 0.4)
max_iteration=1

traces_dir=""
traces_in=""
result_out=""
task_out=""
add_desc=""

usage() {
    echo "Usage: $0 [-d traces_dir] [-i traces_in] [-r result_out] [-t task_out] [-a add_desc]"
    exit 1
}

while getopts "d:i:r:t:a:" opt; do
    case $opt in
        d) traces_dir="$OPTARG" ;;
        i) traces_in="$OPTARG" ;;
        r) result_out="$OPTARG" ;;
        t) task_out="$OPTARG" ;;
        a) add_desc="$OPTARG" ;;
        *) usage ;;
    esac
done

if [ -z "$traces_dir" ] || [ -z "$traces_in" ] || [ -z "$result_out" ] || [ -z "$task_out" ]; then
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
        echo "shell:1:$min_dram:1:~/OptimalClockOffline/build/cacheSimulator $file -o $result_out -a dist-optimal --id-num -r $cache_size -i $max_iteration --generate-datasets --ignore-obj-size -d ignore_obj_size dist_optimal $add_desc" >> $task_out
        echo "shell:1:$min_dram:1:~/OptimalClockOffline/build/cacheSimulator $file -o $result_out -a dist-optimal --id-num -r $cache_size -i $max_iteration --generate-datasets -d dist_optimal $add_desc" >> $task_out
    done
done < "$traces_in"
