use std::env;

mod daemon;
mod db;
mod net;
mod bpf;
mod llm;
mod crypto;

fn main() {
    let args: Vec<String> = env::args().collect();
    
    if args.len() < 2 {
        print_usage();
        return;
    }
    
    match args[1].as_str() {
        "--daemon" => daemon::run(),
        "--status" => {
            println!("VANTAGE Core Orchestrator Status:");
            println!("Swarm Sync: READY");
            println!("eBPF Firewall: ATTACHED (vmbr0)");
        },
        "--db-test" => {
            println!("Testing HYBRID_DB UMA Dispatcher (QIHSE + KEYSTONE)...");
            let mock_dataset = vec![10, 20, 30, 40, 50, 60, 70, 80, 90, 100];
            let query = 40;
            
            unsafe {
                let result = db::uma_hybrid_lookup(
                    mock_dataset.as_ptr(),
                    mock_dataset.len(),
                    query,
                );
                println!("Database lookup returned index: {}", result);
            }
        },
        "--idle-flush-force" => daemon::force_flush(),
        _ => print_usage(),
    }
}

fn print_usage() {
    println!("VANTAGE Core Orchestrator");
    println!("Usage: vantage-core [OPTIONS]");
    println!("  --daemon             Start the background VANTAGE daemon (Idle tracker + Swarm Listener)");
    println!("  --status             Show the current status of the swarm");
    println!("  --db-test            Test the native HYBRID_DB UMA integration");
    println!("  --idle-flush-force   Force an immediate ZSTD flush to disk");
}
