/**
 * MEMSHADOW Protocol v3.0 - Relay System (Rust)
 *
 * Multi-hop relay routing for messages between peers.
 * Supports path caching, load balancing, and automatic failover.
 * Matches Python relay_system.py gold standard.
 */

use std::collections::HashMap;
use std::net::SocketAddr;
use std::time::{Duration, Instant};

/// Relay hop in a route
#[derive(Debug, Clone)]
pub struct RelayHop {
    pub node_id: String,
    pub address: SocketAddr,
    pub latency_ms: f64,
}

/// A complete relay route (sequence of hops)
#[derive(Debug, Clone)]
pub struct RelayRoute {
    pub hops: Vec<RelayHop>,
    pub total_latency_ms: f64,
    pub created_at: Instant,
    pub ttl: Duration,
    pub use_count: u64,
}

impl RelayRoute {
    pub fn new(hops: Vec<RelayHop>, ttl: Duration) -> Self {
        let total_latency = hops.iter().map(|h| h.latency_ms).sum();
        Self {
            hops,
            total_latency_ms: total_latency,
            created_at: Instant::now(),
            ttl,
            use_count: 0,
        }
    }

    pub fn hop_count(&self) -> usize {
        self.hops.len()
    }

    pub fn is_expired(&self) -> bool {
        self.created_at.elapsed() > self.ttl
    }

    pub fn destination(&self) -> Option<&RelayHop> {
        self.hops.last()
    }
}

/// Result of a relay operation
#[derive(Debug, Clone)]
pub struct RelayResult {
    pub success: bool,
    pub route_used: Option<RelayRoute>,
    pub hops_traversed: usize,
    pub error: Option<String>,
}

/// Relay manager
pub struct RelayManager {
    node_id: String,
    route_cache: HashMap<String, Vec<RelayRoute>>,
    max_hops: usize,
    route_ttl: Duration,
    max_cached_routes: usize,
    max_alternatives: usize,
    relay_stats: RelayStats,
}

#[derive(Debug, Clone, Default)]
pub struct RelayStats {
    pub messages_relayed: u64,
    pub bytes_relayed: u64,
    pub routes_discovered: u64,
    pub routes_expired: u64,
    pub relay_failures: u64,
    pub cache_hits: u64,
    pub cache_misses: u64,
}

impl RelayManager {
    pub fn new(node_id: String) -> Self {
        Self {
            node_id,
            route_cache: HashMap::new(),
            max_hops: 5,
            route_ttl: Duration::from_secs(3600),
            max_cached_routes: 1000,
            max_alternatives: 3,
            relay_stats: RelayStats::default(),
        }
    }

    /// Add a discovered route to the cache
    pub fn add_route(&mut self, destination: &str, route: RelayRoute) {
        let routes = self.route_cache.entry(destination.to_string()).or_default();

        // Enforce max alternatives per destination
        if routes.len() >= self.max_alternatives {
            // Remove the worst route (highest latency)
            if let Some(worst_idx) = routes.iter().enumerate()
                .max_by(|(_, a), (_, b)| a.total_latency_ms.partial_cmp(&b.total_latency_ms).unwrap_or(std::cmp::Ordering::Equal))
                .map(|(i, _)| i)
            {
                if routes[worst_idx].total_latency_ms > route.total_latency_ms {
                    routes.remove(worst_idx);
                } else {
                    return; // New route is worse, don't add
                }
            }
        }

        routes.push(route);
        self.relay_stats.routes_discovered += 1;
    }

    /// Get best route to destination
    pub fn get_route(&mut self, destination: &str) -> Option<&RelayRoute> {
        // Clean expired routes first
        if let Some(routes) = self.route_cache.get_mut(destination) {
            let expired_count = routes.iter().filter(|r| r.is_expired()).count();
            routes.retain(|r| !r.is_expired());
            self.relay_stats.routes_expired += expired_count as u64;
        }

        if let Some(routes) = self.route_cache.get(destination) {
            if routes.is_empty() {
                self.relay_stats.cache_misses += 1;
                return None;
            }
            self.relay_stats.cache_hits += 1;
            // Return lowest latency route
            routes.iter().min_by(|a, b| {
                a.total_latency_ms.partial_cmp(&b.total_latency_ms).unwrap_or(std::cmp::Ordering::Equal)
            })
        } else {
            self.relay_stats.cache_misses += 1;
            None
        }
    }

    /// Get alternative routes to destination
    pub fn get_alternative_routes(&self, destination: &str) -> Vec<&RelayRoute> {
        self.route_cache
            .get(destination)
            .map(|routes| routes.iter().filter(|r| !r.is_expired()).collect())
            .unwrap_or_default()
    }

    /// Record a successful relay
    pub fn record_relay(&mut self, destination: &str, bytes: u64) {
        self.relay_stats.messages_relayed += 1;
        self.relay_stats.bytes_relayed += bytes;
        // Increment use count on the route
        if let Some(routes) = self.route_cache.get_mut(destination) {
            if let Some(route) = routes.iter_mut().min_by(|a, b| {
                a.total_latency_ms.partial_cmp(&b.total_latency_ms).unwrap_or(std::cmp::Ordering::Equal)
            }) {
                route.use_count += 1;
            }
        }
    }

    /// Record a relay failure
    pub fn record_failure(&mut self, destination: &str) {
        self.relay_stats.relay_failures += 1;
        // Remove the failed route
        if let Some(routes) = self.route_cache.get_mut(destination) {
            if !routes.is_empty() {
                // Remove worst route
                if let Some(worst_idx) = routes.iter().enumerate()
                    .max_by(|(_, a), (_, b)| a.total_latency_ms.partial_cmp(&b.total_latency_ms).unwrap_or(std::cmp::Ordering::Equal))
                    .map(|(i, _)| i)
                {
                    routes.remove(worst_idx);
                }
            }
        }
    }

    /// Clean expired routes from cache
    pub fn cleanup(&mut self) {
        for routes in self.route_cache.values_mut() {
            let before = routes.len();
            routes.retain(|r| !r.is_expired());
            self.relay_stats.routes_expired += (before - routes.len()) as u64;
        }
        self.route_cache.retain(|_, routes| !routes.is_empty());
    }

    /// Has route to destination?
    pub fn has_route(&self, destination: &str) -> bool {
        self.route_cache
            .get(destination)
            .map(|routes| routes.iter().any(|r| !r.is_expired()))
            .unwrap_or(false)
    }

    /// Get relay statistics
    pub fn stats(&self) -> &RelayStats {
        &self.relay_stats
    }

    /// Set max hops
    pub fn set_max_hops(&mut self, max: usize) {
        self.max_hops = max.max(1).min(10);
    }

    /// Set route TTL
    pub fn set_route_ttl(&mut self, ttl: Duration) {
        self.route_ttl = ttl;
    }

    /// Get cached route count
    pub fn cached_route_count(&self) -> usize {
        self.route_cache.values().map(|r| r.len()).sum()
    }

    /// Get reachable destination count
    pub fn reachable_destinations(&self) -> usize {
        self.route_cache.iter()
            .filter(|(_, routes)| routes.iter().any(|r| !r.is_expired()))
            .count()
    }
}
