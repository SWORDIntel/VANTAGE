/**
 * MEMSHADOW Protocol v3.0 - Handshake (Rust)
 *
 * Secure handshake protocol with version negotiation,
 * capability exchange, and key establishment.
 * Matches Python memshadow_handshake.py gold standard.
 */

use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

use crate::errors::{ErrorCode, MemshadowError};
use crate::version::{ProtocolVersion, VersionCompatibility, VersionNegotiator};

/// Handshake state machine
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum HandshakeState {
    /// Initial state — no handshake initiated
    Idle,
    /// Client has sent HELLO
    HelloSent,
    /// Server received HELLO, sent HELLO_ACK with challenge
    HelloAckSent,
    /// Client received challenge, sent CHALLENGE_RESPONSE
    ChallengeResponseSent,
    /// Server verified response, sent SESSION_ESTABLISHED
    Established,
    /// Handshake failed
    Failed,
}

/// Capabilities advertised during handshake
#[derive(Debug, Clone, Default)]
pub struct Capabilities {
    pub pqc_enabled: bool,
    pub compression_types: Vec<u8>,
    pub max_payload_size: u32,
    pub supports_vlan_relay: bool,
    pub supports_dpi_evasion: bool,
    pub supports_traffic_resistance: bool,
    pub supports_covert_vlan: bool,
    pub supports_hub_fingerprint: bool,
    pub supports_tls_extensions: bool,
    pub supports_gossip: bool,
    pub supports_pfs: bool,
    pub supports_zkp: bool,
    pub supports_temporal_queue: bool,
    pub supports_quantum_entanglement: bool,
    pub supports_morphic_adaptation: bool,
    pub supports_rdma: bool,
    pub supports_dual_stream: bool,
    pub supports_ecc: bool,
}

impl Capabilities {
    /// Encode capabilities as a bitmask (32 bits)
    pub fn as_u32(&self) -> u32 {
        let mut bits: u32 = 0;
        if self.pqc_enabled { bits |= 1 << 0; }
        if self.supports_vlan_relay { bits |= 1 << 1; }
        if self.supports_dpi_evasion { bits |= 1 << 2; }
        if self.supports_traffic_resistance { bits |= 1 << 3; }
        if self.supports_covert_vlan { bits |= 1 << 4; }
        if self.supports_hub_fingerprint { bits |= 1 << 5; }
        if self.supports_tls_extensions { bits |= 1 << 6; }
        if self.supports_gossip { bits |= 1 << 7; }
        if self.supports_pfs { bits |= 1 << 8; }
        if self.supports_zkp { bits |= 1 << 9; }
        if self.supports_temporal_queue { bits |= 1 << 10; }
        if self.supports_quantum_entanglement { bits |= 1 << 11; }
        if self.supports_morphic_adaptation { bits |= 1 << 12; }
        if self.supports_rdma { bits |= 1 << 13; }
        if self.supports_dual_stream { bits |= 1 << 14; }
        if self.supports_ecc { bits |= 1 << 15; }
        bits
    }

    /// Decode capabilities from bitmask
    pub fn from_u32(bits: u32) -> Self {
        Self {
            pqc_enabled: bits & (1 << 0) != 0,
            compression_types: Vec::new(),
            max_payload_size: 16 * 1024 * 1024,
            supports_vlan_relay: bits & (1 << 1) != 0,
            supports_dpi_evasion: bits & (1 << 2) != 0,
            supports_traffic_resistance: bits & (1 << 3) != 0,
            supports_covert_vlan: bits & (1 << 4) != 0,
            supports_hub_fingerprint: bits & (1 << 5) != 0,
            supports_tls_extensions: bits & (1 << 6) != 0,
            supports_gossip: bits & (1 << 7) != 0,
            supports_pfs: bits & (1 << 8) != 0,
            supports_zkp: bits & (1 << 9) != 0,
            supports_temporal_queue: bits & (1 << 10) != 0,
            supports_quantum_entanglement: bits & (1 << 11) != 0,
            supports_morphic_adaptation: bits & (1 << 12) != 0,
            supports_rdma: bits & (1 << 13) != 0,
            supports_dual_stream: bits & (1 << 14) != 0,
            supports_ecc: bits & (1 << 15) != 0,
        }
    }

    /// Compute intersection of two capability sets (negotiated capabilities)
    pub fn intersect(&self, other: &Self) -> Self {
        Self::from_u32(self.as_u32() & other.as_u32())
    }

    /// Full capabilities (all features enabled)
    pub fn full() -> Self {
        Self {
            pqc_enabled: true,
            compression_types: vec![0, 1, 2, 3, 4],
            max_payload_size: 24 * 1024 * 1024,
            supports_vlan_relay: true,
            supports_dpi_evasion: true,
            supports_traffic_resistance: true,
            supports_covert_vlan: true,
            supports_hub_fingerprint: true,
            supports_tls_extensions: true,
            supports_gossip: true,
            supports_pfs: true,
            supports_zkp: true,
            supports_temporal_queue: true,
            supports_quantum_entanglement: true,
            supports_morphic_adaptation: true,
            supports_rdma: true,
            supports_dual_stream: true,
            supports_ecc: true,
        }
    }
}

/// Handshake message types (sub-types of MSG_HANDSHAKE 0x0004)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum HandshakeStep {
    Hello = 0x01,
    HelloAck = 0x02,
    ChallengeResponse = 0x03,
    SessionEstablished = 0x04,
    SessionReject = 0x05,
}

/// Handshake session manager
pub struct HandshakeManager {
    state: HandshakeState,
    local_capabilities: Capabilities,
    peer_capabilities: Option<Capabilities>,
    negotiated_capabilities: Option<Capabilities>,
    version_negotiator: VersionNegotiator,
    challenge: Option<[u8; 32]>,
    session_id: Option<[u8; 16]>,
    session_key: Option<Vec<u8>>,
    timeout: Duration,
    started_at: Option<Instant>,
    peer_id: Option<String>,
}

impl HandshakeManager {
    pub fn new(capabilities: Capabilities) -> Self {
        Self {
            state: HandshakeState::Idle,
            local_capabilities: capabilities,
            peer_capabilities: None,
            negotiated_capabilities: None,
            version_negotiator: VersionNegotiator::new(),
            challenge: None,
            session_id: None,
            session_key: None,
            timeout: Duration::from_secs(30),
            started_at: None,
            peer_id: None,
        }
    }

    /// Create HELLO message payload (client → server)
    /// Format: [version:2][capabilities:4][node_id_len:2][node_id:N][timestamp:8]
    pub fn create_hello(&mut self, node_id: &str) -> Result<Vec<u8>, MemshadowError> {
        if self.state != HandshakeState::Idle {
            return Err(MemshadowError::with_details(
                ErrorCode::InvalidState,
                format!("Cannot send HELLO in state {:?}", self.state),
            ));
        }

        let version = ProtocolVersion::CURRENT;
        let caps = self.local_capabilities.as_u32();
        let node_id_bytes = node_id.as_bytes();
        let timestamp = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap_or_default()
            .as_nanos() as u64;

        let mut payload = Vec::with_capacity(16 + node_id_bytes.len());
        payload.push(HandshakeStep::Hello as u8);
        payload.extend_from_slice(&version.as_u16().to_be_bytes());
        payload.extend_from_slice(&caps.to_be_bytes());
        payload.extend_from_slice(&(node_id_bytes.len() as u16).to_be_bytes());
        payload.extend_from_slice(node_id_bytes);
        payload.extend_from_slice(&timestamp.to_be_bytes());

        self.state = HandshakeState::HelloSent;
        self.started_at = Some(Instant::now());
        self.peer_id = Some(node_id.to_string());

        Ok(payload)
    }

    /// Process received HELLO, create HELLO_ACK with challenge (server side)
    pub fn process_hello(&mut self, payload: &[u8]) -> Result<Vec<u8>, MemshadowError> {
        if payload.is_empty() || payload[0] != HandshakeStep::Hello as u8 {
            return Err(MemshadowError::new(ErrorCode::HandshakeFailed));
        }

        if payload.len() < 9 {
            return Err(MemshadowError::new(ErrorCode::InvalidPayload));
        }

        // Parse peer version
        let peer_version = ProtocolVersion::from_u16(
            u16::from_be_bytes([payload[1], payload[2]]),
        );

        // Check version compatibility
        let compat = self.version_negotiator.check_compatibility(&peer_version);
        if compat == VersionCompatibility::IncompatibleMajor {
            return Err(MemshadowError::new(ErrorCode::VersionIncompatible));
        }

        // Parse peer capabilities
        let peer_caps_bits = u32::from_be_bytes([payload[3], payload[4], payload[5], payload[6]]);
        let peer_caps = Capabilities::from_u32(peer_caps_bits);
        self.peer_capabilities = Some(peer_caps.clone());

        // Negotiate capabilities (intersection)
        self.negotiated_capabilities = Some(self.local_capabilities.intersect(&peer_caps));

        // Parse node ID
        let node_id_len = u16::from_be_bytes([payload[7], payload[8]]) as usize;
        if payload.len() < 9 + node_id_len {
            return Err(MemshadowError::new(ErrorCode::InvalidPayload));
        }
        let node_id = String::from_utf8_lossy(&payload[9..9 + node_id_len]).to_string();
        self.peer_id = Some(node_id);

        // Generate challenge (32 random bytes)
        let mut challenge = [0u8; 32];
        getrandom::getrandom(&mut challenge).map_err(|_| {
            MemshadowError::new(ErrorCode::InternalError)
        })?;
        self.challenge = Some(challenge);

        // Generate session ID
        let mut session_id = [0u8; 16];
        getrandom::getrandom(&mut session_id).map_err(|_| {
            MemshadowError::new(ErrorCode::InternalError)
        })?;
        self.session_id = Some(session_id);

        // Build HELLO_ACK payload
        let neg_caps = self.negotiated_capabilities.as_ref().unwrap().as_u32();
        let mut ack_payload = Vec::with_capacity(64);
        ack_payload.push(HandshakeStep::HelloAck as u8);
        ack_payload.extend_from_slice(&ProtocolVersion::CURRENT.as_u16().to_be_bytes());
        ack_payload.extend_from_slice(&neg_caps.to_be_bytes());
        ack_payload.extend_from_slice(&challenge);
        ack_payload.extend_from_slice(&session_id);

        self.state = HandshakeState::HelloAckSent;
        self.started_at = Some(Instant::now());

        Ok(ack_payload)
    }

    /// Process HELLO_ACK, create CHALLENGE_RESPONSE (client side)
    pub fn process_hello_ack(&mut self, payload: &[u8], hmac_key: &[u8]) -> Result<Vec<u8>, MemshadowError> {
        if self.state != HandshakeState::HelloSent {
            return Err(MemshadowError::new(ErrorCode::InvalidState));
        }

        if payload.is_empty() || payload[0] != HandshakeStep::HelloAck as u8 {
            return Err(MemshadowError::new(ErrorCode::HandshakeFailed));
        }

        if payload.len() < 55 {
            return Err(MemshadowError::new(ErrorCode::InvalidPayload));
        }

        // Parse negotiated capabilities
        let neg_caps_bits = u32::from_be_bytes([payload[3], payload[4], payload[5], payload[6]]);
        self.negotiated_capabilities = Some(Capabilities::from_u32(neg_caps_bits));

        // Extract challenge
        let mut challenge = [0u8; 32];
        challenge.copy_from_slice(&payload[7..39]);
        self.challenge = Some(challenge);

        // Extract session ID
        let mut session_id = [0u8; 16];
        session_id.copy_from_slice(&payload[39..55]);
        self.session_id = Some(session_id);

        // Compute challenge response: HMAC-SHA256(challenge, hmac_key)
        use hmac::{Hmac, Mac};
        use sha2::Sha256;
        type HmacSha256 = Hmac<Sha256>;

        let mut mac = HmacSha256::new_from_slice(hmac_key)
            .map_err(|_| MemshadowError::new(ErrorCode::KeyExchangeFailed))?;
        mac.update(&challenge);
        let response = mac.finalize().into_bytes();

        // Build CHALLENGE_RESPONSE
        let mut resp_payload = Vec::with_capacity(48);
        resp_payload.push(HandshakeStep::ChallengeResponse as u8);
        resp_payload.extend_from_slice(&session_id);
        resp_payload.extend_from_slice(&response);

        self.state = HandshakeState::ChallengeResponseSent;

        Ok(resp_payload)
    }

    /// Process CHALLENGE_RESPONSE, create SESSION_ESTABLISHED (server side)
    pub fn process_challenge_response(&mut self, payload: &[u8], hmac_key: &[u8]) -> Result<Vec<u8>, MemshadowError> {
        if self.state != HandshakeState::HelloAckSent {
            return Err(MemshadowError::new(ErrorCode::InvalidState));
        }

        if payload.is_empty() || payload[0] != HandshakeStep::ChallengeResponse as u8 {
            return Err(MemshadowError::new(ErrorCode::HandshakeFailed));
        }

        if payload.len() < 49 {
            return Err(MemshadowError::new(ErrorCode::InvalidPayload));
        }

        // Verify session ID matches
        let session_id = &payload[1..17];
        if let Some(ref our_session_id) = self.session_id {
            if session_id != our_session_id.as_slice() {
                return Err(MemshadowError::new(ErrorCode::SessionNotFound));
            }
        }

        // Verify challenge response
        let received_response = &payload[17..49];
        let challenge = self.challenge.ok_or(MemshadowError::new(ErrorCode::InvalidState))?;

        use hmac::{Hmac, Mac};
        use sha2::Sha256;
        type HmacSha256 = Hmac<Sha256>;

        let mut mac = HmacSha256::new_from_slice(hmac_key)
            .map_err(|_| MemshadowError::new(ErrorCode::KeyExchangeFailed))?;
        mac.update(&challenge);

        mac.verify_slice(received_response)
            .map_err(|_| MemshadowError::new(ErrorCode::AuthenticationFailed))?;

        // Generate session key (32 bytes)
        let mut session_key = vec![0u8; 32];
        getrandom::getrandom(&mut session_key)
            .map_err(|_| MemshadowError::new(ErrorCode::InternalError))?;
        self.session_key = Some(session_key.clone());

        // Build SESSION_ESTABLISHED
        let mut est_payload = Vec::with_capacity(64);
        est_payload.push(HandshakeStep::SessionEstablished as u8);
        est_payload.extend_from_slice(self.session_id.as_ref().unwrap());

        // Encrypt session key with HMAC key (simplified — real impl uses PQC KEM)
        let mut enc_mac = HmacSha256::new_from_slice(hmac_key)
            .map_err(|_| MemshadowError::new(ErrorCode::KeyExchangeFailed))?;
        enc_mac.update(&session_key);
        let key_mac = enc_mac.finalize().into_bytes();
        est_payload.extend_from_slice(&key_mac);

        self.state = HandshakeState::Established;

        Ok(est_payload)
    }

    /// Get current handshake state
    pub fn state(&self) -> HandshakeState {
        self.state
    }

    /// Check if handshake timed out
    pub fn is_timed_out(&self) -> bool {
        self.started_at
            .map(|s| s.elapsed() > self.timeout)
            .unwrap_or(false)
    }

    /// Get negotiated capabilities
    pub fn negotiated_capabilities(&self) -> Option<&Capabilities> {
        self.negotiated_capabilities.as_ref()
    }

    /// Get session ID
    pub fn session_id(&self) -> Option<&[u8; 16]> {
        self.session_id.as_ref()
    }

    /// Get session key (only available after establishment)
    pub fn session_key(&self) -> Option<&[u8]> {
        if self.state == HandshakeState::Established {
            self.session_key.as_deref()
        } else {
            None
        }
    }

    /// Get peer ID
    pub fn peer_id(&self) -> Option<&str> {
        self.peer_id.as_deref()
    }

    /// Reset handshake state
    pub fn reset(&mut self) {
        self.state = HandshakeState::Idle;
        self.peer_capabilities = None;
        self.negotiated_capabilities = None;
        self.challenge = None;
        self.session_id = None;
        self.session_key = None;
        self.started_at = None;
        self.peer_id = None;
    }
}
