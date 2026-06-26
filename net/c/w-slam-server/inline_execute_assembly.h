/*
 * InlineExecute-Assembly Header
 * 
 * In-process .NET assembly execution
 */

#ifndef INLINE_EXECUTE_ASSEMBLY_H
#define INLINE_EXECUTE_ASSEMBLY_H

#include <windows.h>
#include <stdbool.h>

// Execution result
typedef struct {
    bool success;
    char output[8192];
    size_t output_len;
    int exit_code;
} InlineExecuteResult;

// Execution configuration
typedef struct {
    bool disable_amsi;
    bool disable_etw;
    bool revert_etw;
    char appdomain_name[64];
    char pipe_name[64];
    bool use_mailslot;
    bool use_main_no_args;
} InlineExecuteConfig;

/*
 * Execute .NET Assembly In-Process
 * 
 * Args:
 *   assembly_data: .NET assembly bytes
 *   assembly_len: Assembly size
 *   args: Command-line arguments (space-separated)
 *   config: Execution configuration (can be NULL for defaults)
 *   result: Execution result
 * 
 * Returns:
 *   true on success, false on failure
 */
bool inline_execute_assembly(const uint8_t *assembly_data, size_t assembly_len,
                            const char *args,
                            const InlineExecuteConfig *config,
                            InlineExecuteResult *result);

/*
 * Execute .NET Assembly from File
 * 
 * Args:
 *   assembly_path: Path to .NET assembly file
 *   args: Command-line arguments
 *   config: Execution configuration (can be NULL)
 *   result: Execution result
 * 
 * Returns:
 *   true on success, false on failure
 */
bool inline_execute_assembly_from_file(const char *assembly_path,
                                      const char *args,
                                      const InlineExecuteConfig *config,
                                      InlineExecuteResult *result);

#endif // INLINE_EXECUTE_ASSEMBLY_H
