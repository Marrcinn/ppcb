#!/bin/bash

echo "Running concurrent_basic tests"

# Function to generate a random port number between 2000 and 65000
get_random_port() {
  echo $((2000 + RANDOM % 63000))
}

# Array of file names
files=("100M.txt")
#Array of protocols
protocols=("tcp" "udpr" "udp")
# Set number of workers to 10
no_workers=10


for protocol in "${protocols[@]}"; do
  success=0
  port=$(get_random_port)
  echo "Running concurrent_basic tests for $protocol"
  # For udpr we need to start server in udp mode
  if [ "$protocol" == "udpr" ]; then
    server_protocol="udp"
  else
    server_protocol=$protocol
  fi
  ./ppcbs $server_protocol $port >/dev/null &
  sleep 2
  start=$(date +%s%N)

  # Start 10 concurrent clients and save the time taken by each to ./results/concurrency_tests.txt with formatting "protocol, file, time"
  for i in {1..10}; do
      ./ppcbc $protocol localhost $port < tests/files/$file > /dev/null &
      pids[${#pids[@]}]=$!  # Store the PID of the last background process
  done
  echo "Waiting for clients to finish"
  for pid in "${pids[@]}"; do
    wait "$pid"  # Wait for each background process to finish
    # check success code and increment success counter
    if [ $? -eq 0 ]; then
        success=$((success+1))
    fi
  done
  end=$(date +%s%N)

  duration_ns=$((end - start))

  echo "$protocol, $no_workers, $duration_ns, $success" >> "results/concurrency_tests.txt"
done

echo "Killing the server"
killall ppcbs


