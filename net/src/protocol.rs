/**
 * MEMSHADOW Protocol v3.0 - Core Protocol (Rust)
 * 
 * Core protocol header structure and packing/unpacking.
 */

use hmac::{Hmac, Mac};
use sha2::Sha256;
use subtle::ConstantTimeEq;

type HmacSha256 = Hmac<Sha256>;


pub const MEMSHADOW_MAGIC: u64 = 0x4D53485700000000; // "MSHW" padded to 8 bytes
pub const MEMSHADOW_VERSION_MAJOR: u8 = 3;
pub const MEMSHADOW_VERSION_MINOR: u8 = 0;
pub const MEMSHADOW_VERSION_PATCH: u8 = 0;
pub const MEMSHADOW_HEADER_SIZE: usize = 32;

/// Message priority levels for routing decisions
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub enum Priority {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3,
    Emergency = 4,
}

impl Priority {
    pub fn as_u16(self) -> u16 {
        self as u16
    }
}

/// Message flags for payload handling
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MessageFlags {
    None = 0x0000,
    Encrypted = 0x0001,
    Compressed = 0x0002,
    Batched = 0x0004,
    RequiresAck = 0x0008,
    Fragmented = 0x0010,
    LastFragment = 0x0020,
    FromKernel = 0x0040,
    HighConfidence = 0x0080,
    HasExtension = 0x0100,
    EccEnabled = 0x0200,
    DualStream = 0x0400,
    TelemetryMode = 0x0800,
    IntegrityCheck = 0x1000,  // Per-message HMAC integrity check (HMAC-SHA256)
    Steganographic = 0x2000,
    TrafficShaped = 0x4000,
    QuantumResistant = 0x8000,
    TemporalQueued = 0x10000,
    QuantumEntangled = 0x20000,
    MorphicAdapted = 0x40000,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MessageType {
    // System/Control (0x00xx)
    Heartbeat = 0x0001,
    Ack = 0x0002,
    Nack = 0x0003,
    // Error = 0x0003,  // Same as Nack (use Nack in Rust)
    Handshake = 0x0004,
    Disconnect = 0x0005,
    
    // SHRINK Psychological Intelligence (0x01xx)
    PsychAssessment = 0x0100,
    DarkTriadUpdate = 0x0101,
    RiskUpdate = 0x0102,
    NeuroUpdate = 0x0103,
    TmiUpdate = 0x0104,
    CogarchUpdate = 0x0105,
    // CognitiveUpdate = 0x0105,  // Alias for CogarchUpdate (use CogarchUpdate)
    FullPsych = 0x0106,
    PsychThreatAlert = 0x0110,
    PsychAnomaly = 0x0111,
    PsychRiskThreshold = 0x0112,
    
    // Threat Intelligence (0x02xx)
    ThreatReport = 0x0201,
    IntelReport = 0x0202,
    KnowledgeUpdate = 0x0203,
    BrainIntelReport = 0x0204,
    IntelPropagate = 0x0205,
    
    // Memory Operations (0x03xx)
    MemoryStore = 0x0301,
    MemoryQuery = 0x0302,
    MemoryResponse = 0x0303,
    MemorySync = 0x0304,
    VectorSync = 0x0305,
    
    // Federation/Mesh (0x04xx)
    NodeRegister = 0x0401,
    NodeDeregister = 0x0402,
    Data = 0x0500,
    QueryDistribute = 0x0403,
    // BrainQuery = 0x0403,  // Alias for QueryDistribute (use QueryDistribute)
    QueryResponse = 0x0404,
    
    // Self-Improvement (0x05xx)
    ImprovementAnnounce = 0x0501,
    ImprovementRequest = 0x0502,
    ImprovementPayload = 0x0503,
    ImprovementAck = 0x0504,
    ImprovementReject = 0x0505,
    ImprovementMetrics = 0x0506,
    
    // File Transfer Operations (0x06xx)
    FileTransferStart = 0x0601,
    FileTransferChunk = 0x0602,
    FileTransferEnd = 0x0603,
    FileTransferAbort = 0x0604,
    FileTransferRequest = 0x0605,
    
    // Telemetry/Control (0x07xx)
    Telemetry = 0x0701,  // Telemetry/metrics (uses 16-byte header)
    Control = 0x0702,    // Control messages
    HeaderOnly = 0x0703,  // Header-only messages
    PayloadOnly = 0x0704, // Payload-only messages
    
    // Temporal Message Queuing (0x08xx)
    TemporalQueue = 0x0801,
    TemporalDeliver = 0x0802,
    TemporalCancel = 0x0803,
    TemporalQuery = 0x0804,
    
    // Quantum Entanglement Routing (0x09xx)
    QuantumEntangle = 0x0901,
    QuantumDeliver = 0x0902,
    QuantumCollapse = 0x0903,
    QuantumStateSync = 0x0904,
    
    // Morphic Protocol Adaptation (0x0Axx)
    MorphicProposal = 0x0A01,
    MorphicVote = 0x0A02,
    MorphicTest = 0x0A03,
    MorphicMetrics = 0x0A04,
    MorphicAccept = 0x0A05,
    MorphicReject = 0x0A06,
    MorphicResearch = 0x0A07,
    
    // Peer Management (0x0Bxx)
    PeerRegister = 0x0B01,
    PeerListRequest = 0x0B02,
    PeerListResponse = 0x0B03,
    PeerUpdate = 0x0B04,
    PeerDiscovery = 0x0B05,
    
    // Relay System (0x0Cxx)
    RelayRequest = 0x0C01,
    RelayData = 0x0C02,
    RelayAck = 0x0C03,
    RelayError = 0x0C04,
}

#[derive(Debug, Clone)]
pub struct MemshadowHeader {
    pub magic: u64,
    pub version: u16,
    pub priority: u16,
    pub msg_type: u16,
    pub flags_batch: u16,
    pub payload_len: u32,
    pub timestamp_ns: u64,
    pub sequence_num: u32,
}

impl MemshadowHeader {
    pub fn new(msg_type: MessageType, priority: Priority, sequence_num: u32) -> Self {
        use std::time::{SystemTime, UNIX_EPOCH};
        let now = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap()
            .as_nanos() as u64;
        
        Self {
            magic: MEMSHADOW_MAGIC,
            version: ((MEMSHADOW_VERSION_MAJOR as u16) << 8) | (MEMSHADOW_VERSION_MINOR as u16),
            priority: priority as u16,
            msg_type: msg_type as u16,
            flags_batch: 0,
            payload_len: 0,
            timestamp_ns: now,
            sequence_num,
        }
    }
    
    pub fn pack(&self) -> Vec<u8> {
        let mut buffer = Vec::with_capacity(MEMSHADOW_HEADER_SIZE);
        buffer.extend_from_slice(&self.magic.to_be_bytes());
        buffer.extend_from_slice(&self.version.to_be_bytes());
        buffer.extend_from_slice(&self.priority.to_be_bytes());
        buffer.extend_from_slice(&self.msg_type.to_be_bytes());
        buffer.extend_from_slice(&self.flags_batch.to_be_bytes());
        buffer.extend_from_slice(&self.payload_len.to_be_bytes());
        buffer.extend_from_slice(&self.sequence_num.to_be_bytes());
        buffer.extend_from_slice(&self.timestamp_ns.to_be_bytes());
        buffer
    }
    
    pub fn unpack(data: &[u8]) -> Option<Self> {
        if data.len() < MEMSHADOW_HEADER_SIZE {
            return None;
        }
        
        let mut offset = 0;
        
        let magic = u64::from_be_bytes([
            data[offset], data[offset+1], data[offset+2], data[offset+3],
            data[offset+4], data[offset+5], data[offset+6], data[offset+7]
        ]);
        offset += 8;
        
        let version = u16::from_be_bytes([data[offset], data[offset+1]]);
        offset += 2;
        
        let priority = u16::from_be_bytes([data[offset], data[offset+1]]);
        offset += 2;
        
        let msg_type = u16::from_be_bytes([data[offset], data[offset+1]]);
        offset += 2;
        
        let flags_batch = u16::from_be_bytes([data[offset], data[offset+1]]);
        offset += 2;
        
        let payload_len = u32::from_be_bytes([
            data[offset], data[offset+1], data[offset+2], data[offset+3]
        ]);
        offset += 4;
        
        let sequence_num = u32::from_be_bytes([
            data[offset], data[offset+1], data[offset+2], data[offset+3],
        ]);
        offset += 4;
        
        let timestamp_ns = u64::from_be_bytes([
            data[offset], data[offset+1], data[offset+2], data[offset+3]
            ,data[offset+4], data[offset+5], data[offset+6], data[offset+7]
        ]);
        
        Some(Self {
            magic,
            version,
            priority,
            msg_type,
            flags_batch,
            payload_len,
            timestamp_ns,
            sequence_num,
        })
    }
    
    pub fn validate(&self) -> bool {
        // Check magic (first 4 bytes should be "MSHW")
        let magic_4bytes = (self.magic >> 32) & 0xFFFFFFFF;
        if magic_4bytes != 0x4D534857 {
            return false;
        }
        
        // Check version major
        let version_major = (self.version >> 8) & 0xFF;
        if version_major != MEMSHADOW_VERSION_MAJOR as u16 {
            return false;
        }
        
        true
    }
    
    /// Compute HMAC-SHA256 for message integrity
    pub fn compute_hmac(data: &[u8], key: &[u8]) -> Result<[u8; 32], String> {
        let mut mac = HmacSha256::new_from_slice(key)
            .map_err(|e| format!("HMAC key error: {}", e))?;
        mac.update(data);
        let result = mac.finalize();
        let mut hmac_bytes = [0u8; 32];
        hmac_bytes.copy_from_slice(&result.into_bytes());
        Ok(hmac_bytes)
    }
    
    /// Verify HMAC-SHA256 for message integrity
    pub fn verify_hmac(data: &[u8], key: &[u8], received_hmac: &[u8]) -> Result<bool, String> {
        if received_hmac.len() != 32 {
            return Err("Invalid HMAC length".to_string());
        }
        
        let computed_hmac = Self::compute_hmac(data, key)?;
        
        // Constant-time comparison
        Ok(computed_hmac.ct_eq(received_hmac).into())
    }
    
    /// Pack message with optional HMAC integrity check
    pub fn pack_message(
        &mut self,
        payload: &[u8],
        integrity_key: Option<&[u8]>
    ) -> Vec<u8> {
        let hmac_size = if integrity_key.is_some() { 32 } else { 0 };
        self.payload_len = payload.len() as u32 + hmac_size;
        
        if let Some(_) = integrity_key {
            self.flags_batch |= MessageFlags::IntegrityCheck as u16;
        }
        
        let mut message = self.pack();
        message.extend_from_slice(payload);
        
        // Compute and append HMAC if integrity key provided
        if let Some(key) = integrity_key {
            if let Ok(hmac_bytes) = Self::compute_hmac(&message, key) {
                message.extend_from_slice(&hmac_bytes);
            }
        }
        
        message
    }
    
    /// Unpack message with optional HMAC integrity verification
    pub fn unpack_message(
        data: &[u8],
        integrity_key: Option<&[u8]>
    ) -> Result<(Self, Vec<u8>), String> {
        if data.len() < MEMSHADOW_HEADER_SIZE {
            return Err("Message too short".to_string());
        }
        
        let header = Self::unpack(data).ok_or("Failed to unpack header")?;
        
        // Check if integrity check is enabled
        let has_integrity = (header.flags_batch & MessageFlags::IntegrityCheck as u16) != 0;
        let hmac_size = if has_integrity { 32 } else { 0 };
        let actual_payload_size = header.payload_len as usize - hmac_size;
        
        if data.len() < MEMSHADOW_HEADER_SIZE + header.payload_len as usize {
            return Err("Message too short for payload".to_string());
        }
        
        // Extract payload
        let payload = data[MEMSHADOW_HEADER_SIZE..MEMSHADOW_HEADER_SIZE + actual_payload_size].to_vec();
        
        // Verify HMAC if integrity check enabled
        if has_integrity {
            if let Some(key) = integrity_key {
                let received_hmac = &data[MEMSHADOW_HEADER_SIZE + actual_payload_size..MEMSHADOW_HEADER_SIZE + header.payload_len as usize];
                
                // Compute HMAC over header + payload (without HMAC)
                let message_without_hmac = &data[..MEMSHADOW_HEADER_SIZE + actual_payload_size];
                match Self::verify_hmac(message_without_hmac, key, received_hmac) {
                    Ok(true) => {},
                    Ok(false) => return Err("Message integrity check failed: HMAC mismatch".to_string()),
                    Err(e) => return Err(format!("HMAC verification error: {}", e)),
                }
            } else {
                return Err("Integrity check enabled but no key provided".to_string());
            }
        }
        
        Ok((header, payload))
    }
}
