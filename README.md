## IOswitch

Sends input from one computer to another via a TCP server (i.e. send desktop mouse input to laptop).

The daemon is capable of both sending and receiving inputs. It is controlled through the command line.
Available/Planned commands are:
- [x] list devices (sent/received)
- [x] add devices (TO send input)
- [x] rm device (that IS sending)
- [ ] disable/enable a device (this might not be needed because its kind of the same as adding or removing)
- [x] kill the daemon

TODO:
Make it possible to create a keybind that enables/disables the device with something like xinput to block the input device on one of your devices (this would have to be on both systems).

### Usage

Run the daemon
```
sudo ./build/ioswitchd -p <port>
```

Run commands
```
./build/ioswitchctl -t <command> -p <port> -l <daemon_port> -i <ip> -d </dev/input/eventX>
```

Command can be:
- list
- kill
- add
- rm
- enable (does nothing rn)
- disable (does nothing rn)
