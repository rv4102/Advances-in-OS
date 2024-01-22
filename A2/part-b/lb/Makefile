all: xdp lb

xdp:
	clang -g -O2 -emit-llvm -c xdp_lb.bpf.c -o - | llc -march=bpf -filetype=obj -o xdp_lb.o
lb:
	gcc -o lb lb.c -Ilibbpf/src -Ilibbpf/src/include/uapi -Llibbpf/src -lbpf