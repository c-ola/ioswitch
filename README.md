## IOswitch
Sends input from one computer to another via a TCP server (i.e. send desktop mouse input to laptop). When the host daemon is sending, input is locked on that computer.

Based alot of the screen locking code off of xtrlock http://ftp.debian.org/debian/pool/main/x/xtrlock/.

The daemon is capable of both sending and receiving inputs. It is controlled through the command line.
Available/Planned features are:
- [] list devices (sent/received)
- [x] start sending
- [x] stop sending
- [x] switch start/stop sending state
- [x] kill the daemon
- [x] lock the user input on the computer that is sending
- [] hot reload the config without rerunning the daemon
- [] better input device selection (i.e. select device through just a name, and some kind of script just finds the device path in /dev/input)
- [] multiple destinations to send to in one file config, choose what to send in a command
- [] status command, returns if its sending or not

Planned

### Installation
```
sudo make install
```

### Usage

Warning: it is pretty buggy at the moment, and you might get locked in on your screen.
It might make sense for me to add an emergency exit on the screen locker.

#### Daemon
There are a couple of options for running the daemon.
Note: You have to run the daemon on two devices to make it work.
One is the receiver, which will copy input sent to it. The other is the sender, which sends input from devices on that host.

1. Add yourself to the `input` group with `sudo usermod -a -G input $USER`. Then you can run it with some sort of startup script using `ioswitchd` (for example, the i3 config)
```
ioswitchd -c <config> # atm, paths are based on the current working directory
```

2. Use systemd (probably overkill, but the .service file is in `system/`). Just change the home user name and then copy it to `/etc/systemd/system/`. You will then have to run `sudo systemctl daemon-reload` and enable the service.

3. NOT RECOMMENDED Run the daemon manually with sudo (if your user is not in the `input` group)
```
sudo ioswitchd -c <config>
```

#### Ctl
Run commands
```
ioswitchctl -t <start|stop|switch|kill> -d <daemon_port> -i <daemon_ip> -n <parse_config:1|0> -c <config>
```

You can set this to a keybind. for example, I am using it in i3 with:
```
bindsym $mod+Ctrl+period exec --no-startup-id ioswitchctl -t switch
```

If it is working, you should be able to see the new devices on the receiver system using `xinput`.

#### IMPORTANT
At the moment, the keybind to unlock is meta+Ctrl+period, which is hardcoded into the program, but I do plan to add something that lets you modify it.

### Configuration
Format for the config file is the following.
`variable=value`
Also supports lists.
```
variable={
    val1,
    val2,
}
```
For example:
```
dest_ip=192.168.2.44
devices={
    /dev/input/mouse,
}
```
Variables are:
```
dest_ip  # the ip that the daemon will send input to 
dest_port  # the port that the daemon will send input to 
daemon_ip  # the ip of the daemon that the ctl connects to
daemon_port  # the port of the daemon that the ctl connects to
device  # list of devices that the daemon will send will told to
```

### Issues ¯\_(ツ)_/¯
Probably so many i'm not aware of, there's definitely a chance the daemon just crashes.
Known ones are:
    - last input event gets cut off before EV_SYN when stopping sending, fix is to send an extra EV_SYN
    - lock bind requires 2 presses
    - you sometimes get locked in (I just have lock in with this program tbh)
