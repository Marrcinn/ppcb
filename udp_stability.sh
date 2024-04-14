#!/bin/bash

# Function to generate a random port number between 2000 and 65000
get_random_port() {
  echo $((2000 + RANDOM % 63000))
}

# Run server with UDP
port=$(get_random_port)
make clean
make ppcbs
make ppcbc
./ppcbs udp $port >/dev/null &

# Gradually increase the size of data sent
for size in {1..1000}; do
  # Create a file of size 'size' MB
  dd if=/dev/zero of=data.txt bs=1M count=$size >/dev/null 2>&1

  # Run client with UDP and the generated file
  echo "Sending file of size: $size MB"
  ./ppcbc udp localhost $port < data.txt

  # Check if the client crashed
  if [ $? -ne 0 ]; then
    echo "Client crashed when sending a file of size: $size MB"
    break
  fi
done

# Clean up
rm data.txt
killall ppcbs
make clean