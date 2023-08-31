#!/bin/bash

# Remove the file if it already exists. If not, don't throw an error. Then create it
sudo rm -f mtp_dcn.conf
sudo touch mtp_dcn.conf

# isTor:[true/false] | isTopSpine:[true/false] | tier:[1-N] | startHelloTime:[epoch_timestamp] | topEthPortName:[ethX]
echo "isTor:$1" > mtp_dcn.conf
echo "isTopSpine:$2" >> mtp_dcn.conf
echo "tier:$3" >> mtp_dcn.conf
echo "startHelloTime:$4" >> mtp_dcn.conf
echo "torEthPortName:$5" >> mtp_dcn.conf
