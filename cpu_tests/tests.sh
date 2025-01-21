#!/bin/bash

# Base URL
BASE_URL="https://raw.githubusercontent.com/SingleStepTests/ProcessorTests/refs/heads/main/6502/v1"

# Loop through 00 to FF
for i in $(seq 10 255); do
    # Convert to two-digit hexadecimal
    HEX=$(printf "%02x" $i)
    FILE="${HEX}.json"
    
    # Download the file
    echo "Downloading $FILE..."
    curl -s -O "${BASE_URL}/${FILE}" || { echo "Failed to download $FILE"; break; }
done

echo "Download complete!"