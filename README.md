# nimucsv

C implementation for receiving messages from Nordic Inertial CAN connected IMUs.

## Prequisites

On Windows, install your CAN adapter specific libraries. Currently, Kvaser and Peak adapters are supported.

Kvaser
 1. Download and install CANLIB SDK from https://www.kvaser.com/developer/canlib-sdk/
 2. Copy C:\Program Files (x86)\Kvaser\Canlib\INC into can_if/ext_include
 3. Copy C:\Program Files (x86)\Kvaser\Canlib\Lib\x64\canlib32.lib into can_if/ext_lib

Peak
 1. Download and install PCAN-Basic API and device drivers from https://www.peak-system.com/PCAN-USB.199.0.html?&L=1 
 2. Copy PCANBasic.h from PCAN-Basic API zip file into can_if/ext_include
 3. Copy PCANBasic.dll to the root directory of this repository or to your system directory where it can be found during dynamic linking phase.

On Linux, follow your CAN adapter manufacturer instructions on how to get the adapter to work with socketcan.

## Building

Cross-compiling with docker is the easiest option when targeting Windows.

    docker run -v $(pwd):/work -it dockcross/windows-static-x64-posix:latest /bin/bash

    # once in docker build nimucsv for pcan driver
    make nimucsv_win64_pcan

    # or for kvaser driver
    make nimucsv_win64_kvaser

Compiling for Linux is easier as socketcan interface is used, hence no 3rd party libraries are required. Just run the following command in the same docker image as above.

    make nimucsv_linux