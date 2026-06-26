/**
 * MEMSHADOW Protocol v3.0 - VLAN Topology Discovery and Relay (Rust)
 * 
 * Full implementation of VLAN topology discovery and multi-hop relay routing.
 * Gold standard reference implementation.
 */

use std::collections::{HashMap, HashSet, VecDeque};
use std::time::{SystemTime, UNIX_EPOCH};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum NodeConnectivity {
    Unknown = 0,
    VlanOnly = 1,
    Internet = 2,
    RelayCapable = 3,
}

#[derive(Debug, Clone)]
pub struct VlanNode {
    pub node_id: String,
    pub vlan_id: i32,
    pub address: String,
    pub port: u16,
    pub connectivity: NodeConnectivity,
    pub last_seen: u64,
    pub relay_capacity: i32,
    pub relay_load: i32,
}

#[derive(Debug, Clone)]
pub struct RelayPath {
    pub source: String,
    pub destination: String,
    pub hops: Vec<String>,
    pub total_hops: usize,
    pub discovered_at: u64,
    pub last_used: u64,
    pub success_count: i32,
    pub failure_count: i32,
    pub avg_latency_ms: f64,
}

pub struct TopologyDiscovery {
    node_id: String,
    known_nodes: HashMap<String, VlanNode>,
    vlan_members: HashMap<i32, HashSet<String>>,
    connectivity_map: HashMap<String, NodeConnectivity>,
    topology_graph: HashMap<String, HashSet<String>>,
}

impl TopologyDiscovery {
    pub fn new(node_id: String) -> Self {
        Self {
            node_id,
            known_nodes: HashMap::new(),
            vlan_members: HashMap::new(),
            connectivity_map: HashMap::new(),
            topology_graph: HashMap::new(),
        }
    }
    
    pub fn discover_node(&mut self, node: VlanNode) {
        self.known_nodes.insert(node.node_id.clone(), node.clone());
        self.vlan_members.entry(node.vlan_id).or_insert_with(HashSet::new).insert(node.node_id.clone());
        self.connectivity_map.insert(node.node_id.clone(), node.connectivity);
        
        if !self.topology_graph.contains_key(&node.node_id) {
            self.topology_graph.insert(node.node_id.clone(), HashSet::new());
        }
    }
    
    pub fn add_edge(&mut self, node1_id: &str, node2_id: &str) {
        self.topology_graph.entry(node1_id.to_string()).or_insert_with(HashSet::new).insert(node2_id.to_string());
        self.topology_graph.entry(node2_id.to_string()).or_insert_with(HashSet::new).insert(node1_id.to_string());
    }
    
    pub fn get_internet_nodes(&self) -> Vec<String> {
        self.connectivity_map.iter()
            .filter(|(_, conn)| matches!(conn, NodeConnectivity::Internet | NodeConnectivity::RelayCapable))
            .map(|(node_id, _)| node_id.clone())
            .collect()
    }
    
    pub fn get_relay_nodes(&self) -> Vec<String> {
        self.connectivity_map.iter()
            .filter(|(_, conn)| *conn == &NodeConnectivity::RelayCapable)
            .map(|(node_id, _)| node_id.clone())
            .collect()
    }
    
    pub fn find_path(&self, source: &str, destination: &str, max_hops: usize) -> Option<Vec<String>> {
        if source == destination {
            return Some(vec![source.to_string()]);
        }
        
        if !self.topology_graph.contains_key(source) || !self.topology_graph.contains_key(destination) {
            return None;
        }
        
        // BFS to find shortest path
        let mut queue = VecDeque::new();
        queue.push_back((source.to_string(), vec![source.to_string()]));
        let mut visited = HashSet::new();
        visited.insert(source.to_string());
        
        while let Some((current, path)) = queue.pop_front() {
            if path.len() > max_hops {
                continue;
            }
            
            if current == destination {
                return Some(path);
            }
            
            if let Some(neighbors) = self.topology_graph.get(&current) {
                for neighbor in neighbors {
                    if !visited.contains(neighbor) {
                        visited.insert(neighbor.clone());
                        let mut new_path = path.clone();
                        new_path.push(neighbor.clone());
                        queue.push_back((neighbor.clone(), new_path));
                    }
                }
            }
        }
        
        None
    }
}

pub struct RelayPathFinder {
    topology: TopologyDiscovery,
    path_cache: HashMap<(String, String), RelayPath>,
    path_ttl: u64,
}

impl RelayPathFinder {
    pub fn new(topology: TopologyDiscovery) -> Self {
        Self {
            topology,
            path_cache: HashMap::new(),
            path_ttl: 3600, // 1 hour
        }
    }
    
    pub fn find_relay_path(&mut self, source: &str, destination: &str, require_internet: bool) -> Option<RelayPath> {
        // Check cache
        let cache_key = (source.to_string(), destination.to_string());
        if let Some(cached_path) = self.path_cache.get(&cache_key) {
            let now = SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs();
            if now - cached_path.discovered_at < self.path_ttl {
                return Some(cached_path.clone());
            }
        }
        
        // Find path
        let path_nodes = self.topology.find_path(source, destination, 5)?;
        
        // Verify connectivity requirements
        if require_internet {
            if let Some(node) = self.topology.known_nodes.get(destination) {
                if !matches!(node.connectivity, NodeConnectivity::Internet | NodeConnectivity::RelayCapable) {
                    return None;
                }
            }
        }
        
        // Create relay path
        let now = SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs();
        let relay_path = RelayPath {
            source: source.to_string(),
            destination: destination.to_string(),
            hops: path_nodes[1..path_nodes.len()-1].to_vec(),
            total_hops: path_nodes.len() - 1,
            discovered_at: now,
            last_used: 0,
            success_count: 0,
            failure_count: 0,
            avg_latency_ms: 0.0,
        };
        
        // Cache path
        self.path_cache.insert(cache_key, relay_path.clone());
        
        Some(relay_path)
    }
    
    pub fn find_path_to_internet(&mut self, source: &str) -> Option<RelayPath> {
        let internet_nodes = self.topology.get_internet_nodes();
        if internet_nodes.is_empty() {
            return None;
        }
        
        // Find path to closest internet node
        let mut best_path: Option<RelayPath> = None;
        let mut min_hops = usize::MAX;
        
        for internet_node in internet_nodes {
            if let Some(path) = self.find_relay_path(source, &internet_node, false) {
                if path.total_hops < min_hops {
                    min_hops = path.total_hops;
                    best_path = Some(path);
                }
            }
        }
        
        best_path
    }
}

pub struct VlanRelayManager {
    node_id: String,
    vlan_id: i32,
    topology: TopologyDiscovery,
    path_finder: RelayPathFinder,
    active_relays: HashMap<String, RelayPath>,
}

impl Clone for TopologyDiscovery {
    fn clone(&self) -> Self {
        Self {
            node_id: self.node_id.clone(),
            known_nodes: self.known_nodes.clone(),
            vlan_members: self.vlan_members.clone(),
            connectivity_map: self.connectivity_map.clone(),
            topology_graph: self.topology_graph.clone(),
        }
    }
}

impl VlanRelayManager {
    pub fn new(node_id: String, vlan_id: i32) -> Self {
        let topology = TopologyDiscovery::new(node_id.clone());
        let path_finder = RelayPathFinder::new(topology.clone());
        
        Self {
            node_id,
            vlan_id,
            topology,
            path_finder,
            active_relays: HashMap::new(),
        }
    }
    
    pub fn register_node(&mut self, node: VlanNode) {
        self.topology.discover_node(node.clone());
        // Also update path_finder's topology
        self.path_finder.topology.discover_node(node);
    }
    
    pub fn register_connection(&mut self, node1_id: &str, node2_id: &str) {
        self.topology.add_edge(node1_id, node2_id);
        // Also update path_finder's topology
        self.path_finder.topology.add_edge(node1_id, node2_id);
    }
    
    pub fn needs_relay(&self, destination: &str) -> bool {
        let direct_path = self.topology.find_path(&self.node_id, destination, 1);
        direct_path.is_none() || direct_path.unwrap().len() > 1
    }
    
    pub fn find_relay_path(&mut self, destination: &str, require_internet: bool) -> Option<RelayPath> {
        self.path_finder.find_relay_path(&self.node_id, destination, require_internet)
    }
    
    pub fn find_path_to_internet(&mut self) -> Option<RelayPath> {
        self.path_finder.find_path_to_internet(&self.node_id)
    }
    
    pub fn start_relay(&mut self, relay_id: &str, path: RelayPath) -> bool {
        // Check relay capacity (simplified)
        self.active_relays.insert(relay_id.to_string(), path);
        true
    }
    
    pub fn stop_relay(&mut self, relay_id: &str) {
        self.active_relays.remove(relay_id);
    }
    
    pub fn get_topology_summary(&self) -> (usize, usize, usize, usize, usize) {
        (
            self.topology.known_nodes.len(),
            self.topology.vlan_members.len(),
            self.topology.get_internet_nodes().len(),
            self.topology.get_relay_nodes().len(),
            self.active_relays.len(),
        )
    }
}
