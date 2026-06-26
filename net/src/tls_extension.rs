/**
 * MEMSHADOW TLS Extension Support (Rust)
 * 
 * APT41-style TLS extensions for carrying extended flags.
 * Extended flags are negotiated once per TLS session during handshake.
 */

use std::time::{SystemTime, UNIX_EPOCH};
use hmac::{Hmac, Mac};
use sha2::Sha256;

type HmacSha256 = Hmac<Sha256>;

/// TLS Extension ID for MEMSHADOW
pub const MEMSHADOW_TLS_EXTENSION_ID: u16 = 0xFC01;

/// Extension data size (without type/length header)
pub const MEMSHADOW_TLS_EXTENSION_DATA_SIZE: usize = 42;

/// TLS Extension Handler
pub struct TlsExtensionHandler {
    extension_key: [u8; 32],
}

impl TlsExtensionHandler {
    /// Create new TLS extension handler
    pub fn new(extension_key: Option<&[u8]>) -> Self {
        let key = if let Some(k) = extension_key {
            let mut key = [0u8; 32];
            key.copy_from_slice(&k[..32.min(k.len())]);
            key
        } else {
            // Generate random key
            let mut key = [0u8; 32];
            let timestamp = SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .unwrap()
                .as_secs();
            key[..8].copy_from_slice(&timestamp.to_be_bytes());
            key
        };
        
        Self { extension_key: key }
    }
    
    /// Create TLS extension for ClientHello
    pub fn create_client_extension(
        &self,
        extended_flags: u32,
        protocol_version: u16,
    ) -> Vec<u8> {
        self.create_extension(extended_flags, protocol_version)
    }
    
    /// Create TLS extension for ServerHello
    pub fn create_server_extension(
        &self,
        negotiated_flags: u32,
        protocol_version: u16,
    ) -> Vec<u8> {
        self.create_extension(negotiated_flags, protocol_version)
    }
    
    fn create_extension(&self, flags: u32, protocol_version: u16) -> Vec<u8> {
        let timestamp = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_secs() as u32;
        
        // Build extension data
        let mut extension_data = Vec::with_capacity(MEMSHADOW_TLS_EXTENSION_DATA_SIZE);
        extension_data.extend_from_slice(&protocol_version.to_be_bytes());
        extension_data.extend_from_slice(&flags.to_be_bytes());
        extension_data.extend_from_slice(&timestamp.to_be_bytes());
        
        // Calculate HMAC-SHA256
        let mut mac = HmacSha256::new_from_slice(&self.extension_key)
            .expect("HMAC can take key of any size");
        mac.update(&extension_data);
        let hmac = mac.finalize();
        extension_data.extend_from_slice(&hmac.into_bytes());
        
        // Build TLS extension: type(2) + length(2) + data
        let mut extension = Vec::with_capacity(4 + MEMSHADOW_TLS_EXTENSION_DATA_SIZE);
        extension.extend_from_slice(&MEMSHADOW_TLS_EXTENSION_ID.to_be_bytes());
        extension.extend_from_slice(&(MEMSHADOW_TLS_EXTENSION_DATA_SIZE as u16).to_be_bytes());
        extension.extend_from_slice(&extension_data);
        
        extension
    }
    
    /// Parse TLS extension data
    pub fn parse_extension(&self, extension_data: &[u8]) -> Option<TlsExtensionData> {
        if extension_data.len() < MEMSHADOW_TLS_EXTENSION_DATA_SIZE {
            return None;
        }
        
        let mut offset = 0;
        
        // Parse protocol version
        let version_bytes: [u8; 2] = extension_data[offset..offset+2].try_into().ok()?;
        let protocol_version = u16::from_be_bytes(version_bytes);
        offset += 2;
        
        // Parse extended flags
        let flags_bytes: [u8; 4] = extension_data[offset..offset+4].try_into().ok()?;
        let extended_flags = u32::from_be_bytes(flags_bytes);
        offset += 4;
        
        // Parse timestamp
        let timestamp_bytes: [u8; 4] = extension_data[offset..offset+4].try_into().ok()?;
        let timestamp = u32::from_be_bytes(timestamp_bytes);
        offset += 4;
        
        // Get received HMAC
        let received_hmac = &extension_data[offset..offset+32];
        
        // Verify HMAC
        let mut mac = HmacSha256::new_from_slice(&self.extension_key)
            .expect("HMAC can take key of any size");
        mac.update(&extension_data[..10]);  // version + flags + timestamp
        let expected_hmac = mac.finalize();
        
        if received_hmac != expected_hmac.into_bytes().as_slice() {
            return None;  // HMAC validation failed
        }
        
        Some(TlsExtensionData {
            protocol_version,
            extended_flags,
            timestamp,
        })
    }
    
    /// Negotiate flags (intersection of client and server flags)
    pub fn negotiate_flags(client_flags: u32, server_flags: u32) -> u32 {
        client_flags & server_flags
    }
}

/// Parsed TLS extension data
#[derive(Debug, Clone)]
pub struct TlsExtensionData {
    pub protocol_version: u16,
    pub extended_flags: u32,
    pub timestamp: u32,
}

/// TLS connection context
pub struct TlsConnectionContext {
    extended_flags: u32,
    protocol_version: u16,
    timestamp: u32,
    negotiated: bool,
}

impl TlsConnectionContext {
    /// Create new connection context
    pub fn new() -> Self {
        Self {
            extended_flags: 0,
            protocol_version: 0x0300,
            timestamp: 0,
            negotiated: false,
        }
    }
    
    /// Set negotiated flags
    pub fn set_negotiated_flags(&mut self, flags: u32, protocol_version: u16) {
        self.extended_flags = flags;
        self.protocol_version = protocol_version;
        self.timestamp = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_secs() as u32;
        self.negotiated = true;
    }
    
    /// Get all flags (base + extended)
    pub fn get_all_flags(&self, base_flags: u8) -> u32 {
        if !self.negotiated {
            return base_flags as u32;
        }
        // Base flags in low 8 bits, extended flags in high bits
        (base_flags as u32) | (self.extended_flags << 8)
    }
    
    /// Check if extended flag is set
    pub fn has_flag(&self, flag: u32) -> bool {
        if !self.negotiated {
            return false;
        }
        (self.extended_flags & flag) != 0
    }
    
    /// Get extended flags
    pub fn extended_flags(&self) -> u32 {
        self.extended_flags
    }
}

impl Default for TlsConnectionContext {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_extension_creation() {
        let handler = TlsExtensionHandler::new(None);
        let extension = handler.create_client_extension(0x0800, 0x0300);
        
        assert_eq!(extension.len(), 4 + MEMSHADOW_TLS_EXTENSION_DATA_SIZE);
        assert_eq!(extension[0..2], MEMSHADOW_TLS_EXTENSION_ID.to_be_bytes());
    }
    
    #[test]
    fn test_extension_parsing() {
        let handler = TlsExtensionHandler::new(None);
        let extension = handler.create_client_extension(0x0800, 0x0300);
        
        let parsed = handler.parse_extension(&extension[4..]);
        assert!(parsed.is_some());
        assert_eq!(parsed.unwrap().extended_flags, 0x0800);
    }
    
    #[test]
    fn test_flag_negotiation() {
        let client_flags = 0x0803;  // TELEMETRY_MODE | ENCRYPTED | COMPRESSED
        let server_flags = 0x0801;  // TELEMETRY_MODE | ENCRYPTED
        
        let negotiated = TlsExtensionHandler::negotiate_flags(client_flags, server_flags);
        assert_eq!(negotiated, 0x0801);  // Intersection
    }
    
    #[test]
    fn test_connection_context() {
        let mut ctx = TlsConnectionContext::new();
        ctx.set_negotiated_flags(0x0800, 0x0300);
        
        assert_eq!(ctx.extended_flags(), 0x0800);
        assert!(ctx.has_flag(0x0800));
        assert!(!ctx.has_flag(0x0400));
        
        let all_flags = ctx.get_all_flags(0x03);  // ENCRYPTED | COMPRESSED
        assert_eq!(all_flags, 0x080003);  // Extended flags in high bits
    }
}
