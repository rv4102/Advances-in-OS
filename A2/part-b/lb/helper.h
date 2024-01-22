//Group members: Rushil Venkateswar(20CS30045), Jay Kumar Thakur(20CS30024)
#include<linux/types.h>
#include<bpf/bpf_helpers.h>
#include<linux/bpf.h>
#include<linux/if_ether.h>
#include<linux/ip.h>
#include<linux/udp.h>
#include<arpa/inet.h>
#include <netinet/in.h>
#include <bpf/bpf_endian.h>


static __always_inline __u16
csum_fold_helper(__u64 csum)
{
    int i;
#pragma unroll
    for (i = 0; i < 4; i++)
    {
        if (csum >> 16)
            csum = (csum & 0xffff) + (csum >> 16);
    }
    return ~csum;
}
//Calculate iphdr checksum
static __always_inline __u16
iph_csum(struct iphdr *iph)
{
    iph->check = 0;
    unsigned long long csum = bpf_csum_diff(0, 0, (unsigned int *)iph, sizeof(struct iphdr), 0);
    return csum_fold_helper(csum);
}
//Update udphdr checksum when source port is changed 
static __always_inline __u16 
update_udp_header_port(struct udphdr* udp, __u16 *new_val)
{
    __u16 old_check = udp->check;
    __u32 new_csum_value;
    __u32 new_csum_comp;
    __u32 undo;

 
    undo = ~((__u32) udp->check) + ~((__u32) udp->source);

  
    new_csum_value = undo + (undo < ~((__u32) udp->source)) + (__u32) *new_val;

    new_csum_comp = new_csum_value + (new_csum_value < ((__u32) *new_val));

    /* Add any overflow of the 16 bit value to itself */
    new_csum_comp = (new_csum_comp & 0xFFFF) + (new_csum_comp >> 16);

    /* Check that overflow added above did not cause another overflow */
    new_csum_comp = (new_csum_comp & 0xFFFF) + (new_csum_comp >> 16);
    udp->check = (__u16) ~new_csum_comp;
    /* Cast to 16 bit one's compliment of sum of headers */
    // udp->check = (__u16) ~new_csum_comp;
    udp->source = (__u16)bpf_ntohs(*new_val);
    return udp->check;
}