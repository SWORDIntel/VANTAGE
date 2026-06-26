/**
 * MEMSHADOW Protocol v3.0 - Peer Manager (Rust)
 *
 * Large local network peer management with health monitoring,
 * load balancing, and automatic failover.
 * Matches Python peer_manager.py gold standard.
 */

use std::collections::HashMap;
use std::net::SocketAddr;
use std::time::{Duration, Instant};

/// Peer connection status
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PeerStatus {
    Connected,
    Connecting,
    Disconnected,
    Banned,
    Unreachable,
}

/// Peer health metrics
#[derive(Debug, Clone)]
pub struct PeerHealth {
    pub latency_ms: f64,
    pub packet_loss: f64,
    pub bandwidth_bps: u64,
    pub uptime: Duration,
    pub last_ping: Instant,
    pub ping_failures: u32,
    pub max_ping_failures: u32,
}

impl Default for PeerHealth {
    fn default() -> Self {
        Self {
            latency_ms: 0.0,
            packet_loss: 0.0,
            bandwidth_bps: 0,
            uptime: Duration::ZERO,
            last_ping: Instant::now(),
            ping_failures: 0,
            max_ping_failures: 3,
        }
    }
}

impl PeerHealth {
    pub fn score(&self) -> f64 {
        let latency_score = 1.0 / (1.0 + self.latency_ms / 100.0);
        let loss_score = 1.0 - self.packet_loss;
        let bw_score = (self.bandwidth_bps as f64).log2().max(0.0) / 30.0;
        latency_score * 0.4 + loss_score * 0.4 + bw_score * 0.2
    }

    pub fn is_healthy(&self) -> bool {
        self.ping_failures < self.max_ping_failures && self.packet_loss < 0.5
    }

    pub fn record_ping(&mut self, latency_ms: f64) {
        self.latency_ms = self.latency_ms * 0.7 + latency_ms * 0.3; // EWMA
        self.last_ping = Instant::now();
        self.ping_failures = 0;
    }

    pub fn record_ping_failure(&mut self) {
        self.ping_failures += 1;
    }
}

/// Peer information
#[derive(Debug, Clone)]
pub struct PeerInfo {
    pub node_id: String,
    pub address: SocketAddr,
    pub status: PeerStatus,
    pub health: PeerHealth,
    pub capabilities: u32,
    pub version: u16,
    pub metadata: HashMap<String, String>,
    pub connected_since: Option<Instant>,
    pub messages_sent: u64,
    pub messages_received: u64,
    pub bytes_sent: u64,
    pub bytes_received: u64,
}

impl PeerInfo {
    pub fn new(node_id: String, address: SocketAddr) -> Self {
        Self {
            node_id,
            address,
            status: PeerStatus::Disconnected,
            health: PeerHealth::default(),
            capabilities: 0,
            version: 0x0300,
            metadata: HashMap::new(),
            connected_since: None,
            messages_sent: 0,
            messages_received: 0,
            bytes_sent: 0,
            bytes_received: 0,
        }
    }
}

/// Peer selection strategy
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SelectionStrategy {
    /// Select peer with best health score
    BestHealth,
    /// Round-robin among connected peers
    RoundRobin,
    /// Random selection
    Random,
    /// Lowest latency
    LowestLatency,
    /// Highest bandwidth
    HighestBandwidth,
}

/// Peer manager
pub struct PeerManager {
    peers: HashMap<String, PeerInfo>,
    max_peers: usize,
    ping_interval: Duration,
    ban_duration: Duration,
    selection_strategy: SelectionStrategy,
    round_robin_index: usize,
}

impl PeerManager {
    pub fn new(max_peers: usize) -> Self {
        Self {
            peers: HashMap::new(),
            max_peers: if max_peers == 0 { 256 } else { max_peers },
            ping_interval: Duration::from_secs(10),
            ban_duration: Duration::from_secs(3600),
            selection_strategy: SelectionStrategy::BestHealth,
            round_robin_index: 0,
        }
    }

    /// Add or update a peer
    pub fn add_peer(&mut self, peer: PeerInfo) -> bool {
        if self.peers.len() >= self.max_peers && !self.peers.contains_key(&peer.node_id) {
            return false;
        }
        self.peers.insert(peer.node_id.clone(), peer);
        true
    }

    /// Remove a peer
    pub fn remove_peer(&mut self, node_id: &str) -> Option<PeerInfo> {
        self.peers.remove(node_id)
    }

    /// Get peer by ID
    pub fn get_peer(&self, node_id: &str) -> Option<&PeerInfo> {
        self.peers.get(node_id)
    }

    /// Get mutable peer by ID
    pub fn get_peer_mut(&mut self, node_id: &str) -> Option<&mut PeerInfo> {
        self.peers.get_mut(node_id)
    }

    /// Get all connected peers
    pub fn connected_peers(&self) -> Vec<&PeerInfo> {
        self.peers.values().filter(|p| p.status == PeerStatus::Connected).collect()
    }

    /// Get all peers
    pub fn all_peers(&self) -> Vec<&PeerInfo> {
        self.peers.values().collect()
    }

    /// Select best peer based on strategy
    pub fn select_peer(&mut self) -> Option<&PeerInfo> {
        let connected: Vec<&str> = self.peers.iter()
            .filter(|(_, p)| p.status == PeerStatus::Connected && p.health.is_healthy())
            .map(|(id, _)| id.as_str())
            .collect();

        if connected.is_empty() {
            return None;
        }

        let selected_id = match self.selection_strategy {
            SelectionStrategy::BestHealth => {
                connected.iter()
                    .max_by(|a, b| {
                        let ha = self.peers[**a].health.score();
                        let hb = self.peers[**b].health.score();
                        ha.partial_cmp(&hb).unwrap_or(std::cmp::Ordering::Equal)
                    })
                    .copied()
            }
            SelectionStrategy::LowestLatency => {
                connected.iter()
                    .min_by(|a, b| {
                        let la = self.peers[**a].health.latency_ms;
                        let lb = self.peers[**b].health.latency_ms;
                        la.partial_cmp(&lb).unwrap_or(std::cmp::Ordering::Equal)
                    })
                    .copied()
            }
            SelectionStrategy::HighestBandwidth => {
                connected.iter()
                    .max_by_key(|id| self.peers[**id].health.bandwidth_bps)
                    .copied()
            }
            SelectionStrategy::RoundRobin => {
                self.round_robin_index = (self.round_robin_index + 1) % connected.len();
                Some(connected[self.round_robin_index])
            }
            SelectionStrategy::Random => {
                let mut rng = [0u8; 1];
                getrandom::getrandom(&mut rng).unwrap_or_default();
                let idx = rng[0] as usize % connected.len();
                Some(connected[idx])
            }
        };

        selected_id.and_then(|id| self.peers.get(id))
    }

    /// Mark peer as connected
    pub fn connect_peer(&mut self, node_id: &str) {
        if let Some(peer) = self.peers.get_mut(node_id) {
            peer.status = PeerStatus::Connected;
            peer.connected_since = Some(Instant::now());
        }
    }

    /// Mark peer as disconnected
    pub fn disconnect_peer(&mut self, node_id: &str) {
        if let Some(peer) = self.peers.get_mut(node_id) {
            peer.status = PeerStatus::Disconnected;
            peer.connected_since = None;
        }
    }

    /// Ban a peer
    pub fn ban_peer(&mut self, node_id: &str) {
        if let Some(peer) = self.peers.get_mut(node_id) {
            peer.status = PeerStatus::Banned;
            peer.connected_since = None;
        }
    }

    /// Run periodic health checks
    pub fn tick(&mut self) {
        let peers_needing_ping: Vec<String> = self.peers.iter()
            .filter(|(_, p)| {
                p.status == PeerStatus::Connected
                    && p.health.last_ping.elapsed() > self.ping_interval
            })
            .map(|(id, _)| id.clone())
            .collect();

        for node_id in &peers_needing_ping {
            if let Some(peer) = self.peers.get_mut(node_id) {
                if !peer.health.is_healthy() {
                    peer.status = PeerStatus::Unreachable;
                }
            }
        }
    }

    /// Set selection strategy
    pub fn set_strategy(&mut self, strategy: SelectionStrategy) {
        self.selection_strategy = strategy;
    }

    /// Peer count by status
    pub fn count_by_status(&self, status: PeerStatus) -> usize {
        self.peers.values().filter(|p| p.status == status).count()
    }

    /// Total peer count
    pub fn total_count(&self) -> usize {
        self.peers.len()
    }
}
