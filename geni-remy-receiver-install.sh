#!/bin/sh
# Usual directory for downloading software in ProtoGENI hosts is `/local`
#cd /local

##### Check if file is there #####
if [ ! -f "./installed.txt" ]
then
       #### Create the file ####
        sudo touch "./installed.txt"

       #### Run  one-time commands ####

       #Install necessary packages
        #sudo apt-get update& EPID=$!
        #wait $EPID
        #sudo apt-get -y install git-core unzip cmake libpcap-dev libxerces-c2-dev libpcre3-dev flex bison g++ autoconf libtool pkg-config libboost-dev libboost1.40-all-dev gawk & EPID=$!
        #sudo apt-get -y install g++ & EPID=$!
        #wait $EPID
        #sudo apt-get upgrade g++ &EPID=$!
        #wait $EPID

        #sudo mkdir remycc
        #cd remycc
        #sudo mkdir default
        #cd default

        # Get my data
        #wget web.mit.edu/venkatar/www/default-remy.tar
        #tar -xzvf default-remy.tar
        #rm default-remy.tar

        # Compile it
        #sudo ./compile.sh

        cd ~
        #sudo apt-get install protobuf-compiler

        # Install the correct version of g++
        sudo apt-get install --force-yes --yes python-software-properties
        sudo add-apt-repository ppa:ubuntu-toolchain-r/test
        sudo apt-get update
        sudo apt-get install --force-yes --yes libstdc++6-4.7-dev

        sudo apt-get install --force-yes --yes iperf tcptrace


        # Install sockperf
        #cd ~
        #sudo git clone https://github.com/mellanox/sockperf
        #cd sockperf
        #sudo ./autogen.sh
        #sudo ./congigure
        #sudo make
        #sudo make install

        # Install Protobuf version 2.5.0
        cd ~
        wget https://github.com/google/protobuf/releases/download/v2.5.0/protobuf-2.5.0.tar.gz
        tar -xzvf protobuf-2.5.0.tar.gz
        cd protobuf-2.5.0
        sudo ./autogen.sh
        sudo ./configure
        sudo make
        sudo make check
        sudo make install
        sudo cp /usr/local/lib/libprotoc.so.8 /usr/lib

        # Get the genericCC binaries
        cd ~
        mkdir genericCC
        cd genericCC
        sudo wget http://web.mit.edu/venkatar/www/genericCC-bin.tar.gz
        sudo tar -xzvf genericCC-bin.tar.gz
        sudo cp libudt.so /usr/lib/

        # Add /usr/local/lib to LD_LIBRARY_PATH 
        sudo printf "\nLD_LIBRARY_PATH=/usr/local/lib\nexport LD_LIBRARY_PATH\n" >> /users/venkatar/.bashrc
        source /users/venkatar/.bashrc
fi
##### Run Boot-time commands
# Start my service -- assume it was installed at /usr/local/bin
#sudo /usr/local/bin/myproject-server  
# ./receiver
                 