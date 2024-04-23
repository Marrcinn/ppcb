#!/bin/bash

# Function to generate a random port number between 2000 and 65000
get_random_port() {
  echo $((2000 + RANDOM % 63000))
}

# Array of file names
files=("1K.txt" "10K.txt" "100K.txt" "1M.txt" "10M.txt" "100M.txt" "1G.txt")
# Array of protocols
protocols=("tcp" "udp" "udpr")


for protocol in "${protocols[@]}"; do
  echo "Starting tests for protocol $protocol"
  port=$(get_random_port)

  # For udpr we need to start server in udp mode
  if [ "$protocol" == "udpr" ]; then
    server_protocol="udp"
  else
    server_protocol=$protocol
  fi
  ./ppcbs $server_protocol $port > ./logs/server/basic_test_log_${protocol}.txt &
  sleep 2
  for file in "${files[@]}"; do
    output=$({ time ./ppcbc $protocol localhost $port < tests/files/$file >logs/client/basic_test_log_${protocol}_${file}; } 2>&1)
    if [ $? -eq 0 ]; then
      real_time=$(echo "$output" | grep real | awk -F'[ms]' '{print $1*60 + $2}')
    else
      real_time=-1
    fi
    echo "$protocol, $file, $real_time" >> "results/basic_tests.txt"
  done
  killall ppcbs
done

echo "Finished basic_tests"
