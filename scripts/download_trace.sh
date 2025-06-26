#!/bin/bash

input_file="$1"
if [[ -z $input_file ]]; then
    echo "[arg] input_file required"
    exit 1
fi

output_dir="$2"
if [[ -z $input_file ]]; then
    echo "[arg] output_file required"
    exit 1
fi

mkdir -p $output_dir

while IFS= read -r link; do
    if [ -z "$link" ] || [[ "$link" == \#* ]]; then
        continue
    fi
    filename=$(basename $link)
    if [ -s $output_dir/$filename ]; then
        echo "Skipping downloading trace, $filename exist"
        continue
    fi
    wget -q -O $output_dir/$filename $link
    echo "Finished downloading trace $filename"
done < "$input_file"

echo "Finished downloading trace"
