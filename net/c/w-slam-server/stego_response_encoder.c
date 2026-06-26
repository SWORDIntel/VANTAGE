/*
 * Steganography Response Encoder
 * 
 * Hides encrypted MEMSHADOW responses in boring-looking formats:
 * - JSON API responses
 * - XML configuration files
 * - HTML comments
 * 
 * All implementations are real, following DSMIL cursorrules.
 */

#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#pragma comment(lib, "advapi32.lib")

// Steganography format types
typedef enum {
    STEGO_FORMAT_JSON = 0,
    STEGO_FORMAT_XML = 1,
    STEGO_FORMAT_HTML = 2
} StegoFormat;

// Base64-like encoding (custom alphabet for obfuscation)
static const char BASE64_ALPHABET[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


/*
 * Base64 Encode
 */
static bool base64_encode(const uint8_t *input, size_t input_len, char **output, size_t *output_len) {
    if (!input || !output || !output_len) {
        return false;
    }
    
    size_t encoded_len = ((input_len + 2) / 3) * 4;
    *output = (char *)malloc(encoded_len + 1);
    if (!*output) {
        return false;
    }
    
    size_t j = 0;
    for (size_t i = 0; i < input_len; i += 3) {
        uint32_t octet_a = i < input_len ? input[i] : 0;
        uint32_t octet_b = i + 1 < input_len ? input[i + 1] : 0;
        uint32_t octet_c = i + 2 < input_len ? input[i + 2] : 0;
        
        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;
        
        (*output)[j++] = BASE64_ALPHABET[(triple >> 18) & 0x3F];
        (*output)[j++] = BASE64_ALPHABET[(triple >> 12) & 0x3F];
        (*output)[j++] = (i + 1 < input_len) ? BASE64_ALPHABET[(triple >> 6) & 0x3F] : '=';
        (*output)[j++] = (i + 2 < input_len) ? BASE64_ALPHABET[triple & 0x3F] : '=';
    }
    
    (*output)[j] = '\0';
    *output_len = j;
    
    return true;
}


/*
 * Encode as JSON API Response
 */
bool stego_encode_json(const uint8_t *data, size_t data_len, char **output, size_t *output_len) {
    if (!data || !output || !output_len) {
        return false;
    }
    
    // Base64 encode the data
    char *encoded = NULL;
    size_t encoded_len = 0;
    if (!base64_encode(data, data_len, &encoded, &encoded_len)) {
        return false;
    }
    
    // Get current timestamp
    time_t now = time(NULL);
    
    // Build JSON response
    size_t json_len = 256 + encoded_len;
    *output = (char *)malloc(json_len);
    if (!*output) {
        free(encoded);
        return false;
    }
    
    // Calculate SHA256 checksum
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    char checksum_hex[65] = {0};
    
    if (CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
            CryptHashData(hHash, (BYTE *)encoded, (DWORD)encoded_len, 0);
            
            DWORD hash_len = 32;
            uint8_t hash[32];
            if (CryptGetHashParam(hHash, HP_HASHVAL, hash, &hash_len, 0)) {
                // Convert to hex
                for (int i = 0; i < 32; i++) {
                    snprintf(checksum_hex + (i * 2), 3, "%02x", hash[i]);
                }
            }
            
            CryptDestroyHash(hHash);
        }
        CryptReleaseContext(hProv, 0);
    }
    
    *output_len = snprintf(*output, json_len,
        "{\n"
        "  \"status\": \"ok\",\n"
        "  \"timestamp\": %lld,\n"
        "  \"data\": \"%s\",\n"
        "  \"version\": \"1.0\",\n"
        "  \"checksum\": \"%s\"\n"
        "}",
        (long long)now, encoded, checksum_hex
    );
    
    free(encoded);
    
    return true;
}


/*
 * Encode as XML Configuration
 */
bool stego_encode_xml(const uint8_t *data, size_t data_len, char **output, size_t *output_len) {
    if (!data || !output || !output_len) {
        return false;
    }
    
    // Base64 encode the data
    char *encoded = NULL;
    size_t encoded_len = 0;
    if (!base64_encode(data, data_len, &encoded, &encoded_len)) {
        return false;
    }
    
    // Get current timestamp
    SYSTEMTIME st;
    GetSystemTime(&st);
    
    // Build XML response
    size_t xml_len = 512 + encoded_len;
    *output = (char *)malloc(xml_len);
    if (!*output) {
        free(encoded);
        return false;
    }
    
    *output_len = snprintf(*output, xml_len,
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<config version=\"1.0\">\n"
        "  <timestamp>%04d-%02d-%02dT%02d:%02d:%02dZ</timestamp>\n"
        "  <setting name=\"cache\">%s</setting>\n"
        "  <setting name=\"version\">1.0</setting>\n"
        "</config>",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
        encoded
    );
    
    free(encoded);
    
    return true;
}


/*
 * Encode as HTML Comment
 */
bool stego_encode_html(const uint8_t *data, size_t data_len, char **output, size_t *output_len) {
    if (!data || !output || !output_len) {
        return false;
    }
    
    // Base64 encode the data
    char *encoded = NULL;
    size_t encoded_len = 0;
    if (!base64_encode(data, data_len, &encoded, &encoded_len)) {
        return false;
    }
    
    // Build HTML with comment
    size_t html_len = 512 + encoded_len;
    *output = (char *)malloc(html_len);
    if (!*output) {
        free(encoded);
        return false;
    }
    
    *output_len = snprintf(*output, html_len,
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "  <meta charset=\"UTF-8\">\n"
        "  <title>Page</title>\n"
        "</head>\n"
        "<body>\n"
        "  <!-- Analytics: %s -->\n"
        "  <p>Loading...</p>\n"
        "</body>\n"
        "</html>",
        encoded
    );
    
    free(encoded);
    
    return true;
}


/*
 * Encode Response with Steganography
 * Selects format based on configuration
 */
bool stego_encode_response(const uint8_t *data, size_t data_len,
                           StegoFormat format,
                           char **output, size_t *output_len) {
    switch (format) {
        case STEGO_FORMAT_JSON:
            return stego_encode_json(data, data_len, output, output_len);
            
        case STEGO_FORMAT_XML:
            return stego_encode_xml(data, data_len, output, output_len);
            
        case STEGO_FORMAT_HTML:
            return stego_encode_html(data, data_len, output, output_len);
            
        default:
            return false;
    }
}


/*
 * Decode Steganography Response
 * Extracts data from JSON/XML/HTML
 */
bool stego_decode_response(const char *stego_data, size_t stego_len,
                           StegoFormat format,
                           uint8_t **data, size_t *data_len) {
    if (!stego_data || !data || !data_len) {
        return false;
    }
    
    // Extract base64 data from format
    const char *base64_start = NULL;
    const char *base64_end = NULL;
    
    switch (format) {
        case STEGO_FORMAT_JSON:
            base64_start = strstr(stego_data, "\"data\": \"");
            if (base64_start) {
                base64_start += 9;
                base64_end = strchr(base64_start, '"');
            }
            break;
            
        case STEGO_FORMAT_XML:
            base64_start = strstr(stego_data, "<setting name=\"cache\">");
            if (base64_start) {
                base64_start += 22;
                base64_end = strstr(base64_start, "</setting>");
            }
            break;
            
        case STEGO_FORMAT_HTML:
            base64_start = strstr(stego_data, "<!-- Analytics: ");
            if (base64_start) {
                base64_start += 16;
                base64_end = strstr(base64_start, " -->");
            }
            break;
    }
    
    if (!base64_start || !base64_end) {
        return false;
    }
    
    // Base64 decode
    size_t encoded_len = base64_end - base64_start;
    *data_len = (encoded_len * 3) / 4;
    *data = (uint8_t *)malloc(*data_len);
    if (!*data) {
        return false;
    }
    
    // Decode base64
    const char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t out_idx = 0;
    
    for (size_t i = 0; i < encoded_len && out_idx < *data_len; i += 4) {
        uint32_t value = 0;
        int padding = 0;
        
        // Decode 4 base64 chars to 3 bytes
        for (int j = 0; j < 4 && (i + j) < encoded_len; j++) {
            char c = base64_start[i + j];
            if (c == '=') {
                padding++;
                continue;
            }
            
            const char *pos = strchr(alphabet, c);
            if (pos) {
                value = (value << 6) | (pos - alphabet);
            }
        }
        
        // Extract 3 bytes
        if (out_idx < *data_len) (*data)[out_idx++] = (value >> 16) & 0xFF;
        if (out_idx < *data_len && padding < 2) (*data)[out_idx++] = (value >> 8) & 0xFF;
        if (out_idx < *data_len && padding < 1) (*data)[out_idx++] = value & 0xFF;
    }
    
    *data_len = out_idx;
    
    return true;
}


/*
 * Select Random Steganography Format
 * Adds unpredictability
 */
StegoFormat stego_select_random_format(void) {
    return (StegoFormat)(rand() % 3);
}
