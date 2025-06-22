#!/bin/bash

datasets_dir=""
model_dir=""
model=""
task_out=""
add_desc=""
output_desc=""
obj_count=""

usage() {
    echo "Usage: bash $0 -d datasets_dir -o model_dir -m model -M model_desc -a add_desc -c obj_count [-t task_out]"
    exit 1
}

# Parse options
while getopts "d:o:m:a:t:M:c:" opt; do
    case $opt in
        d) datasets_dir="$OPTARG" ;;
        o) model_dir="$OPTARG" ;;
        m) model="$OPTARG" ;;
        t) task_out="$OPTARG" ;;
        a) add_desc="$OPTARG" ;;
        M) model_desc="$OPTARG" ;;
        c) obj_count="$OPTARG" ;;
        *) usage ;;
    esac
done

if [[ -z $task_out ]]; then
    task_out="$HOME/metrics-$model.taskfile"
fi

if [ -n "$add_desc" ]; then
    add_desc=",$add_desc"
fi

if [ -n "$model_desc" ]; then
    model_desc=",$model_desc"
fi

if [ -z "$datasets_dir" ] || [ -z "$model_dir" ] || [ -z "$model" ]; then
    usage
fi

shopt -s nullglob
echo "" > $task_out


Metrics() {
    descs="$1"
    model_desc="$2"
    if [ -z "$model_desc" ]; then
        model_desc=$descs
    fi
    test_datasets=""
    for desc in $descs; do
        echo "desc: $desc"
        for file in $datasets_dir/*\["$desc,test"\].csv; do
            test_datasets+="'$file',"
        done
    done
    echo "test_datasets: $test_datasets"
    if [ -z "$test_datasets" ]; then
        return
    fi
    ram_usage=32
    cpu_usage=4
    test_datasets=${test_datasets%?}
    treshold=(0.3 0.5 0.6 0.7 0.8 0.9)
    part=(0.01 0.1 0.2 0.4 0.8 0.9 1)
    start=0
    stop=0
    for t in ${treshold[@]}; do
        stop=0
        for p in "${part[@]}"; do
            start=$stop
            stop="$p*$obj_count"
            echo "\
shell:1:$ram_usage:$cpu_usage:cd $HOME/OptimalClockOffline/python/ML && \
python -c \"\
import $model as m;\
import var;\
m.SetupModel();\
m.LoadONNX('$model_dir/$model[$model_desc].onnx');\
m.AddDatasets($test_datasets);\
m.ZipfLoadDatasets($start,$stop);\
m.SetTestData();\
m.Test($t)\" > $model_dir/$model[$model_desc,top=$p,treshold=$t].md" >> $task_out
        done
        echo "\
shell:1:$ram_usage:$cpu_usage:cd $HOME/OptimalClockOffline/python/ML && \
python -c \"\
import $model as m;\
import var;\
m.SetupModel();\
m.LoadONNX('$model_dir/$model[$model_desc].onnx');\
m.AddDatasets($test_datasets);\
m.LoadDatasets();\
m.SetTestData();\
m.Test($t)\" > $model_dir/$model[$model_desc,treshold=$t].md" >> $task_out
    done
}

relative_size=(0.001 0.01 0.1 0.2 0.4)
for size in ${relative_size[@]}; do
    Metrics "$size$add_desc"
    Metrics "$size,ignore_obj_size$add_desc"
done

descs=""
for s in ${relative_size[@]}; do
    descs+="$s$add_desc "
done
Metrics "$descs" "All$add_desc"

descs=""
for s in ${relative_size[@]}; do
    descs+="$s,ignore_obj_size$add_desc "
done
Metrics "$descs" "All,ignore_obj_size$add_desc"

echo "Task Generated: $task_out"
