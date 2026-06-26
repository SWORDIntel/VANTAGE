use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;

fn main() {
    let db_src_dir = Path::new("../db/src");
    let net_c_dir = Path::new("../net/c");
    let qihse_dir = PathBuf::from("../vendor/QIHSE");
    let keystone_dir = std::env::var("KEYSTONE_DIR")
        .map(|d| PathBuf::from(&d))
        .unwrap_or_else(|_| PathBuf::from("../vendor/KEYSTONE"));

    // ── Build QIHSE libqihse.so for current silicon if not present ──
    let qihse_so = qihse_dir.join("libqihse.so");
    if !qihse_so.exists() {
        let build_script = qihse_dir.join("scripts/build-native.sh");
        if build_script.exists() {
            println!("cargo:warning=Building QIHSE for current silicon (build-native.sh --auto)...");
            let result = Command::new("sh")
                .arg(&build_script)
                .arg("--auto")
                .arg("--verbose")
                .current_dir(&qihse_dir)
                .status();
            match result {
                Ok(s) if s.success() => {
                    println!("cargo:warning=QIHSE built successfully for current silicon.");
                }
                _ => {
                    println!("cargo:warning=QIHSE build-native.sh failed, falling back to make lib...");
                    let _ = Command::new("make")
                        .arg("lib")
                        .current_dir(&qihse_dir)
                        .status();
                }
            }
        } else {
            println!("cargo:warning=build-native.sh not found, trying make lib...");
            let _ = Command::new("make")
                .arg("lib")
                .current_dir(&qihse_dir)
                .status();
        }
    }

    // ── Link against QIHSE shared library ──
    println!("cargo:rustc-link-search=native={}", qihse_dir.display());
    if qihse_so.exists() {
        println!("cargo:rustc-link-lib=dylib=qihse");
    } else {
        println!("cargo:warning=libqihse.so not found - QIHSE features will use dlopen fallback");
    }
    println!("cargo:rerun-if-changed={}", qihse_dir.join("libqihse.so").display());

    // ── Compile DB (C++) ──
    let mut db_build = cc::Build::new();
    db_build.cpp(true)
        .include("../db/include")
        .include(qihse_dir.join("not_stisla/include"))
        .include(keystone_dir.join("include"))
        .include(qihse_dir.join("include"))
        .include(qihse_dir.join("algorithms"))
        .flag("-fopenmp");
    if let Ok(entries) = fs::read_dir(db_src_dir) {
        for entry in entries.flatten() {
            let path = entry.path();
            if path.extension().and_then(|s| s.to_str()) == Some("cpp") {
                let file_name = path.file_name().unwrap().to_string_lossy();
                if file_name != "main.cpp" {
                    db_build.file(&path);
                }
            }
        }
    }

    // ── Compile KEYSTONE (required by db) ──
    let keystone_src_dir = keystone_dir.join("src");
    let mut keystone_build = cc::Build::new();
    keystone_build.include(keystone_dir.join("include"))
        .include(keystone_dir.join("include/hardware"))
        .include(keystone_dir.join("include/models"));

    if let Ok(entries) = fs::read_dir(&keystone_src_dir) {
        for entry in entries.flatten() {
            let path = entry.path();
            if path.extension().and_then(|s| s.to_str()) == Some("c") {
                let file_name = path.file_name().unwrap().to_string_lossy();
                if file_name != "keystone_tar_zst.c" {
                    keystone_build.file(&path);
                }
            }
        }
    }
    keystone_build.compile("keystone_lib");
    db_build.compile("hybrid_db");

    // ── Compile NET (C) ──
    let mut net_build = cc::Build::new();
    let dirs = vec![net_c_dir];
    for dir in dirs {
        if let Ok(entries) = fs::read_dir(dir) {
            for entry in entries.flatten() {
                let path = entry.path();
                if path.extension().and_then(|s| s.to_str()) == Some("c") {
                    let file_name = path.file_name().unwrap().to_string_lossy();
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
}
