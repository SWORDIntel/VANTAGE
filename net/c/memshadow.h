/**
 * MEMSHADOW Protocol v3.0 - C Header
 * 
 * Binary protocol implementation for C.
 * All values use big-endian (network byte order).
 */

#ifndef MEMSHADOW_H
#define MEMSHADOW_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Protocol Constants */
#define MEMSHADOW_MAGIC 0x4D53485700000000ULL  // "MSHW" padded to 8 bytes
#define MEMSHADOW_VERSION_MAJOR 3
#define MEMSHADOW_VERSION_MINOR 0
#define MEMSHADOW_HEADER_SIZE 32
#define HEADER_SIZE MEMSHADOW_HEADER_SIZE
#define MEMSHADOW_MAX_PAYLOAD_NORMAL (16 * 1024 * 1024)      // 16MB
#define MEMSHADOW_MAX_PAYLOAD_FILE_TRANSFER (24 * 1024 * 1024) // 24MB

/* Message Types */
typedef enum {
    /* System/Control (0x00xx) */
    MSG_HEARTBEAT = 0x0001,
    MSG_ACK = 0x0002,
    MSG_NACK = 0x0003,
    MSG_ERROR = 0x0003,  /* Alias for NACK */
    MSG_HANDSHAKE = 0x0004,
    MSG_DISCONNECT = 0x0005,
    
    /* SHRINK Psychological Intelligence (0x01xx) */
    MSG_PSYCH_ASSESSMENT = 0x0100,
    MSG_DARK_TRIAD_UPDATE = 0x0101,
    MSG_RISK_UPDATE = 0x0102,
    MSG_NEURO_UPDATE = 0x0103,
    MSG_TMI_UPDATE = 0x0104,
    MSG_COGARCH_UPDATE = 0x0105,
    MSG_COGNITIVE_UPDATE = 0x0105,  /* Alias */
    MSG_FULL_PSYCH = 0x0106,
    MSG_PSYCH_THREAT_ALERT = 0x0110,
    MSG_PSYCH_ANOMALY = 0x0111,
    MSG_PSYCH_RISK_THRESHOLD = 0x0112,
    
    /* Threat Intelligence (0x02xx) */
    MSG_THREAT_REPORT = 0x0201,
    MSG_INTEL_REPORT = 0x0202,
    MSG_KNOWLEDGE_UPDATE = 0x0203,
    MSG_BRAIN_INTEL_REPORT = 0x0204,
    MSG_INTEL_PROPAGATE = 0x0205,
    
    /* Memory Operations (0x03xx) */
    MSG_MEMORY_STORE = 0x0301,
    MSG_MEMORY_QUERY = 0x0302,
    MSG_MEMORY_RESPONSE = 0x0303,
    MSG_MEMORY_SYNC = 0x0304,
    MSG_VECTOR_SYNC = 0x0305,
    
    /* Federation/Mesh (0x04xx) */
    MSG_NODE_REGISTER = 0x0401,
    MSG_NODE_DEREGISTER = 0x0402,
    MSG_QUERY_DISTRIBUTE = 0x0403,
    MSG_BRAIN_QUERY = 0x0403,  /* Alias */
    MSG_QUERY_RESPONSE = 0x0404,
    
    /* Self-Improvement (0x05xx) */
    MSG_IMPROVEMENT_ANNOUNCE = 0x0501,
    MSG_IMPROVEMENT_REQUEST = 0x0502,
    MSG_IMPROVEMENT_PAYLOAD = 0x0503,
    MSG_IMPROVEMENT_ACK = 0x0504,
    MSG_IMPROVEMENT_REJECT = 0x0505,
    MSG_IMPROVEMENT_METRICS = 0x0506,
    
    /* File Transfer Operations (0x06xx) */
    MSG_FILE_TRANSFER_START = 0x0601,
    MSG_FILE_TRANSFER_CHUNK = 0x0602,
    MSG_FILE_TRANSFER_END = 0x0603,
    MSG_FILE_TRANSFER_ABORT = 0x0604,
    MSG_FILE_TRANSFER_REQUEST = 0x0605,
    
    /* Telemetry/Control (0x07xx) */
    MSG_TELEMETRY = 0x0701,  /* Telemetry/metrics (uses 16-byte header) */
    MSG_CONTROL = 0x0702,    /* Control messages */
    MSG_HEADER_ONLY = 0x0703, /* Header-only messages */
    MSG_PAYLOAD_ONLY = 0x0704, /* Payload-only messages */
    MSG_DATA = 0x07FF,       /* Generic data message (for testing) */
    
    /* Temporal Message Queuing (0x08xx) */
    MSG_TEMPORAL_QUEUE = 0x0801,
    MSG_TEMPORAL_DELIVER = 0x0802,
    MSG_TEMPORAL_CANCEL = 0x0803,
    MSG_TEMPORAL_QUERY = 0x0804,
    
    /* Quantum Entanglement Routing (0x09xx) */
    MSG_QUANTUM_ENTANGLE = 0x0901,
    MSG_QUANTUM_DELIVER = 0x0902,
    MSG_QUANTUM_COLLAPSE = 0x0903,
    MSG_QUANTUM_STATE_SYNC = 0x0904,
    
    /* Morphic Protocol Adaptation (0x0Axx) */
    MSG_MORPHIC_PROPOSAL = 0x0A01,
    MSG_MORPHIC_VOTE = 0x0A02,
    MSG_MORPHIC_TEST = 0x0A03,
    MSG_MORPHIC_METRICS = 0x0A04,
    MSG_MORPHIC_ACCEPT = 0x0A05,
    MSG_MORPHIC_REJECT = 0x0A06,
    MSG_MORPHIC_RESEARCH = 0x0A07,
    
    /* Peer Management (0x0Bxx) */
    MSG_PEER_REGISTER = 0x0B01,
    MSG_PEER_LIST_REQUEST = 0x0B02,
    MSG_PEER_LIST_RESPONSE = 0x0B03,
    MSG_PEER_UPDATE = 0x0B04,
    MSG_PEER_DISCOVERY = 0x0B05,
    
    /* Relay System (0x0Cxx) */
    MSG_RELAY_REQUEST = 0x0C01,
    MSG_RELAY_DATA = 0x0C02,
    MSG_RELAY_ACK = 0x0C03,
    MSG_RELAY_ERROR = 0x0C04,

    /* DSOS Intelligence Operations (0xD0xx) */
    MSG_DSOS_INTELLIGENCE_COLLECTION = 0xD001,
    MSG_DSOS_INTELLIGENCE_PROCESSING = 0xD002,
    MSG_DSOS_INTELLIGENCE_DISSEMINATION = 0xD003,
    MSG_DSOS_INTELLIGENCE_ARCHIVAL = 0xD004,
    MSG_DSOS_INTELLIGENCE_QUERY = 0xD005,
    MSG_DSOS_INTELLIGENCE_REPORTING = 0xD006,
    MSG_DSOS_INTELLIGENCE_CORRELATION = 0xD007,
    MSG_DSOS_INTELLIGENCE_SYNTHESIS = 0xD008,

    /* AI Processing Operations (0xD3xx) */
    MSG_DSOS_AI_MODEL_DEPLOYMENT = 0xD301,
    MSG_DSOS_AI_INFERENCE_REQUEST = 0xD302,
    MSG_DSOS_AI_TRAINING_COORDINATION = 0xD303,
    MSG_DSOS_AI_MODEL_SYNCHRONIZATION = 0xD304,
    MSG_DSOS_AI_QUANTUM_PROCESSING = 0xD305,
    MSG_DSOS_AI_NEUROMORPHIC_EXECUTION = 0xD306,
    MSG_DSOS_AI_EXPLAINABILITY = 0xD307,
    MSG_DSOS_AI_ADVERSARIAL_TESTING = 0xD308,

    /* Advanced Routing Operations (0xD4xx) */
    MSG_DSOS_MOE_ROUTING = 0xD401,
    MSG_DSOS_TOPOLOGY_AWARE_ROUTING = 0xD402,
    MSG_DSOS_LOAD_BALANCING = 0xD403,
    MSG_DSOS_PRIORITY_BASED_ROUTING = 0xD404,
    MSG_DSOS_RESILIENCE_ROUTING = 0xD405,
    MSG_DSOS_COVERT_ROUTING = 0xD406,
    MSG_DSOS_MULTICAST_INTELLIGENCE = 0xD407,
    MSG_DSOS_BROADCAST_ALERT = 0xD408,

    /* Security and Cryptographic Operations (0xD5xx) */
    MSG_DSOS_PQC_KEY_EXCHANGE = 0xD501,
    MSG_DSOS_STEGANOGRAPHY_ENCODING = 0xD502,
    MSG_DSOS_COVERT_CHANNEL_ESTABLISHMENT = 0xD503,
    MSG_DSOS_TAMPER_DETECTION = 0xD504,
    MSG_DSOS_ZERO_KNOWLEDGE_PROOF = 0xD505,
    MSG_DSOS_QUANTUM_KEY_DISTRIBUTION = 0xD506,
    MSG_DSOS_HOMOMORPHIC_ENCRYPTION = 0xD507,
    MSG_DSOS_SECURE_MULTIPARTY_COMPUTATION = 0xD508,

    /* Orchestration and Coordination (0xD6xx) */
    MSG_DSOS_SPOE_INTELLIGENCE_ORCHESTRATION = 0xD601,
    MSG_DSOS_WORKFLOW_EXECUTION = 0xD602,
    MSG_DSOS_RESOURCE_ALLOCATION = 0xD603,
    MSG_DSOS_PERFORMANCE_MONITORING = 0xD604,
    MSG_DSOS_FAILURE_RECOVERY = 0xD605,
    MSG_DSOS_LOAD_SHEDDING = 0xD606,
    MSG_DSOS_MAINTENANCE_COORDINATION = 0xD607,
    MSG_DSOS_AUDIT_LOG_SYNCHRONIZATION = 0xD608,
} memshadow_msg_type_t;

/* Message Flags */
typedef enum {
    /* Core flags (packed into 1 byte in header) */
    FLAG_NONE = 0x0000,
    FLAG_ENCRYPTED = 0x0001,
    FLAG_COMPRESSED = 0x0002,
    FLAG_BATCHED = 0x0004,
    FLAG_REQUIRES_ACK = 0x0008,
    FLAG_FRAGMENTED = 0x0010,
    FLAG_LAST_FRAGMENT = 0x0020,
    FLAG_FROM_KERNEL = 0x0040,
    FLAG_HIGH_CONFIDENCE = 0x0080,
    
    /* Extended flags (require payload extension header) */
    FLAG_HAS_EXTENSION = 0x0100,      /* Payload contains extension header */
    FLAG_ECC_ENABLED = 0x0200,        /* Error correction code enabled */
    FLAG_DUAL_STREAM = 0x0400,        /* Header/payload separated into alternate streams */
    FLAG_TELEMETRY_MODE = 0x0800,     /* Ultra-lightweight telemetry mode (minimal header) */
    FLAG_INTEGRITY_CHECK = 0x1000,    /* Per-message HMAC integrity check (HMAC-SHA384) */
    FLAG_STEGANOGRAPHIC = 0x2000,     /* Payload is steganographically hidden */
    FLAG_TRAFFIC_SHAPED = 0x4000,     /* Traffic shaped to mimic other protocols */
    FLAG_QUANTUM_RESISTANT = 0x4000,  /* Quantum-resistant encryption enabled */
    FLAG_TEMPORAL_QUEUED = 0x8000,    /* Message is temporally queued (future/past delivery) */
    FLAG_QUANTUM_ENTANGLED = 0x10000, /* Message uses quantum entanglement routing */
    FLAG_MORPHIC_ADAPTED = 0x20000,   /* Message uses morphed protocol structure */
} memshadow_flags_t;

/* Priority Levels */
typedef enum {
    PRIORITY_LOW = 0,
    PRIORITY_NORMAL = 1,
    PRIORITY_HIGH = 2,
    PRIORITY_CRITICAL = 3,
    PRIORITY_EMERGENCY = 4,
    PRIORITY_LOWEST = PRIORITY_LOW,
} memshadow_priority_t;

/* Error Codes */
typedef enum {
    ERR_INVALID_MAGIC = 0x0001,
    ERR_INVALID_VERSION = 0x0002,
    ERR_INVALID_HEADER = 0x0003,
    ERR_INVALID_PAYLOAD = 0x0004,
    ERR_INVALID_MESSAGE_TYPE = 0x0005,
    ERR_MESSAGE_TOO_LARGE = 0x0008,
    ERR_INVALID_SIGNATURE = 0x0101,
    ERR_AUTHENTICATION_FAILED = 0x0102,
    ERR_KEY_EXCHANGE_FAILED = 0x0106,
    ERR_PEER_NOT_FOUND = 0x0201,
    ERR_CONNECTION_FAILED = 0x0203,
    ERR_NAT_TRAVERSAL_FAILED = 0x0205,
    ERR_HANDSHAKE_TIMEOUT = 0x0301,
    ERR_VERSION_INCOMPATIBLE = 0x0306,
    ERR_RATE_LIMIT_EXCEEDED = 0x0401,
    ERR_FILE_TRANSFER_FAILED = 0x0601,
    ERR_COMPRESSION_FAILED = 0x0608,
    ERR_HARDWARE_UNAVAILABLE = 0x060A,
    ERR_INTERNAL_ERROR = 0x0A01,
} memshadow_error_code_t;

/* Protocol Header Structure */
typedef struct {
    uint64_t magic;          // 8 bytes - Protocol magic
    uint16_t version;        // 2 bytes - Protocol version
    uint16_t priority;      // 2 bytes - Message priority
    uint16_t msg_type;       // 2 bytes - Message type
    uint16_t flags_batch;    // 2 bytes - Flags (low) + batch_count (high)
    uint32_t payload_len;    // 4 bytes - Payload length
    uint64_t timestamp_ns;    // 8 bytes - Nanosecond timestamp
    uint32_t sequence_num;   // 4 bytes - Sequence number
} memshadow_header_t;

/* Compression Types */
typedef enum {
    COMPRESS_NONE = 0,
    COMPRESS_GZIP = 1,
    COMPRESS_ZLIB = 2,
    COMPRESS_KANZI = 3,
    COMPRESS_QATZIP = 4,
} memshadow_compression_t;

/* Compression Type Aliases (for compatibility) */
#define COMPRESSION_NONE COMPRESS_NONE
#define COMPRESSION_GZIP COMPRESS_GZIP
#define COMPRESSION_KANZI COMPRESS_KANZI
#define COMPRESSION_QATZIP COMPRESS_QATZIP

/* Version Structure */
typedef struct {
    uint8_t major;
    uint8_t minor;
} memshadow_version_t;

/* Function: Pack header to binary (big-endian) */
int memshadow_pack_header(const memshadow_header_t *header, uint8_t *buffer, size_t buffer_size);

/* Function: Unpack header from binary (big-endian) */
int memshadow_unpack_header(const uint8_t *buffer, size_t buffer_size, memshadow_header_t *header);

/* Function: Validate header */
bool memshadow_validate_header(const memshadow_header_t *header);

/* Function: Create error message */
int memshadow_create_error_message(
    memshadow_error_code_t error_code,
    const char *details,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *message_size
);

/* Function: Parse error message */
int memshadow_parse_error_message(
    const uint8_t *buffer,
    size_t buffer_size,
    memshadow_error_code_t *error_code,
    char *details_buffer,
    size_t details_buffer_size
);

/* Function: Get error message string */
const char *memshadow_get_error_message(memshadow_error_code_t error_code);

/* Function: Check version compatibility */
typedef enum {
    VERSION_COMPATIBLE = 0,
    VERSION_INCOMPATIBLE_MAJOR = 1,
    VERSION_BACKWARD_COMPATIBLE = 2,
    VERSION_UPGRADE_REQUIRED = 3,
} memshadow_version_compat_t;

memshadow_version_compat_t memshadow_check_version_compatibility(
    const memshadow_version_t *local_version,
    const memshadow_version_t *peer_version
);

/* Function: Endianness conversion helpers */
uint16_t memshadow_htobe16(uint16_t host_value);
uint32_t memshadow_htobe32(uint32_t host_value);
uint64_t memshadow_htobe64(uint64_t host_value);
uint16_t memshadow_be16toh(uint16_t be_value);
uint32_t memshadow_be32toh(uint32_t be_value);
uint64_t memshadow_be64toh(uint64_t be_value);

/* Additional endianness functions for compatibility */
uint32_t memshadow_pack_uint32(uint32_t value, uint8_t *buffer);
uint32_t memshadow_unpack_uint32(const uint8_t *buffer);

/* Function: Compression detection */
bool memshadow_detect_kanzi(void);
bool memshadow_detect_qatzip(void);
memshadow_compression_t memshadow_get_preferred_compression(void);

/* Function: Compress data */
int memshadow_compress(
    const uint8_t *input,
    size_t input_size,
    uint8_t *output,
    size_t output_size,
    size_t *compressed_size,
    memshadow_compression_t compression_type
);

/* Function: Decompress data */
int memshadow_decompress(
    const uint8_t *input,
    size_t input_size,
    uint8_t *output,
    size_t output_size,
    size_t *decompressed_size,
    memshadow_compression_t compression_type
);

/* Function: Get current timestamp (nanoseconds) */
uint64_t memshadow_get_timestamp_ns(void);

/* Function: Get next sequence number */
uint32_t memshadow_get_next_sequence(uint32_t *sequence_counter);

/* Function: Compute HMAC-SHA384 for message integrity */
int memshadow_compute_hmac(
    const uint8_t *data,
    size_t data_size,
    const uint8_t *key,
    size_t key_size,
    uint8_t *hmac_output,
    size_t hmac_output_size
);

/* Function: Verify HMAC-SHA384 for message integrity */
int memshadow_verify_hmac(
    const uint8_t *data,
    size_t data_size,
    const uint8_t *key,
    size_t key_size,
    const uint8_t *received_hmac,
    size_t received_hmac_size
);

/* Function: Pack message with optional HMAC integrity check */
int memshadow_pack_message(
    const memshadow_header_t *header,
    const uint8_t *payload,
    size_t payload_size,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *message_size,
    const uint8_t *integrity_key,  /* Optional: NULL to disable HMAC */
    size_t integrity_key_size
);

/* Function: Unpack message with optional HMAC integrity verification */
int memshadow_unpack_message(
    const uint8_t *buffer,
    size_t buffer_size,
    memshadow_header_t *header,
    uint8_t *payload_buffer,
    size_t payload_buffer_size,
    size_t *payload_size,
    const uint8_t *integrity_key,  /* Optional: NULL to skip verification */
    size_t integrity_key_size
);

/* TLS Extension Support */
#include "memshadow_tls.h"

/* PQC Support */
#include "memshadow_pqc.h"

/* VLAN Topology Support */
#include "memshadow_vlan_topology.h"

/* DPI Evasion Support */
#include "memshadow_dpi_evasion.h"

/* Traffic Analysis Resistance Support */
#include "memshadow_traffic_analysis_resistance.h"

/* Covert VLAN Support */
#include "memshadow_covert_vlan.h"

/* Gossip Protocol Support */
#include "memshadow_gossip.h"

#ifdef __cplusplus
}
#endif

#endif /* MEMSHADOW_H */
