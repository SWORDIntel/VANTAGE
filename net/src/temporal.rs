/**
 * MEMSHADOW Protocol v3.0 - Temporal Queue (Rust)
 *
 * Time-delayed message delivery — schedule messages for future
 * or conditional delivery. Supports TTL, priority ordering,
 * and cancellation.
 * Matches Python temporal_queue.py gold standard.
 */

use std::collections::BinaryHeap;
use std::cmp::Ordering;
use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

/// Temporal delivery mode
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TemporalMode {
    /// Deliver at exact timestamp
    Scheduled,
    /// Deliver after delay from now
    Delayed,
    /// Deliver when condition met (polled externally)
    Conditional,
    /// Deliver in order of priority when polled
    PriorityQueue,
}

/// A temporally queued message
#[derive(Debug, Clone)]
pub struct TemporalMessage {
    pub id: String,
    pub payload: Vec<u8>,
    pub destination: String,
    pub mode: TemporalMode,
    pub deliver_at: Instant,
    pub created_at: Instant,
    pub ttl: Duration,
    pub priority: u8,
    pub cancelled: bool,
    pub delivered: bool,
    pub retry_count: u32,
    pub max_retries: u32,
}

impl TemporalMessage {
    pub fn delayed(id: String, payload: Vec<u8>, destination: String, delay: Duration) -> Self {
        Self {
            id,
            payload,
            destination,
            mode: TemporalMode::Delayed,
            deliver_at: Instant::now() + delay,
            created_at: Instant::now(),
            ttl: delay + Duration::from_secs(3600),
            priority: 2,
            cancelled: false,
            delivered: false,
            retry_count: 0,
            max_retries: 3,
        }
    }

    pub fn scheduled(id: String, payload: Vec<u8>, destination: String, deliver_at: Instant) -> Self {
        Self {
            id,
            payload,
            destination,
            mode: TemporalMode::Scheduled,
            deliver_at,
            created_at: Instant::now(),
            ttl: Duration::from_secs(86400),
            priority: 2,
            cancelled: false,
            delivered: false,
            retry_count: 0,
            max_retries: 3,
        }
    }

    pub fn is_ready(&self) -> bool {
        !self.cancelled && !self.delivered && Instant::now() >= self.deliver_at
    }

    pub fn is_expired(&self) -> bool {
        self.created_at.elapsed() > self.ttl
    }

    pub fn can_retry(&self) -> bool {
        self.retry_count < self.max_retries
    }
}

// Priority ordering for BinaryHeap (higher priority + earlier delivery first)
impl PartialEq for TemporalMessage {
    fn eq(&self, other: &Self) -> bool {
        self.id == other.id
    }
}

impl Eq for TemporalMessage {}

impl PartialOrd for TemporalMessage {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for TemporalMessage {
    fn cmp(&self, other: &Self) -> Ordering {
        // Higher priority first, then earlier delivery time
        other.priority.cmp(&self.priority)
            .then_with(|| other.deliver_at.cmp(&self.deliver_at))
    }
}

/// Temporal queue manager
pub struct TemporalQueue {
    queue: Vec<TemporalMessage>,
    max_size: usize,
    stats: TemporalStats,
}

#[derive(Debug, Clone, Default)]
pub struct TemporalStats {
    pub enqueued: u64,
    pub delivered: u64,
    pub cancelled: u64,
    pub expired: u64,
    pub retried: u64,
}

impl TemporalQueue {
    pub fn new(max_size: usize) -> Self {
        Self {
            queue: Vec::new(),
            max_size: if max_size == 0 { 10000 } else { max_size },
            stats: TemporalStats::default(),
        }
    }

    /// Enqueue a message for delayed/scheduled delivery
    pub fn enqueue(&mut self, msg: TemporalMessage) -> Result<(), TemporalError> {
        if self.queue.len() >= self.max_size {
            // Try cleanup first
            self.cleanup_expired();
            if self.queue.len() >= self.max_size {
                return Err(TemporalError::QueueFull);
            }
        }
        self.stats.enqueued += 1;
        self.queue.push(msg);
        // Sort by delivery time (soonest first)
        self.queue.sort_by(|a, b| a.deliver_at.cmp(&b.deliver_at));
        Ok(())
    }

    /// Get all messages ready for delivery
    pub fn get_ready(&mut self) -> Vec<TemporalMessage> {
        let mut ready = Vec::new();
        let mut remaining = Vec::new();

        for msg in self.queue.drain(..) {
            if msg.cancelled || msg.is_expired() {
                if msg.cancelled { self.stats.cancelled += 1; }
                if msg.is_expired() { self.stats.expired += 1; }
                continue;
            }
            if msg.is_ready() {
                self.stats.delivered += 1;
                ready.push(msg);
            } else {
                remaining.push(msg);
            }
        }

        self.queue = remaining;
        ready
    }

    /// Cancel a queued message by ID
    pub fn cancel(&mut self, id: &str) -> bool {
        if let Some(msg) = self.queue.iter_mut().find(|m| m.id == id) {
            msg.cancelled = true;
            self.stats.cancelled += 1;
            true
        } else {
            false
        }
    }

    /// Retry a failed delivery
    pub fn retry(&mut self, mut msg: TemporalMessage, delay: Duration) -> Result<(), TemporalError> {
        if !msg.can_retry() {
            return Err(TemporalError::MaxRetriesExceeded);
        }
        msg.retry_count += 1;
        msg.deliver_at = Instant::now() + delay;
        msg.delivered = false;
        self.stats.retried += 1;
        self.enqueue(msg)
    }

    /// Get message by ID (peek)
    pub fn get_message(&self, id: &str) -> Option<&TemporalMessage> {
        self.queue.iter().find(|m| m.id == id)
    }

    /// Cleanup expired and cancelled messages
    pub fn cleanup_expired(&mut self) {
        let before = self.queue.len();
        self.queue.retain(|m| !m.cancelled && !m.is_expired());
        let removed = before - self.queue.len();
        self.stats.expired += removed as u64;
    }

    /// Queue size
    pub fn len(&self) -> usize {
        self.queue.len()
    }

    /// Is queue empty?
    pub fn is_empty(&self) -> bool {
        self.queue.is_empty()
    }

    /// Get statistics
    pub fn stats(&self) -> &TemporalStats {
        &self.stats
    }

    /// Count messages ready now
    pub fn ready_count(&self) -> usize {
        self.queue.iter().filter(|m| m.is_ready()).count()
    }

    /// Time until next delivery
    pub fn time_until_next(&self) -> Option<Duration> {
        self.queue.iter()
            .filter(|m| !m.cancelled && !m.is_expired() && !m.is_ready())
            .min_by_key(|m| m.deliver_at)
            .map(|m| m.deliver_at.saturating_duration_since(Instant::now()))
    }
}

impl Default for TemporalQueue {
    fn default() -> Self {
        Self::new(10000)
    }
}

/// Temporal queue errors
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TemporalError {
    QueueFull,
    MessageNotFound,
    MaxRetriesExceeded,
    MessageExpired,
}

impl std::fmt::Display for TemporalError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::QueueFull => write!(f, "Temporal queue full"),
            Self::MessageNotFound => write!(f, "Message not found in queue"),
            Self::MaxRetriesExceeded => write!(f, "Maximum retries exceeded"),
            Self::MessageExpired => write!(f, "Message has expired"),
        }
    }
}

impl std::error::Error for TemporalError {}
