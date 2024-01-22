FROM ubuntu:22.04
ADD . /usr/src
WORKDIR /usr/src
EXPOSE 8080
RUN apt-get update && apt-get install -y gcc clang llvm libbpf-dev iproute2 make git
RUN apt-get update
RUN apt-get install -y bpfcc-tools
RUN apt-get install -y linux-tools-common linux-tools-$(uname -r)
RUN apt-get install -y gcc-multilib