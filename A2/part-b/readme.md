# Implementing load balancer using XDP eBPF

## Build  and run the server image on a docker container
`cd <path  of directory containing server source code>`\
`docker build -t my-server .`\
`docker run -it my-server /bin/bash`\ - Do it on 3 terminals to get 3 servers running
Run `make ` - In the command prompt displayed

## Build  and run the load balancer image with eBPF loaded on a docker container
`cd <path  of directory containing load balancer source code>`\
`docker build -t my-lb .`\
`docker ps` - Get Container ID of server images
`docker inspect <server_container_id>` - Get IP address of server client
`docker run --privileged -it lb /bin/bash`\
Run `make ` - In the command prompt displayed
Run `./lb <Server IPs space seperated>` - In command prompt

## Build and run client image to establish connection with server and send TCP packets on another docker container
`cd <path  of directory containing client source code>`\
`docker build -t my-client .`\
`docker ps` - To get container ID of load balancer\
`docket inspect <container ID of my-lb>` - Get the IP address of my-lb container\
`docker run -it  my-client /bin/bash`\
Run` gcc client.c -o client ` - On the command prompt displayed\
Run `./client <ip_address_of_lb> 8080 L R` On the command prompt displayed, where L,R is the range of sleep times, use this to test the load balancer on different ranges of sleep time

## To log the trace messages of eBPF on console
RUN `sudo cat  /sys/kernel/debug/tracing/trace_pipe` on the host terminal