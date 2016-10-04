#!/bin/bash

sudo sysctl net.ipv4.ip_default_ttl=70

rm -rf /home/pi/logs/*.log

#/usr/local/libexec/bluetooth/bluetoothd -d -n &> /home/pi/logs/bluetooth.log &
#echo "Bluez daemon spawned"

#ofonod -d -n &> /home/pi/logs/ofono.log &
#echo "Ofono daemon spawned"

#pulseaudio -v &> /home/pi/logs/pulseaudio.log &
#echo "PulseAudio daemon spawned"

#sleep 1

#hciconfig hci0 up
#echo "Bluetooth adapter turned on"

#hciconfig hci0 class 0x240404
#echo "Bluetooth adapter class set to 0x240404"

#hciconfig hci0 piscan
#echo "Bluetooth adapter put in discovery mode"

#python /home/pi/speakspoke/python_scripts/simple-agent.py &> /home/pi/logs/pairing.log &
#echo "Started bluetooth pairing agent"

##### ALEXA-START
node /home/pi/speakspoke/alexa/companionService/bin/www &> /home/pi/logs/alexa-compService.log &
echo "Alexa companion service started"
##### ALEXA-END

node /home/pi/speakspoke/app.js &> /home/pi/logs/speakspoke.log &
echo "Speakspoke app spawned"

echo "Startup complete"
