/**
 * MEMSHADOW Protocol v3.0 - Morphic Adaptation Implementation (C)
 */

#include "memshadow_morphic.h"
#include "memshadow.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __linux__
#include <sys/random.h>
#endif

static void morphic_gen_id(char *out) {
    uint8_t bytes[MEMSHADOW_MORPHIC_ID_SIZE];
#ifdef __linux__
    getrandom(bytes, sizeof(bytes), 0);
#else
    for (int i = 0; i < MEMSHADOW_MORPHIC_ID_SIZE; i++) bytes[i] = (uint8_t)(rand() & 0xFF);
#endif
    for (int i = 0; i < MEMSHADOW_MORPHIC_ID_SIZE; i++)
        snprintf(out + i * 2, 3, "%02x", bytes[i]);
}

void memshadow_morphic_init(memshadow_morphic_manager_t *mgr, const char *node_id) {
    memset(mgr, 0, sizeof(*mgr));
    strncpy(mgr->node_id, node_id, sizeof(mgr->node_id) - 1);
}

int memshadow_morphic_propose(memshadow_morphic_manager_t *mgr,
                               memshadow_adaptation_type_t type,
                               const char *description,
                               char *out_proposal_id) {
    if (!mgr || !description) return -1;
    if (mgr->proposal_count >= MEMSHADOW_MORPHIC_MAX_PROPOSALS) return -2;

    memshadow_morphic_proposal_t *p = &mgr->proposals[mgr->proposal_count];
    memset(p, 0, sizeof(*p));

    morphic_gen_id(p->id);
    strncpy(p->proposer_id, mgr->node_id, sizeof(p->proposer_id) - 1);
    p->type = type;
    strncpy(p->description, description, sizeof(p->description) - 1);
    p->status = ADAPT_VOTING;
    p->created_at_ns = memshadow_get_timestamp_ns();
    p->voting_deadline_ns = p->created_at_ns + 300ULL * 1000000000ULL; /* 5 minutes */
    p->test_duration_ms = 60000;
    p->required_majority = 0.67;

    /* Self-vote accept */
    strncpy(p->votes[0].voter_id, mgr->node_id, sizeof(p->votes[0].voter_id) - 1);
    p->votes[0].vote = VOTE_ACCEPT;
    p->vote_count = 1;

    if (out_proposal_id) strcpy(out_proposal_id, p->id);

    mgr->proposal_count++;
    return 0;
}

int memshadow_morphic_vote(memshadow_morphic_manager_t *mgr,
                            const char *proposal_id,
                            const char *voter_id,
                            memshadow_vote_type_t vote) {
    if (!mgr || !proposal_id || !voter_id) return -1;

    for (uint8_t i = 0; i < mgr->proposal_count; i++) {
        if (strcmp(mgr->proposals[i].id, proposal_id) == 0) {
            memshadow_morphic_proposal_t *p = &mgr->proposals[i];
            if (p->status != ADAPT_VOTING) return -2;
            if (p->vote_count >= MEMSHADOW_MORPHIC_MAX_VOTERS) return -3;

            /* Check if already voted */
            for (uint16_t v = 0; v < p->vote_count; v++) {
                if (strcmp(p->votes[v].voter_id, voter_id) == 0) {
                    p->votes[v].vote = vote; /* Update vote */
                    return 0;
                }
            }

            strncpy(p->votes[p->vote_count].voter_id, voter_id,
                    sizeof(p->votes[p->vote_count].voter_id) - 1);
            p->votes[p->vote_count].vote = vote;
            p->vote_count++;
            return 0;
        }
    }
    return -4; /* Not found */
}

bool memshadow_morphic_has_majority(const memshadow_morphic_proposal_t *proposal) {
    if (!proposal || proposal->vote_count == 0) return false;
    uint16_t accepts = 0, rejects = 0;
    for (uint16_t i = 0; i < proposal->vote_count; i++) {
        if (proposal->votes[i].vote == VOTE_ACCEPT) accepts++;
        else if (proposal->votes[i].vote == VOTE_REJECT) rejects++;
    }
    uint16_t total = accepts + rejects;
    if (total == 0) return false;
    return ((double)accepts / (double)total) >= proposal->required_majority;
}

bool memshadow_morphic_metrics_improved(const memshadow_adaptation_metrics_t *m) {
    if (!m) return false;
    bool lat_ok = m->latency_after_ms <= m->latency_before_ms * 1.1;
    bool tp_ok = m->throughput_after_bps >= (uint64_t)(m->throughput_before_bps * 0.9);
    bool err_ok = m->error_rate_after <= m->error_rate_before * 1.5;
    bool det_ok = m->detection_risk_after <= m->detection_risk_before;
    return lat_ok && tp_ok && err_ok && det_ok;
}

void memshadow_morphic_tick(memshadow_morphic_manager_t *mgr) {
    if (!mgr) return;
    uint64_t now = memshadow_get_timestamp_ns();

    for (uint8_t i = 0; i < mgr->proposal_count; i++) {
        memshadow_morphic_proposal_t *p = &mgr->proposals[i];

        if (p->status == ADAPT_VOTING && now > p->voting_deadline_ns) {
            if (memshadow_morphic_has_majority(p)) {
                p->status = ADAPT_TESTING;
                p->test_started_at_ns = now;
            } else {
                p->status = ADAPT_REJECTED;
            }
        }

        if (p->status == ADAPT_TESTING) {
            uint64_t test_elapsed_ms = (now - p->test_started_at_ns) / 1000000ULL;
            if (test_elapsed_ms >= p->test_duration_ms) {
                if (p->has_metrics) {
                    p->status = memshadow_morphic_metrics_improved(&p->metrics) ?
                                ADAPT_ACCEPTED : ADAPT_ROLLED_BACK;
                } else {
                    p->status = ADAPT_ACCEPTED;
                }
            }
        }
    }
}

int memshadow_morphic_apply(memshadow_morphic_manager_t *mgr, const char *proposal_id) {
    if (!mgr || !proposal_id) return -1;

    for (uint8_t i = 0; i < mgr->proposal_count; i++) {
        if (strcmp(mgr->proposals[i].id, proposal_id) == 0) {
            if (mgr->proposals[i].status != ADAPT_ACCEPTED) return -2;
            if (mgr->applied_count >= MEMSHADOW_MORPHIC_MAX_APPLIED) return -3;

            mgr->proposals[i].status = ADAPT_APPLIED;
            strcpy(mgr->applied_ids[mgr->applied_count], proposal_id);
            mgr->applied_count++;

            if (mgr->history_count < MEMSHADOW_MORPHIC_MAX_APPLIED) {
                memshadow_morphic_record_t *rec = &mgr->history[mgr->history_count++];
                strcpy(rec->proposal_id, proposal_id);
                rec->type = mgr->proposals[i].type;
                rec->status = ADAPT_APPLIED;
                rec->applied_at_ns = memshadow_get_timestamp_ns();
            }
            return 0;
        }
    }
    return -4;
}

int memshadow_morphic_rollback(memshadow_morphic_manager_t *mgr, const char *proposal_id) {
    if (!mgr || !proposal_id) return -1;

    for (uint8_t i = 0; i < mgr->proposal_count; i++) {
        if (strcmp(mgr->proposals[i].id, proposal_id) == 0) {
            if (mgr->proposals[i].status != ADAPT_APPLIED) return -2;
            mgr->proposals[i].status = ADAPT_ROLLED_BACK;

            /* Remove from applied list */
            for (uint8_t a = 0; a < mgr->applied_count; a++) {
                if (strcmp(mgr->applied_ids[a], proposal_id) == 0) {
                    memmove(&mgr->applied_ids[a], &mgr->applied_ids[a + 1],
                            (mgr->applied_count - a - 1) * sizeof(mgr->applied_ids[0]));
                    mgr->applied_count--;
                    break;
                }
            }

            /* Update history */
            for (uint8_t h = 0; h < mgr->history_count; h++) {
                if (strcmp(mgr->history[h].proposal_id, proposal_id) == 0) {
                    mgr->history[h].status = ADAPT_ROLLED_BACK;
                    mgr->history[h].rolled_back_at_ns = memshadow_get_timestamp_ns();
                    break;
                }
            }
            return 0;
        }
    }
    return -4;
}

const memshadow_morphic_proposal_t *memshadow_morphic_get_proposal(
    const memshadow_morphic_manager_t *mgr, const char *proposal_id) {
    if (!mgr || !proposal_id) return NULL;
    for (uint8_t i = 0; i < mgr->proposal_count; i++) {
        if (strcmp(mgr->proposals[i].id, proposal_id) == 0) return &mgr->proposals[i];
    }
    return NULL;
}

uint8_t memshadow_morphic_active_count(const memshadow_morphic_manager_t *mgr) {
    if (!mgr) return 0;
    uint8_t count = 0;
    for (uint8_t i = 0; i < mgr->proposal_count; i++) {
        if (mgr->proposals[i].status == ADAPT_VOTING || mgr->proposals[i].status == ADAPT_TESTING)
            count++;
    }
    return count;
}

uint8_t memshadow_morphic_applied_count(const memshadow_morphic_manager_t *mgr) {
    return mgr ? mgr->applied_count : 0;
}
