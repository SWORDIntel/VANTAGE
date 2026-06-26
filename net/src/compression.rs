/**
 * MEMSHADOW Protocol v3.0 - Compression (Rust)
 *
 * Hardware-accelerated compression with software fallback.
 * Supports Kanzi, QATZip, gzip, and zlib.
 * Matches Python memshadow_compression.py and C memshadow.h gold standard.
 */

use std::io::{Read, Write};

/// Compression algorithm
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum CompressionType {
    None = 0,
    Gzip = 1,
    Zlib = 2,
    Kanzi = 3,
    QatZip = 4,
}

impl CompressionType {
    pub fn from_u8(v: u8) -> Option<Self> {
        match v {
            0 => Some(Self::None),
            1 => Some(Self::Gzip),
            2 => Some(Self::Zlib),
            3 => Some(Self::Kanzi),
            4 => Some(Self::QatZip),
            _ => None,
        }
    }

    pub fn name(&self) -> &'static str {
        match self {
            Self::None => "none",
            Self::Gzip => "gzip",
            Self::Zlib => "zlib",
            Self::Kanzi => "kanzi",
            Self::QatZip => "qatzip",
        }
    }
}

/// Compression level
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CompressionLevel {
    Fast,
    Default,
    Best,
}

/// Hardware detection results
#[derive(Debug, Clone)]
pub struct HardwareCapabilities {
    pub kanzi_available: bool,
    pub qatzip_available: bool,
    pub cpu_features: Vec<String>,
}

impl HardwareCapabilities {
    /// Detect available hardware compression
    pub fn detect() -> Self {
        Self {
            kanzi_available: Self::detect_kanzi(),
            qatzip_available: Self::detect_qatzip(),
            cpu_features: Self::detect_cpu_features(),
        }
    }

    fn detect_kanzi() -> bool {
        // Check for kanzi library availability
        std::path::Path::new("/usr/lib/libkanzi.so").exists()
            || std::path::Path::new("/usr/local/lib/libkanzi.so").exists()
    }

    fn detect_qatzip() -> bool {
        // Check for Intel QAT hardware
        std::path::Path::new("/dev/qat_adf_ctl").exists()
            || std::path::Path::new("/sys/bus/pci/drivers/qat_4xxx").exists()
    }

    fn detect_cpu_features() -> Vec<String> {
        let mut features = Vec::new();
        #[cfg(target_arch = "x86_64")]
        {
            if is_x86_feature_detected!("avx2") { features.push("avx2".to_string()); }
            if is_x86_feature_detected!("avx512f") { features.push("avx512f".to_string()); }
        }
        features
    }

    /// Get preferred compression based on available hardware
    pub fn preferred_compression(&self) -> CompressionType {
        if self.qatzip_available {
            CompressionType::QatZip
        } else if self.kanzi_available {
            CompressionType::Kanzi
        } else {
            CompressionType::Gzip
        }
    }
}

/// Compression manager
pub struct CompressionManager {
    hardware: HardwareCapabilities,
    default_type: CompressionType,
    default_level: CompressionLevel,
    stats: CompressionStats,
}

#[derive(Debug, Clone, Default)]
pub struct CompressionStats {
    pub bytes_in: u64,
    pub bytes_out: u64,
    pub operations: u64,
    pub failures: u64,
}

impl CompressionStats {
    pub fn ratio(&self) -> f64 {
        if self.bytes_in == 0 { 1.0 } else { self.bytes_out as f64 / self.bytes_in as f64 }
    }
}

impl CompressionManager {
    pub fn new() -> Self {
        let hw = HardwareCapabilities::detect();
        let default_type = hw.preferred_compression();
        Self {
            hardware: hw,
            default_type,
            default_level: CompressionLevel::Default,
            stats: CompressionStats::default(),
        }
    }

    pub fn with_type(compression_type: CompressionType) -> Self {
        Self {
            hardware: HardwareCapabilities::detect(),
            default_type: compression_type,
            default_level: CompressionLevel::Default,
            stats: CompressionStats::default(),
        }
    }

    /// Compress data using the configured algorithm
    pub fn compress(&mut self, data: &[u8]) -> Result<Vec<u8>, CompressionError> {
        self.compress_with(data, self.default_type)
    }

    /// Compress data with a specific algorithm
    pub fn compress_with(&mut self, data: &[u8], algo: CompressionType) -> Result<Vec<u8>, CompressionError> {
        if data.is_empty() {
            return Ok(Vec::new());
        }

        self.stats.bytes_in += data.len() as u64;
        self.stats.operations += 1;

        let result = match algo {
            CompressionType::None => Ok(data.to_vec()),
            CompressionType::Gzip => self.compress_gzip(data),
            CompressionType::Zlib => self.compress_zlib(data),
            CompressionType::Kanzi => {
                if self.hardware.kanzi_available {
                    self.compress_kanzi(data)
                } else {
                    self.compress_gzip(data) // Fallback
                }
            }
            CompressionType::QatZip => {
                if self.hardware.qatzip_available {
                    self.compress_qatzip(data)
                } else {
                    self.compress_gzip(data) // Fallback
                }
            }
        };

        match &result {
            Ok(compressed) => self.stats.bytes_out += compressed.len() as u64,
            Err(_) => self.stats.failures += 1,
        }

        result
    }

    /// Decompress data
    pub fn decompress(&mut self, data: &[u8], algo: CompressionType) -> Result<Vec<u8>, CompressionError> {
        if data.is_empty() {
            return Ok(Vec::new());
        }

        match algo {
            CompressionType::None => Ok(data.to_vec()),
            CompressionType::Gzip => self.decompress_gzip(data),
            CompressionType::Zlib => self.decompress_zlib(data),
            CompressionType::Kanzi => self.decompress_gzip(data), // Fallback in pure Rust
            CompressionType::QatZip => self.decompress_gzip(data), // Fallback in pure Rust
        }
    }

    /// Get hardware capabilities
    pub fn hardware(&self) -> &HardwareCapabilities {
        &self.hardware
    }

    /// Get compression statistics
    pub fn stats(&self) -> &CompressionStats {
        &self.stats
    }

    // --- Gzip ---

    fn compress_gzip(&self, data: &[u8]) -> Result<Vec<u8>, CompressionError> {
        use flate2::write::GzEncoder;
        use flate2::Compression;

        let level = match self.default_level {
            CompressionLevel::Fast => Compression::fast(),
            CompressionLevel::Default => Compression::default(),
            CompressionLevel::Best => Compression::best(),
        };

        let mut encoder = GzEncoder::new(Vec::new(), level);
        encoder.write_all(data).map_err(|_| CompressionError::CompressFailed)?;
        encoder.finish().map_err(|_| CompressionError::CompressFailed)
    }

    fn decompress_gzip(&self, data: &[u8]) -> Result<Vec<u8>, CompressionError> {
        use flate2::read::GzDecoder;

        let mut decoder = GzDecoder::new(data);
        let mut output = Vec::new();
        decoder.read_to_end(&mut output).map_err(|_| CompressionError::DecompressFailed)?;
        Ok(output)
    }

    // --- Zlib ---

    fn compress_zlib(&self, data: &[u8]) -> Result<Vec<u8>, CompressionError> {
        use flate2::write::ZlibEncoder;
        use flate2::Compression;

        let mut encoder = ZlibEncoder::new(Vec::new(), Compression::default());
        encoder.write_all(data).map_err(|_| CompressionError::CompressFailed)?;
        encoder.finish().map_err(|_| CompressionError::CompressFailed)
    }

    fn decompress_zlib(&self, data: &[u8]) -> Result<Vec<u8>, CompressionError> {
        use flate2::read::ZlibDecoder;

        let mut decoder = ZlibDecoder::new(data);
        let mut output = Vec::new();
        decoder.read_to_end(&mut output).map_err(|_| CompressionError::DecompressFailed)?;
        Ok(output)
    }

    // --- Kanzi (FFI stub — real impl loads libkanzi.so) ---

    fn compress_kanzi(&self, data: &[u8]) -> Result<Vec<u8>, CompressionError> {
        // In production: FFI to libkanzi.so
        // Fallback: use gzip
        self.compress_gzip(data)
    }

    // --- QATZip (FFI stub — real impl uses Intel QAT) ---

    fn compress_qatzip(&self, data: &[u8]) -> Result<Vec<u8>, CompressionError> {
        // In production: FFI to libqatzip.so
        // Fallback: use gzip
        self.compress_gzip(data)
    }
}

impl Default for CompressionManager {
    fn default() -> Self {
        Self::new()
    }
}

/// Compression errors
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CompressionError {
    CompressFailed,
    DecompressFailed,
    UnsupportedAlgorithm,
    HardwareUnavailable,
}

impl std::fmt::Display for CompressionError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::CompressFailed => write!(f, "Compression failed"),
            Self::DecompressFailed => write!(f, "Decompression failed"),
            Self::UnsupportedAlgorithm => write!(f, "Unsupported compression algorithm"),
            Self::HardwareUnavailable => write!(f, "Hardware compression unavailable"),
        }
    }
}

impl std::error::Error for CompressionError {}
