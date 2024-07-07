## IOswitch

Sends input from one computer to another via a TCP server (i.e. send desktop mouse input to laptop). The client sends the inputs of a device, and the server accept and simulates them.

Run Server
```
./build/server
```

Run Client
```
sudo ./build/client <ip> -p <port> -d </dev/input/eventX>
```
