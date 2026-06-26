/**
 * MEMSHADOW Protocol v3.0 - Stealth Mode (Rust)
 *
 * Unified stealth operations combining DPI evasion, traffic analysis
 * resistance, and covert channel features.
 * Matches Python memshadow_stealth.py gold standard.
 */

use std::time::{Duration, Instant};

/// Stealth level configuration
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub enum StealthLevel {
    /// No stealth — raw protocol
    None = 0,
    /// Basic: padding + jitter
    Low = 1,
    /// Moderate: protocol mimicry + timing mask
    Medium = 2,
    /// High: full DPI evasion + traffic shaping + dummy traffic
    High = 3,
    /// Maximum: all features + flow rotation + fingerprint randomization
    Maximum = 4,
}

/// Protocol to mimic for DPI evasion
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MimicProtocol {
    None,
    Http,
    Https,
    Dns,
    Smtp,
    Ssh,
    Websocket,
}

/// Stealth configuration
#[derive(Debug, Clone)]
pub struct StealthConfig {
    pub level: StealthLevel,
    pub mimic_protocol: MimicProtocol,
    pub padding_min: usize,
    pub padding_max: usize,
    pub jitter_base_ms: u64,
    pub jitter_range_ms: u64,
    pub dummy_traffic_rate: f64,
    pub flow_rotation_interval: Duration,
    pub normalize_packet_sizes: bool,
    pub randomize_headers: bool,
}

impl Default for StealthConfig {
    fn default() -> Self {
        Self {
            level: StealthLevel::Medium,
            mimic_protocol: MimicProtocol::Https,
            padding_min: 16,
            padding_max: 256,
            jitter_base_ms: 100,
            jitter_range_ms: 50,
            dummy_traffic_rate: 0.10,
            flow_rotation_interval: Duration::from_secs(60),
            normalize_packet_sizes: true,
            randomize_headers: true,
        }
    }
}

impl StealthConfig {
    pub fn for_level(level: StealthLevel) -> Self {
        match level {
            StealthLevel::None => Self {
                level,
                mimic_protocol: MimicProtocol::None,
                padding_min: 0,
                padding_max: 0,
                jitter_base_ms: 0,
                jitter_range_ms: 0,
                dummy_traffic_rate: 0.0,
                flow_rotation_interval: Duration::from_secs(0),
                normalize_packet_sizes: false,
                randomize_headers: false,
            },
            StealthLevel::Low => Self {
                level,
                mimic_protocol: MimicProtocol::None,
                padding_min: 8,
                padding_max: 64,
                jitter_base_ms: 50,
                jitter_range_ms: 25,
                dummy_traffic_rate: 0.0,
                flow_rotation_interval: Duration::from_secs(300),
                normalize_packet_sizes: false,
                randomize_headers: false,
            },
            StealthLevel::Medium => Self::default(),
            StealthLevel::High => Self {
                level,
                mimic_protocol: MimicProtocol::Https,
                padding_min: 32,
                padding_max: 512,
                jitter_base_ms: 100,
                jitter_range_ms: 50,
                dummy_traffic_rate: 0.10,
                flow_rotation_interval: Duration::from_secs(60),
                normalize_packet_sizes: true,
                randomize_headers: true,
            },
            StealthLevel::Maximum => Self {
                level,
                mimic_protocol: MimicProtocol::Https,
                padding_min: 64,
                padding_max: 1024,
                jitter_base_ms: 200,
                jitter_range_ms: 100,
                dummy_traffic_rate: 0.15,
                flow_rotation_interval: Duration::from_secs(30),
                normalize_packet_sizes: true,
                randomize_headers: true,
            },
        }
    }
}

/// Common packet sizes for normalization (match real traffic patterns)
const NORMALIZED_SIZES: &[usize] = &[64, 128, 256, 512, 576, 1024, 1280, 1400, 1460, 1500];

/// Stealth engine
pub struct StealthEngine {
    config: StealthConfig,
    flow_id: u64,
    last_flow_rotation: Instant,
    packets_sent: u64,
    dummy_packets_sent: u64,
    bytes_padded: u64,
}

impl StealthEngine {
    pub fn new(config: StealthConfig) -> Self {
        let mut flow_id_bytes = [0u8; 8];
        getrandom::getrandom(&mut flow_id_bytes).unwrap_or_default();
        Self {
            config,
            flow_id: u64::from_be_bytes(flow_id_bytes),
            last_flow_rotation: Instant::now(),
            packets_sent: 0,
            dummy_packets_sent: 0,
            bytes_padded: 0,
        }
    }

    /// Apply stealth transformations to outgoing data
    pub fn apply(&mut self, data: &[u8]) -> Vec<u8> {
        if self.config.level == StealthLevel::None {
            return data.to_vec();
        }

        let mut output = data.to_vec();

        // 1. Pad to normalized size
        if self.config.normalize_packet_sizes {
            output = self.normalize_size(output);
        } else if self.config.padding_max > 0 {
            output = self.add_padding(output);
        }

        // 2. Wrap in protocol mimicry
        if self.config.mimic_protocol != MimicProtocol::None {
            output = self.wrap_protocol(output);
        }

        // 3. Randomize non-critical header fields
        if self.config.randomize_headers {
            self.randomize_header_fields(&mut output);
        }

        // 4. Check flow rotation
        self.maybe_rotate_flow();

        self.packets_sent += 1;
        output
    }

    /// Strip stealth transformations from incoming data
    pub fn strip(&self, data: &[u8]) -> Vec<u8> {
        if self.config.level == StealthLevel::None {
            return data.to_vec();
        }

        let mut output = data.to_vec();

        // Reverse protocol wrapping
        if self.config.mimic_protocol != MimicProtocol::None {
            output = self.unwrap_protocol(output);
        }

        // Remove padding
        output = self.remove_padding(output);

        output
    }

    /// Generate a dummy traffic packet
    pub fn generate_dummy_packet(&mut self) -> Vec<u8> {
        let size = NORMALIZED_SIZES[self.packets_sent as usize % NORMALIZED_SIZES.len()];
        let mut dummy = vec![0u8; size];
        getrandom::getrandom(&mut dummy).unwrap_or_default();
        // Mark as dummy (first byte = 0xFF, won't appear in real MEMSHADOW headers)
        dummy[0] = 0xFF;
        self.dummy_packets_sent += 1;
        self.apply(&dummy)
    }

    /// Check if a dummy packet should be sent (based on configured rate)
    pub fn should_send_dummy(&self) -> bool {
        if self.config.dummy_traffic_rate <= 0.0 {
            return false;
        }
        let mut rng_byte = [0u8; 1];
        getrandom::getrandom(&mut rng_byte).unwrap_or_default();
        (rng_byte[0] as f64 / 255.0) < self.config.dummy_traffic_rate
    }

    /// Get jitter delay for timing resistance
    pub fn get_jitter_delay(&self) -> Duration {
        if self.config.jitter_base_ms == 0 {
            return Duration::ZERO;
        }
        let mut rng = [0u8; 2];
        getrandom::getrandom(&mut rng).unwrap_or_default();
        let jitter_offset = (u16::from_be_bytes(rng) as u64) % (self.config.jitter_range_ms * 2 + 1);
        let jitter_ms = self.config.jitter_base_ms + jitter_offset - self.config.jitter_range_ms;
        Duration::from_millis(jitter_ms)
    }

    /// Get current flow ID (rotates periodically)
    pub fn flow_id(&self) -> u64 {
        self.flow_id
    }

    /// Get stealth statistics
    pub fn stats(&self) -> StealthStats {
        StealthStats {
            packets_sent: self.packets_sent,
            dummy_packets_sent: self.dummy_packets_sent,
            bytes_padded: self.bytes_padded,
            flow_id: self.flow_id,
            level: self.config.level,
        }
    }

    // --- Internal helpers ---

    fn normalize_size(&mut self, data: Vec<u8>) -> Vec<u8> {
        let target = NORMALIZED_SIZES
            .iter()
            .find(|&&s| s >= data.len())
            .copied()
            .unwrap_or(1500);

        if data.len() >= target {
            return data;
        }

        let pad_len = target - data.len();
        self.bytes_padded += pad_len as u64;

        let mut output = Vec::with_capacity(target + 2);
        // Prepend original length (2 bytes, big-endian)
        output.extend_from_slice(&(data.len() as u16).to_be_bytes());
        output.extend_from_slice(&data);
        // Fill remaining with random padding
        let mut padding = vec![0u8; target - output.len()];
        getrandom::getrandom(&mut padding).unwrap_or_default();
        output.extend_from_slice(&padding);
        output
    }

    fn add_padding(&mut self, data: Vec<u8>) -> Vec<u8> {
        let mut rng = [0u8; 2];
        getrandom::getrandom(&mut rng).unwrap_or_default();
        let range = self.config.padding_max - self.config.padding_min;
        let pad_len = self.config.padding_min + (u16::from_be_bytes(rng) as usize % (range + 1));
        self.bytes_padded += pad_len as u64;

        let mut output = Vec::with_capacity(data.len() + pad_len + 2);
        output.extend_from_slice(&(data.len() as u16).to_be_bytes());
        output.extend_from_slice(&data);
        let mut padding = vec![0u8; pad_len];
        getrandom::getrandom(&mut padding).unwrap_or_default();
        output.extend_from_slice(&padding);
        output
    }

    fn remove_padding(&self, data: Vec<u8>) -> Vec<u8> {
        if data.len() < 2 {
            return data;
        }
        let original_len = u16::from_be_bytes([data[0], data[1]]) as usize;
        if original_len + 2 <= data.len() {
            data[2..2 + original_len].to_vec()
        } else {
            data
        }
    }

    fn wrap_protocol(&self, data: Vec<u8>) -> Vec<u8> {
        match self.config.mimic_protocol {
            MimicProtocol::Http => self.wrap_http(data),
            MimicProtocol::Https | MimicProtocol::Websocket => self.wrap_tls_record(data),
            MimicProtocol::Dns => self.wrap_dns(data),
            _ => data,
        }
    }

    fn unwrap_protocol(&self, data: Vec<u8>) -> Vec<u8> {
        match self.config.mimic_protocol {
            MimicProtocol::Http => self.unwrap_http(data),
            MimicProtocol::Https | MimicProtocol::Websocket => self.unwrap_tls_record(data),
            MimicProtocol::Dns => self.unwrap_dns(data),
            _ => data,
        }
    }

    fn wrap_http(&self, data: Vec<u8>) -> Vec<u8> {
        let header = format!(
            "POST /api/v1/data HTTP/1.1\r\nHost: cdn.example.com\r\nContent-Type: application/octet-stream\r\nContent-Length: {}\r\nX-Request-ID: {:016x}\r\n\r\n",
            data.len(),
            self.flow_id
        );
        let mut output = Vec::with_capacity(header.len() + data.len());
        output.extend_from_slice(header.as_bytes());
        output.extend_from_slice(&data);
        output
    }

    fn unwrap_http(&self, data: Vec<u8>) -> Vec<u8> {
        if let Some(pos) = data.windows(4).position(|w| w == b"\r\n\r\n") {
            data[pos + 4..].to_vec()
        } else {
            data
        }
    }

    fn wrap_tls_record(&self, data: Vec<u8>) -> Vec<u8> {
        // TLS Application Data record: type(1) + version(2) + length(2) + data
        let mut output = Vec::with_capacity(5 + data.len());
        output.push(0x17); // Application Data
        output.push(0x03); // TLS 1.2
        output.push(0x03);
        output.extend_from_slice(&(data.len() as u16).to_be_bytes());
        output.extend_from_slice(&data);
        output
    }

    fn unwrap_tls_record(&self, data: Vec<u8>) -> Vec<u8> {
        if data.len() > 5 && data[0] == 0x17 {
            let payload_len = u16::from_be_bytes([data[3], data[4]]) as usize;
            if data.len() >= 5 + payload_len {
                return data[5..5 + payload_len].to_vec();
            }
        }
        data
    }

    fn wrap_dns(&self, data: Vec<u8>) -> Vec<u8> {
        // DNS-like wrapper: transaction_id(2) + flags(2) + qdcount(2) + ancount(2) + data
        let mut output = Vec::with_capacity(12 + data.len());
        let mut txid = [0u8; 2];
        getrandom::getrandom(&mut txid).unwrap_or_default();
        output.extend_from_slice(&txid);
        output.extend_from_slice(&[0x01, 0x00]); // Standard query
        output.extend_from_slice(&[0x00, 0x01]); // 1 question
        output.extend_from_slice(&[0x00, 0x01]); // 1 answer
        output.extend_from_slice(&[0x00, 0x00]); // 0 authority
        output.extend_from_slice(&[0x00, 0x00]); // 0 additional
        output.extend_from_slice(&data);
        output
    }

    fn unwrap_dns(&self, data: Vec<u8>) -> Vec<u8> {
        if data.len() > 12 {
            data[12..].to_vec()
        } else {
            data
        }
    }

    fn randomize_header_fields(&self, _data: &mut Vec<u8>) {
        // Randomize non-critical bytes to avoid fingerprinting
        // Implementation varies by protocol
    }

    fn maybe_rotate_flow(&mut self) {
        if self.config.flow_rotation_interval.is_zero() {
            return;
        }
        if self.last_flow_rotation.elapsed() >= self.config.flow_rotation_interval {
            let mut new_flow = [0u8; 8];
            getrandom::getrandom(&mut new_flow).unwrap_or_default();
            self.flow_id = u64::from_be_bytes(new_flow);
            self.last_flow_rotation = Instant::now();
        }
    }
}

/// Stealth statistics
#[derive(Debug, Clone)]
pub struct StealthStats {
    pub packets_sent: u64,
    pub dummy_packets_sent: u64,
    pub bytes_padded: u64,
    pub flow_id: u64,
    pub level: StealthLevel,
}
