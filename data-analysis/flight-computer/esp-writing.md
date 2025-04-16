1. Find the USB port that the ESP is using
(assume its /dev/ttyUSB0)
2. check group permissions with `ls -l /dev/ttyUSB0`
3. add yourself to the group with `sudo adduser $(whoami) dialout`
4. reboot with `sudo reboot`
5. Now you should be able to connect to the port, check with `esptool.py --port /dev/ttyUSB0 read_mac`
6. Now just do whatever idf.py tells you (smile)