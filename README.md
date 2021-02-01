# hash_server

## introductions

This is test project to implement TCP server on plain linux which receive any data and as return it sent back one by one each line hash. 
For calculating hash it using openssl and epoll for managing connection. Server creates thread pool 2xCPU.

## installation

For hash_server compilation it reqire gcc >= 7.0, openssl and epoll support in linux kernel.
Until docker is not ready in project dir:
```
mkdir build
cd build
cmake ..
make
```
As result hash_server elf file must appear in build directory.

## Tests manual

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
./build/test/hash_server_test
```

Currently it testing hash functions.

## TODO
[ ] Made app as service
[ ] Add docker files
[ ] Remake with using STL coroutines (require gcc >= 10.0)
[ ] Improve speed with over 10k connections
[ ] Complete GPROF analysis
