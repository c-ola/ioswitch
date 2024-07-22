## IOswitch

Sends input from one computer to another via a TCP server (i.e. send desktop mouse input to laptop).

The daemon is capable of both sending and receiving inputs. It is controlled through the command line.
Available/Planned commands are:
- [x] list devices (sent/received)
- [x] add devices (TO send input)
- [x] rm device (that IS sending)
- [x] kill the daemon
- [x] keybinds to stop sending

### Installation
```
sudo make install
```

### Usage

Run the daemon (it must be run on more than one device)
```
sudo ioswitchd -p <port>
```

Run commands
```
ioswitchctl -t <command> -p <port> -l <daemon_port> -i <ip> -d </dev/input/eventX>
```

At the moment, the default command to stop sending and re-enable is meta+shift+comma
You can also have a binding to start it, for example, I am using in i3:
```
bindsym $mod+Shift+period exec --no-startup-id ioswitchrun
```
You can modify ioswitchrun and ioswitchstop to suit the devices that you want to start/stop sending.
TODO: make a script that can grab the xinput ids for each device based on name (they change sometimes), and also grab all the ids for each device, since some devices have multiple

Command can be:
- list
- kill
- add
- bind (adds the device and indicates that it should have its input events listened to for a specific binding)
- rm
- enable (does nothing rn)
- disable (does nothing rn)


### Issues
- If the receiver shutsdown, the sender's devices are not re-enabled
- Probably many others i'm not aware of
