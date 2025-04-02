#!/bin/bash

LOGFILE="output.log"

# Ensure the log file is empty before starting
> "$LOGFILE"

while true; do
    echo "Starting make run..." | tee -a "$LOGFILE"
    make run &
    PID=$!
    sleep 1
    kill $PID 2>/dev/null
    echo "Process interrupted. Restarting..." | tee -a "$LOGFILE"
done
