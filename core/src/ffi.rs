use std::ffi::CStr;
use std::os::raw::{c_char, c_int, c_uchar, c_uint};

#[link(name = "kanzi", kind = "dylib")]
extern "C" {
    // Simulated libkanzi FFI signatures
    fn kanzi_compress(
        src: *const c_uchar,
        src_len: c_uint,
        dst: *mut c_uchar,
        dst_capacity: c_uint,
        out_len: *mut c_uint,
    ) -> c_int;

    fn kanzi_get_version() -> *const c_char;
}

pub struct KanziCompressor;

impl KanziCompressor {
    /// Safe FFI boundary for Kanzi Compression
    pub fn compress(data: &[u8]) -> Result<Vec<u8>, String> {
        if data.is_empty() {
            return Err("Cannot compress empty data".to_string());
        }

        if data.len() > u32::MAX as usize {
            return Err("Data too large for Kanzi".to_string());
        }

        // Allocate destination buffer with safety margins
        let dst_capacity = (data.len() + 1024) as u32; 
        let mut dst_buffer: Vec<u8> = vec![0; dst_capacity as usize];
        let mut out_len: c_uint = 0;

        // FFI Call
        let result = unsafe {
            kanzi_compress(
                data.as_ptr(),
                data.len() as c_uint,
                dst_buffer.as_mut_ptr(),
                dst_capacity,
                &mut out_len,
            )
        };

        if result != 0 {
            return Err(format!("libkanzi compression failed with code: {}", result));
        }

        // Boundary Check: Never trust the C out_len blindly
        if out_len > dst_capacity {
            return Err("Critical FFI Boundary Violation: Kanzi returned out_len > capacity!".to_string());
        }

        dst_buffer.truncate(out_len as usize);
        Ok(dst_buffer)
    }

    /// Safely handles raw C strings from FFI
    pub fn get_version() -> String {
        let ptr = unsafe { kanzi_get_version() };
        if ptr.is_null() {
            return "Unknown (Null Pointer)".to_string();
        }

        // Safe conversion preventing buffer over-reads and enforcing UTF-8
        let c_str = unsafe { CStr::from_ptr(ptr) };
        c_str.to_string_lossy().into_owned()
    }
}
