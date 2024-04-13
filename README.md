# Description
This is a very simple and dumb chat server where multiple clients can simultaneously connect and broadcast messages to each other.
Under the hood, it makes use of TCP (transmission control protocol) via Berkeley Sockets.

The purpose is to apply concepts I learned in network programming.

# How to build
First, Make sure you have `CMake` installed and available in your `PATH`.

Then,

```shell
mkdir build
cd build
cmake ..
make
```

This will create an executable called `server` in the build directory.

# How to use
To start the server from the command line, simply run `./server <PORT>`.
You'll see something like this:
```shell
server: listening to connections on port 3000
```

In order to connect to the server, you can use `telnet` or any similar program.
```shell
telnet localhost <PORT>
```
Open it in multiple terminal windows to simulate multiple clients and have fun!
