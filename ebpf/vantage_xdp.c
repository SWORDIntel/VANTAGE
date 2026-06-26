#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <linux/if_ether.h>
#include <linux/ip.h>

/* Fallback for bpf_htons if <bpf/bpf_endian.h> is not included */
#ifndef bpf_htons
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define bpf_htons(x) __builtin_bswap16(x)
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define bpf_htons(x) (x)
#else
#define bpf_htons(x) __builtin_bswap16(x)
#endif
#endif

/*
 * Define the BPF Map to store 64-bit IPs.
 * We use BPF_MAP_TYPE_HASH since it's optimal for lookups.
 */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 100000);
    __type(key, __u32); // Changed to __u32 to match pure IPv4 saddr
    __type(value, __u32); // The value type doesn't strictly matter for pure blocking
} hybrid_db_map SEC(".maps");

SEC("xdp")
int vantage_xdp_firewall(struct xdp_md *ctx) {
    // Cast context pointers to generic void pointers for boundaries
    void *data_end = (void *)(long)ctx->data_end;
    void *data = (void *)(long)ctx->data;

    // Extract Ethernet Header
    struct ethhdr *eth = data;
    
    // Verify Ethernet header fits within packet bounds
    if ((void *)(eth + 1) > data_end)
        return XDP_PASS;

    __u16 h_proto = eth->h_proto;
    int hdr_offset = sizeof(*eth);

    // Unroll VLAN tag unwrapping (Max 2 tags for typical QinQ)
    #pragma unroll
    for (int i = 0; i < 2; i++) {
        if (h_proto == bpf_htons(ETH_P_8021Q) || h_proto == bpf_htons(ETH_P_8021AD)) {
            struct vlan_hdr {
                __be16 h_vlan_TCI;
                __be16 h_vlan_encapsulated_proto;
            };
            struct vlan_hdr *vhdr = data + hdr_offset;
            
            if ((void *)(vhdr + 1) > data_end)
                return XDP_PASS;
                
            h_proto = vhdr->h_vlan_encapsulated_proto;
            hdr_offset += sizeof(*vhdr);
        } else {
            break;
        }
    }

    // We only want to inspect IPv4 packets (ETH_P_IP = 0x0800)
    if (h_proto != bpf_htons(ETH_P_IP))
        return XDP_PASS;

    // Extract IP Header
    struct iphdr *ip = data + hdr_offset;
    
    // Verify IP header fits within packet bounds
    if ((void *)(ip + 1) > data_end)
        return XDP_PASS;

    // Extract IPv4 source address directly as 32-bit
    __u32 src_ip32 = ip->saddr;

    // Look up the 32-bit IP in our malicious HYBRID_DB blocklist
    __u32 *val = bpf_map_lookup_elem(&hybrid_db_map, &src_ip32);
    if (val) {
        // IP was found in the map, drop the packet
        return XDP_DROP;
    }

    // IP not found, let it pass
    return XDP_PASS;
}

char _license[] SEC("license") = "GPL";
