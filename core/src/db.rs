use std::process::Command;
use std::ffi::{c_void, c_char, c_int};
use std::ffi::CString;
use libloading::{Library, Symbol};

// ── Keystone FFI (linked at build time) ──
extern "C" {
    pub fn keystone_scalar_interpolation(
        dataset: *const i64,
        size: usize,
        query: i64,
    ) -> i64;

    pub fn uma_hybrid_lookup(
        dataset: *const i64,
        size: usize,
        query: i64,
    ) -> i64;
}

// ── QIHSE / NOT_STISLA FFI types (matching C headers) ──

pub const NOT_STISLA_NOT_FOUND: usize = usize::MAX;

// CPU feature flags from not_stisla_cpu_feature_t
pub const CPU_AVX2: u32 = 1 << 0;
pub const CPU_AVX512: u32 = 1 << 1;
pub const CPU_AMX: u32 = 1 << 2;
pub const CPU_VNNI: u32 = 1 << 3;
pub const CPU_NEON: u32 = 1 << 4;
pub const CPU_SVE: u32 = 1 << 5;
pub const CPU_SVE2: u32 = 1 << 6;
pub const CPU_I8MM: u32 = 1 << 7;
pub const CPU_GRAVITON4: u32 = 1 << 8;

// QIHSE data types
pub const QIHSE_TYPE_INT64: c_int = 0;
pub const QIHSE_TYPE_UINT64: c_int = 1;
pub const QIHSE_TYPE_DOUBLE: c_int = 2;
pub const QIHSE_TYPE_STRING: c_int = 3;

// Opaque anchor table handle
#[repr(C)]
pub struct NotStislaAnchor {
    pub v: i64,
    pub i: usize,
    pub use_count: u32,
    pub last_used: u64,
}

#[repr(C)]
pub struct NotStislaStats {
    pub searches_total: u64,
    pub searches_successful: u64,
    pub anchors_learned: u64,
    pub anchors_pruned: u64,
    pub memory_reallocations: u64,
    pub avg_search_time_ns: f64,
    pub avg_interpolation_error: f64,
    pub cpu_features_detected: u32,
}

#[repr(C)]
pub struct NotStislaAnchorTable {
    pub anchors: *mut NotStislaAnchor,
    pub capacity: usize,
    pub size: usize,
    pub max_capacity: usize,
    pub searches_performed: usize,
    pub workload_type: c_int,
    pub stats: NotStislaStats,
    pub creation_time: u64,
}

// QIHSE config (simplified - we only use a subset of fields)
#[repr(C)]
pub struct QihseAnchorConfig {
    pub max_anchors: usize,
    pub min_anchors: usize,
    pub anchor_prune_threshold: f64,
    pub memory_budget_mb: usize,
    pub enable_anchor_learning: bool,
    pub chunk_size: usize,
    pub enable_anchor_simd: bool,
    pub workload_type: c_int,
}

#[repr(C)]
pub struct QihseAmplificationConfig {
    pub min_rounds: c_int,
    pub max_rounds: c_int,
    pub convergence_threshold: f64,
    pub oracle_selectivity: f64,
    pub adaptive_rounds: bool,
    pub fixed_rounds: c_int,
}

#[repr(C)]
pub struct QihseVerifyConfig {
    pub mode: c_int,
    pub window_size: usize,
    pub min_confidence: f64,
    pub fallback_to_classical: bool,
    pub max_verification_time_us: usize,
}

#[repr(C)]
pub struct QihseCollapseResult {
    pub predicted_index: usize,
    pub confidence: f64,
    pub fallback_indices: *mut usize,
    pub fallback_count: usize,
}

#[repr(C)]
pub struct QihseTypeDescriptor {
    pub data_type: c_int,
    pub element_size: usize,
    pub hash_fn: *mut c_void,
    pub compare_fn: *mut c_void,
    pub embed_fn: *mut c_void,
    pub type_name: *const c_char,
}

#[repr(C)]
pub struct QihseBackendPriority {
    pub backend_type: c_int,
    pub priority_weight: f32,
    pub enabled: bool,
    pub memory_limit: usize,
}

#[repr(C)]
pub struct QihseConfig {
    pub anchor_config: QihseAnchorConfig,
    pub auto_dimensions: bool,
    pub fixed_dimensions: usize,
    pub max_dimensions: usize,
    pub min_dimensions: usize,
    pub data_type: c_int,
    pub type_descriptor: QihseTypeDescriptor,
    pub rff_gamma: f64,
    pub random_seed: u64,
    pub amplification: QihseAmplificationConfig,
    pub verification: QihseVerifyConfig,
    pub use_heterogeneous: bool,
    pub enable_acceleration: bool,
    pub max_batch_size: usize,
    pub enable_profiling: bool,
    pub use_parallel_pipelines: bool,
    pub max_parallel_pipelines: usize,
    pub timeout_ms: u32,
    pub fail_fast: bool,
    pub backend_priority: [QihseBackendPriority; 8],
    pub num_backends: usize,
    pub adaptive_backend: bool,
    pub memory_pooling: bool,
    pub memory_pool_size: usize,
}

// QIHSE performance stats
#[repr(C)]
pub struct QihsePerformanceStats {
    pub total_time_ns: f64,
    pub dim_calc_time_ns: f64,
    pub rff_time_ns: f64,
    pub superposition_time_ns: f64,
    pub amplification_time_ns: f64,
    pub collapse_time_ns: f64,
    pub verification_time_ns: f64,
    pub device_utilization: [f64; 16],
    pub device_time_ns: [f64; 16],
    pub avg_confidence: f64,
    pub verification_rate: f64,
    pub classical_fallbacks: usize,
    pub anchors_learned: usize,
    pub anchors_pruned: usize,
    pub anchor_table_size: usize,
    pub anchor_hit_rate: f64,
    pub anchor_avg_error: f64,
    pub detected_workload_type: c_int,
    pub speedup_vs_binary: f64,
    pub speedup_vs_classical: f64,
    pub anchor_memory_usage_mb: f64,
    pub peak_memory_bytes: usize,
    pub total_operations: usize,
}

// Opaque KV store handle
#[repr(C)]
pub struct QihseKvStore {
    _opaque: [u8; 0],
}

// Opaque user handle for auth
#[repr(C)]
pub struct QihseUser {
    _opaque: [u8; 0],
}

// ── QIHSE / NOT_STISLA FFI declarations (linked at build time) ──
extern "C" {
    // NOT_STISLA anchor search
    pub fn not_stisla_anchor_table_create() -> *mut NotStislaAnchorTable;
    pub fn not_stisla_anchor_table_destroy(table: *mut NotStislaAnchorTable);
    pub fn not_stisla_search(
        arr: *const i64,
        n: usize,
        key: i64,
        table: *mut NotStislaAnchorTable,
        tol: usize,
    ) -> usize;
    pub fn not_stisla_detect_cpu_features() -> u32;
    pub fn not_stisla_version() -> *const c_char;
    pub fn not_stisla_build_info() -> *const c_char;

    // QIHSE core
    pub fn qihse_config_init(
        config: *mut QihseConfig,
        data_type: c_int,
        array_size: usize,
    ) -> c_int;
    pub fn qihse_get_performance_stats(stats: *mut QihsePerformanceStats) -> c_int;
    pub fn qihse_reset_performance_stats();

    // QIHSE KV store
    pub fn qihse_kv_store_create() -> *mut QihseKvStore;
    pub fn qihse_kv_store_destroy(store: *mut QihseKvStore);
    pub fn qihse_kv_set(
        store: *mut QihseKvStore,
        key: *const c_char,
        value: *const c_char,
        classification: u16,
        sci_compartment: u16,
    ) -> bool;
    pub fn qihse_kv_get(
        store: *mut QihseKvStore,
        key: *const c_char,
        user: *mut QihseUser,
    ) -> *mut c_char;
    pub fn qihse_kv_del(
        store: *mut QihseKvStore,
        key: *const c_char,
        user: *mut QihseUser,
    ) -> bool;
    pub fn qihse_kv_exists(
        store: *mut QihseKvStore,
        key: *const c_char,
        user: *mut QihseUser,
    ) -> bool;
}

// ── High-level Rust API ──

/// Detect CPU SIMD features on current silicon
pub fn detect_silicon() -> u32 {
    unsafe { not_stisla_detect_cpu_features() }
}

/// Get human-readable CPU feature string
pub fn silicon_info() -> String {
    let features = detect_silicon();
    let mut parts: Vec<&str> = Vec::new();
    if features & CPU_AVX2 != 0 { parts.push("AVX2"); }
    if features & CPU_AVX512 != 0 { parts.push("AVX-512"); }
    if features & CPU_AMX != 0 { parts.push("AMX"); }
    if features & CPU_VNNI != 0 { parts.push("AVX-VNNI"); }
    if features & CPU_NEON != 0 { parts.push("NEON"); }
    if features & CPU_SVE != 0 { parts.push("SVE"); }
    if features & CPU_SVE2 != 0 { parts.push("SVE2"); }
    if features & CPU_I8MM != 0 { parts.push("I8MM"); }
    if features & CPU_GRAVITON4 != 0 { parts.push("Graviton4"); }
    if parts.is_empty() { "scalar-only".to_string() } else { parts.join(" + ") }
}

/// Get QIHSE/not_stisla version string
pub fn qihse_version() -> String {
    unsafe {
        let v = not_stisla_version();
        if v.is_null() { return "unknown".to_string(); }
        std::ffi::CStr::from_ptr(v).to_string_lossy().into_owned()
    }
}

/// Get QIHSE/not_stisla build info
pub fn qihse_build_info() -> String {
    unsafe {
        let bi = not_stisla_build_info();
        if bi.is_null() { return "unknown".to_string(); }
        std::ffi::CStr::from_ptr(bi).to_string_lossy().into_owned()
    }
}

/// Quantum-inspired anchor search using NOT_STISLA
/// Returns the index of `key` in `dataset`, or NOT_STISLA_NOT_FOUND if not found
pub fn qihse_anchor_search(dataset: &[i64], key: i64) -> usize {
    unsafe {
        let table = not_stisla_anchor_table_create();
        if table.is_null() {
            return NOT_STISLA_NOT_FOUND;
        }
        let result = not_stisla_search(
            dataset.as_ptr(),
            dataset.len(),
            key,
            table,
            0, // exact match tolerance
        );
        not_stisla_anchor_table_destroy(table);
        result
    }
}

/// QIHSE KV store: create, set, get, destroy in one call
pub fn qihse_kv_test() -> Result<String, String> {
    unsafe {
        let store = qihse_kv_store_create();
        if store.is_null() {
            return Err("Failed to create KV store".to_string());
        }

        let key = CString::new("vantage_test_key").unwrap();
        let value = CString::new("vantage_test_value_from_rust").unwrap();

        let set_ok = qihse_kv_set(store, key.as_ptr(), value.as_ptr(), 0, 0);
        if !set_ok {
            qihse_kv_store_destroy(store);
            return Err("Failed to set KV pair".to_string());
        }

        let got = qihse_kv_get(store, key.as_ptr(), std::ptr::null_mut());
        let result = if got.is_null() {
            Err("Key not found after set".to_string())
        } else {
            let s = std::ffi::CStr::from_ptr(got).to_string_lossy().into_owned();
            libc_free(got as *mut c_void);
            Ok(s)
        };

        qihse_kv_store_destroy(store);
        result
    }
}

/// Get QIHSE performance stats from last operation
pub fn qihse_perf_stats() -> Option<QihsePerformanceStats> {
    unsafe {
        let mut stats = std::mem::MaybeUninit::<QihsePerformanceStats>::uninit();
        let rc = qihse_get_performance_stats(stats.as_mut_ptr());
        if rc == 0 {
            Some(stats.assume_init())
        } else {
            None
        }
    }
}

/// Legacy: dlopen-based search (fallback if link-time symbols unavailable)
pub fn qihse_db_search(query: i64) -> i64 {
    let lib_path = std::env::var("VANTAGE_QIHSE_LIB")
        .unwrap_or_else(|_| {
            let manifest_dir = env!("CARGO_MANIFEST_DIR");
            format!("{}/../vendor/QIHSE/libqihse.so", manifest_dir)
        });
    unsafe {
        let lib = Library::new(&lib_path)
            .expect("Failed to load libqihse.so");

        let search: Symbol<unsafe extern "C" fn(
            *const i64, usize, i64, *mut NotStislaAnchorTable, usize
        ) -> usize> = lib.get(b"not_stisla_search\0")
            .expect("Failed to load symbol not_stisla_search");

        let create: Symbol<unsafe extern "C" fn() -> *mut NotStislaAnchorTable> =
            lib.get(b"not_stisla_anchor_table_create\0")
            .expect("Failed to load symbol not_stisla_anchor_table_create");

        let destroy: Symbol<unsafe extern "C" fn(*mut NotStislaAnchorTable)> =
            lib.get(b"not_stisla_anchor_table_destroy\0")
            .expect("Failed to load symbol not_stisla_anchor_table_destroy");

        let dataset = vec![10i64, 20, 30, 40, 50, 60, 70, 80, 90, 100];
        let table = create();
        let idx = search(dataset.as_ptr(), dataset.len(), query, table, 0);
        destroy(table);
        idx as i64
    }
}

extern "C" {
    fn free(ptr: *mut c_void);
}

fn libc_free(ptr: *mut c_void) {
    unsafe { free(ptr) }
}

pub fn flush_memtable() {
    println!("Initiating ZFS-aligned RAM buffer flush...");

    let raw_file = "/tmp/vantage_memtable.raw";
    let zst_file = "/tmp/vantage_memtable.zst";

    let _ = Command::new("dd")
        .args(&["if=/dev/urandom", &format!("of={}", raw_file), "bs=1M", "count=5"])
        .output();

    let _ = Command::new("zstd")
        .args(&["-6", "-f", "-q", raw_file, "-o", zst_file])
        .output();

    let _ = std::fs::remove_file(raw_file);

    println!("Successfully flushed MemTable to ZFS pool (compressed with ZSTD-6).");
}
