Libraries needed
1.WiringPi:
   As long as you have Git installed, these commands should be all you need to download and install Wiring Pi.
   git clone https://github.com/WiringPi/WiringPi.git

   This will make a folder in your current directory called WiringPi. Head to the Wiring Pi directory.
   cd WiringPi

   Then pull the latest changes from the origin.
   git pull origin

   Then enter the following command. The ./build is a script to build Wiring Pi from the source files. This builds the helper files, modifies some paths in Linux and gets WiringPi ready to rock.
   ./build
2.GNU install:
   sudo apt install --reinstall build-essential
   sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf
3.CURL:
   sudo apt-get install libcurl4-openssl-dev
4.JSON:
   sudo apt-get install libjsoncpp-dev

   
compile:g++ -o test node.cpp -lwiringPi ilcurl -ljsoncpp -I/usr/include/jsoncpp
