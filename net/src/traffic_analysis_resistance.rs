/**
 * MEMSHADOW Protocol v3.0 - Traffic Analysis Resistance (Rust)
 * 
 * Full implementation of traffic analysis resistance techniques.
 * Gold standard reference implementation.
 */

use std::collections::{HashMap, VecDeque};
use std::time::{SystemTime, UNIX_EPOCH};
use rand::Rng;

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub enum ResistanceLevel {
    None = 0,
    Basic = 1,
    Intermediate = 2,
    Advanced = 3,
    Maximum = 4,
}

const COMMON_SIZES: [usize; 8] = [64, 128, 256, 512, 576, 1024, 1280, 1500];
const FLOW_ROTATION_INTERVAL: u64 = 60;

pub struct PacketSizeNormalizer {
    common_sizes: Vec<usize>,
    size_history: VecDeque<usize>,
}

impl PacketSizeNormalizer {
    pub fn new() -> Self {
        Self {
            common_sizes: COMMON_SIZES.to_vec(),
            size_history: VecDeque::with_capacity(1000),
        }
    }
    
    pub fn normalize(&mut self, data: &[u8]) -> Vec<Vec<u8>> {
        let mut packets = Vec::new();
        let mut remaining = data;
        
        while !remaining.is_empty() {
            let target_size = self.common_sizes[rand::thread_rng().gen_range(0..self.common_sizes.len())];
            let chunk_size = remaining.len().min(target_size);
            
            let mut chunk = remaining[..chunk_size].to_vec();
            
            // Pad to exact size if needed
            if chunk.len() < target_size {
                chunk.resize(target_size, 0);
            }
            
            self.size_history.push_back(chunk.len());
            if self.size_history.len() > 1000 {
                self.size_history.pop_front();
            }
            
            packets.push(chunk);
            remaining = &remaining[chunk_size..];
        }
        
        packets
    }
    
    pub fn add_dummy_packets(&self, count: usize) -> Vec<Vec<u8>> {
        let mut packets = Vec::new();
        
        for _ in 0..count {
            let size = self.common_sizes[rand::thread_rng().gen_range(0..self.common_sizes.len())];
            let packet: Vec<u8> = (0..size).map(|_| rand::thread_rng().gen()).collect();
            packets.push(packet);
        }
        
        packets
    }
}

pub struct TimingPatternMasker {
    base_delay: f64,
    jitter_range: f64,
    timing_history: VecDeque<f64>,
}

impl TimingPatternMasker {
    pub fn new() -> Self {
        Self {
            base_delay: 0.1,  // 100ms
            jitter_range: 0.05,  // ±50ms
            timing_history: VecDeque::with_capacity(1000),
        }
    }
    
    pub fn get_next_delay(&mut self) -> f64 {
        let jitter = rand::thread_rng().gen_range(-self.jitter_range..=self.jitter_range);
        let delay = (self.base_delay + jitter).max(0.01);
        
        self.timing_history.push_back(delay);
        if self.timing_history.len() > 1000 {
            self.timing_history.pop_front();
        }
        
        delay
    }
    
    pub fn mask_timing_pattern(&self, delays: &[f64]) -> Vec<f64> {
        delays.iter().map(|delay| {
            let noise = rand::thread_rng().gen_range(-self.jitter_range..=self.jitter_range);
            (*delay + noise).max(0.01)
        }).collect()
    }
    
    pub fn add_dummy_timing(&mut self, count: usize) -> Vec<f64> {
        (0..count).map(|_| self.get_next_delay()).collect()
    }
}

pub struct FlowCorrelationResistance {
    flow_ids: HashMap<String, u32>,
    rotation_interval: u64,
    last_rotation: u64,
}

impl FlowCorrelationResistance {
    pub fn new() -> Self {
        Self {
            flow_ids: HashMap::new(),
            rotation_interval: FLOW_ROTATION_INTERVAL,
            last_rotation: SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs(),
        }
    }
    
    pub fn get_flow_id(&mut self, source: &str, destination: &str) -> u32 {
        let flow_key = format!("{}:{}", source, destination);
        
        // Rotate flow IDs periodically
        let now = SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs();
        if now - self.last_rotation > self.rotation_interval {
            self.flow_ids.clear();
            self.last_rotation = now;
        }
        
        *self.flow_ids.entry(flow_key).or_insert_with(|| rand::thread_rng().gen())
    }
    
    pub fn randomize_flow_id(&self) -> u32 {
        rand::thread_rng().gen()
    }
    
    pub fn should_rotate(&self) -> bool {
        let now = SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs();
        (now - self.last_rotation) > self.rotation_interval
    }
}

pub struct ProtocolFingerprintResistance {
    fingerprint_variations: Vec<&'static str>,
}

impl ProtocolFingerprintResistance {
    pub fn new() -> Self {
        Self {
            fingerprint_variations: vec![
                "mozilla_firefox",
                "chrome",
                "safari",
                "edge",
                "curl",
                "wget",
            ],
        }
    }
    
    pub fn randomize_header_fields(&self) -> (String, u8, u16) {
        let user_agent = self.fingerprint_variations[rand::thread_rng().gen_range(0..self.fingerprint_variations.len())].to_string();
        let ttl = rand::thread_rng().gen_range(32..=255);
        let window_size = rand::thread_rng().gen_range(1024..=65535);
        (user_agent, ttl, window_size)
    }
    
    pub fn add_fingerprint_noise(&self, packet: &[u8]) -> Vec<u8> {
        let padding_size = rand::thread_rng().gen_range(0..17);
        let mut result = packet.to_vec();
        result.extend((0..padding_size).map(|_| rand::thread_rng().gen::<u8>()));
        result
    }
}

pub struct TrafficAnalysisResistance {
    level: ResistanceLevel,
    size_normalizer: PacketSizeNormalizer,
    timing_masker: TimingPatternMasker,
    flow_resistance: FlowCorrelationResistance,
    fingerprint_resistance: ProtocolFingerprintResistance,
    dummy_traffic_rate: f64,
}

impl TrafficAnalysisResistance {
    pub fn new(level: ResistanceLevel) -> Self {
        let dummy_traffic_rate = if level >= ResistanceLevel::Advanced { 0.1 } else { 0.0 };
        
        Self {
            level,
            size_normalizer: PacketSizeNormalizer::new(),
            timing_masker: TimingPatternMasker::new(),
            flow_resistance: FlowCorrelationResistance::new(),
            fingerprint_resistance: ProtocolFingerprintResistance::new(),
            dummy_traffic_rate,
        }
    }
    
    pub fn process_packets(&mut self, packets: &[Vec<u8>], add_dummy: bool) -> (Vec<Vec<u8>>, Vec<f64>) {
        let mut processed = Vec::new();
        
        // Normalize packet sizes
        for packet in packets {
            let mut normalized = self.size_normalizer.normalize(packet);
            processed.append(&mut normalized);
        }
        
        // Generate delays
        let mut delays: Vec<f64> = (0..processed.len()).map(|_| self.timing_masker.get_next_delay()).collect();
        
        // Add dummy traffic
        if add_dummy && self.dummy_traffic_rate > 0.0 {
            let dummy_count = (processed.len() as f64 * self.dummy_traffic_rate) as usize;
            let dummy_packets = self.size_normalizer.add_dummy_packets(dummy_count);
            let dummy_delays = self.timing_masker.add_dummy_timing(dummy_packets.len());
            
            // Interleave dummy packets
            for (dummy_pkt, dummy_delay) in dummy_packets.into_iter().zip(dummy_delays) {
                let insert_pos = rand::thread_rng().gen_range(0..=processed.len());
                processed.insert(insert_pos, dummy_pkt);
                delays.insert(insert_pos, dummy_delay);
            }
        }
        
        // Mask timing patterns
        if self.level >= ResistanceLevel::Advanced {
            delays = self.timing_masker.mask_timing_pattern(&delays);
        }
        
        (processed, delays)
    }
    
    pub fn get_flow_id(&mut self, source: &str, destination: &str) -> u32 {
        self.flow_resistance.get_flow_id(source, destination)
    }
    
    pub fn randomize_headers(&self) -> (String, u8, u16) {
        self.fingerprint_resistance.randomize_header_fields()
    }
    
    pub fn add_fingerprint_noise(&self, packet: &[u8]) -> Vec<u8> {
        self.fingerprint_resistance.add_fingerprint_noise(packet)
    }
}
