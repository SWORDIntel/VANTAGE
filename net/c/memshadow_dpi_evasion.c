/**
 * MEMSHADOW Protocol - DPI Evasion Implementation
 * 
 * Implements Deep Packet Inspection evasion techniques.
 * Supports protocol mimicry, traffic shaping, and steganography.
 */

#include "memshadow_dpi_evasion.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* Default packet sizes for normalization */
static const size_t DEFAULT_PACKET_SIZES[] = {64, 128, 256, 512, 1024, 1280, 1500};

/* Common HTTP hosts */
static const char *COMMON_HOSTS[] = {
    "www.google.com",
    "www.cloudflare.com",
    "www.microsoft.com",
    "www.amazonaws.com",
    "cdn.jsdelivr.net",
    NULL
};

/* Common HTTP paths */
static const char *COMMON_PATHS[] = {
    "/api/v1/",
    "/static/js/",
    "/css/bootstrap.min.css",
    "/favicon.ico",
    "/robots.txt",
    NULL
};

/* Common User-Agents */
static const char *USER_AGENTS[] = {
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36",
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36",
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36",
    NULL
};

const size_t *memshadow_dpi_evasion_get_default_packet_sizes(size_t *count) {
    if (count) {
        *count = sizeof(DEFAULT_PACKET_SIZES) / sizeof(DEFAULT_PACKET_SIZES[0]);
    }
    return DEFAULT_PACKET_SIZES;
}

/* Forward declarations for protocol-specific functions */
static int wrap_as_http(const uint8_t *payload, size_t payload_size, uint8_t **wrapped_payload, size_t *wrapped_size);
static int wrap_as_https(const uint8_t *payload, size_t payload_size, uint8_t **wrapped_payload, size_t *wrapped_size);
static int wrap_as_dns(const uint8_t *payload, size_t payload_size, uint8_t **wrapped_payload, size_t *wrapped_size);
static int wrap_as_ntp(const uint8_t *payload, size_t payload_size, uint8_t **wrapped_payload, size_t *wrapped_size);
static int wrap_as_icmp(const uint8_t *payload, size_t payload_size, uint8_t **wrapped_payload, size_t *wrapped_size);
static int wrap_as_smtp(const uint8_t *payload, size_t payload_size, uint8_t **wrapped_payload, size_t *wrapped_size);
static int wrap_as_ftp(const uint8_t *payload, size_t payload_size, uint8_t **wrapped_payload, size_t *wrapped_size);
static int wrap_as_ssh(const uint8_t *payload, size_t payload_size, uint8_t **wrapped_payload, size_t *wrapped_size);
static int wrap_as_custom(const uint8_t *payload, size_t payload_size, uint8_t **wrapped_payload, size_t *wrapped_size);

static int unwrap_from_http(const uint8_t *wrapped_payload, size_t wrapped_size, uint8_t **payload, size_t *payload_size);
static int unwrap_from_https(const uint8_t *wrapped_payload, size_t wrapped_size, uint8_t **payload, size_t *payload_size);
static int unwrap_from_dns(const uint8_t *wrapped_payload, size_t wrapped_size, uint8_t **payload, size_t *payload_size);
static int unwrap_from_ntp(const uint8_t *wrapped_payload, size_t wrapped_size, uint8_t **payload, size_t *payload_size);
static int unwrap_from_icmp(const uint8_t *wrapped_payload, size_t wrapped_size, uint8_t **payload, size_t *payload_size);
static int unwrap_from_smtp(const uint8_t *wrapped_payload, size_t wrapped_size, uint8_t **payload, size_t *payload_size);
static int unwrap_from_ftp(const uint8_t *wrapped_payload, size_t wrapped_size, uint8_t **payload, size_t *payload_size);
static int unwrap_from_ssh(const uint8_t *wrapped_payload, size_t wrapped_size, uint8_t **payload, size_t *payload_size);
static int unwrap_from_custom(const uint8_t *wrapped_payload, size_t wrapped_size, uint8_t **payload, size_t *payload_size);

static bool looks_like_http(const uint8_t *payload, size_t payload_size);
static bool looks_like_https(const uint8_t *payload, size_t payload_size);
static bool looks_like_dns(const uint8_t *payload, size_t payload_size);
static bool looks_like_ntp(const uint8_t *payload, size_t payload_size);
static bool looks_like_icmp(const uint8_t *payload, size_t payload_size);
static bool looks_like_smtp(const uint8_t *payload, size_t payload_size);
static bool looks_like_ftp(const uint8_t *payload, size_t payload_size);
static bool looks_like_ssh(const uint8_t *payload, size_t payload_size);

/* DPI Evasion Manager Functions */
int memshadow_dpi_evasion_manager_init(
    memshadow_dpi_evasion_manager_t **manager,
    memshadow_dpi_evasion_mode_t mode
) {
    if (!manager) {
        return -1;
    }

    *manager = calloc(1, sizeof(memshadow_dpi_evasion_manager_t));
    if (!*manager) {
        return -1;
    }

    (*manager)->mode = mode;
    (*manager)->internal_state = NULL;

    return 0;
}

void memshadow_dpi_evasion_manager_cleanup(
    memshadow_dpi_evasion_manager_t *manager
) {
    if (manager) {
        // Clean up internal state if needed
        free(manager->internal_state);
        free(manager);
    }
}

int memshadow_dpi_evasion_wrap_payload(
    memshadow_dpi_evasion_manager_t *manager,
    const uint8_t *payload,
    size_t payload_size,
    memshadow_dpi_protocol_t protocol,
    uint8_t **wrapped_payload,
    size_t *wrapped_size
) {
    if (!manager || !payload || !wrapped_payload || !wrapped_size) {
        return -1;
    }

    if (manager->mode == DPI_EVASION_NONE) {
        *wrapped_payload = malloc(payload_size);
        if (!*wrapped_payload) return -1;
        memcpy(*wrapped_payload, payload, payload_size);
        *wrapped_size = payload_size;
        return 0;
    }

    switch (protocol) {
        case DPI_PROTOCOL_HTTP:
            return wrap_as_http(payload, payload_size, wrapped_payload, wrapped_size);
        case DPI_PROTOCOL_HTTPS:
            return wrap_as_https(payload, payload_size, wrapped_payload, wrapped_size);
        case DPI_PROTOCOL_DNS:
            return wrap_as_dns(payload, payload_size, wrapped_payload, wrapped_size);
        case DPI_PROTOCOL_NTP:
            return wrap_as_ntp(payload, payload_size, wrapped_payload, wrapped_size);
        case DPI_PROTOCOL_ICMP:
            return wrap_as_icmp(payload, payload_size, wrapped_payload, wrapped_size);
        case DPI_PROTOCOL_SMTP:
            return wrap_as_smtp(payload, payload_size, wrapped_payload, wrapped_size);
        case DPI_PROTOCOL_FTP:
            return wrap_as_ftp(payload, payload_size, wrapped_payload, wrapped_size);
        case DPI_PROTOCOL_SSH:
            return wrap_as_ssh(payload, payload_size, wrapped_payload, wrapped_size);
        case DPI_PROTOCOL_CUSTOM:
            return wrap_as_custom(payload, payload_size, wrapped_payload, wrapped_size);
        default:
            return -1;
    }
}

int memshadow_dpi_evasion_unwrap_payload(
    memshadow_dpi_evasion_manager_t *manager,
    const uint8_t *wrapped_payload,
    size_t wrapped_size,
    memshadow_dpi_protocol_t protocol,
    uint8_t **payload,
    size_t *payload_size
) {
    if (!manager || !wrapped_payload || !payload || !payload_size) {
        return -1;
    }

    if (manager->mode == DPI_EVASION_NONE) {
        *payload = malloc(wrapped_size);
        if (!*payload) return -1;
        memcpy(*payload, wrapped_payload, wrapped_size);
        *payload_size = wrapped_size;
        return 0;
    }

    switch (protocol) {
        case DPI_PROTOCOL_HTTP:
            return unwrap_from_http(wrapped_payload, wrapped_size, payload, payload_size);
        case DPI_PROTOCOL_HTTPS:
            return unwrap_from_https(wrapped_payload, wrapped_size, payload, payload_size);
        case DPI_PROTOCOL_DNS:
            return unwrap_from_dns(wrapped_payload, wrapped_size, payload, payload_size);
        case DPI_PROTOCOL_NTP:
            return unwrap_from_ntp(wrapped_payload, wrapped_size, payload, payload_size);
        case DPI_PROTOCOL_ICMP:
            return unwrap_from_icmp(wrapped_payload, wrapped_size, payload, payload_size);
        case DPI_PROTOCOL_SMTP:
            return unwrap_from_smtp(wrapped_payload, wrapped_size, payload, payload_size);
        case DPI_PROTOCOL_FTP:
            return unwrap_from_ftp(wrapped_payload, wrapped_size, payload, payload_size);
        case DPI_PROTOCOL_SSH:
            return unwrap_from_ssh(wrapped_payload, wrapped_size, payload, payload_size);
        case DPI_PROTOCOL_CUSTOM:
            return unwrap_from_custom(wrapped_payload, wrapped_size, payload, payload_size);
        default:
            return -1;
    }
}

bool memshadow_dpi_evasion_looks_like_protocol(
    memshadow_dpi_evasion_manager_t *manager,
    const uint8_t *payload,
    size_t payload_size,
    memshadow_dpi_protocol_t protocol
) {
    if (!manager || !payload) {
        return false;
    }

    switch (protocol) {
        case DPI_PROTOCOL_HTTP:
            return looks_like_http(payload, payload_size);
        case DPI_PROTOCOL_HTTPS:
            return looks_like_https(payload, payload_size);
        case DPI_PROTOCOL_DNS:
            return looks_like_dns(payload, payload_size);
        case DPI_PROTOCOL_NTP:
            return looks_like_ntp(payload, payload_size);
        case DPI_PROTOCOL_ICMP:
            return looks_like_icmp(payload, payload_size);
        case DPI_PROTOCOL_SMTP:
            return looks_like_smtp(payload, payload_size);
        case DPI_PROTOCOL_FTP:
            return looks_like_ftp(payload, payload_size);
        case DPI_PROTOCOL_SSH:
            return looks_like_ssh(payload, payload_size);
        case DPI_PROTOCOL_CUSTOM:
            return true; // Custom always "looks like" itself
        default:
            return false;
    }
}

/* HTTP Protocol Mimicry */
static int wrap_as_http(
    const uint8_t *payload,
    size_t payload_size,
    uint8_t **wrapped_payload,
    size_t *wrapped_size
) {
    const char *http_template =
        "POST /api/data HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: %s\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n";

    // Select random host and user agent
    srand((unsigned int)time(NULL));
    const char *host = COMMON_HOSTS[rand() % 4]; // First 4 hosts
    const char *ua = USER_AGENTS[rand() % 2]; // First 2 user agents

    // Calculate header size
    size_t header_size = snprintf(NULL, 0, http_template, host, ua, payload_size) + 1;

    *wrapped_size = header_size + payload_size;
    *wrapped_payload = malloc(*wrapped_size);

    if (!*wrapped_payload) {
        return -1;
    }

    // Format header
    int written = snprintf((char*)*wrapped_payload, header_size, http_template, host, ua, payload_size);
    if (written < 0) {
        free(*wrapped_payload);
        return -1;
    }

    // Copy payload
    memcpy(*wrapped_payload + written, payload, payload_size);

    return 0;
}

static int unwrap_from_http(
    const uint8_t *wrapped_payload,
    size_t wrapped_size,
    uint8_t **payload,
    size_t *payload_size
) {
    // Look for double CRLF (end of headers)
    const char *crlf_crlf = "\r\n\r\n";
    const uint8_t *header_end = (const uint8_t*)strstr((const char*)wrapped_payload, crlf_crlf);

    if (!header_end) {
        return -1;
    }

    size_t header_size = (header_end - wrapped_payload) + 4; // +4 for CRLFCRLF

    if (header_size >= wrapped_size) {
        return -1;
    }

    *payload_size = wrapped_size - header_size;
    *payload = malloc(*payload_size);

    if (!*payload) {
        return -1;
    }

    memcpy(*payload, wrapped_payload + header_size, *payload_size);
    return 0;
}

static bool looks_like_http(const uint8_t *payload, size_t payload_size) {
    if (payload_size < 10) return false;

    const char *data = (const char*)payload;
    return strstr(data, "HTTP/") != NULL ||
           strstr(data, "GET ") != NULL ||
           strstr(data, "POST ") != NULL ||
           strstr(data, "Host:") != NULL;
}

/* HTTPS Protocol Mimicry */
static int wrap_as_https(
    const uint8_t *payload,
    size_t payload_size,
    uint8_t **wrapped_payload,
    size_t *wrapped_size
) {
    // TLS record header (5 bytes) + payload
    const size_t header_size = 5;
    *wrapped_size = header_size + payload_size;
    *wrapped_payload = malloc(*wrapped_size);

    if (!*wrapped_payload) {
        return -1;
    }

    // TLS record header for application data
    (*wrapped_payload)[0] = 23; // Content type: Application data
    (*wrapped_payload)[1] = 0x03; // TLS major version
    (*wrapped_payload)[2] = 0x03; // TLS minor version
    uint16_t length = (uint16_t)payload_size;
    (*wrapped_payload)[3] = (uint8_t)(length >> 8);   // Length high
    (*wrapped_payload)[4] = (uint8_t)(length & 0xFF); // Length low

    // Copy payload
    memcpy(*wrapped_payload + header_size, payload, payload_size);

    return 0;
}

static int unwrap_from_https(
    const uint8_t *wrapped_payload,
    size_t wrapped_size,
    uint8_t **payload,
    size_t *payload_size
) {
    const size_t header_size = 5;

    if (wrapped_size < header_size) {
        return -1;
    }

    // Extract length from TLS header
    uint16_t length = (uint16_t)((wrapped_payload[3] << 8) | wrapped_payload[4]);

    if (header_size + length > wrapped_size) {
        return -1;
    }

    *payload_size = length;
    *payload = malloc(*payload_size);

    if (!*payload) {
        return -1;
    }

    memcpy(*payload, wrapped_payload + header_size, *payload_size);
    return 0;
}

static bool looks_like_https(const uint8_t *payload, size_t payload_size) {
    if (payload_size < 5) return false;

    // Check for TLS handshake (0x16) or application data (0x17)
    return (payload[0] == 0x16 || payload[0] == 0x17) &&
           payload[1] == 0x03 && payload[2] == 0x03;
}

/* DNS Protocol Mimicry */
static int wrap_as_dns(
    const uint8_t *payload,
    size_t payload_size,
    uint8_t **wrapped_payload,
    size_t *wrapped_size
) {
    // DNS header (12 bytes) + encoded payload
    const size_t header_size = 12;
    *wrapped_size = header_size + payload_size;
    *wrapped_payload = malloc(*wrapped_size);

    if (!*wrapped_payload) {
        return -1;
    }

    // DNS header: transaction ID, flags, questions, etc.
    (*wrapped_payload)[0] = 0x12; // Transaction ID high
    (*wrapped_payload)[1] = 0x34; // Transaction ID low
    (*wrapped_payload)[2] = 0x01; // Flags (standard query)
    (*wrapped_payload)[3] = 0x00; // Flags
    (*wrapped_payload)[4] = 0x00; // Questions high
    (*wrapped_payload)[5] = 0x01; // Questions low (1 question)
    (*wrapped_payload)[6] = 0x00; // Answer RRs
    (*wrapped_payload)[7] = 0x00; // Answer RRs
    (*wrapped_payload)[8] = 0x00; // Authority RRs
    (*wrapped_payload)[9] = 0x00; // Authority RRs
    (*wrapped_payload)[10] = 0x00; // Additional RRs
    (*wrapped_payload)[11] = 0x00; // Additional RRs

    // Encode payload as DNS name (simplified)
    memcpy(*wrapped_payload + header_size, payload, payload_size);

    return 0;
}

static int unwrap_from_dns(
    const uint8_t *wrapped_payload,
    size_t wrapped_size,
    uint8_t **payload,
    size_t *payload_size
) {
    const size_t header_size = 12;

    if (wrapped_size < header_size) {
        return -1;
    }

    *payload_size = wrapped_size - header_size;
    *payload = malloc(*payload_size);

    if (!*payload) {
        return -1;
    }

    memcpy(*payload, wrapped_payload + header_size, *payload_size);
    return 0;
}

static bool looks_like_dns(const uint8_t *payload, size_t payload_size) {
    if (payload_size < 12) return false;

    // DNS query check: QR bit should be 0 (query), and reasonable question count
    return (payload[2] & 0x80) == 0 && payload[5] <= 10;
}

/* NTP Protocol Mimicry */
static int wrap_as_ntp(
    const uint8_t *payload,
    size_t payload_size,
    uint8_t **wrapped_payload,
    size_t *wrapped_size
) {
    // NTP packet (48 bytes minimum) + payload
    const size_t ntp_size = 48;
    size_t total_size = ntp_size + payload_size;

    if (total_size > 1500) { // MTU limit
        total_size = 1500;
        payload_size = total_size - ntp_size;
    }

    *wrapped_size = total_size;
    *wrapped_payload = malloc(*wrapped_size);

    if (!*wrapped_payload) {
        return -1;
    }

    // NTP header (simplified)
    memset(*wrapped_payload, 0, ntp_size);
    (*wrapped_payload)[0] = 0x1B; // LI=0, VN=3, Mode=3 (client)

    // Copy payload after NTP header
    memcpy(*wrapped_payload + ntp_size, payload, payload_size);

    return 0;
}

static int unwrap_from_ntp(
    const uint8_t *wrapped_payload,
    size_t wrapped_size,
    uint8_t **payload,
    size_t *payload_size
) {
    const size_t ntp_size = 48;

    if (wrapped_size < ntp_size) {
        return -1;
    }

    *payload_size = wrapped_size - ntp_size;
    *payload = malloc(*payload_size);

    if (!*payload) {
        return -1;
    }

    memcpy(*payload, wrapped_payload + ntp_size, *payload_size);
    return 0;
}

static bool looks_like_ntp(const uint8_t *payload, size_t payload_size) {
    if (payload_size < 48) return false;

    // NTP version check: VN field should be 3 or 4
    uint8_t li_vn_mode = payload[0];
    uint8_t vn = (li_vn_mode >> 3) & 0x07;
    return vn >= 3 && vn <= 4;
}

/* ICMP Protocol Mimicry */
static int wrap_as_icmp(
    const uint8_t *payload,
    size_t payload_size,
    uint8_t **wrapped_payload,
    size_t *wrapped_size
) {
    // ICMP echo request header (8 bytes) + payload
    const size_t icmp_header_size = 8;
    *wrapped_size = icmp_header_size + payload_size;
    *wrapped_payload = malloc(*wrapped_size);

    if (!*wrapped_payload) {
        return -1;
    }

    // ICMP echo request header
    (*wrapped_payload)[0] = 8; // Type: Echo request
    (*wrapped_payload)[1] = 0; // Code: 0
    // Checksum: calculated below
    (*wrapped_payload)[2] = 0;
    (*wrapped_payload)[3] = 0;
    (*wrapped_payload)[4] = 0; // Identifier high
    (*wrapped_payload)[5] = 1; // Identifier low
    (*wrapped_payload)[6] = 0; // Sequence high
    (*wrapped_payload)[7] = 1; // Sequence low

    // Copy payload
    memcpy(*wrapped_payload + icmp_header_size, payload, payload_size);

    // Calculate ICMP checksum (simplified)
    uint32_t checksum = 0;
    for (size_t i = 0; i < *wrapped_size; i += 2) {
        if (i + 1 < *wrapped_size) {
            checksum += ((*wrapped_payload)[i] << 8) | (*wrapped_payload)[i + 1];
        } else {
            checksum += (*wrapped_payload)[i] << 8;
        }
    }

    while (checksum >> 16) {
        checksum = (checksum & 0xFFFF) + (checksum >> 16);
    }

    (*wrapped_payload)[2] = (uint8_t)(~checksum & 0xFF);
    (*wrapped_payload)[3] = (uint8_t)((~checksum >> 8) & 0xFF);

    return 0;
}

static int unwrap_from_icmp(
    const uint8_t *wrapped_payload,
    size_t wrapped_size,
    uint8_t **payload,
    size_t *payload_size
) {
    const size_t icmp_header_size = 8;

    if (wrapped_size < icmp_header_size) {
        return -1;
    }

    *payload_size = wrapped_size - icmp_header_size;
    *payload = malloc(*payload_size);

    if (!*payload) {
        return -1;
    }

    memcpy(*payload, wrapped_payload + icmp_header_size, *payload_size);
    return 0;
}

static bool looks_like_icmp(const uint8_t *payload, size_t payload_size) {
    if (payload_size < 8) return false;

    // ICMP type check (echo request/response: 0 or 8)
    return payload[0] == 0 || payload[0] == 8;
}

/* SMTP Protocol Mimicry */
static int wrap_as_smtp(
    const uint8_t *payload,
    size_t payload_size,
    uint8_t **wrapped_payload,
    size_t *wrapped_size
) {
    const char *smtp_data = "DATA\r\n";
    const char *smtp_end = "\r\n.\r\n";

    size_t smtp_data_len = strlen(smtp_data);
    size_t smtp_end_len = strlen(smtp_end);

    *wrapped_size = smtp_data_len + payload_size + smtp_end_len;
    *wrapped_payload = malloc(*wrapped_size);

    if (!*wrapped_payload) {
        return -1;
    }

    memcpy(*wrapped_payload, smtp_data, smtp_data_len);
    memcpy(*wrapped_payload + smtp_data_len, payload, payload_size);
    memcpy(*wrapped_payload + smtp_data_len + payload_size, smtp_end, smtp_end_len);

    return 0;
}

static int unwrap_from_smtp(
    const uint8_t *wrapped_payload,
    size_t wrapped_size,
    uint8_t **payload,
    size_t *payload_size
) {
    const char *smtp_data = "DATA\r\n";
    const char *smtp_end = "\r\n.\r\n";

    size_t smtp_data_len = strlen(smtp_data);
    size_t smtp_end_len = strlen(smtp_end);

    if (wrapped_size < smtp_data_len + smtp_end_len) {
        return -1;
    }

    if (memcmp(wrapped_payload, smtp_data, smtp_data_len) != 0) {
        return -1;
    }

    size_t payload_end = wrapped_size - smtp_end_len;
    if (payload_end < smtp_data_len) {
        return -1;
    }

    if (memcmp(wrapped_payload + payload_end, smtp_end, smtp_end_len) != 0) {
        return -1;
    }

    *payload_size = payload_end - smtp_data_len;
    *payload = malloc(*payload_size);

    if (!*payload) {
        return -1;
    }

    memcpy(*payload, wrapped_payload + smtp_data_len, *payload_size);
    return 0;
}

static bool looks_like_smtp(const uint8_t *payload, size_t payload_size) {
    if (payload_size < 6) return false;

    const char *data = (const char*)payload;
    return strstr(data, "MAIL FROM:") != NULL ||
           strstr(data, "RCPT TO:") != NULL ||
           strstr(data, "DATA") != NULL;
}

/* FTP Protocol Mimicry */
static int wrap_as_ftp(
    const uint8_t *payload,
    size_t payload_size,
    uint8_t **wrapped_payload,
    size_t *wrapped_size
) {
    const char *ftp_cmd = "STOR secret_data.txt\r\n";
    size_t ftp_cmd_len = strlen(ftp_cmd);

    *wrapped_size = ftp_cmd_len + payload_size;
    *wrapped_payload = malloc(*wrapped_size);

    if (!*wrapped_payload) {
        return -1;
    }

    memcpy(*wrapped_payload, ftp_cmd, ftp_cmd_len);
    memcpy(*wrapped_payload + ftp_cmd_len, payload, payload_size);

    return 0;
}

static int unwrap_from_ftp(
    const uint8_t *wrapped_payload,
    size_t wrapped_size,
    uint8_t **payload,
    size_t *payload_size
) {
    const char *ftp_cmd = "STOR secret_data.txt\r\n";
    size_t ftp_cmd_len = strlen(ftp_cmd);

    if (wrapped_size < ftp_cmd_len) {
        return -1;
    }

    if (memcmp(wrapped_payload, ftp_cmd, ftp_cmd_len) != 0) {
        return -1;
    }

    *payload_size = wrapped_size - ftp_cmd_len;
    *payload = malloc(*payload_size);

    if (!*payload) {
        return -1;
    }

    memcpy(*payload, wrapped_payload + ftp_cmd_len, *payload_size);
    return 0;
}

static bool looks_like_ftp(const uint8_t *payload, size_t payload_size) {
    if (payload_size < 4) return false;

    const char *data = (const char*)payload;
    return strstr(data, "STOR ") != NULL ||
           strstr(data, "RETR ") != NULL ||
           strstr(data, "USER ") != NULL;
}

/* SSH Protocol Mimicry */
static int wrap_as_ssh(
    const uint8_t *payload,
    size_t payload_size,
    uint8_t **wrapped_payload,
    size_t *wrapped_size
) {
    // SSH binary packet format (simplified)
    // Length (4 bytes) + Padding length (1 byte) + Payload + Padding + MAC

    uint8_t padding_length = 8; // Simplified
    uint32_t packet_length = 1 + (uint32_t)payload_size + padding_length; // +1 for padding length byte

    *wrapped_size = 4 + packet_length; // Length + packet
    *wrapped_payload = malloc(*wrapped_size);

    if (!*wrapped_payload) {
        return -1;
    }

    // Packet length (big endian)
    (*wrapped_payload)[0] = (uint8_t)(packet_length >> 24);
    (*wrapped_payload)[1] = (uint8_t)(packet_length >> 16);
    (*wrapped_payload)[2] = (uint8_t)(packet_length >> 8);
    (*wrapped_payload)[3] = (uint8_t)packet_length;

    // Padding length
    (*wrapped_payload)[4] = padding_length;

    // Payload
    memcpy(*wrapped_payload + 5, payload, payload_size);

    // Padding (simplified)
    memset(*wrapped_payload + 5 + payload_size, 0, padding_length);

    return 0;
}

static int unwrap_from_ssh(
    const uint8_t *wrapped_payload,
    size_t wrapped_size,
    uint8_t **payload,
    size_t *payload_size
) {
    if (wrapped_size < 9) { // Minimum SSH packet size
        return -1;
    }

    // Read packet length
    uint32_t packet_length = ((uint32_t)wrapped_payload[0] << 24) |
                            ((uint32_t)wrapped_payload[1] << 16) |
                            ((uint32_t)wrapped_payload[2] << 8) |
                            (uint32_t)wrapped_payload[3];

    if (4 + packet_length > wrapped_size) {
        return -1;
    }

    uint8_t padding_length = wrapped_payload[4];
    *payload_size = packet_length - 1 - padding_length; // -1 for padding length byte
    *payload = malloc(*payload_size);

    if (!*payload) {
        return -1;
    }

    memcpy(*payload, wrapped_payload + 5, *payload_size);
    return 0;
}

static bool looks_like_ssh(const uint8_t *payload, size_t payload_size) {
    if (payload_size < 4) return false;

    // SSH packets start with packet length
    // Check if first byte has high bit set (typical for SSH)
    return (payload[0] & 0x80) != 0;
}

/* Custom Protocol Mimicry (XOR encoding) */
static int wrap_as_custom(
    const uint8_t *payload,
    size_t payload_size,
    uint8_t **wrapped_payload,
    size_t *wrapped_size
) {
    // Simple XOR encoding with fixed key
    const uint8_t key = 0xAA;

    *wrapped_size = payload_size;
    *wrapped_payload = malloc(*wrapped_size);

    if (!*wrapped_payload) {
        return -1;
    }

    for (size_t i = 0; i < payload_size; i++) {
        (*wrapped_payload)[i] = payload[i] ^ key;
    }

    return 0;
}

static int unwrap_from_custom(
    const uint8_t *wrapped_payload,
    size_t wrapped_size,
    uint8_t **payload,
    size_t *payload_size
) {
    // Reverse XOR encoding
    return wrap_as_custom(wrapped_payload, wrapped_size, payload, payload_size);
}

/* Additional utility functions */
int memshadow_dpi_evasion_generate_http_header(
    const char *method,
    const char *path,
    const char *host,
    const char *user_agent,
    uint8_t **header,
    size_t *header_size
) {
    if (!method || !path || !host || !user_agent || !header || !header_size) {
        return -1;
    }

    const char *template =
        "%s %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: %s\r\n"
        "Accept: */*\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";

    *header_size = snprintf(NULL, 0, template, method, path, host, user_agent) + 1;
    *header = malloc(*header_size);

    if (!*header) {
        return -1;
    }

    snprintf((char*)*header, *header_size, template, method, path, host, user_agent);
    return 0;
}

int memshadow_dpi_evasion_normalize_packet_size(
    const uint8_t *packet,
    size_t packet_size,
    size_t target_size,
    uint8_t **normalized_packet,
    size_t *normalized_size
) {
    if (!packet || !normalized_packet || !normalized_size) {
        return -1;
    }

    *normalized_size = target_size;
    *normalized_packet = malloc(*normalized_size);

    if (!*normalized_packet) {
        return -1;
    }

    if (packet_size < target_size) {
        // Pad packet
        memcpy(*normalized_packet, packet, packet_size);
        memset(*normalized_packet + packet_size, 0, target_size - packet_size);
    } else {
        // Truncate packet
        memcpy(*normalized_packet, packet, target_size);
    }

        return 0;
}

int memshadow_dpi_evasion_apply_steganography(
    const uint8_t *data,
    size_t data_size,
    uint32_t key,
    uint8_t **encoded_data,
    size_t *encoded_size
) {
    if (!data || !encoded_data || !encoded_size) {
        return -1;
    }

    *encoded_size = data_size;
    *encoded_data = malloc(*encoded_size);

    if (!*encoded_data) {
        return -1;
    }

    // Simple XOR-based steganography
    uint8_t key_bytes[4];
    key_bytes[0] = (uint8_t)(key & 0xFF);
    key_bytes[1] = (uint8_t)((key >> 8) & 0xFF);
    key_bytes[2] = (uint8_t)((key >> 16) & 0xFF);
    key_bytes[3] = (uint8_t)((key >> 24) & 0xFF);

    for (size_t i = 0; i < data_size; i++) {
        (*encoded_data)[i] = data[i] ^ key_bytes[i % 4];
    }

    return 0;
}

int memshadow_dpi_evasion_remove_steganography(
    const uint8_t *encoded_data,
    size_t encoded_size,
    uint32_t key,
    uint8_t **data,
    size_t *data_size
) {
    // XOR is symmetric, so same function works for both
    return memshadow_dpi_evasion_apply_steganography(encoded_data, encoded_size, key, data, data_size);
}

int memshadow_dpi_evasion_generate_dns_query(
    const char *domain,
    uint8_t **query,
    size_t *query_size
) {
    if (!domain || !query || !query_size) {
        return -1;
    }

    // DNS header (12 bytes) + domain name + query record
    size_t domain_len = strlen(domain);
    *query_size = 12 + domain_len + 2 + 4; // +2 for compression, +4 for QTYPE/QCLASS
    *query = malloc(*query_size);

    if (!*query) {
    return -1;
}

    // DNS header
    (*query)[0] = 0x12; // Transaction ID
    (*query)[1] = 0x34;
    (*query)[2] = 0x01; // Standard query
    (*query)[3] = 0x00;
    (*query)[4] = 0x00; // Questions
    (*query)[5] = 0x01;
    memset(*query + 6, 0, 6); // Other counts

    // Domain name encoding (simplified)
    memcpy(*query + 12, domain, domain_len);
    (*query)[12 + domain_len] = 0; // Null terminator

    // Query type (A record = 1) and class (IN = 1)
    (*query)[12 + domain_len + 1] = 0;
    (*query)[12 + domain_len + 2] = 1;
    (*query)[12 + domain_len + 3] = 0;
    (*query)[12 + domain_len + 4] = 1;

    return 0;
}

int memshadow_dpi_evasion_generate_tls_record(
    const uint8_t *data,
    size_t data_size,
    uint8_t **record,
    size_t *record_size
) {
    if (!data || !record || !record_size) {
        return -1;
    }

    const size_t header_size = 5;
    *record_size = header_size + data_size;
    *record = malloc(*record_size);

    if (!*record) {
        return -1;
    }

    // TLS record header
    (*record)[0] = 23; // Application data
    (*record)[1] = 0x03; // TLS 1.2/1.3
    (*record)[2] = 0x03;
    (*record)[3] = (uint8_t)(data_size >> 8);
    (*record)[4] = (uint8_t)(data_size & 0xFF);

    memcpy(*record + header_size, data, data_size);
    return 0;
}