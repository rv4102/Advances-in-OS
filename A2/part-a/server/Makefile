all: xdp server

xdp:
	clang -g -O2 -emit-llvm -c xdp_filter.bpf.c -o - | llc -march=bpf -filetype=obj -o xdp_filter.o
server:
	gcc -o server server.c -Ilibbpf/src -Ilibbpf/src/include/uapi -Llibbpf/src -lbpf
	./server
