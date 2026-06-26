/**
 * MEMSHADOW Hub Fingerprint Verification - Rust Implementation
 * 
 * CSNA 2.0 compliant hub identity fingerprint verification.
 * Uses SHA-384 for fingerprint computation (CSNA 2.0 compliant).
 * 
 * Provides irrevocable binding to hub identity through public key fingerprint verification.
 */

use serde::{Deserialize, Serialize};
use sha2::{Sha384, Digest};
use std::collections::HashMap;
use std::fs;
use std::path::{Path, PathBuf};
use std::time::{SystemTime, UNIX_EPOCH};
use std::io;

/// SHA-384 fingerprint size (48 bytes = 96 hex characters)
pub const HUB_FINGERPRINT_SIZE: usize = 48;
pub const HUB_FINGERPRINT_HEX_SIZE: usize = 96;

/// Hub fingerprint information
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct HubFingerprint {
    pub hub_node_id: String,
    #[serde(serialize_with = "serialize_hex", deserialize_with = "deserialize_hex")]
    pub fingerprint: Vec<u8>,  // SHA-384 fingerprint (48 bytes)
    #[serde(serialize_with = "serialize_hex", deserialize_with = "deserialize_hex")]
    pub public_key: Vec<u8>,   // Public key bytes
    pub first_seen_timestamp: i64,
    pub last_verified_timestamp: i64,
    pub verification_count: u32,
}

fn serialize_hex<S>(bytes: &Vec<u8>, serializer: S) -> Result<S::Ok, S::Error>
where
    S: serde::Serializer,
{
    serializer.serialize_str(&hex::encode(bytes))
}

fn deserialize_hex<'de, D>(deserializer: D) -> Result<Vec<u8>, D::Error>
where
    D: serde::Deserializer<'de>,
{
    let s = String::deserialize(deserializer)?;
    hex::decode(&s).map_err(serde::de::Error::custom)
}

impl HubFingerprint {
    /// Compute SHA-384 fingerprint from public key (CSNA 2.0 compliant)
    pub fn compute_fingerprint(public_key: &[u8]) -> Vec<u8> {
        let mut hasher = Sha384::new();
        hasher.update(public_key);
        hasher.finalize().to_vec()
    }
    
    /// Compute fingerprint as hex string
    pub fn compute_fingerprint_hex(public_key: &[u8]) -> String {
        hex::encode(Self::compute_fingerprint(public_key))
    }
    
    /// Create new fingerprint from public key
    pub fn new(hub_node_id: String, public_key: Vec<u8>) -> Self {
        let fingerprint = Self::compute_fingerprint(&public_key);
        let timestamp = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_secs() as i64;
        
        Self {
            hub_node_id,
            fingerprint,
            public_key,
            first_seen_timestamp: timestamp,
            last_verified_timestamp: timestamp,
            verification_count: 1,
        }
    }
    
    /// Verify fingerprint matches public key
    pub fn verify(&self, public_key: &[u8]) -> bool {
        let computed = Self::compute_fingerprint(public_key);
        self.fingerprint == computed
    }
}

/// Hub Fingerprint Manager
/// 
/// Manages hub fingerprint verification with irrevocable binding.
/// Once a hub is detected and its fingerprint stored, all subsequent
/// connections must match that fingerprint or be rejected.
pub struct HubFingerprintManager {
    storage_path: PathBuf,
    fingerprints: HashMap<String, HubFingerprint>,
}

impl HubFingerprintManager {
    /// Create a new fingerprint manager
    /// 
    /// # Arguments
    /// * `storage_path` - Path to fingerprint storage file (None = default: ~/.dsmil/hub_fingerprints.json)
    pub fn new(storage_path: Option<&Path>) -> io::Result<Self> {
        let path = if let Some(p) = storage_path {
            p.to_path_buf()
        } else {
            let mut default_path = dirs::home_dir()
                .ok_or_else(|| io::Error::new(io::ErrorKind::NotFound, "Home directory not found"))?;
            default_path.push(".dsmil");
            default_path.push("hub_fingerprints.json");
            default_path
        };
        
        let mut manager = Self {
            storage_path: path.clone(),
            fingerprints: HashMap::new(),
        };
        
        // Load existing fingerprints
        manager.load_fingerprints()?;
        
        Ok(manager)
    }
    
    /// Load fingerprints from storage file
    fn load_fingerprints(&mut self) -> io::Result<()> {
        if !self.storage_path.exists() {
            // Create parent directory if needed
            if let Some(parent) = self.storage_path.parent() {
                fs::create_dir_all(parent)?;
            }
            return Ok(());
        }
        
        let content = fs::read_to_string(&self.storage_path)?;
        let fingerprints: HashMap<String, HubFingerprint> = serde_json::from_str(&content)
            .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, format!("Invalid JSON: {}", e)))?;
        
        self.fingerprints = fingerprints;
        Ok(())
    }
    
    /// Save fingerprints to storage file
    fn save_fingerprints(&self) -> io::Result<()> {
        // Create parent directory if needed
        if let Some(parent) = self.storage_path.parent() {
            fs::create_dir_all(parent)?;
        }
        
        let content = serde_json::to_string_pretty(&self.fingerprints)
            .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, format!("JSON serialization error: {}", e)))?;
        
        // Write atomically
        let temp_path = self.storage_path.with_extension("tmp");
        fs::write(&temp_path, content)?;
        fs::rename(&temp_path, &self.storage_path)?;
        
        Ok(())
    }
    
    /// Compute SHA-384 fingerprint (CSNA 2.0 compliant)
    pub fn compute_fingerprint(public_key: &[u8]) -> Vec<u8> {
        HubFingerprint::compute_fingerprint(public_key)
    }
    
    /// Compute fingerprint as hex string
    pub fn compute_fingerprint_hex(public_key: &[u8]) -> String {
        HubFingerprint::compute_fingerprint_hex(public_key)
    }
    
    /// Register or verify hub fingerprint
    /// 
    /// First call registers the hub fingerprint irrevocably.
    /// Subsequent calls verify the fingerprint matches.
    /// 
    /// # Returns
    /// * `Ok(())` - Hub is valid/registered
    /// * `Err(String)` - Error message (fingerprint mismatch or other error)
    pub fn register_hub(&mut self, hub_node_id: &str, public_key: &[u8]) -> Result<(), String> {
        let fingerprint = Self::compute_fingerprint(public_key);
        let fingerprint_hex = hex::encode(&fingerprint);
        
        if let Some(stored) = self.fingerprints.get(hub_node_id) {
            // Hub already registered - verify fingerprint matches
            if stored.fingerprint != fingerprint {
                let expected_hex = hex::encode(&stored.fingerprint);
                return Err(format!(
                    "Hub fingerprint mismatch (SHA-384)! Expected: {}..., Got: {}... \
                     This may indicate hub impersonation.",
                    &expected_hex[..16], &fingerprint_hex[..16]
                ));
            }
            
            // Fingerprint matches - update verification stats
            let mut updated = stored.clone();
            updated.last_verified_timestamp = SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .unwrap()
                .as_secs() as i64;
            updated.verification_count += 1;
            updated.public_key = public_key.to_vec();
            
            self.fingerprints.insert(hub_node_id.to_string(), updated);
            self.save_fingerprints().map_err(|e| format!("Failed to save fingerprints: {}", e))?;
            
            Ok(())
        } else {
            // New hub - register fingerprint irrevocably
            let new_fp = HubFingerprint::new(hub_node_id.to_string(), public_key.to_vec());
            
            eprintln!(
                "WARNING: NEW HUB REGISTERED: {} (SHA-384 fingerprint: {}...). \
                 This binding is IRREVOCABLE - all future connections must match this fingerprint.",
                hub_node_id, &fingerprint_hex[..16]
            );
            
            self.fingerprints.insert(hub_node_id.to_string(), new_fp);
            self.save_fingerprints().map_err(|e| format!("Failed to save fingerprints: {}", e))?;
            
            Ok(())
        }
    }
    
    /// Verify hub fingerprint matches stored value
    pub fn verify_hub(&mut self, hub_node_id: &str, public_key: &[u8]) -> Result<(), String> {
        self.register_hub(hub_node_id, public_key)
    }
    
    /// Check if hub is registered
    pub fn is_hub_registered(&self, hub_node_id: &str) -> bool {
        self.fingerprints.contains_key(hub_node_id)
    }
    
    /// Get stored fingerprint for a hub
    pub fn get_hub_fingerprint(&self, hub_node_id: &str) -> Option<&HubFingerprint> {
        self.fingerprints.get(hub_node_id)
    }
    
    /// Register hub's own identity (hub self-registration)
    /// 
    /// Called by a hub node to register its own public key fingerprint.
    /// This creates an irrevocable binding to the hub's identity.
    /// 
    /// # Returns
    /// * `Ok(())` - Hub identity registered successfully
    /// * `Err(String)` - Error message (fingerprint mismatch or other error)
    pub fn register_hub_self(&mut self, hub_node_id: &str, public_key: &[u8]) -> Result<(), String> {
        // Check if already registered
        if let Some(stored) = self.fingerprints.get(hub_node_id) {
            let fingerprint = Self::compute_fingerprint(public_key);
            
            // Verify it matches existing registration
            if stored.fingerprint != fingerprint {
                let expected_hex = hex::encode(&stored.fingerprint);
                let got_hex = hex::encode(&fingerprint);
                return Err(format!(
                    "Hub identity already registered with different fingerprint. \
                     Expected: {}..., Got: {}... \
                     This binding is irrevocable.",
                    &expected_hex[..16], &got_hex[..16]
                ));
            }
            
            // Matches existing registration - update timestamp
            let mut updated = stored.clone();
            updated.last_verified_timestamp = SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .unwrap()
                .as_secs() as i64;
            
            self.fingerprints.insert(hub_node_id.to_string(), updated);
            self.save_fingerprints().map_err(|e| format!("Failed to save fingerprints: {}", e))?;
            
            eprintln!("Hub self-verification successful: {}", hub_node_id);
            return Ok(());
        }
        
        // New self-registration - use same logic as register_hub
        self.register_hub(hub_node_id, public_key)
    }
    
    /// Clear a hub fingerprint (for testing/recovery only)
    /// 
    /// WARNING: This should only be used for testing or recovery scenarios.
    /// In production, fingerprints should be irrevocable.
    pub fn clear_fingerprint(&mut self, hub_node_id: &str) -> Result<(), String> {
        eprintln!("WARNING: CLEARING hub fingerprint for {} - this should only be done for testing/recovery!", hub_node_id);
        
        if self.fingerprints.remove(hub_node_id).is_some() {
            self.save_fingerprints().map_err(|e| format!("Failed to save fingerprints: {}", e))?;
            Ok(())
        } else {
            Err(format!("Hub {} not found", hub_node_id))
        }
    }
    
    /// Get all registered fingerprints
    pub fn get_all_fingerprints(&self) -> &HashMap<String, HubFingerprint> {
        &self.fingerprints
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_fingerprint_computation() {
        let public_key = b"test-public-key";
        let fp1 = HubFingerprint::compute_fingerprint(public_key);
        let fp2 = HubFingerprint::compute_fingerprint(public_key);
        
        assert_eq!(fp1.len(), HUB_FINGERPRINT_SIZE);
        assert_eq!(fp1, fp2); // Deterministic
    }
    
    #[test]
    fn test_fingerprint_hex() {
        let public_key = b"test-public-key";
        let hex = HubFingerprint::compute_fingerprint_hex(public_key);
        
        assert_eq!(hex.len(), HUB_FINGERPRINT_HEX_SIZE);
        assert!(hex.chars().all(|c| c.is_ascii_hexdigit()));
    }
    
    #[test]
    fn test_register_and_verify() {
        let temp_dir = std::env::temp_dir();
        let storage_path = temp_dir.join("test_hub_fingerprints.json");
        
        let mut manager = HubFingerprintManager::new(Some(&storage_path)).unwrap();
        
        let hub_id = "test-hub-001";
        let public_key = b"test-public-key";
        
        // First registration should succeed
        assert!(manager.register_hub(hub_id, public_key).is_ok());
        assert!(manager.is_hub_registered(hub_id));
        
        // Verify with same key should succeed
        assert!(manager.verify_hub(hub_id, public_key).is_ok());
        
        // Verify with different key should fail
        let different_key = b"different-public-key";
        assert!(manager.verify_hub(hub_id, different_key).is_err());
        
        // Cleanup
        let _ = fs::remove_file(&storage_path);
    }
}
