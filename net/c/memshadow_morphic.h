/**
 * MEMSHADOW Protocol v3.0 - Morphic Adaptation (C)
 *
 * Self-adapting protocol structure. Peers propose, vote on,
 * and apply protocol modifications at runtime.
 */

#ifndef MEMSHADOW_MORPHIC_H
#define MEMSHADOW_MORPHIC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MEMSHADOW_MORPHIC_MAX_PROPOSALS  16
#define MEMSHADOW_MORPHIC_MAX_VOTERS     64
#define MEMSHADOW_MORPHIC_MAX_APPLIED    32
#define MEMSHADOW_MORPHIC_ID_SIZE        16

typedef enum {
    ADAPT_HEADER_REORDER = 0,
    ADAPT_FIELD_RESIZE = 1,
    ADAPT_NEW_FIELD = 2,
    ADAPT_REMOVE_FIELD = 3,
    ADAPT_COMPRESSION_CHANGE = 4,
    ADAPT_ENCRYPTION_CHANGE = 5,
    ADAPT_TIMING_CHANGE = 6,
    ADAPT_PADDING_CHANGE = 7,
} memshadow_adaptation_type_t;

typedef enum {
    ADAPT_PROPOSED = 0,
    ADAPT_VOTING = 1,
    ADAPT_TESTING = 2,
    ADAPT_ACCEPTED = 3,
    ADAPT_REJECTED = 4,
    ADAPT_APPLIED = 5,
    ADAPT_ROLLED_BACK = 6,
} memshadow_adaptation_status_t;

typedef enum {
    VOTE_ACCEPT = 0,
    VOTE_REJECT = 1,
    VOTE_ABSTAIN = 2,
} memshadow_vote_type_t;

typedef struct {
    char voter_id[64];
    memshadow_vote_type_t vote;
} memshadow_vote_entry_t;

typedef struct {
    double latency_before_ms;
    double latency_after_ms;
    uint64_t throughput_before_bps;
    uint64_t throughput_after_bps;
    double error_rate_before;
    double error_rate_after;
    double detection_risk_before;
    double detection_risk_after;
} memshadow_adaptation_metrics_t;

typedef struct {
    char     id[MEMSHADOW_MORPHIC_ID_SIZE * 2 + 1];
    char     proposer_id[64];
    memshadow_adaptation_type_t type;
    char     description[256];
    memshadow_adaptation_status_t status;
    memshadow_vote_entry_t votes[MEMSHADOW_MORPHIC_MAX_VOTERS];
    uint16_t vote_count;
    uint64_t created_at_ns;
    uint64_t voting_deadline_ns;
    uint64_t test_duration_ms;
    uint64_t test_started_at_ns;
    memshadow_adaptation_metrics_t metrics;
    bool     has_metrics;
    double   required_majority;
} memshadow_morphic_proposal_t;

typedef struct {
    char     proposal_id[MEMSHADOW_MORPHIC_ID_SIZE * 2 + 1];
    memshadow_adaptation_type_t type;
    memshadow_adaptation_status_t status;
    uint64_t applied_at_ns;
    uint64_t rolled_back_at_ns;
} memshadow_morphic_record_t;

typedef struct {
    char     node_id[64];
    memshadow_morphic_proposal_t proposals[MEMSHADOW_MORPHIC_MAX_PROPOSALS];
    uint8_t  proposal_count;
    memshadow_morphic_record_t   history[MEMSHADOW_MORPHIC_MAX_APPLIED];
    uint8_t  history_count;
    char     applied_ids[MEMSHADOW_MORPHIC_MAX_APPLIED][MEMSHADOW_MORPHIC_ID_SIZE * 2 + 1];
    uint8_t  applied_count;
} memshadow_morphic_manager_t;

void memshadow_morphic_init(memshadow_morphic_manager_t *mgr, const char *node_id);

int memshadow_morphic_propose(memshadow_morphic_manager_t *mgr,
                               memshadow_adaptation_type_t type,
                               const char *description,
                               char *out_proposal_id);

int memshadow_morphic_vote(memshadow_morphic_manager_t *mgr,
                            const char *proposal_id,
                            const char *voter_id,
                            memshadow_vote_type_t vote);

void memshadow_morphic_tick(memshadow_morphic_manager_t *mgr);

int memshadow_morphic_apply(memshadow_morphic_manager_t *mgr, const char *proposal_id);
int memshadow_morphic_rollback(memshadow_morphic_manager_t *mgr, const char *proposal_id);

const memshadow_morphic_proposal_t *memshadow_morphic_get_proposal(
    const memshadow_morphic_manager_t *mgr, const char *proposal_id);

bool memshadow_morphic_has_majority(const memshadow_morphic_proposal_t *proposal);
bool memshadow_morphic_metrics_improved(const memshadow_adaptation_metrics_t *metrics);

uint8_t memshadow_morphic_active_count(const memshadow_morphic_manager_t *mgr);
uint8_t memshadow_morphic_applied_count(const memshadow_morphic_manager_t *mgr);

#ifdef __cplusplus
}
#endif

#endif /* MEMSHADOW_MORPHIC_H */
