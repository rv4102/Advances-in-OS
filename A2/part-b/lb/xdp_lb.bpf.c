//Group members: Rushil Venkateswar(20CS30045), Jay Kumar Thakur(20CS30024)
#include"helper.h"
struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, 128);
	__type(key, int);
	__type(value, __u32);
} ip_addrs SEC(".maps");
struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, 128);
	__type(key, int);
	__type(value, int);
} free_threads SEC(".maps");
SEC("xdp")
int lb_packets(struct xdp_md *ctx){
     void *data_end = (void *)(long)ctx->data_end;
    void *data = (void *)(long)ctx->data;
    struct ethhdr *eth = data;
    //Check is packet packaging is correct or not
    if(data + sizeof(struct ethhdr) > data_end){
        char fmt[] = "Invalid packet packaging, hence dropped\n";
        bpf_trace_printk(fmt, sizeof(fmt));
        return XDP_DROP;
    }
     if(bpf_ntohs(eth->h_proto) != ETH_P_IP){
        char fmt[] = "Not an IP packet, packet passed\n";
         bpf_trace_printk(fmt, sizeof(fmt));
         return XDP_PASS;
    }
    struct iphdr *ip = data + sizeof(struct ethhdr);
    
    if(data + sizeof(struct ethhdr) + sizeof(struct iphdr) > data_end){
        char fmt[] = "Invalid packet packaging, hence dropped\n";
        bpf_trace_printk(fmt, sizeof(fmt));
        return XDP_DROP;
    }
    if(ip->protocol != IPPROTO_UDP){
        char fmt[] = "Not a UDP packet, packet passed\n";
         bpf_trace_printk(fmt, sizeof(fmt));
        return XDP_PASS;
    }
    struct udphdr *udp = data + sizeof(struct ethhdr) + sizeof(struct iphdr);
    if(data + sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr) > data_end){
        char fmt[] = "Invalid packet packaging, hence dropped\n";
        bpf_trace_printk(fmt, sizeof(fmt));
        return XDP_DROP;
    }
    //Check if the packet is for the loadbalancer or not
    if(ntohs(udp->dest) != 8080){
        char fmt[] = "Not directed to port 8080, packet passed\n";
         bpf_trace_printk(fmt, sizeof(fmt));
        return XDP_PASS;
    }
    //If for load balancer check if it is from any server or not
    int is_server = 0;
    int zero = 0, key = 1, map_empty = 0;
    __u32 *value;
    __u32 ip_addr;
    //Check for ip address of each server and if it matches with the source ip
    value = bpf_map_lookup_elem(&ip_addrs, &key);
    if(value != NULL){
        ip_addr = *value;
        if(ip_addr == ip->saddr){
            is_server = key;
        }
    }
    else
        map_empty++;
    key = 2;
    value = bpf_map_lookup_elem(&ip_addrs, &key);
    if(value != NULL){
        ip_addr = *value;
        if(ip_addr == ip->saddr){
            is_server = key;
        }
    }
    else
        map_empty++;
    key = 3;
    value = bpf_map_lookup_elem(&ip_addrs, &key);
    if(value != NULL){
        ip_addr = *value;
        if(ip_addr == ip->saddr){
            is_server = key;
        }
    }
    else
        map_empty++;
    //If packet is from server or if the server map is empty then pass the packet up network stack
    if(is_server || map_empty == 3){
        char server_msg[] = "Server %u has got a free thread\n";
        bpf_trace_printk(server_msg, sizeof(server_msg), is_server);
        return XDP_PASS;
    }
    //Now check for free threads in each server, the logic is to choose the server with maximum number of free threads
    zero = 0; key = 1;
    int max_f = 0, servidx = -1;
    int free_f;
    map_empty = 0;
    int *valuev;
    //Check for each server index
    valuev = bpf_map_lookup_elem(&free_threads, &key);
    if(valuev != NULL){
        free_f = *valuev;
        if(free_f > max_f){
            servidx = key;
            max_f = free_f;
        }
    }
    else
        map_empty++;
    key  = 2;
    valuev = bpf_map_lookup_elem(&free_threads, &key);
    if(valuev != NULL){
        free_f = *valuev;
        if(free_f > max_f){
            servidx = key;
            max_f = free_f;
        }
    }
    else
        map_empty++;
    key = 3;
    valuev = bpf_map_lookup_elem(&free_threads, &key);
    if(valuev != NULL){
        free_f = *valuev;
        if(free_f > max_f){
            servidx = key;
            max_f = free_f;
        }
    }
    else
        map_empty++;
    //If max_f = 0 which means no server has any free thread or if free_thread map is empty pass the packet up the network stack
    if(!max_f || map_empty == 3){
        char queue_msg[] = "No free server, packet pushed into queue\n";
        bpf_trace_printk(queue_msg, sizeof(queue_msg));
        return XDP_PASS;
    }
    __u32 dest_ip;
    //Check the IP address of the chosen server
    value  = bpf_map_lookup_elem(&ip_addrs, &servidx);
    if(value == NULL)
        return XDP_PASS;
    char redirect_msg[] = "Packet redirected to server %u\n";
    bpf_trace_printk(redirect_msg, sizeof(redirect_msg), servidx);
    //Update the free_thread of the server, change the source & destination IP and source UDP port to 8080(dest UDP port is still 8080)
    //Calculate checksums and redirect the packet.
    dest_ip = *value;
    max_f--;
    bpf_map_update_elem(&free_threads, &servidx, &max_f, 2);
    ip->saddr = ip->daddr;
    eth->h_source[5] = ip->saddr >> 24;
    ip->daddr = dest_ip;
    eth->h_dest[5] = dest_ip >> 24;
    ip->check = iph_csum(ip);
    __u16 dest_port = 8080;
    udp->check = update_udp_header_port(udp, &dest_port);
    return XDP_TX;
   
}


char _license[] SEC("license") = "GPL";
