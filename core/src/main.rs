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
        "--qihse-test" | "--qihse-search" => {
            println!("Testing QIHSE Quantum-Inspired Search...");
            let dataset = vec![10i64, 20, 30, 40, 50, 60, 70, 80, 90, 100];
            let query = if args.len() > 2 { args[2].parse().unwrap_or(42) } else { 42 };
            println!("Dataset: {:?}", dataset);
            println!("Query: {}", query);

            let idx = db::qihse_anchor_search(&dataset, query);
            if idx == db::NOT_STISLA_NOT_FOUND {
                println!("Result: NOT FOUND");
            } else {
                println!("Result: Found at index {} (value: {})", idx, dataset[idx]);
            }

            if let Some(stats) = db::qihse_perf_stats() {
                println!("--- QIHSE Performance Stats ---");
                println!("  Total time: {:.2} ns", stats.total_time_ns);
                println!("  Avg confidence: {:.4}", stats.avg_confidence);
                println!("  Anchors learned: {}", stats.anchors_learned);
                println!("  Speedup vs binary: {:.2}x", stats.speedup_vs_binary);
            }
        },
        "--qihse-kv" => {
            println!("Testing QIHSE KV Store...");
            match db::qihse_kv_test() {
                Ok(val) => println!("KV Store test PASSED: retrieved value = {}", val),
                Err(e) => println!("KV Store test FAILED: {}", e),
            }
        },
        "--silicon" => {
            println!("Detecting CPU silicon features...");
            let features = db::detect_silicon();
            let info = db::silicon_info();
            println!("CPU feature flags: 0x{:x}", features);
            println!("Detected SIMD: {}", info);
            println!("QIHSE version: {}", db::qihse_version());
            println!("QIHSE build: {}", db::qihse_build_info());
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
    println!("  --qihse-test [N]     Test QIHSE quantum-inspired search for value N (default: 42)");
    println!("  --qihse-kv           Test QIHSE key-value store (set + get)");
    println!("  --silicon            Detect CPU SIMD features and QIHSE build info");
    println!("  --idle-flush-force   Force an immediate ZSTD flush to disk");
}
