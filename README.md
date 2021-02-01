# hash_server

## Introduction

This is test project to implement TCP server on plain linux which receive any data and as return it sent back one by one each line hash. 
For calculating hash it using openssl and epoll for managing connection. Server creates thread pool 2xCPU. 

Because tast didn't allow to used extra memory it doesn't store read data to queue which is more effective way to parallel hash calculation but in that case is requre much more memory.

Attention: server check only linux new line format: \n. Windows new line format will include \r into hash and will not match to linux.

Default TCP port 5555

## Installation

For hash_server compilation it reqire gcc >= 7.0, openssl and epoll support in linux kernel.
Until docker is not ready in project dir:
```
mkdir build
cd build
cmake ..
make
make install
```
As result hash_server elf file must appear in build directory.

for manual compilation:
```
g++ -lpthread -lcrypto -O2 -std=c++17 ./main.cpp ./hash_server.cpp
```

## Using
For using hash_server need to start server and it will calculate hash for any data sent to its port.

## Test tools:
For developing and testing was used folowing test tools:

g++ ... -fsanitize=address

g++ ... -fsanitize=thread

g++ ... -fsanitize=undefined

g++ ... -pg

valgrind

## Manual test

After building server can be tested with using any tcp clients for example netcat:
```
nc 127.0.0.1 5555
------------
type in terminal something and press enter
```
or send file:
```
cat ./file_name | nc 127.0.0.1 5555 -q1
```

## Autotests

To build tests need to folow next steps:

```
mkdir build
cd build
cmake ..  -DCOMPILE_TESTS=ON
make
```
To start tests need to execute command from project directory:

```
./test/hash_server_test
```

Currently it testing hash functions.

## TODO
[ ] Raw pointer and dinamic allocated objects life cicle

[ ] Read port number from command line 

[ ] Made app as service

[ ] Other modules unit tests

[ ] Check all error printing messages if it really has errno

[ ] Make code in C++ way (raw pointers, C function)

[ ] Add docker files

[ ] Remake with using STL coroutines (require gcc >= 10.0)

[ ] Improve speed with over 10k connections

[ ] Complete GPROF analysis

[ ] Too many comments in code made it difficult to read. Need to refactor code and remove extra comments

