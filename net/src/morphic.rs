/**
 * MEMSHADOW Protocol v3.0 - Morphic Adaptation (Rust)
 *
 * Self-adapting protocol structure. Peers propose, vote on,
 * and apply protocol modifications at runtime.
 * Matches Python morphic_adaptation.py gold standard.
 */

use std::collections::HashMap;
use std::time::{Duration, Instant};

/// Adaptation type
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum AdaptationType {
    HeaderReorder,
    FieldResize,
    NewField,
    RemoveField,
    CompressionChange,
    EncryptionChange,
    TimingChange,
    PaddingChange,
}

/// Adaptation status
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AdaptationStatus {
    Proposed,
    Voting,
    Testing,
    Accepted,
    Rejected,
    Applied,
    RolledBack,
}

/// Vote type
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum VoteType {
    Accept,
    Reject,
    Abstain,
}

/// A proposal for protocol adaptation
#[derive(Debug, Clone)]
pub struct AdaptationProposal {
    pub id: String,
    pub proposer_id: String,
    pub adaptation_type: AdaptationType,
    pub description: String,
    pub parameters: HashMap<String, String>,
    pub status: AdaptationStatus,
    pub votes: HashMap<String, VoteType>,
    pub created_at: Instant,
    pub voting_deadline: Instant,
    pub test_duration: Duration,
    pub test_started_at: Option<Instant>,
    pub test_metrics: Option<AdaptationMetrics>,
    pub required_majority: f64,
}

impl AdaptationProposal {
    pub fn new(
        id: String,
        proposer_id: String,
        adaptation_type: AdaptationType,
        description: String,
    ) -> Self {
        Self {
            id,
            proposer_id,
            adaptation_type,
            description,
            parameters: HashMap::new(),
            status: AdaptationStatus::Proposed,
            votes: HashMap::new(),
            created_at: Instant::now(),
            voting_deadline: Instant::now() + Duration::from_secs(300),
            test_duration: Duration::from_secs(60),
            test_started_at: None,
            test_metrics: None,
            required_majority: 0.67,
        }
    }

    pub fn add_vote(&mut self, voter_id: &str, vote: VoteType) {
        self.votes.insert(voter_id.to_string(), vote);
    }

    pub fn vote_tally(&self) -> (usize, usize, usize) {
        let accepts = self.votes.values().filter(|v| **v == VoteType::Accept).count();
        let rejects = self.votes.values().filter(|v| **v == VoteType::Reject).count();
        let abstains = self.votes.values().filter(|v| **v == VoteType::Abstain).count();
        (accepts, rejects, abstains)
    }

    pub fn has_majority(&self) -> bool {
        let (accepts, rejects, _) = self.vote_tally();
        let total_votes = accepts + rejects;
        if total_votes == 0 {
            return false;
        }
        (accepts as f64 / total_votes as f64) >= self.required_majority
    }

    pub fn is_voting_expired(&self) -> bool {
        Instant::now() > self.voting_deadline
    }

    pub fn is_test_complete(&self) -> bool {
        self.test_started_at
            .map(|t| t.elapsed() >= self.test_duration)
            .unwrap_or(false)
    }
}

/// Metrics collected during adaptation testing
#[derive(Debug, Clone)]
pub struct AdaptationMetrics {
    pub latency_before_ms: f64,
    pub latency_after_ms: f64,
    pub throughput_before_bps: u64,
    pub throughput_after_bps: u64,
    pub error_rate_before: f64,
    pub error_rate_after: f64,
    pub detection_risk_before: f64,
    pub detection_risk_after: f64,
}

impl AdaptationMetrics {
    pub fn is_improvement(&self) -> bool {
        let latency_improved = self.latency_after_ms <= self.latency_before_ms * 1.1;
        let throughput_improved = self.throughput_after_bps >= (self.throughput_before_bps as f64 * 0.9) as u64;
        let error_ok = self.error_rate_after <= self.error_rate_before * 1.5;
        let detection_improved = self.detection_risk_after <= self.detection_risk_before;

        latency_improved && throughput_improved && error_ok && detection_improved
    }
}

/// Morphic adaptation manager
pub struct MorphicManager {
    node_id: String,
    proposals: HashMap<String, AdaptationProposal>,
    applied_adaptations: Vec<String>,
    adaptation_history: Vec<AdaptationRecord>,
    max_concurrent_proposals: usize,
    max_applied: usize,
}

#[derive(Debug, Clone)]
pub struct AdaptationRecord {
    pub proposal_id: String,
    pub adaptation_type: AdaptationType,
    pub status: AdaptationStatus,
    pub applied_at: Option<Instant>,
    pub rolled_back_at: Option<Instant>,
}

impl MorphicManager {
    pub fn new(node_id: String) -> Self {
        Self {
            node_id,
            proposals: HashMap::new(),
            applied_adaptations: Vec::new(),
            adaptation_history: Vec::new(),
            max_concurrent_proposals: 5,
            max_applied: 20,
        }
    }

    /// Create a new adaptation proposal
    pub fn propose(
        &mut self,
        adaptation_type: AdaptationType,
        description: &str,
        parameters: HashMap<String, String>,
    ) -> Result<String, MorphicError> {
        if self.proposals.len() >= self.max_concurrent_proposals {
            return Err(MorphicError::TooManyProposals);
        }

        let mut id_bytes = [0u8; 8];
        getrandom::getrandom(&mut id_bytes).unwrap_or_default();
        let id = hex::encode(id_bytes);

        let mut proposal = AdaptationProposal::new(
            id.clone(),
            self.node_id.clone(),
            adaptation_type,
            description.to_string(),
        );
        proposal.parameters = parameters;
        proposal.status = AdaptationStatus::Voting;

        // Self-vote accept
        proposal.add_vote(&self.node_id, VoteType::Accept);

        self.proposals.insert(id.clone(), proposal);
        Ok(id)
    }

    /// Vote on a proposal
    pub fn vote(&mut self, proposal_id: &str, vote: VoteType) -> Result<(), MorphicError> {
        let proposal = self.proposals.get_mut(proposal_id)
            .ok_or(MorphicError::ProposalNotFound)?;

        if proposal.status != AdaptationStatus::Voting {
            return Err(MorphicError::InvalidState);
        }

        proposal.add_vote(&self.node_id, vote);
        Ok(())
    }

    /// Process incoming vote from peer
    pub fn receive_vote(&mut self, proposal_id: &str, voter_id: &str, vote: VoteType) -> Result<(), MorphicError> {
        let proposal = self.proposals.get_mut(proposal_id)
            .ok_or(MorphicError::ProposalNotFound)?;

        if proposal.status != AdaptationStatus::Voting {
            return Err(MorphicError::InvalidState);
        }

        proposal.add_vote(voter_id, vote);
        Ok(())
    }

    /// Check proposals and advance state machine
    pub fn tick(&mut self) {
        let ids: Vec<String> = self.proposals.keys().cloned().collect();

        for id in ids {
            if let Some(proposal) = self.proposals.get_mut(&id) {
                match proposal.status {
                    AdaptationStatus::Voting => {
                        if proposal.is_voting_expired() {
                            if proposal.has_majority() {
                                proposal.status = AdaptationStatus::Testing;
                                proposal.test_started_at = Some(Instant::now());
                            } else {
                                proposal.status = AdaptationStatus::Rejected;
                            }
                        }
                    }
                    AdaptationStatus::Testing => {
                        if proposal.is_test_complete() {
                            if let Some(ref metrics) = proposal.test_metrics {
                                if metrics.is_improvement() {
                                    proposal.status = AdaptationStatus::Accepted;
                                } else {
                                    proposal.status = AdaptationStatus::RolledBack;
                                }
                            } else {
                                // No metrics collected — accept by default
                                proposal.status = AdaptationStatus::Accepted;
                            }
                        }
                    }
                    _ => {}
                }
            }
        }
    }

    /// Apply an accepted proposal
    pub fn apply(&mut self, proposal_id: &str) -> Result<(), MorphicError> {
        let proposal = self.proposals.get_mut(proposal_id)
            .ok_or(MorphicError::ProposalNotFound)?;

        if proposal.status != AdaptationStatus::Accepted {
            return Err(MorphicError::InvalidState);
        }

        if self.applied_adaptations.len() >= self.max_applied {
            return Err(MorphicError::TooManyApplied);
        }

        proposal.status = AdaptationStatus::Applied;
        self.applied_adaptations.push(proposal_id.to_string());
        self.adaptation_history.push(AdaptationRecord {
            proposal_id: proposal_id.to_string(),
            adaptation_type: proposal.adaptation_type,
            status: AdaptationStatus::Applied,
            applied_at: Some(Instant::now()),
            rolled_back_at: None,
        });

        Ok(())
    }

    /// Rollback an applied adaptation
    pub fn rollback(&mut self, proposal_id: &str) -> Result<(), MorphicError> {
        let proposal = self.proposals.get_mut(proposal_id)
            .ok_or(MorphicError::ProposalNotFound)?;

        if proposal.status != AdaptationStatus::Applied {
            return Err(MorphicError::InvalidState);
        }

        proposal.status = AdaptationStatus::RolledBack;
        self.applied_adaptations.retain(|id| id != proposal_id);

        if let Some(record) = self.adaptation_history.iter_mut()
            .find(|r| r.proposal_id == proposal_id)
        {
            record.status = AdaptationStatus::RolledBack;
            record.rolled_back_at = Some(Instant::now());
        }

        Ok(())
    }

    /// Get proposal by ID
    pub fn get_proposal(&self, id: &str) -> Option<&AdaptationProposal> {
        self.proposals.get(id)
    }

    /// Get active proposals
    pub fn active_proposals(&self) -> Vec<&AdaptationProposal> {
        self.proposals.values()
            .filter(|p| matches!(p.status, AdaptationStatus::Voting | AdaptationStatus::Testing))
            .collect()
    }

    /// Get applied adaptation count
    pub fn applied_count(&self) -> usize {
        self.applied_adaptations.len()
    }

    /// Get adaptation history
    pub fn history(&self) -> &[AdaptationRecord] {
        &self.adaptation_history
    }
}

/// Morphic adaptation errors
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MorphicError {
    ProposalNotFound,
    InvalidState,
    TooManyProposals,
    TooManyApplied,
    TestFailed,
}

impl std::fmt::Display for MorphicError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::ProposalNotFound => write!(f, "Proposal not found"),
            Self::InvalidState => write!(f, "Invalid adaptation state"),
            Self::TooManyProposals => write!(f, "Too many concurrent proposals"),
            Self::TooManyApplied => write!(f, "Too many applied adaptations"),
            Self::TestFailed => write!(f, "Adaptation test failed"),
        }
    }
}

impl std::error::Error for MorphicError {}
