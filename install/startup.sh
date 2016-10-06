#!/bin/bash

sudo sysctl net.ipv4.ip_default_ttl=70

#rm -rf /home/pi/logs/*.log

##### ALEXA-START
node /home/pi/speechdot/alexa/companionService/bin/www &> /home/pi/logs/alexa-compService.log &
echo "Alexa companion service started"
##### ALEXA-END

node /home/pi/speechdot/app.js &> /home/pi/logs/speechdot.log &
echo "Speakspoke app spawned"

echo "Startup complete"
