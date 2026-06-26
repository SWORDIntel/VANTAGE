use serde::Serialize;
use std::env;
use std::process::Command;

pub mod hybrid_db;

#[derive(Serialize)]
struct ToolStatus {
    installed: bool,
}

#[derive(Serialize)]
struct HealthStatus {
    git: ToolStatus,
    jq: ToolStatus,
    curl: ToolStatus,
    wezterm: ToolStatus,
}

fn check_tool(name: &str) -> bool {
    match Command::new(name).arg("--version").output() {
        Ok(_) => true,
        Err(e) => {
            if e.kind() == std::io::ErrorKind::NotFound {
                false
            } else {
                true
            }
        }
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        eprintln!("Usage: sentinel-core <command>");
        std::process::exit(1);
    }

    let command = &args[1];
    match command.as_str() {
        "health" => {
            let db_res = hybrid_db::db_search(10);
            println!("Hybrid DB test result: {}", db_res);
            let status = HealthStatus {
                git: ToolStatus { installed: check_tool("git") },
                jq: ToolStatus { installed: check_tool("jq") },
                curl: ToolStatus { installed: check_tool("curl") },
                wezterm: ToolStatus { installed: check_tool("wezterm") },
            };
            match serde_json::to_string_pretty(&status) {
                Ok(json) => println!("{}", json),
                Err(e) => {
                    eprintln!("Error serializing JSON: {}", e);
                    std::process::exit(1);
                }
            }
        }
        "osint" => {
            println!("OSINT command placeholder");
            let result = hybrid_db::db_search(42);
            println!("Hybrid DB OSINT Search result: {}", result);
        }
        _ => {
            eprintln!("Unknown command: {}", command);
            std::process::exit(1);
        }
    }
}
