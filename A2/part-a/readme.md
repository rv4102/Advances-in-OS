# Implementing eBPF XDP program to drop even data UDP packets sent to a fixed port

## Build  and run the server image with the eBPF program attached on a docker container
`cd <path  of directory containing server source code>`\
`docker build -t my-server .`\
`docker run --privileged  -it my-server make`\


## Build and run client image to send UDP packets on another docker container
`cd <path  of directory containing client source code>`\
`docker build -t my-client .`\
`docker ps` - To get container ID of my-server\
`docket inspect <container ID of my-server>` - Get the IP address of my-server container\
`docker run -it  my-client /bin/bash`\
Run` make ` - On the command prompt displayed\
Run `./client <ip_address_of_server> 8080` On the command prompt displayed

## To log the trace messages of eBPF on console
RUN `sudo cat  /sys/kernel/debug/tracing/trace_pipe` on host terminal

The application is being tested for client sending integer values in the range [0-127]
