## IOswitch

SHOULD CONVERT TO SOMETHING SIMILAR TO THE i3lock https://github.com/i3/i3lock/blob/main/i3lock.c IMPLEMENTATION FOR STEALING FOCUS


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

There are a couple of options for running the daemon

1. Run the daemon manually with sudo 
```
sudo ioswitchd -p <port>
```
2. Add yourself to the `input` group with `sudo usermod -a -G input $USER`. Then you can run it with some sort of startup script using `ioswitchd -p <port>` (for example, the i3 config)

3. Use systemd (probably overkill, but the .service file is in `system/`). Just change the home user name and then copy it to `/etc/systemd/system/`. You will then have to run `sudo systemctl daemon-reload` and enable the service.


Run commands
```
ioswitchctl -t <command> -p <port> -l <daemon_port> -i <ip> -d </dev/input/eventX>
```

You can modify the device lists in the scripts before installing depending on what you want to activate/disable when running the keybinds.

At the moment, the default command to stop sending run the stop script is meta+shift+comma
You can also have your own binding to start it, for example, I am using it in i3 with:
```
bindsym $mod+Shift+period exec --no-startup-id ioswitchrun
```
You can modify ioswitchrun and ioswitchstop to suit the devices that you want to start/stop sending.
TODO: make a script that can grab the xinput ids for each device based on name (they change sometimes), and also grab all the ids for each device, since some devices have multiple

Commands can be:
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
