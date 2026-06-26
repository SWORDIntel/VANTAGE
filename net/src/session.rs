/**
 * MEMSHADOW Protocol v3.0 - Session Management (Rust)
 *
 * Session lifecycle, key rotation, and timeout management.
 * Matches Go memshadow_session.go and Python gold standard.
 */

use std::collections::HashMap;
use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

/// Session state
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SessionState {
    Active,
    Idle,
    Expired,
    Closing,
}

/// A single protocol session
#[derive(Debug, Clone)]
pub struct Session {
    pub session_id: String,
    pub peer_id: String,
    pub state: SessionState,
    pub created_at: Instant,
    pub last_activity: Instant,
    pub timeout: Duration,
    pub session_key: Vec<u8>,
    pub hmac_key: Vec<u8>,
    pub key_rotation_count: u64,
    pub messages_sent: u64,
    pub messages_received: u64,
    pub bytes_sent: u64,
    pub bytes_received: u64,
}

impl Session {
    pub fn new(session_id: String, peer_id: String, session_key: Vec<u8>, hmac_key: Vec<u8>, timeout: Duration) -> Self {
        Self {
            session_id,
            peer_id,
            state: SessionState::Active,
            created_at: Instant::now(),
            last_activity: Instant::now(),
            timeout,
            session_key,
            hmac_key,
            key_rotation_count: 0,
            messages_sent: 0,
            messages_received: 0,
            bytes_sent: 0,
            bytes_received: 0,
        }
    }

    pub fn is_active(&self) -> bool {
        self.state == SessionState::Active
    }

    pub fn is_expired(&self) -> bool {
        self.state == SessionState::Expired || self.last_activity.elapsed() > self.timeout
    }

    pub fn touch(&mut self) {
        self.last_activity = Instant::now();
        if self.state == SessionState::Idle {
            self.state = SessionState::Active;
        }
    }

    pub fn record_send(&mut self, bytes: u64) {
        self.messages_sent += 1;
        self.bytes_sent += bytes;
        self.touch();
    }

    pub fn record_receive(&mut self, bytes: u64) {
        self.messages_received += 1;
        self.bytes_received += bytes;
        self.touch();
    }

    pub fn rotate_key(&mut self, new_key: Vec<u8>, new_hmac: Vec<u8>) {
        self.session_key = new_key;
        self.hmac_key = new_hmac;
        self.key_rotation_count += 1;
        self.touch();
    }

    pub fn close(&mut self) {
        self.state = SessionState::Closing;
    }

    pub fn expire(&mut self) {
        self.state = SessionState::Expired;
    }

    pub fn age(&self) -> Duration {
        self.created_at.elapsed()
    }

    pub fn idle_time(&self) -> Duration {
        self.last_activity.elapsed()
    }
}

/// Session manager handling multiple concurrent sessions
pub struct SessionManager {
    sessions: HashMap<String, Session>,
    default_timeout: Duration,
    max_sessions: usize,
    key_rotation_interval: Duration,
}

impl SessionManager {
    pub fn new(default_timeout: Duration, max_sessions: usize) -> Self {
        Self {
            sessions: HashMap::new(),
            default_timeout: if default_timeout.is_zero() { Duration::from_secs(30) } else { default_timeout },
            max_sessions: if max_sessions == 0 { 1024 } else { max_sessions },
            key_rotation_interval: Duration::from_secs(3600),
        }
    }

    /// Create a new session
    pub fn create_session(&mut self, peer_id: &str, session_key: Vec<u8>, hmac_key: Vec<u8>) -> Result<String, SessionError> {
        if self.sessions.len() >= self.max_sessions {
            // Try cleanup first
            self.cleanup_expired();
            if self.sessions.len() >= self.max_sessions {
                return Err(SessionError::LimitReached);
            }
        }

        let session_id = generate_session_id();
        let session = Session::new(
            session_id.clone(),
            peer_id.to_string(),
            session_key,
            hmac_key,
            self.default_timeout,
        );
        self.sessions.insert(session_id.clone(), session);
        Ok(session_id)
    }

    /// Get a session by ID
    pub fn get_session(&self, session_id: &str) -> Option<&Session> {
        self.sessions.get(session_id)
    }

    /// Get a mutable session by ID
    pub fn get_session_mut(&mut self, session_id: &str) -> Option<&mut Session> {
        self.sessions.get_mut(session_id)
    }

    /// Find session by peer ID
    pub fn find_by_peer(&self, peer_id: &str) -> Option<&Session> {
        self.sessions.values().find(|s| s.peer_id == peer_id && s.is_active())
    }

    /// Close a session
    pub fn close_session(&mut self, session_id: &str) -> bool {
        if let Some(session) = self.sessions.get_mut(session_id) {
            session.close();
            true
        } else {
            false
        }
    }

    /// Remove a session entirely
    pub fn remove_session(&mut self, session_id: &str) -> Option<Session> {
        self.sessions.remove(session_id)
    }

    /// Cleanup expired sessions
    pub fn cleanup_expired(&mut self) {
        self.sessions.retain(|_, s| !s.is_expired());
    }

    /// Check if any sessions need key rotation
    pub fn sessions_needing_rotation(&self) -> Vec<&str> {
        self.sessions
            .iter()
            .filter(|(_, s)| {
                s.is_active() && s.age() > self.key_rotation_interval * (s.key_rotation_count as u32 + 1)
            })
            .map(|(id, _)| id.as_str())
            .collect()
    }

    /// Get active session count
    pub fn active_count(&self) -> usize {
        self.sessions.values().filter(|s| s.is_active()).count()
    }

    /// Get total session count
    pub fn total_count(&self) -> usize {
        self.sessions.len()
    }

    /// Set key rotation interval
    pub fn set_key_rotation_interval(&mut self, interval: Duration) {
        self.key_rotation_interval = interval;
    }

    /// Run periodic maintenance
    pub fn tick(&mut self) {
        let now = Instant::now();
        for session in self.sessions.values_mut() {
            if session.is_active() && session.idle_time() > session.timeout / 2 {
                session.state = SessionState::Idle;
            }
            if session.idle_time() > session.timeout {
                session.expire();
            }
        }
        // Remove sessions expired for more than 5 minutes
        self.sessions.retain(|_, s| {
            !(s.state == SessionState::Expired && s.idle_time() > Duration::from_secs(300))
        });
    }

    /// Get session statistics
    pub fn stats(&self) -> SessionStats {
        let mut stats = SessionStats::default();
        for session in self.sessions.values() {
            match session.state {
                SessionState::Active => stats.active += 1,
                SessionState::Idle => stats.idle += 1,
                SessionState::Expired => stats.expired += 1,
                SessionState::Closing => stats.closing += 1,
            }
            stats.total_messages_sent += session.messages_sent;
            stats.total_messages_received += session.messages_received;
            stats.total_bytes_sent += session.bytes_sent;
            stats.total_bytes_received += session.bytes_received;
        }
        stats
    }
}

impl Default for SessionManager {
    fn default() -> Self {
        Self::new(Duration::from_secs(30), 1024)
    }
}

/// Aggregate session statistics
#[derive(Debug, Default, Clone)]
pub struct SessionStats {
    pub active: usize,
    pub idle: usize,
    pub expired: usize,
    pub closing: usize,
    pub total_messages_sent: u64,
    pub total_messages_received: u64,
    pub total_bytes_sent: u64,
    pub total_bytes_received: u64,
}

/// Session errors
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SessionError {
    LimitReached,
    NotFound,
    AlreadyExists,
    Expired,
    InvalidState,
}

impl std::fmt::Display for SessionError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::LimitReached => write!(f, "Maximum session limit reached"),
            Self::NotFound => write!(f, "Session not found"),
            Self::AlreadyExists => write!(f, "Session already exists"),
            Self::Expired => write!(f, "Session expired"),
            Self::InvalidState => write!(f, "Invalid session state"),
        }
    }
}

impl std::error::Error for SessionError {}

fn generate_session_id() -> String {
    let mut bytes = [0u8; 16];
    getrandom::getrandom(&mut bytes).unwrap_or_default();
    hex::encode(bytes)
}
