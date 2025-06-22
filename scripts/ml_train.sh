#!/bin/bash

datasets_dir=""
out_dir=""
model=""
task_out=""
add_desc=""

usage() {
    echo "Usage: bash $0 -d datasets_dir -o out_dir -m model -a add_desc [-t task_out]"
    exit 1
}

# Parse options
while getopts "d:o:m:a:t:" opt; do
    case $opt in
        d) datasets_dir="$OPTARG" ;;
        o) out_dir="$OPTARG" ;;
        m) model="$OPTARG" ;;
        t) task_out="$OPTARG" ;;
        a) add_desc="$OPTARG" ;;
        *) usage ;;
    esac
done

if [[ -z $task_out ]]; then
    task_out="$HOME/train-$model.taskfile"
fi
if [ -n "$add_desc" ]; then
    add_desc=",$add_desc"
fi

if [ -z "$datasets_dir" ] || [ -z "$out_dir" ] || [ -z "$model" ]; then
    usage
fi

shopt -s nullglob
echo "" > $task_out

train() {
    descs="$1"
    model_desc="$2"
    if [ -z "$model_desc" ]; then
        model_desc=$descs
    fi
    ram_usage=48

    train_datasets=""
    for desc in $descs; do
        echo "desc: $desc"
        for file in $datasets_dir/*\["$desc,train"\].csv; do
            train_datasets+="'$file',"
        done
    done
    echo "train_datasets: $train_datasets"
    if [ -z "$train_datasets" ]; then
        return
    fi
    train_datasets=${train_datasets%?}

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
    test_datasets=${test_datasets%?}

    echo "shell:1:$ram_usage:8:cd $HOME/OptimalClockOffline/python/ML && \
python -c \"\
import $model as m;\
m.SetupModel();\
m.AddDatasets($train_datasets);\
m.LoadDatasets();\
m.SetTrainData();\
m.Train();\
m.ExportONNX('$out_dir/$model[$model_desc].onnx');\
m.ResetDatasets();\
m.AddDatasets($test_datasets);\
m.LoadDatasets();\
m.SetTestData();\
m.Test()\" > $out_dir/$model[$model_desc].md" >> $task_out
}

relative_size=(0.001 0.01 0.1 0.2 0.4)
for size in ${relative_size[@]}; do
    train "$size$add_desc"
    train "$size,ignore_obj_size$add_desc"
done

descs=""
for s in ${relative_size[@]}; do
    descs+="$s$add_desc "
done
train "$descs" "All$add_desc"

descs=""
for s in ${relative_size[@]}; do
    descs+="$s,ignore_obj_size$add_desc "
done
train "$descs" "All,ignore_obj_size$add_desc"

echo "Task Generated: $task_out"
