#!/bin/bash

# creates the Purte Data extension based on the main branch on github
# script has to be executable

set -e

# remounting root filesystem read/write...
sudo mount -o remount,rw /

# removing old Gravity directory 
rm -r -f Gravity

# cloning latest Gravity from GitHub...
GIT_SSL_NO_VERIFY=true git clone https://github.com/attackallmonsters/Gravity.git

# building release version
cd Gravity || { echo "directory Gravity not found"; exit 1; }
make release

echo "build complete"