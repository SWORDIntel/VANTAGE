use std::fs;
use std::path::Path;

fn main() {
    let db_src_dir = Path::new("../db/src");
    let net_c_dir = Path::new("../net/c");
    let net_c_w_slam_dir = Path::new("../net/c/w-slam-server");

    // Compile DB (C++)
    let mut db_build = cc::Build::new();
    db_build.cpp(true)
        .include("../db/include")
        .include("/home/john/QIHSE/not_stisla/include")
        .include("/home/john/KEYSTONE/include")
        .include("/home/john/QIHSE/qihse")
        .flag("-fopenmp");
    if let Ok(entries) = fs::read_dir(db_src_dir) {
        for entry in entries.flatten() {
            let path = entry.path();
            if path.extension().and_then(|s| s.to_str()) == Some("cpp") {
                let file_name = path.file_name().unwrap().to_string_lossy();
                // Avoid compiling main.cpp to prevent multiple definition of `main`
                if file_name != "main.cpp" {
                    db_build.file(&path);
                }
            }
        }
    }
    
    // Also compile KEYSTONE since it's required by db
    let keystone_src_dir = Path::new("/home/john/KEYSTONE/src");
    let mut keystone_build = cc::Build::new();
    keystone_build.include("/home/john/KEYSTONE/include")
        .include("/home/john/KEYSTONE/include/hardware")
        .include("/home/john/KEYSTONE/include/models");
        
    if let Ok(entries) = fs::read_dir(keystone_src_dir) {
        for entry in entries.flatten() {
            let path = entry.path();
            if path.extension().and_then(|s| s.to_str()) == Some("c") {
                let file_name = path.file_name().unwrap().to_string_lossy();
                if file_name != "keystone_tar_zst.c" { // usually standalone or tool
                    keystone_build.file(&path);
                }
            }
        }
    }
    keystone_build.compile("keystone_lib");
    db_build.compile("hybrid_db");

    // Compile NET (C)
    let mut net_build = cc::Build::new();
    let dirs = vec![net_c_dir];
    for dir in dirs {
        if let Ok(entries) = fs::read_dir(dir) {
            for entry in entries.flatten() {
                let path = entry.path();
                if path.extension().and_then(|s| s.to_str()) == Some("c") {
                    let file_name = path.file_name().unwrap().to_string_lossy();
                    // Avoid compiling test files or executables
                    if !file_name.starts_with("test_") && file_name != "conformance_probe.c" {
                        net_build.file(&path);
                    }
                }
            }
        }
    }
    net_build.compile("msnet");

    println!("cargo:rerun-if-changed=../db/src");
    println!("cargo:rerun-if-changed=../net/c");
    println!("cargo:rustc-link-search=native=/home/john/QIHSE/qihse");
    println!("cargo:rustc-link-lib=dylib=qihse");
}
