/**
 * MEMSHADOW Protocol v3.0 - Gossip/DHT Discovery (Rust)
 *
 * Decentralized peer discovery using gossip protocol and DHT track selection.
 * No bootstrap nodes required — fully self-organizing.
 * Matches Python memshadow_gossip.py gold standard.
 */

use std::collections::HashMap;
use std::net::SocketAddr;
use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

/// DHT track for network segmentation
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct DhtTrack {
    pub track_id: u64,
    pub name: String,
    pub max_nodes: usize,
}

/// Peer state in the gossip protocol
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PeerState {
    Alive,
    Suspect,
    Dead,
}

/// Gossip peer information
#[derive(Debug, Clone)]
pub struct GossipPeer {
    pub node_id: String,
    pub address: SocketAddr,
    pub state: PeerState,
    pub heartbeat: u64,
    pub last_seen: Instant,
    pub metadata: HashMap<String, String>,
    pub tracks: Vec<u64>,
}

impl GossipPeer {
    pub fn new(node_id: String, address: SocketAddr) -> Self {
        Self {
            node_id,
            address,
            state: PeerState::Alive,
            heartbeat: 0,
            last_seen: Instant::now(),
            metadata: HashMap::new(),
            tracks: Vec::new(),
        }
    }

    pub fn is_alive(&self) -> bool {
        self.state == PeerState::Alive
    }

    pub fn mark_suspect(&mut self) {
        if self.state == PeerState::Alive {
            self.state = PeerState::Suspect;
        }
    }

    pub fn mark_dead(&mut self) {
        self.state = PeerState::Dead;
    }

    pub fn mark_alive(&mut self) {
        self.state = PeerState::Alive;
        self.last_seen = Instant::now();
    }
}

/// Gossip message types
#[derive(Debug, Clone)]
pub enum GossipMessage {
    /// Periodic ping with peer list digest
    Ping {
        sender_id: String,
        heartbeat: u64,
        peer_digest: Vec<(String, u64)>,
    },
    /// Response to ping with full peer data
    Ack {
        sender_id: String,
        heartbeat: u64,
        peers: Vec<GossipPeerInfo>,
    },
    /// Indirect ping through a third peer
    PingReq {
        sender_id: String,
        target_id: String,
    },
    /// Announce new node joining
    Join {
        node_id: String,
        address: SocketAddr,
        tracks: Vec<u64>,
        metadata: HashMap<String, String>,
    },
    /// Announce node leaving
    Leave {
        node_id: String,
    },
    /// Anti-entropy full state sync
    FullSync {
        sender_id: String,
        peers: Vec<GossipPeerInfo>,
    },
}

/// Serializable peer info for gossip messages
#[derive(Debug, Clone)]
pub struct GossipPeerInfo {
    pub node_id: String,
    pub address: SocketAddr,
    pub state: PeerState,
    pub heartbeat: u64,
    pub tracks: Vec<u64>,
}

/// Selective announcement blacklist entry
#[derive(Debug, Clone)]
pub struct BlacklistEntry {
    pub node_id: String,
    pub reason: String,
    pub added_at: Instant,
    pub expires: Option<Instant>,
}

/// Gossip protocol manager
pub struct GossipManager {
    pub node_id: String,
    pub address: SocketAddr,
    peers: HashMap<String, GossipPeer>,
    tracks: Vec<DhtTrack>,
    active_tracks: Vec<u64>,
    blacklist: Vec<BlacklistEntry>,
    heartbeat_counter: u64,
    gossip_interval: Duration,
    suspect_timeout: Duration,
    dead_timeout: Duration,
    fanout: usize,
    last_gossip: Instant,
    metadata: HashMap<String, String>,
}

impl GossipManager {
    pub fn new(node_id: String, address: SocketAddr) -> Self {
        Self {
            node_id,
            address,
            peers: HashMap::new(),
            tracks: Vec::new(),
            active_tracks: Vec::new(),
            blacklist: Vec::new(),
            heartbeat_counter: 0,
            gossip_interval: Duration::from_secs(1),
            suspect_timeout: Duration::from_secs(5),
            dead_timeout: Duration::from_secs(30),
            fanout: 3,
            last_gossip: Instant::now(),
            metadata: HashMap::new(),
        }
    }

    /// Add a DHT track for network segmentation
    pub fn add_track(&mut self, track: DhtTrack) {
        let track_id = track.track_id;
        if !self.tracks.iter().any(|t| t.track_id == track_id) {
            self.tracks.push(track);
            self.active_tracks.push(track_id);
        }
    }

    /// Add a peer to the known peers list
    pub fn add_peer(&mut self, peer: GossipPeer) {
        if !self.is_blacklisted(&peer.node_id) {
            self.peers.insert(peer.node_id.clone(), peer);
        }
    }

    /// Remove a peer
    pub fn remove_peer(&mut self, node_id: &str) {
        self.peers.remove(node_id);
    }

    /// Get all alive peers
    pub fn alive_peers(&self) -> Vec<&GossipPeer> {
        self.peers.values().filter(|p| p.is_alive()).collect()
    }

    /// Get all peers (including suspect/dead)
    pub fn all_peers(&self) -> Vec<&GossipPeer> {
        self.peers.values().collect()
    }

    /// Get peer by ID
    pub fn get_peer(&self, node_id: &str) -> Option<&GossipPeer> {
        self.peers.get(node_id)
    }

    /// Get peers on a specific DHT track
    pub fn peers_on_track(&self, track_id: u64) -> Vec<&GossipPeer> {
        self.peers
            .values()
            .filter(|p| p.is_alive() && p.tracks.contains(&track_id))
            .collect()
    }

    /// Add node to blacklist (selective announcement / evasion)
    pub fn blacklist_node(&mut self, node_id: &str, reason: &str, ttl: Option<Duration>) {
        let expires = ttl.map(|d| Instant::now() + d);
        self.blacklist.push(BlacklistEntry {
            node_id: node_id.to_string(),
            reason: reason.to_string(),
            added_at: Instant::now(),
            expires,
        });
        // Remove peer if currently known
        self.peers.remove(node_id);
    }

    /// Check if a node is blacklisted
    pub fn is_blacklisted(&self, node_id: &str) -> bool {
        self.blacklist.iter().any(|e| {
            e.node_id == node_id
                && e.expires.map_or(true, |exp| Instant::now() < exp)
        })
    }

    /// Remove expired blacklist entries
    pub fn clean_blacklist(&mut self) {
        let now = Instant::now();
        self.blacklist
            .retain(|e| e.expires.map_or(true, |exp| now < exp));
    }

    /// Create a PING gossip message
    pub fn create_ping(&mut self) -> GossipMessage {
        self.heartbeat_counter += 1;
        let peer_digest: Vec<(String, u64)> = self
            .peers
            .iter()
            .filter(|(id, _)| !self.is_blacklisted(id))
            .map(|(id, p)| (id.clone(), p.heartbeat))
            .collect();

        GossipMessage::Ping {
            sender_id: self.node_id.clone(),
            heartbeat: self.heartbeat_counter,
            peer_digest,
        }
    }

    /// Create a JOIN announcement
    pub fn create_join(&self) -> GossipMessage {
        GossipMessage::Join {
            node_id: self.node_id.clone(),
            address: self.address,
            tracks: self.active_tracks.clone(),
            metadata: self.metadata.clone(),
        }
    }

    /// Create a LEAVE announcement
    pub fn create_leave(&self) -> GossipMessage {
        GossipMessage::Leave {
            node_id: self.node_id.clone(),
        }
    }

    /// Process an incoming gossip message
    pub fn process_message(&mut self, msg: GossipMessage) -> Option<GossipMessage> {
        match msg {
            GossipMessage::Ping {
                sender_id,
                heartbeat,
                peer_digest,
            } => {
                if self.is_blacklisted(&sender_id) {
                    return None;
                }

                // Update sender's heartbeat
                if let Some(peer) = self.peers.get_mut(&sender_id) {
                    peer.heartbeat = heartbeat;
                    peer.mark_alive();
                }

                // Find peers we have that sender doesn't (or has older versions)
                let mut new_peers = Vec::new();
                for (id, peer) in &self.peers {
                    if self.is_blacklisted(id) {
                        continue;
                    }
                    let sender_has = peer_digest.iter().find(|(pid, _)| pid == id);
                    match sender_has {
                        None => new_peers.push(GossipPeerInfo {
                            node_id: id.clone(),
                            address: peer.address,
                            state: peer.state,
                            heartbeat: peer.heartbeat,
                            tracks: peer.tracks.clone(),
                        }),
                        Some((_, hb)) if *hb < peer.heartbeat => {
                            new_peers.push(GossipPeerInfo {
                                node_id: id.clone(),
                                address: peer.address,
                                state: peer.state,
                                heartbeat: peer.heartbeat,
                                tracks: peer.tracks.clone(),
                            });
                        }
                        _ => {}
                    }
                }

                Some(GossipMessage::Ack {
                    sender_id: self.node_id.clone(),
                    heartbeat: self.heartbeat_counter,
                    peers: new_peers,
                })
            }

            GossipMessage::Ack {
                sender_id,
                heartbeat,
                peers,
            } => {
                if self.is_blacklisted(&sender_id) {
                    return None;
                }

                // Update sender
                if let Some(peer) = self.peers.get_mut(&sender_id) {
                    peer.heartbeat = heartbeat;
                    peer.mark_alive();
                }

                // Merge received peer info
                for info in peers {
                    if self.is_blacklisted(&info.node_id) || info.node_id == self.node_id {
                        continue;
                    }
                    if let Some(existing) = self.peers.get_mut(&info.node_id) {
                        if info.heartbeat > existing.heartbeat {
                            existing.heartbeat = info.heartbeat;
                            existing.address = info.address;
                            existing.tracks = info.tracks;
                            if info.state == PeerState::Alive {
                                existing.mark_alive();
                            }
                        }
                    } else {
                        let mut peer = GossipPeer::new(info.node_id, info.address);
                        peer.heartbeat = info.heartbeat;
                        peer.state = info.state;
                        peer.tracks = info.tracks;
                        self.peers.insert(peer.node_id.clone(), peer);
                    }
                }
                None
            }

            GossipMessage::Join {
                node_id,
                address,
                tracks,
                metadata,
            } => {
                if self.is_blacklisted(&node_id) || node_id == self.node_id {
                    return None;
                }
                let mut peer = GossipPeer::new(node_id, address);
                peer.tracks = tracks;
                peer.metadata = metadata;
                self.peers.insert(peer.node_id.clone(), peer);
                None
            }

            GossipMessage::Leave { node_id } => {
                if let Some(peer) = self.peers.get_mut(&node_id) {
                    peer.mark_dead();
                }
                None
            }

            GossipMessage::PingReq {
                sender_id,
                target_id,
            } => {
                // Indirect ping: we ping the target on behalf of the sender
                if self.is_blacklisted(&sender_id) {
                    return None;
                }
                if let Some(peer) = self.peers.get(&target_id) {
                    if peer.is_alive() {
                        return Some(GossipMessage::Ack {
                            sender_id: self.node_id.clone(),
                            heartbeat: self.heartbeat_counter,
                            peers: vec![GossipPeerInfo {
                                node_id: target_id,
                                address: peer.address,
                                state: peer.state,
                                heartbeat: peer.heartbeat,
                                tracks: peer.tracks.clone(),
                            }],
                        });
                    }
                }
                None
            }

            GossipMessage::FullSync { sender_id, peers } => {
                if self.is_blacklisted(&sender_id) {
                    return None;
                }
                for info in peers {
                    if self.is_blacklisted(&info.node_id) || info.node_id == self.node_id {
                        continue;
                    }
                    let mut peer = GossipPeer::new(info.node_id, info.address);
                    peer.heartbeat = info.heartbeat;
                    peer.state = info.state;
                    peer.tracks = info.tracks;
                    self.peers
                        .entry(peer.node_id.clone())
                        .or_insert(peer);
                }
                None
            }
        }
    }

    /// Run periodic maintenance: detect suspects and dead nodes
    pub fn tick(&mut self) {
        let now = Instant::now();
        for peer in self.peers.values_mut() {
            if peer.state == PeerState::Alive && now.duration_since(peer.last_seen) > self.suspect_timeout {
                peer.mark_suspect();
            }
            if peer.state == PeerState::Suspect && now.duration_since(peer.last_seen) > self.dead_timeout {
                peer.mark_dead();
            }
        }
        // Remove long-dead peers
        self.peers.retain(|_, p| {
            !(p.state == PeerState::Dead && now.duration_since(p.last_seen) > self.dead_timeout * 3)
        });
        self.clean_blacklist();
    }

    /// Select random peers for gossip fanout (excluding blacklisted)
    pub fn select_gossip_targets(&self) -> Vec<&GossipPeer> {
        use std::collections::HashSet;
        let alive: Vec<&GossipPeer> = self
            .peers
            .values()
            .filter(|p| p.is_alive() && !self.is_blacklisted(&p.node_id))
            .collect();

        if alive.len() <= self.fanout {
            return alive;
        }

        // Simple selection: take first N (in production, use random sampling)
        alive.into_iter().take(self.fanout).collect()
    }

    /// Get gossip fanout
    pub fn fanout(&self) -> usize {
        self.fanout
    }

    /// Set gossip fanout
    pub fn set_fanout(&mut self, fanout: usize) {
        self.fanout = fanout.max(1);
    }

    /// Get gossip interval
    pub fn gossip_interval(&self) -> Duration {
        self.gossip_interval
    }

    /// Set gossip interval
    pub fn set_gossip_interval(&mut self, interval: Duration) {
        self.gossip_interval = interval;
    }

    /// Should gossip now?
    pub fn should_gossip(&self) -> bool {
        self.last_gossip.elapsed() >= self.gossip_interval
    }

    /// Mark gossip as sent
    pub fn mark_gossiped(&mut self) {
        self.last_gossip = Instant::now();
    }

    /// Set node metadata
    pub fn set_metadata(&mut self, key: &str, value: &str) {
        self.metadata.insert(key.to_string(), value.to_string());
    }

    /// Peer count (alive only)
    pub fn alive_peer_count(&self) -> usize {
        self.peers.values().filter(|p| p.is_alive()).count()
    }

    /// Total peer count
    pub fn total_peer_count(&self) -> usize {
        self.peers.len()
    }
}
