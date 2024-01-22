//Group members: Rushil Venkateswar(20CS30045), Jay Kumar Thakur(20CS30024)
#include <linux/types.h>
#include <bpf/bpf_helpers.h>
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <arpa/inet.h>
#include <bpf/bpf_endian.h>

SEC("xdp")
int filter_packets(struct xdp_md *ctx) {
    void *data_end = (void *)(long)ctx->data_end;
    void *data = (void *)(long)ctx->data;
    struct ethhdr *eth = data;

    // Check if packet packaging is correct or not using headers
    if (data + sizeof(struct ethhdr) > data_end)
    {
        char fmt[] = "Invalid packet packaging, hence dropped\n";
        bpf_trace_printk(fmt, sizeof(fmt));
        return XDP_DROP;
    }
    if (bpf_ntohs(eth->h_proto) != ETH_P_IP)
    {
        char fmt[] = "Not an IP packet, packet passed\n";
        bpf_trace_printk(fmt, sizeof(fmt));
        return XDP_PASS;
    }
    struct iphdr *ip = data + sizeof(struct ethhdr);
    if (data + sizeof(struct ethhdr) + sizeof(struct iphdr) > data_end)
    {
        char fmt[] = "Invalid packet packaging, hence dropped\n";
        bpf_trace_printk(fmt, sizeof(fmt));
        return XDP_DROP;
    }
    if (ip->protocol != IPPROTO_UDP)
    {
        char fmt[] = "Not a UDP packet, packet passed\n";
        bpf_trace_printk(fmt, sizeof(fmt));
        return XDP_PASS;
    }
    struct udphdr *udp = data + sizeof(struct ethhdr) + sizeof(struct iphdr);
    if (data + sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr) > data_end)
    {
        char fmt[] = "Invalid packet packaging, hence dropped\n";
        bpf_trace_printk(fmt, sizeof(fmt));
        return XDP_DROP;
    }
    if (ntohs(udp->dest) != 8080)
    {
        char fmt[] = "Not directed to port 8080, packet passed\n";
        bpf_trace_printk(fmt, sizeof(fmt));
        return XDP_PASS;
    }
    
    void *payload = udp+1;
    int payloadLength = data_end - payload;
    if(payload + sizeof(int) > data_end)
        return XDP_PASS;
    int *payloadData = (int*)payload;
    if((*payloadData) % 2 == 0){
        char msg[] = "Packet with even payload %u directed to port 8080, hence dropped\n";
        bpf_trace_printk(msg, sizeof(msg), *payloadData);
        return XDP_DROP;
    }
    char msg[] = "Packet with odd payload %u directed to port 8080, hence passed\n";
    bpf_trace_printk(msg, sizeof(msg), *payloadData);
    return XDP_PASS;
}

char _license[] SEC("license") = "GPL";
