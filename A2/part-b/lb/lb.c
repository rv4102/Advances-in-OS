//Group members: Rushil Venkateswar(20CS30045), Jay Kumar Thakur(20CS30024)
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include<linux/bpf.h>
#include<bpf/libbpf.h>
#include<bpf/bpf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/resource.h>
#include<net/if.h>
#include<linux/if_link.h>
#include <arpa/inet.h>
int PORT = 8080;
struct bpf_object *bpfobj;
struct bpf_map *ipm, *threadm;
int queue[100000];
int startidx = 0, endindx = -1;
//Function to load and attach eBPF program xdp_lb to interface "eth0"
void loadEBPF(){
    
    int err  = 0;
    struct rlimit rlim = {
		.rlim_cur = 512UL << 20,
		.rlim_max = 512UL << 20,
	};

	err = setrlimit(RLIMIT_MEMLOCK, &rlim);
    if (err) {
		printf("failed to change rlimit\n");
		exit(EXIT_FAILURE);
	}
    struct bpf_link *link = NULL;
    bpfobj = bpf_object__open_file("xdp_lb.o", NULL);

    if(libbpf_get_error(bpfobj)){
        printf("failed to open and/or load BPF object\n");
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
    int progfd = bpf_program__nth_fd(bpf_object__find_program_by_name(bpfobj, "lb_packets"), 0);
    if (progfd < 0) {
        printf("Error: Finding XDP program\n");
        bpf_object__close(bpfobj);
        exit(EXIT_FAILURE);
    }
    if(bpf_set_link_xdp_fd(ifindex, progfd, XDP_FLAGS_SKB_MODE)){
        printf("Error: Attaching XDP program to interface\n");
        close(progfd);
        bpf_object__close(bpfobj);
        exit(EXIT_FAILURE);
    }

}
int main(int argc, char *argv[]){
    if(argc != 4){
        printf("Incorrect number of arguments\n");
        exit(EXIT_FAILURE);
    }
    int sockfd;
    //Socket creation
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) <= 0){
        printf("Socket creation failed\n");
        exit(EXIT_FAILURE);
    }
    //Load balancer receives server IPs through command line
    struct sockaddr_in servaddr1, servaddr2, servaddr3, recvaddr, lbaddr;
    lbaddr.sin_family = AF_INET;
    lbaddr.sin_port = htons(PORT);
    lbaddr.sin_addr.s_addr = INADDR_ANY;
    if(bind(sockfd, (struct sockaddr *)&lbaddr, sizeof(lbaddr)) < 0){
        printf("Bind failed\n");
        exit(EXIT_FAILURE);
    }
    servaddr1.sin_family = AF_INET;
    servaddr1.sin_port = htons(PORT);
    servaddr1.sin_addr.s_addr = inet_addr(argv[1]);

    servaddr2.sin_family = AF_INET;
    servaddr2.sin_port = htons(PORT);
    servaddr2.sin_addr.s_addr = inet_addr(argv[2]);

    servaddr3.sin_family = AF_INET;
    servaddr3.sin_port = htons(PORT);
    servaddr3.sin_addr.s_addr = inet_addr(argv[3]);
   
    in_addr_t ip1,ip2,ip3;
    ip1 = servaddr1.sin_addr.s_addr;
    ip2 = servaddr2.sin_addr.s_addr;
    ip3 = servaddr3.sin_addr.s_addr;
    int rlen = sizeof(recvaddr);
    loadEBPF();
    //Load balancer stores the server IPs and initial number of free threads in the eBPF maps
    int ipmap, threadmap;
    ipm = bpf_object__find_map_by_name(bpfobj, "ip_addrs");
    ipmap = bpf_map__fd(ipm);
    if(ipmap < 0){
        printf("Finding map in obj file failed\n");
        exit(EXIT_FAILURE);
    }
    threadm = bpf_object__find_map_by_name(bpfobj, "free_threads");
    threadmap = bpf_map__fd(threadm);
    if(threadmap < 0){
        printf("Finding map in obj file failed\n");
        exit(EXIT_FAILURE);
    }
    int servidx = 1;
    int free_thread = 5;
    bpf_map_update_elem(ipmap, &servidx, (__u32 *)&ip1, 0);
    bpf_map_update_elem(threadmap, &servidx, &free_thread, 0);
    servidx = 2;
    bpf_map_update_elem(ipmap, &servidx, (__u32 *)&ip2, 0);
    bpf_map_update_elem(threadmap, &servidx, &free_thread, 0);
    servidx = 3;
    bpf_map_update_elem(ipmap, &servidx, (__u32 *)&ip3, 0);
    bpf_map_update_elem(threadmap, &servidx, &free_thread, 0);
    while(1){
        int recval = 0;
        //Load balancer receives a packet
        recvfrom(sockfd, &recval, sizeof(recval), 0, ( struct sockaddr *)&recvaddr, &rlen);
        int serveridx = 0;
        //Checks if it is received from any server or any client
        if(recvaddr.sin_addr.s_addr == servaddr1.sin_addr.s_addr){
            serveridx = 1;
        }
        else if(recvaddr.sin_addr.s_addr == servaddr2.sin_addr.s_addr){
            serveridx = 2;
        }
        else if(recvaddr.sin_addr.s_addr == servaddr3.sin_addr.s_addr){
            serveridx  = 3;
        }
        else{
            //If received from client pushes the data in queue
            queue[++endindx] = recval;
        }
        if(serveridx){
            //If received from server checks if the queue is empty or not
            if(startidx > endindx){
                //If empty then updates the number of free threads of the server
                int free_threadsv;
                int err = bpf_map_lookup_elem(threadmap, &serveridx, &free_threadsv);
                free_threadsv++;
                bpf_map_update_elem(threadmap, &serveridx, &free_threadsv, 0);
            }
            else{
                //Else dequeues a packet data and sends it to the corresponding server
                int sendv = queue[startidx++];
                sendto(sockfd, &sendv, sizeof(sendv), 0, (const struct sockaddr *)&recvaddr, sizeof(recvaddr));
            }
        }
    }
    return 0;
}