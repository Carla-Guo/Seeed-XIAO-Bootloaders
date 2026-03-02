import subprocess
import datetime
import shlex
from serial.tools import list_ports
from time import sleep
import os


product_PROGRAM='test_code.ino.bin'

def timeout_command(command, timeout = 30):
    """
    call shell-command and either return its output or kill it
    if it doesn't normally exit within timeout seconds and return None
    """

    if type(command) == type(''):
        command = shlex.split(command)
    start = datetime.datetime.now()
    process = subprocess.Popen(command) # , stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    resultcode = process.poll()
    while resultcode is None:
        now = datetime.datetime.now()
        if (now - start).seconds > timeout:
            process.kill()
            return -1
        sleep(0.01)
        resultcode = process.poll()
    return resultcode



def find_device1():
        timeout = 2
        while timeout != 0:
            port1 = None
            for p in list_ports.comports():
                if p[2].upper().startswith('FTDIBUS\\VID_2886+PID_802F') or p[2].upper().startswith(
                    'USB VID:PID=2886:802F'):
                    port1 = p[0]
                    print('find port: ' + port1)
                    return port1

            sleep(0.1)
            timeout -= 1

        print('No seeeduino xiao is found')
        return None

def find_device2():
        timeout = 2
        while timeout != 0:
            port2 = None
            for p in list_ports.comports():
                if p[2].upper().startswith('FTDIBUS\\VID_2886+PID_002F') or p[2].upper().startswith(
                    'USB VID:PID=2886:002F'):
                    port2 = p[0]
                    print('find port: ' + port2)
                    return port2

            sleep(0.1)
            timeout -= 1

        print('No seeeduino xiao found')
        return None
  
def write_test():
        print('Write seeeduino xiao test program')
        sleep(2)
        port0 = find_device1()
        #if not port0:
        #    return -1
        command_rst = 'MODE %s:1200,N,8,1' % (port0)
        os.system(command_rst)
        print('Write seeeduino xiao test program')
        sleep(8)
        port00 = find_device2()
        if not port00:
            return -1
        command = 'bossac -i -d --port=%s -U true -i -e -w -v %s' % ( port00, product_PROGRAM)
        timeout_command(command,5)
        command = 'bossac -R' 
        return timeout_command(command,5)
        



write_test()