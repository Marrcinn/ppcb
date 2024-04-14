#!/bin/bash

make clean
make ppcbc
make ppcbs


# Function to generate a random port number between 2000 and 65000
get_random_port() {
  echo $((2000 + RANDOM % 63000))
}

# Array of file names
files=("1K.txt" "10K.txt" "100K.txt" "1M.txt" "10M.txt" "100M.txt" "1G.txt")

# Run server and client with TCP
port=$(get_random_port)

./ppcbs tcp $port >/dev/null &
sleep 2
for file in "${files[@]}"; do
  echo "$file:"
  time ./ppcbc tcp localhost $port < $file
done

# Run server and client with UDP
port=$(get_random_port)

./ppcbs udp $port >/dev/null &
sleep 2
for file in "${files[@]}"; do
  echo "$file:"
  time ./ppcbc udp localhost $port < $file
done

port=$(get_random_port)


./ppcbs udp $port >/dev/null &
sleep 2
for file in "${files[@]}"; do
  echo "$file:"
  time ./ppcbc udpr localhost $port < $file
done

# Kill the server
killall ppcbs

ping -c 1000000 -i 0.0000001 localhost

# Clean up
make clean

