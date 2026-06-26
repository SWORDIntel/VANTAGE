/**
 * MEMSHADOW Protocol v3.0 - Error Codes (Rust)
 * 
 * 100+ error codes matching Python gold standard.
 * Each error has a code, category, and human-readable message.
 */

use std::fmt;

/// Error categories for grouping related errors
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ErrorCategory {
    Protocol,
    Security,
    Network,
    Session,
    FileTransfer,
    Compression,
    Internal,
    RateLimit,
    Version,
    Quantum,
    Morphic,
    Temporal,
}

/// MEMSHADOW protocol error codes
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u16)]
pub enum ErrorCode {
    // Protocol errors (0x00xx)
    InvalidMagic = 0x0001,
    InvalidVersion = 0x0002,
    InvalidHeader = 0x0003,
    InvalidPayload = 0x0004,
    InvalidMessageType = 0x0005,
    InvalidPriority = 0x0006,
    InvalidFlags = 0x0007,
    MessageTooLarge = 0x0008,
    PayloadSizeMismatch = 0x0009,
    InvalidSequenceNumber = 0x000A,
    DuplicateSequenceNumber = 0x000B,
    InvalidBatchCount = 0x000C,
    InvalidTimestamp = 0x000D,
    HeaderCorrupted = 0x000E,
    PayloadCorrupted = 0x000F,
    InvalidExtensionType = 0x0010,
    ExtensionTooLarge = 0x0011,
    MissingRequiredExtension = 0x0012,
    InvalidFragmentIndex = 0x0013,
    FragmentReassemblyFailed = 0x0014,
    
    // Security errors (0x01xx)
    InvalidSignature = 0x0101,
    AuthenticationFailed = 0x0102,
    EncryptionFailed = 0x0103,
    DecryptionFailed = 0x0104,
    IntegrityCheckFailed = 0x0105,
    KeyExchangeFailed = 0x0106,
    CertificateExpired = 0x0107,
    CertificateRevoked = 0x0108,
    InsufficientPermissions = 0x0109,
    TlsHandshakeFailed = 0x010A,
    HmacVerificationFailed = 0x010B,
    ReplayAttackDetected = 0x010C,
    PqcKeyGenerationFailed = 0x010D,
    PqcEncapsulationFailed = 0x010E,
    PqcDecapsulationFailed = 0x010F,
    ZkpVerificationFailed = 0x0110,
    
    // Network errors (0x02xx)
    PeerNotFound = 0x0201,
    PeerDisconnected = 0x0202,
    ConnectionFailed = 0x0203,
    ConnectionTimeout = 0x0204,
    NatTraversalFailed = 0x0205,
    RelayFailed = 0x0206,
    DhtLookupFailed = 0x0207,
    GossipPropagationFailed = 0x0208,
    VlanTopologyError = 0x0209,
    DpiEvasionFailed = 0x020A,
    CovertChannelFailed = 0x020B,
    BandwidthExceeded = 0x020C,
    RoutingFailed = 0x020D,
    MulticastError = 0x020E,
    
    // Session errors (0x03xx)
    HandshakeTimeout = 0x0301,
    HandshakeFailed = 0x0302,
    SessionExpired = 0x0303,
    SessionNotFound = 0x0304,
    SessionAlreadyExists = 0x0305,
    VersionIncompatible = 0x0306,
    SessionKeyRotationFailed = 0x0307,
    SessionLimitReached = 0x0308,
    
    // Rate limiting (0x04xx)
    RateLimitExceeded = 0x0401,
    BurstLimitExceeded = 0x0402,
    ConnectionLimitExceeded = 0x0403,
    MessageQuotaExceeded = 0x0404,
    BandwidthQuotaExceeded = 0x0405,
    
    // File transfer errors (0x06xx)
    FileTransferFailed = 0x0601,
    FileNotFound = 0x0602,
    FileTooLarge = 0x0603,
    FileCorrupted = 0x0604,
    FilePermissionDenied = 0x0605,
    TransferInterrupted = 0x0606,
    ChunkMissing = 0x0607,
    CompressionFailed = 0x0608,
    DecompressionFailed = 0x0609,
    HardwareUnavailable = 0x060A,
    
    // Internal errors (0x0Axx)
    InternalError = 0x0A01,
    OutOfMemory = 0x0A02,
    NotImplemented = 0x0A03,
    InvalidState = 0x0A04,
    ConfigurationError = 0x0A05,
    
    // Quantum errors (0x0Bxx)
    QuantumEntanglementFailed = 0x0B01,
    QuantumStateCollapsed = 0x0B02,
    QuantumGroupNotFound = 0x0B03,
    QuantumDecoherence = 0x0B04,
    
    // Morphic errors (0x0Cxx)
    MorphicProposalRejected = 0x0C01,
    MorphicTestFailed = 0x0C02,
    MorphicIncompatible = 0x0C03,
    
    // Temporal errors (0x0Dxx)
    TemporalQueueFull = 0x0D01,
    TemporalDeliveryFailed = 0x0D02,
    TemporalExpired = 0x0D03,
}

impl ErrorCode {
    pub fn as_u16(self) -> u16 {
        self as u16
    }
    
    pub fn category(self) -> ErrorCategory {
        match (self as u16) >> 8 {
            0x00 => ErrorCategory::Protocol,
            0x01 => ErrorCategory::Security,
            0x02 => ErrorCategory::Network,
            0x03 => ErrorCategory::Session,
            0x04 => ErrorCategory::RateLimit,
            0x06 => ErrorCategory::FileTransfer,
            0x0A => ErrorCategory::Internal,
            0x0B => ErrorCategory::Quantum,
            0x0C => ErrorCategory::Morphic,
            0x0D => ErrorCategory::Temporal,
            _ => ErrorCategory::Internal,
        }
    }
    
    pub fn message(self) -> &'static str {
        match self {
            Self::InvalidMagic => "Invalid protocol magic bytes",
            Self::InvalidVersion => "Unsupported protocol version",
            Self::InvalidHeader => "Malformed header structure",
            Self::InvalidPayload => "Invalid payload data",
            Self::InvalidMessageType => "Unknown message type",
            Self::InvalidPriority => "Invalid priority level",
            Self::InvalidFlags => "Invalid flag combination",
            Self::MessageTooLarge => "Message exceeds maximum size",
            Self::PayloadSizeMismatch => "Payload size does not match header",
            Self::InvalidSequenceNumber => "Invalid sequence number",
            Self::DuplicateSequenceNumber => "Duplicate sequence number detected",
            Self::InvalidBatchCount => "Invalid batch count",
            Self::InvalidTimestamp => "Invalid timestamp",
            Self::HeaderCorrupted => "Header data corrupted",
            Self::PayloadCorrupted => "Payload data corrupted",
            Self::InvalidExtensionType => "Unknown extension type",
            Self::ExtensionTooLarge => "Extension exceeds maximum size",
            Self::MissingRequiredExtension => "Required extension missing",
            Self::InvalidFragmentIndex => "Invalid fragment index",
            Self::FragmentReassemblyFailed => "Fragment reassembly failed",
            Self::InvalidSignature => "Invalid cryptographic signature",
            Self::AuthenticationFailed => "Authentication failed",
            Self::EncryptionFailed => "Encryption operation failed",
            Self::DecryptionFailed => "Decryption operation failed",
            Self::IntegrityCheckFailed => "Message integrity check failed",
            Self::KeyExchangeFailed => "Key exchange failed",
            Self::CertificateExpired => "Certificate has expired",
            Self::CertificateRevoked => "Certificate has been revoked",
            Self::InsufficientPermissions => "Insufficient permissions",
            Self::TlsHandshakeFailed => "TLS handshake failed",
            Self::HmacVerificationFailed => "HMAC verification failed",
            Self::ReplayAttackDetected => "Replay attack detected",
            Self::PqcKeyGenerationFailed => "PQC key generation failed",
            Self::PqcEncapsulationFailed => "PQC encapsulation failed",
            Self::PqcDecapsulationFailed => "PQC decapsulation failed",
            Self::ZkpVerificationFailed => "ZKP verification failed",
            Self::PeerNotFound => "Peer not found in network",
            Self::PeerDisconnected => "Peer disconnected",
            Self::ConnectionFailed => "Connection failed",
            Self::ConnectionTimeout => "Connection timed out",
            Self::NatTraversalFailed => "NAT traversal failed",
            Self::RelayFailed => "Relay routing failed",
            Self::DhtLookupFailed => "DHT lookup failed",
            Self::GossipPropagationFailed => "Gossip propagation failed",
            Self::VlanTopologyError => "VLAN topology error",
            Self::DpiEvasionFailed => "DPI evasion failed",
            Self::CovertChannelFailed => "Covert channel operation failed",
            Self::BandwidthExceeded => "Bandwidth limit exceeded",
            Self::RoutingFailed => "Message routing failed",
            Self::MulticastError => "Multicast operation failed",
            Self::HandshakeTimeout => "Handshake timed out",
            Self::HandshakeFailed => "Handshake failed",
            Self::SessionExpired => "Session has expired",
            Self::SessionNotFound => "Session not found",
            Self::SessionAlreadyExists => "Session already exists",
            Self::VersionIncompatible => "Protocol version incompatible",
            Self::SessionKeyRotationFailed => "Session key rotation failed",
            Self::SessionLimitReached => "Maximum sessions reached",
            Self::RateLimitExceeded => "Rate limit exceeded",
            Self::BurstLimitExceeded => "Burst limit exceeded",
            Self::ConnectionLimitExceeded => "Connection limit exceeded",
            Self::MessageQuotaExceeded => "Message quota exceeded",
            Self::BandwidthQuotaExceeded => "Bandwidth quota exceeded",
            Self::FileTransferFailed => "File transfer failed",
            Self::FileNotFound => "File not found",
            Self::FileTooLarge => "File too large for transfer",
            Self::FileCorrupted => "File data corrupted",
            Self::FilePermissionDenied => "File permission denied",
            Self::TransferInterrupted => "Transfer interrupted",
            Self::ChunkMissing => "File chunk missing",
            Self::CompressionFailed => "Compression failed",
            Self::DecompressionFailed => "Decompression failed",
            Self::HardwareUnavailable => "Hardware accelerator unavailable",
            Self::InternalError => "Internal error",
            Self::OutOfMemory => "Out of memory",
            Self::NotImplemented => "Feature not implemented",
            Self::InvalidState => "Invalid state transition",
            Self::ConfigurationError => "Configuration error",
            Self::QuantumEntanglementFailed => "Quantum entanglement failed",
            Self::QuantumStateCollapsed => "Quantum state collapsed",
            Self::QuantumGroupNotFound => "Quantum entanglement group not found",
            Self::QuantumDecoherence => "Quantum decoherence detected",
            Self::MorphicProposalRejected => "Morphic adaptation proposal rejected",
            Self::MorphicTestFailed => "Morphic adaptation test failed",
            Self::MorphicIncompatible => "Morphic adaptation incompatible",
            Self::TemporalQueueFull => "Temporal queue full",
            Self::TemporalDeliveryFailed => "Temporal delivery failed",
            Self::TemporalExpired => "Temporal message expired",
        }
    }
    
    pub fn from_u16(code: u16) -> Option<Self> {
        // Use a match to safely convert
        match code {
            0x0001 => Some(Self::InvalidMagic),
            0x0002 => Some(Self::InvalidVersion),
            0x0003 => Some(Self::InvalidHeader),
            0x0004 => Some(Self::InvalidPayload),
            0x0005 => Some(Self::InvalidMessageType),
            0x0006 => Some(Self::InvalidPriority),
            0x0007 => Some(Self::InvalidFlags),
            0x0008 => Some(Self::MessageTooLarge),
            0x0009 => Some(Self::PayloadSizeMismatch),
            0x000A => Some(Self::InvalidSequenceNumber),
            0x000B => Some(Self::DuplicateSequenceNumber),
            0x000C => Some(Self::InvalidBatchCount),
            0x000D => Some(Self::InvalidTimestamp),
            0x000E => Some(Self::HeaderCorrupted),
            0x000F => Some(Self::PayloadCorrupted),
            0x0010 => Some(Self::InvalidExtensionType),
            0x0011 => Some(Self::ExtensionTooLarge),
            0x0012 => Some(Self::MissingRequiredExtension),
            0x0013 => Some(Self::InvalidFragmentIndex),
            0x0014 => Some(Self::FragmentReassemblyFailed),
            0x0101 => Some(Self::InvalidSignature),
            0x0102 => Some(Self::AuthenticationFailed),
            0x0103 => Some(Self::EncryptionFailed),
            0x0104 => Some(Self::DecryptionFailed),
            0x0105 => Some(Self::IntegrityCheckFailed),
            0x0106 => Some(Self::KeyExchangeFailed),
            0x0107 => Some(Self::CertificateExpired),
            0x0108 => Some(Self::CertificateRevoked),
            0x0109 => Some(Self::InsufficientPermissions),
            0x010A => Some(Self::TlsHandshakeFailed),
            0x010B => Some(Self::HmacVerificationFailed),
            0x010C => Some(Self::ReplayAttackDetected),
            0x010D => Some(Self::PqcKeyGenerationFailed),
            0x010E => Some(Self::PqcEncapsulationFailed),
            0x010F => Some(Self::PqcDecapsulationFailed),
            0x0110 => Some(Self::ZkpVerificationFailed),
            0x0201 => Some(Self::PeerNotFound),
            0x0202 => Some(Self::PeerDisconnected),
            0x0203 => Some(Self::ConnectionFailed),
            0x0204 => Some(Self::ConnectionTimeout),
            0x0205 => Some(Self::NatTraversalFailed),
            0x0206 => Some(Self::RelayFailed),
            0x0207 => Some(Self::DhtLookupFailed),
            0x0208 => Some(Self::GossipPropagationFailed),
            0x0209 => Some(Self::VlanTopologyError),
            0x020A => Some(Self::DpiEvasionFailed),
            0x020B => Some(Self::CovertChannelFailed),
            0x020C => Some(Self::BandwidthExceeded),
            0x020D => Some(Self::RoutingFailed),
            0x020E => Some(Self::MulticastError),
            0x0301 => Some(Self::HandshakeTimeout),
            0x0302 => Some(Self::HandshakeFailed),
            0x0303 => Some(Self::SessionExpired),
            0x0304 => Some(Self::SessionNotFound),
            0x0305 => Some(Self::SessionAlreadyExists),
            0x0306 => Some(Self::VersionIncompatible),
            0x0307 => Some(Self::SessionKeyRotationFailed),
            0x0308 => Some(Self::SessionLimitReached),
            0x0401 => Some(Self::RateLimitExceeded),
            0x0402 => Some(Self::BurstLimitExceeded),
            0x0403 => Some(Self::ConnectionLimitExceeded),
            0x0404 => Some(Self::MessageQuotaExceeded),
            0x0405 => Some(Self::BandwidthQuotaExceeded),
            0x0601 => Some(Self::FileTransferFailed),
            0x0602 => Some(Self::FileNotFound),
            0x0603 => Some(Self::FileTooLarge),
            0x0604 => Some(Self::FileCorrupted),
            0x0605 => Some(Self::FilePermissionDenied),
            0x0606 => Some(Self::TransferInterrupted),
            0x0607 => Some(Self::ChunkMissing),
            0x0608 => Some(Self::CompressionFailed),
            0x0609 => Some(Self::DecompressionFailed),
            0x060A => Some(Self::HardwareUnavailable),
            0x0A01 => Some(Self::InternalError),
            0x0A02 => Some(Self::OutOfMemory),
            0x0A03 => Some(Self::NotImplemented),
            0x0A04 => Some(Self::InvalidState),
            0x0A05 => Some(Self::ConfigurationError),
            0x0B01 => Some(Self::QuantumEntanglementFailed),
            0x0B02 => Some(Self::QuantumStateCollapsed),
            0x0B03 => Some(Self::QuantumGroupNotFound),
            0x0B04 => Some(Self::QuantumDecoherence),
            0x0C01 => Some(Self::MorphicProposalRejected),
            0x0C02 => Some(Self::MorphicTestFailed),
            0x0C03 => Some(Self::MorphicIncompatible),
            0x0D01 => Some(Self::TemporalQueueFull),
            0x0D02 => Some(Self::TemporalDeliveryFailed),
            0x0D03 => Some(Self::TemporalExpired),
            _ => None,
        }
    }
}

impl fmt::Display for ErrorCode {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "MSHW-{:04X}: {}", self.as_u16(), self.message())
    }
}

/// MEMSHADOW protocol error with optional details
#[derive(Debug, Clone)]
pub struct MemshadowError {
    pub code: ErrorCode,
    pub details: Option<String>,
}

impl MemshadowError {
    pub fn new(code: ErrorCode) -> Self {
        Self { code, details: None }
    }
    
    pub fn with_details(code: ErrorCode, details: impl Into<String>) -> Self {
        Self { code, details: Some(details.into()) }
    }
}

impl fmt::Display for MemshadowError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match &self.details {
            Some(details) => write!(f, "{} ({})", self.code, details),
            None => write!(f, "{}", self.code),
        }
    }
}

impl std::error::Error for MemshadowError {}
