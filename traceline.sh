#!/bin/bash

addr2line=$DEVKITPRO/devkitA64/bin/aarch64-none-elf-addr2line
CRASH_LOG_PATH="/Volumes/SWITCH SD/atmosphere/crash_reports/01680761388_410000000000ff15.log"
SYMBOL_TABLE="./http-botbase.elf"

while read -r line; do
    address=$(echo $line | awk '{print $2}')
    symbol=$($addr2line -e $SYMBOL_TABLE -a $address)
    echo "$line"
    echo "Symbol: $symbol"
    file=$(echo "$symbol" | awk -F':' '{print $1}')
    line_num=$(echo "$symbol" | awk -F':' '{print $2}')
    if [[ "$file" != "?" && "$line_num" != "?" ]]; then
        echo "File: $file"
        echo "Line Number: $line_num"
    fi
done < "$CRASH_LOG_PATH"