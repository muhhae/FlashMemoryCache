relative_cache_sizes=(0.001 0.01 0.1 0.2 0.4)
ml_treshold=(0.3 0.5 0.6 0.7 0.8 0.9)
max_iteration=1
basedir="/mnt/v0"

traces_txt="$1"

usage() {
    echo "Usage: bash $0 -r traces_txt -d traces_dir -o out_dir -m model -a add_desc -t task_out"
    exit 1
}

# Parse options
while getopts "d:o:m:a:t:r:" opt; do
    case $opt in
        d) traces_dir="$OPTARG" ;;
        o) out_dir="$OPTARG" ;;
        m) model="$OPTARG" ;;
        t) task_out="$OPTARG" ;;
        r) traces_txt="$OPTARG" ;;
        a) add_desc="$OPTARG" ;;
        *) usage ;;
    esac
done
if [ -n "$add_desc" ]; then
    add_desc=",$add_desc"
fi
if [ -z "$traces_dir" ] || [ -z "$out_dir" ] || [ -z "$model" ] || [ -z "$traces_txt" ]; then
    usage
fi


features=""
case "$model" in
    "LR_1")
        features="-I F32 -F rtime_since vtime_since clock_freq lifetime_freq"
        ;;
    "LR_1_robust_scaler")
        features="-I F32 -F rtime_since vtime_since clock_freq lifetime_freq"
        ;;
    "LR_1_std_scaler")
        features="-I F32 -F rtime_since vtime_since clock_freq lifetime_freq"
        ;;
    "LR_1_std_scaler_imba")
        features="-I F32 -F rtime_since vtime_since clock_freq lifetime_freq"
        ;;
    "LR_1_log")
        features="-I F32 -F rtime_since_log vtime_since_log clock_freq_log lifetime_freq_log"
        ;;
    "LR_1_mean")
        features="-I F32 -F rtime_since_log vtime_since_log clock_freq_std lifetime_freq_std"
        ;;
    "LR_2")
        features="-I F32 -F vtime_since clock_freq"
        ;;
    "LR_2_log")
        features="-I F32 -F vtime_since_log clock_freq_log"
        ;;
    "LR_2_mean")
        features="-I F32 -F vtime_since_log clock_freq_std"
        ;;
    "LR_3")
        features="-I F32 -F rtime_since clock_freq"
        ;;
    "LR_3_log")
        features="-I F32 -F rtime_since_log clock_freq_log"
        ;;
    "LR_3_mean")
        features="-I F32 -F rtime_since_log clock_freq_std"
        ;;
    "LR_4")
        features="-I F32 -F rtime_since vtime_since rtime_between clock_freq lifetime_freq"
        ;;
    "LR_4_std_scaler")
        features="-I F32 -F rtime_since vtime_since rtime_between clock_freq lifetime_freq"
        ;;
    "LR_4_std_scaler_imba")
        features="-I F32 -F rtime_since vtime_since rtime_between clock_freq lifetime_freq"
        ;;
    "LR_4_robust_scaler")
        features="-I F32 -F rtime_since vtime_since rtime_between clock_freq lifetime_freq"
        ;;
    "LR_4_log")
        features="-I F32 -F rtime_since_log vtime_since_log rtime_between_log clock_freq_log lifetime_freq_log"
        ;;
    "LR_4_mean")
        features="-I F32 -F rtime_since_log vtime_since_log rtime_between_log clock_freq_std lifetime_freq_std"
        ;;
    "LR_5")
        features="-I F32 -F rtime_since rtime_between clock_freq lifetime_freq"
        ;;
    "LR_5_imba")
        features="-I F32 -F rtime_since rtime_between clock_freq lifetime_freq"
        ;;
    "LR_6")
        features="-I F32 -F rtime_since rtime_between clock_freq lifetime_freq"
        ;;
    "LR_6_imba")
        features="-I F32 -F rtime_since rtime_between clock_freq lifetime_freq"
        ;;
    "LR_7")
        features="-I F32 -F rtime_since_log vtime_since_log clock_freq lifetime_freq"
        ;;
    "LR_7_decay_rtime")
        features="-I F32 -F rtime_since_log vtime_since_log clock_freq_decayed_rtime lifetime_freq_decayed_rtime"
        ;;
    "LR_7_decay_vtime")
        features="-I F32 -F rtime_since_log vtime_since_log clock_freq_decayed_vtime lifetime_freq_decayed_vtime"
        ;;
    "LR_8")
        features="-I F32 -F rtime_since_log clock_freq lifetime_freq"
        ;;
    "LR_8_decay_rtime")
        features="-I F32 -F rtime_since_log clock_freq_decayed_rtime lifetime_freq_decayed_rtime"
        ;;
    "LR_8_decay_vtime")
        features="-I F32 -F rtime_since_log clock_freq_decayed_vtime lifetime_freq_decayed_vtime"
        ;;
    "LR_9")
        features="-I F32 -F vtime_since_log clock_freq lifetime_freq"
        ;;
    "LR_9_decay_rtime")
        features="-I F32 -F vtime_since_log clock_freq_decayed_rtime lifetime_freq_decayed_rtime"
        ;;
    "LR_9_decay_vtime")
        features="-I F32 -F vtime_since_log clock_freq_decayed_vtime lifetime_freq_decayed_vtime"
        ;;
    "LR_decay_rtime")
        features="-I F32 -F clock_freq_decayed_rtime"
        ;;
    "LR_decay_vtime")
        features="-I F32 -F clock_freq_decayed_vtime"
        ;;
    "LR_7_w_0_5")
        features="-I F32 -F rtime_since_log vtime_since_log clock_freq lifetime_freq"
        ;;
    "LR_7_decay_rtime_w_0_5")
        features="-I F32 -F rtime_since_log vtime_since_log clock_freq_decayed_rtime lifetime_freq_decayed_rtime"
        ;;
    "LR_7_decay_vtime_w_0_5")
        features="-I F32 -F rtime_since_log vtime_since_log clock_freq_decayed_vtime lifetime_freq_decayed_vtime"
        ;;
    "LR_7_w_0_75")
        features="-I F32 -F rtime_since_log vtime_since_log clock_freq lifetime_freq"
        ;;
    "LR_7_decay_rtime_w_0_75")
        features="-I F32 -F rtime_since_log vtime_since_log clock_freq_decayed_rtime lifetime_freq_decayed_rtime"
        ;;
    "LR_7_decay_vtime_w_0_75")
        features="-I F32 -F rtime_since_log vtime_since_log clock_freq_decayed_vtime lifetime_freq_decayed_vtime"
        ;;
    "LR_8_w_0_5")
        features="-I F32 -F rtime_since_log clock_freq lifetime_freq"
        ;;
    "LR_8_decay_rtime_w_0_5")
        features="-I F32 -F rtime_since_log clock_freq_decayed_rtime lifetime_freq_decayed_rtime"
        ;;
    "LR_8_decay_vtime_w_0_5")
        features="-I F32 -F rtime_since_log clock_freq_decayed_vtime lifetime_freq_decayed_vtime"
        ;;
    "LR_8_w_0_75")
        features="-I F32 -F rtime_since_log clock_freq lifetime_freq"
        ;;
    "LR_8_decay_rtime_w_0_75")
        features="-I F32 -F rtime_since_log clock_freq_decayed_rtime lifetime_freq_decayed_rtime"
        ;;
    "LR_8_decay_vtime_w_0_75")
        features="-I F32 -F rtime_since_log clock_freq_decayed_vtime lifetime_freq_decayed_vtime"
        ;;
    "LR_9_w_0_5")
        features="-I F32 -F vtime_since_log clock_freq lifetime_freq"
        ;;
    "LR_9_decay_rtime_w_0_5")
        features="-I F32 -F vtime_since_log clock_freq_decayed_rtime lifetime_freq_decayed_rtime"
        ;;
    "LR_9_decay_vtime_w_0_5")
        features="-I F32 -F vtime_since_log clock_freq_decayed_vtime lifetime_freq_decayed_vtime"
        ;;
    "LR_9_w_0_75")
        features="-I F32 -F vtime_since_log clock_freq lifetime_freq"
        ;;
    "LR_9_decay_rtime_w_0_75")
        features="-I F32 -F vtime_since_log clock_freq_decayed_rtime lifetime_freq_decayed_rtime"
        ;;
    "LR_9_decay_vtime_w_0_75")
        features="-I F32 -F vtime_since_log clock_freq_decayed_vtime lifetime_freq_decayed_vtime"
        ;;
    "LR_decay_rtime_w_0_5")
        features="-I F32 -F clock_freq_decayed_rtime"
        ;;
    "LR_decay_vtime_w_0_5")
        features="-I F32 -F clock_freq_decayed_vtime"
        ;;
    "LR_decay_rtime_w_0_75")
        features="-I F32 -F clock_freq_decayed_rtime"
        ;;
    "LR_decay_vtime_w_0_75")
        features="-I F32 -F clock_freq_decayed_vtime"
        ;;
    "LR_7_id")
        features="-I F32 -F obj_id rtime_since_log vtime_since_log clock_freq lifetime_freq"
        ;;
    "LR_8_id")
        features="-I F32 -F obj_id rtime_since_log clock_freq lifetime_freq"
        ;;
    "LR_9_id")
        features="-I F32 -F obj_id vtime_since_log clock_freq lifetime_freq"
        ;;
    "LR_id")
        features="-I F32 -F obj_id"
        ;;
    *)
        echo "Unknown model"; exit 1;
        ;;
esac

model_dir="$HOME/OptimalClockOffline/python/ML/model/$model"

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
        for treshold in "${ml_treshold[@]}";do
            echo "shell:1:$min_dram:1:~/OptimalClockOffline/build/cacheSimulator $file $features -o $out_dir --id-num -r $cache_size -i 1 -H $treshold --ignore-obj-size -d ignore_obj_size,ML$add_desc,model=${model}_$cache_size,treshold=$treshold -a ML -m $model_dir[$cache_size,ignore_obj_size$add_desc].onnx" >> $task_out
            echo "shell:1:$min_dram:1:~/OptimalClockOffline/build/cacheSimulator $file $features -o $out_dir --id-num -r $cache_size -i 1 -H $treshold --ignore-obj-size -d ignore_obj_size,ML$add_desc,model=${model}_All,treshold=$treshold -a ML -m $model_dir[All,ignore_obj_size$add_desc].onnx" >> $task_out
            echo "shell:1:$min_dram:1:~/OptimalClockOffline/build/cacheSimulator $file $features -o $out_dir --id-num -r $cache_size -i 1 -H $treshold -d ML$add_desc,model=${model}_$cache_size,treshold=$treshold -a ML -m $model_dir[$cache_size$add_desc].onnx" >> $task_out
            echo "shell:1:$min_dram:1:~/OptimalClockOffline/build/cacheSimulator $file $features -o $out_dir --id-num -r $cache_size -i 1 -H $treshold -d ML$add_desc,model=${model}_All,treshold=$treshold -a ML -m $model_dir[All$add_desc].onnx" >> $task_out
        done
    done
done < "$traces_txt"
