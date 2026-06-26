use std::process::Command;

pub struct BpfFirewall {
    map_name: String,
}

impl BpfFirewall {
    pub fn new() -> Self {
        BpfFirewall {
            map_name: "hybrid_db_map".to_string(),
        }
    }

    pub fn block_ip(&self, ip_uint: u64) {
        println!("eBPF Map Updater: Inserting 64-bit IP {} into Kernel Firewall...", ip_uint);
        
        // Convert u64 to byte array for bpftool syntax
        // key format for bpftool: hex bytes separated by space
        let bytes = ip_uint.to_ne_bytes();
        let key_str = bytes.iter()
            .map(|b| format!("{:02x}", b))
            .collect::<Vec<String>>()
            .join(" ");
            
        // value doesn't strictly matter based on vantage_xdp.c, but map needs 4 bytes (u32)
        let val_str = "01 00 00 00"; 
        
        // Execute bpftool to update the map
        // We use pinned map or look up by name
        let status = Command::new("sudo")
            .args(&[
                "bpftool", "map", "update", "name", &self.map_name,
                "key", "hex"
            ])
            .args(key_str.split_whitespace())
            .args(&["value", "hex"])
            .args(val_str.split_whitespace())
            .status();
            
        if let Ok(st) = status {
            if st.success() {
                println!("Packet drop rule securely injected into kernel via eBPF (<10ns enforcement).");
            } else {
                println!("Warning: bpftool map update failed (Map might not be loaded or requires root permissions).");
            }
        } else {
            println!("Warning: Could not execute bpftool. Is it installed?");
        }
    }
}
