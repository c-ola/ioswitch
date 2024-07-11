## IOswitch

Sends input from one computer to another via a TCP server (i.e. send desktop mouse input to laptop).

The daemon is capable of both sending and receiving inputs. It is controlled through the command line.
Available/Planned commands are:
    - list devices (sent/received)
    - [x] add devices (TO send input)
    - rm device (that IS sending)
    - disable/enable a device
    - [x] kill the daemon




### Usage

Run the daemon
```
sudo ./build/ioswitchd -p <port>
```

Run commands
```
./build/ioswitchctl -s <command_num> -p <port> -l <daemon_port> -i <ip> -d </dev/input/eventX>
```
