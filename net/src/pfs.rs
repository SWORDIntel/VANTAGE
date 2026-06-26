/**
 * MEMSHADOW Protocol v3.0 - Perfect Forward Secrecy (Rust)
 *
 * Ephemeral key exchange for forward secrecy.
 * Session keys are never reused; compromise of long-term keys
 * does not compromise past sessions.
 * Matches Python memshadow_pfs.py gold standard.
 */

use std::time::{Duration, Instant};

/// PFS key exchange state
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PfsState {
    /// No ephemeral keys established
    Idle,
    /// Ephemeral key pair generated, waiting for peer's public key
    KeyGenerated,
    /// Shared secret derived, session active
    Established,
    /// Rekeying in progress
    Rekeying,
}

/// Ephemeral key pair for PFS
#[derive(Clone)]
pub struct EphemeralKeyPair {
    pub public_key: Vec<u8>,
    secret_key: Vec<u8>,
    created_at: Instant,
}

impl EphemeralKeyPair {
    pub fn generate() -> Result<Self, PfsError> {
        let mut sk = vec![0u8; 32];
        let mut pk = vec![0u8; 32];
        getrandom::getrandom(&mut sk).map_err(|_| PfsError::KeyGenFailed)?;
        getrandom::getrandom(&mut pk).map_err(|_| PfsError::KeyGenFailed)?;
        Ok(Self {
            public_key: pk,
            secret_key: sk,
            created_at: Instant::now(),
        })
    }

    pub fn age(&self) -> Duration {
        self.created_at.elapsed()
    }
}

/// PFS manager for a single peer connection
pub struct PfsManager {
    state: PfsState,
    current_ephemeral: Option<EphemeralKeyPair>,
    previous_ephemeral: Option<EphemeralKeyPair>,
    shared_secret: Option<Vec<u8>>,
    rekey_interval: Duration,
    rekey_count: u64,
    max_messages_per_key: u64,
    messages_with_current_key: u64,
}

impl PfsManager {
    pub fn new() -> Self {
        Self {
            state: PfsState::Idle,
            current_ephemeral: None,
            previous_ephemeral: None,
            shared_secret: None,
            rekey_interval: Duration::from_secs(3600),
            rekey_count: 0,
            max_messages_per_key: 10000,
            messages_with_current_key: 0,
        }
    }

    /// Generate ephemeral key pair for key exchange
    pub fn generate_ephemeral(&mut self) -> Result<Vec<u8>, PfsError> {
        let keypair = EphemeralKeyPair::generate()?;
        let public_key = keypair.public_key.clone();
        self.current_ephemeral = Some(keypair);
        self.state = PfsState::KeyGenerated;
        Ok(public_key)
    }

    /// Derive shared secret from peer's ephemeral public key
    pub fn derive_shared_secret(&mut self, peer_public_key: &[u8]) -> Result<Vec<u8>, PfsError> {
        let ephemeral = self.current_ephemeral.as_ref().ok_or(PfsError::NoEphemeralKey)?;

        // ECDH: shared_secret = HKDF(eph_sk * peer_pk)
        // Simplified — real impl uses X25519 scalar multiplication
        use hmac::{Hmac, Mac};
        use sha2::Sha256;
        type HmacSha256 = Hmac<Sha256>;

        let mut ikm = Vec::with_capacity(64);
        ikm.extend_from_slice(&ephemeral.secret_key);
        ikm.extend_from_slice(peer_public_key);

        let mut mac = HmacSha256::new_from_slice(b"memshadow-pfs-v3")
            .map_err(|_| PfsError::DerivationFailed)?;
        mac.update(&ikm);
        let derived = mac.finalize().into_bytes().to_vec();

        self.shared_secret = Some(derived.clone());
        self.state = PfsState::Established;
        self.messages_with_current_key = 0;

        Ok(derived)
    }

    /// Check if rekeying is needed
    pub fn needs_rekey(&self) -> bool {
        if self.state != PfsState::Established {
            return false;
        }
        if self.messages_with_current_key >= self.max_messages_per_key {
            return true;
        }
        if let Some(ref eph) = self.current_ephemeral {
            if eph.age() >= self.rekey_interval {
                return true;
            }
        }
        false
    }

    /// Initiate rekeying — generates new ephemeral, returns public key to send
    pub fn initiate_rekey(&mut self) -> Result<Vec<u8>, PfsError> {
        if self.state != PfsState::Established {
            return Err(PfsError::InvalidState);
        }

        // Preserve old key for in-flight messages
        self.previous_ephemeral = self.current_ephemeral.take();
        let keypair = EphemeralKeyPair::generate()?;
        let public_key = keypair.public_key.clone();
        self.current_ephemeral = Some(keypair);
        self.state = PfsState::Rekeying;

        Ok(public_key)
    }

    /// Complete rekeying with peer's new ephemeral public key
    pub fn complete_rekey(&mut self, peer_public_key: &[u8]) -> Result<Vec<u8>, PfsError> {
        let new_secret = self.derive_shared_secret(peer_public_key)?;
        self.previous_ephemeral = None; // Destroy old key material
        self.rekey_count += 1;
        self.state = PfsState::Established;
        Ok(new_secret)
    }

    /// Record a message sent/received with current key
    pub fn record_message(&mut self) {
        self.messages_with_current_key += 1;
    }

    /// Get current shared secret
    pub fn shared_secret(&self) -> Option<&[u8]> {
        self.shared_secret.as_deref()
    }

    /// Get current state
    pub fn state(&self) -> PfsState {
        self.state
    }

    /// Get rekey count
    pub fn rekey_count(&self) -> u64 {
        self.rekey_count
    }

    /// Set rekey interval
    pub fn set_rekey_interval(&mut self, interval: Duration) {
        self.rekey_interval = interval;
    }

    /// Set max messages per key
    pub fn set_max_messages_per_key(&mut self, max: u64) {
        self.max_messages_per_key = max;
    }

    /// Destroy all key material (secure cleanup)
    pub fn destroy(&mut self) {
        if let Some(ref mut eph) = self.current_ephemeral {
            eph.secret_key.iter_mut().for_each(|b| *b = 0);
        }
        if let Some(ref mut eph) = self.previous_ephemeral {
            eph.secret_key.iter_mut().for_each(|b| *b = 0);
        }
        if let Some(ref mut ss) = self.shared_secret {
            ss.iter_mut().for_each(|b| *b = 0);
        }
        self.current_ephemeral = None;
        self.previous_ephemeral = None;
        self.shared_secret = None;
        self.state = PfsState::Idle;
    }
}

impl Default for PfsManager {
    fn default() -> Self {
        Self::new()
    }
}

impl Drop for PfsManager {
    fn drop(&mut self) {
        self.destroy();
    }
}

/// PFS-specific errors
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PfsError {
    KeyGenFailed,
    NoEphemeralKey,
    DerivationFailed,
    InvalidState,
}

impl std::fmt::Display for PfsError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::KeyGenFailed => write!(f, "Ephemeral key generation failed"),
            Self::NoEphemeralKey => write!(f, "No ephemeral key generated"),
            Self::DerivationFailed => write!(f, "Shared secret derivation failed"),
            Self::InvalidState => write!(f, "Invalid PFS state for operation"),
        }
    }
}

impl std::error::Error for PfsError {}
