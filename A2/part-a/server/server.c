//Group members: Rushil Venkateswar(20CS30045), Jay Kumar Thakur(20CS30024)
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include<bpf/libbpf.h>
#include<bpf/bpf.h>
#include<linux/bpf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/resource.h>
#include<net/if.h>
#include<linux/if_link.h>

int PORT = 8080;

//Function to load and attach the compiled eBPF object file xdp_filter to interface "eth0"
void loadEBPF(){
    int err, ifindex;
    struct bpf_object *bpfobj;
    struct bpf_link *link = NULL;

    err = 0;
    struct rlimit rlim = {
		.rlim_cur = 512UL << 20,
		.rlim_max = 512UL << 20,
	};

	err = setrlimit(RLIMIT_MEMLOCK, &rlim);
    if (err) {
		printf("failed to change rlimit\n");
		exit(EXIT_FAILURE);
	}

    bpfobj = bpf_object__open_file("xdp_filter.o", NULL);
    if(libbpf_get_error(bpfobj)){
        printf("failed to open or load BPF object\n");
		exit(EXIT_FAILURE);
    }
    if(bpf_object__load(bpfobj)){
        printf("failed to load BPF object %d\n", err);
        bpf_link__destroy(link);
        bpf_object__close(bpfobj);
        exit(EXIT_FAILURE);

    }

    int ifindex = if_nametoindex("eth0");
    if(!ifindex){
        printf("Invalid interface name\n");
        exit(EXIT_FAILURE);
    }
    int progfd = bpf_program__nth_fd(bpf_object__find_program_by_name(bpfobj, "filter_packets"), 0);
    if (progfd < 0) {
        printf("Error: Finding XDP program\n");
        bpf_object__close(bpfobj);
        exit(EXIT_FAILURE);
    }
    if(bpf_set_link_xdp_fd(ifindex, progfd, XDP_FLAGS_UPDATE_IF_NOEXIST)){
        printf("Error: Attaching XDP program to interface\n");
        close(progfd);
        bpf_object__close(bpfobj);
        exit(EXIT_FAILURE);
    }

}
int main(){
    //Loading eBPF
    loadEBPF();

    //Socket creation and binding
    int sockfd;
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) <= 0){
        printf("Socket creation failure\n");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in servaddr, recvaddr;
    int addrlen = sizeof(servaddr);
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port  = htons(PORT);

    if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
        printf("Socket Bind failed\n");
        exit(EXIT_FAILURE);
    }
    int buf;
    int rlen = sizeof(recvaddr);
    
    //Receive packets from client
    while(1){
        recvfrom(sockfd, &buf, sizeof(buf), 0, (struct sockaddr *)&recvaddr, &rlen);
        printf("Received data: %d\n", buf);
        fflush(stdout);

    }
    close(sockfd);
    return 0;
}
