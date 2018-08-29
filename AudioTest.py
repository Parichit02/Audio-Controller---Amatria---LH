import serial
from serial import SerialException
import time

SOM = b'\xff'

START = b'\x04'
STOP =b'\x05'
PLAY = b'\x06'

check =0
ser = serial.Serial('COM24')


while check==0:
        print(ser.name)         
        ser.write(SOM+START)
        print("START")
        time.sleep(5)
        
        ser.write(SOM+STOP)
        print("STOP") 
        time.sleep(1)
        
        ser.write(SOM+PLAY)
        print("PLAY")
        
        time.sleep(5)
        ser.write(SOM+START)
        print("STOP Playing")
        check=1
        

