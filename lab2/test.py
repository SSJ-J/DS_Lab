#!/usr/bin/python

import subprocess
import sys
import time

cmd = './dsdv'
ports = [3031, 3032, 3033, 3034, 3035, 3036]
files = ['a.dat', 'b.dat', 'c.dat', 'd.dat', 'e.dat', 'f.dat']

def open_terminal(n):
    for i in range(n):
        subprocess.call(["xdotool", "key", "ctrl+shift+t"])

# n in [1, 6]
def open_dsdv(n):
    subprocess.call([cmd, str(ports[n-1]), files[n-1]])

def kill_dsdv():
    subprocess.call(["pkill", "dsdv"])

def open_all():
    command = ["gnome-terminal"]
    for i in range(6):
        command.append("--tab")
        command.append("--title=%s"%(files[i]))
        command.append("-e")
        command.append("%s %d %s" % (cmd, ports[i], files[i]))
    subprocess.call(command)

if __name__ == '__main__':
    if len(sys.argv) == 1:
        open_all()  
    elif sys.argv[1] == 'k':
        kill_dsdv()
    elif sys.argv[1] == '-t':
        n = int(sys.argv[2])
        open_terminal(n)
    else:
        n = int(sys.argv[1])
        open_dsdv(n)
