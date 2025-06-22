#!/bin/bash

input_file="traces.txt"
output_file="reasonable_traces.txt"

MIN_SIZE=$((512 * 1024 * 1024))  # 512 MB
MAX_SIZE=$((32 * 1024 * 1024 * 1024))  # 48 GB

get_file_size() {
    url=$1
    size=$(curl -sI "$url" | grep -i "Content-Length" | awk '{print $2}' | tr -d '\r')
    echo "$size"
}

while IFS= read -r link; do
    if [ -z "$link" ] || [[ "$link" == \#* ]]; then
        continue
    fi

    link="https://$link"
    file_size=$(get_file_size "$link")

    retry_count=0
    max_retries=5

    while [ -z "$file_size" ] && [ "$retry_count" -lt "$max_retries" ]; do
        file_size=$(get_file_size "$link")
        if [ -z "$file_size" ]; then
            retry_count=$((retry_count + 1))
            echo "Retry $retry_count: failed to get size for $link"
            sleep 1
        fi
    done

    if [ -z "$file_size" ]; then
        echo "Skipped $link"
        echo "size is empty"
        continue
    fi
    if [ "$file_size" -le "$MIN_SIZE" ]; then
        echo "Skipped $link with size $(( file_size/1024/1024 )) MB"
        echo "size is less then $(( MIN_SIZE/1024/1024 )) MB"
        continue
    fi
    if [ "$file_size" -ge "$MAX_SIZE" ]; then
        echo "Skipped $link with size $(( file_size/1024/1024/1024 )) GB"
        echo "size is greater then $(( MAX_SIZE/1024/1024/1024 )) GB"
        continue
    fi
    echo "$link" >> "$output_file"
done < "$input_file"

echo "Finished checking links. Valid links are in $output_file."
