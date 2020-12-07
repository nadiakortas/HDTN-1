# High Speed Delay Tolerant Network #
## Overview ##
 Delay Tolerant Networking (or DTN) has been identified as a key technology to facilitate the development and growth of future space networks. Existing DTN implementations have emphasized operation in constrained environments with relatively limited resources and low data speeds. As various technologies have advanced, however, both the rate and efficiency with which data can be effectively transferred have grown incredibly quickly. This has left existing craft unable to utilize more than a small fraction of available capacity. Further, to date, most known implementations of DTN have been designed to operate on the spacecraft themselves. HDTN takes advantage of modern hardware platforms to offer substantial improvement on latency and throughput with respect to DTN implementations that exist today. Specifically, our implementation maintains compatibility with existing implementations of DTN that conform to IETF RFC 5050, while simultaneously defining a new data format that is better suited to higher-rate operation in many cases. It defines and adopts a massively parallel, pipelined, and message oriented architecture, which allows the system to scale gracefully as the resources available to the system increase. HDTN's architecture additionally supports hooks for replacing various elements of the processing pipeline with specialized hardware accelerators, which can be used to offer improved Size, Weight, and Power (SWaP) characteristics at the cost of increased development complexity and cost.
## Build Environment ##
* HDTN build environment requires CMake version 3.13
* Testing has been done with gcc version 8.3.0 (Debian 8.3.0-6) 
* Target: x86_64-linux-gnu 
* Tested on Debian 10
* Known issues:
* Ubuntu distributions may install an older CMake version that is not compatible
* Mac OS may not support recvmmsg and sendmmsg functions, recvmsg and sendmsg could be used

## Quick Start ##
### Build HDTN ###
* cd ~/hdtn
* mkdir build
* cmake ..
* make
### Run HDTN ###
Note: You made need to increase the maximum number of files the operating system will allow to have open to run the storage component. On Debian, this can be done by setting the hard and soft limits for "nofile" to unlimited in /etc/security/limits.conf.

You can use tcpdump to test the HDTN ingress and egress.
* sudo tcpdump -i lo -vv -s0 port 4557
In another terminal, run:
* ./runscript

### Run Unit Tests ###
After building HDTN (see above), the unit tests can be run with the command within the build directory:
* ./tests/unit_tests/unit-tests 