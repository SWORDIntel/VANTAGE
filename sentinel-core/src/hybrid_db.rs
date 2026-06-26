use libloading::{Library, Symbol};

pub fn db_search(query: i64) -> i64 {
    unsafe {
        let lib = Library::new("/home/john/QIHSE/qihse/libqihse.so")
            .expect("Failed to load libqihse.so");
        
        let qihse_init: Symbol<unsafe extern "C" fn()> = lib.get(b"qihse_init\0")
            .expect("Failed to load symbol qihse_init");
        qihse_init();
        
        let qihse_search: Symbol<unsafe extern "C" fn(i64) -> i64> = lib.get(b"qihse_search\0")
            .expect("Failed to load symbol qihse_search");
        qihse_search(query)
    }
}
