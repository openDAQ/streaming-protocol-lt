# Streaming Protocol

This project consists of a C++ library `streaming-protocol` implementing the producer and consumer side of the streaming protocol.
It supports raw tcp and websocket connection. Under Linux, UNIX domain sockets and writing to/reading from file are also supported.


## Prerequisites

### Used Libraries
We try to use as much existing and prooved software as possbile in order to keep implementation and testing effort as low as possible.
All libraries used carry a generous license. See the licenses for details.

For communication we use boost asio and boost beast. Tools are using program_options

The open source project nlohmann/json is being used as [JSON composer and parser](https://github.com/nlohmann/json ""). 
It is used because it also handles [msgpack](https://msgpack.org/index.html) which is used by the streaming protocol. 

The unit tests provided do use the [googletest library](https://github.com/google/googletest).


## Build

cmake is used as build system. To build under Linux, create a build directory somwhere, change into this directory and call the following commands:

```
cmake < path to source root directory >
make -j8
sudo make install
```

Under Windows, MSVC is able to build cmake projects.

## Ho to Use it

There is an example client implementation `tool/StreamingClient`. call it without parameters to get a short syntax help.
