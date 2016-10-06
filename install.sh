#!/bin/bash

#--------------------------------------------------------------------
function tst {
    echo "===> Executing: $*"
    if ! $*; then
        echo "Exiting script due to error from: $*"
        exit 1
    fi
}
#--------------------------------------------------------------------

#tst sudo apt-get install libttspico-utils

##### ALEXA-START
tst sudo apt-get install openssl sox -y

tst cd ~/speechdot/alexa/client
tst /bin/bash generate.sh my_device 123456

tst cd ~/speechdot/alexa/companionService
tst npm install
##### ALEXA-END

tst cd ~/speechdot/sensory/Samples
tst make

tst cd ~/speechdot/lib/fcntl
tst npm install

tst cd ~/speechdot
tst npm install

tst sudo rm -rf /etc/asound.conf

tst sudo rm -rf /etc/pulse/
tst sudo rm -rf /etc/bluetooth/
tst sudo rm -rf /etc/startup.sh
tst sudo rm -rf /etc/rc.local
tst sudo rm -rf /etc/machine-info
tst sudo rm -rf ~/logs

tst sudo mkdir /etc/pulse
tst sudo cp -r ./install/pulse/* /etc/pulse
	
tst sudo cp ./install/asound.conf /etc/

tst sudo cp -r ./install/bluetooth /etc/
tst sudo cp ./install/startup.sh /etc/
tst sudo cp ./install/rc.local /etc/
tst sudo cp ./install/machine-info /etc/

tst sudo mkdir ~/logs

echo "Install Complete, Reboot!"
