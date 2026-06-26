/**
 * MEMSHADOW Protocol v3.0 - Version Negotiation (Rust)
 *
 * Version compatibility checking and upgrade negotiation.
 * Matches Python memshadow_version.py gold standard.
 */

use std::time::{Duration, Instant};

/// Protocol version
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct ProtocolVersion {
    pub major: u8,
    pub minor: u8,
}

impl ProtocolVersion {
    pub const CURRENT: Self = Self { major: 3, minor: 0 };

    pub fn new(major: u8, minor: u8) -> Self {
        Self { major, minor }
    }

    pub fn as_u16(&self) -> u16 {
        ((self.major as u16) << 8) | (self.minor as u16)
    }

    pub fn from_u16(version: u16) -> Self {
        Self {
            major: (version >> 8) as u8,
            minor: (version & 0xFF) as u8,
        }
    }
}

impl std::fmt::Display for ProtocolVersion {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}.{}", self.major, self.minor)
    }
}

/// Result of version compatibility check
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum VersionCompatibility {
    /// Same major and minor — fully compatible
    Compatible,
    /// Different major version — incompatible, do not respond
    IncompatibleMajor,
    /// Higher minor version — backward compatible, suggest upgrade
    BackwardCompatible,
    /// Lower minor version — stage upgrade, send after 12h
    UpgradeRequired,
}

/// Version negotiation manager
pub struct VersionNegotiator {
    local_version: ProtocolVersion,
    peer_versions: Vec<PeerVersionInfo>,
    upgrade_delay: Duration,
}

/// Tracked peer version info
#[derive(Debug, Clone)]
pub struct PeerVersionInfo {
    pub peer_id: String,
    pub version: ProtocolVersion,
    pub first_seen: Instant,
    pub upgrade_staged: bool,
    pub upgrade_sent: bool,
}

impl VersionNegotiator {
    pub fn new() -> Self {
        Self {
            local_version: ProtocolVersion::CURRENT,
            peer_versions: Vec::new(),
            upgrade_delay: Duration::from_secs(12 * 3600), // 12 hours
        }
    }

    pub fn with_version(version: ProtocolVersion) -> Self {
        Self {
            local_version: version,
            peer_versions: Vec::new(),
            upgrade_delay: Duration::from_secs(12 * 3600),
        }
    }

    /// Check compatibility with a peer's version
    pub fn check_compatibility(&self, peer_version: &ProtocolVersion) -> VersionCompatibility {
        if self.local_version.major != peer_version.major {
            VersionCompatibility::IncompatibleMajor
        } else if self.local_version.minor == peer_version.minor {
            VersionCompatibility::Compatible
        } else if peer_version.minor > self.local_version.minor {
            VersionCompatibility::BackwardCompatible
        } else {
            VersionCompatibility::UpgradeRequired
        }
    }

    /// Register a peer's version and return compatibility
    pub fn register_peer(&mut self, peer_id: &str, version: ProtocolVersion) -> VersionCompatibility {
        let compat = self.check_compatibility(&version);

        // Update or insert peer info
        if let Some(peer) = self.peer_versions.iter_mut().find(|p| p.peer_id == peer_id) {
            peer.version = version;
        } else {
            self.peer_versions.push(PeerVersionInfo {
                peer_id: peer_id.to_string(),
                version,
                first_seen: Instant::now(),
                upgrade_staged: compat == VersionCompatibility::UpgradeRequired,
                upgrade_sent: false,
            });
        }

        compat
    }

    /// Check if staged upgrade should be sent to peer (after 12h delay)
    pub fn should_send_upgrade(&self, peer_id: &str) -> bool {
        self.peer_versions.iter().any(|p| {
            p.peer_id == peer_id
                && p.upgrade_staged
                && !p.upgrade_sent
                && p.first_seen.elapsed() >= self.upgrade_delay
        })
    }

    /// Mark upgrade as sent for peer
    pub fn mark_upgrade_sent(&mut self, peer_id: &str) {
        if let Some(peer) = self.peer_versions.iter_mut().find(|p| p.peer_id == peer_id) {
            peer.upgrade_sent = true;
        }
    }

    /// Get all peers needing upgrade
    pub fn peers_needing_upgrade(&self) -> Vec<&PeerVersionInfo> {
        self.peer_versions
            .iter()
            .filter(|p| p.upgrade_staged && !p.upgrade_sent && p.first_seen.elapsed() >= self.upgrade_delay)
            .collect()
    }

    /// Get local version
    pub fn local_version(&self) -> &ProtocolVersion {
        &self.local_version
    }

    /// Get peer count
    pub fn peer_count(&self) -> usize {
        self.peer_versions.len()
    }

    /// Remove disconnected peer
    pub fn remove_peer(&mut self, peer_id: &str) {
        self.peer_versions.retain(|p| p.peer_id != peer_id);
    }
}

impl Default for VersionNegotiator {
    fn default() -> Self {
        Self::new()
    }
}
