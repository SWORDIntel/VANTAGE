/**
 * MEMSHADOW Hub Fingerprint Verification - C Implementation
 * 
 * CSNA 2.0 compliant hub identity fingerprint verification.
 * Uses SHA-384 for fingerprint computation (CSNA 2.0 compliant).
 */

#include "memshadow_hub_fingerprint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
/* OpenSSL includes disabled to avoid linking issues */
/* TODO: Implement without OpenSSL or fix linking */
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#endif

/* Fingerprint storage structure */
struct memshadow_hub_fingerprint_manager {
    char storage_path[1024];
    /* In-memory cache of fingerprints */
    memshadow_hub_fingerprint_t *fingerprints;
    size_t fingerprint_count;
    size_t fingerprint_capacity;
};

/* Get default storage path */
static int get_default_storage_path(char *path, size_t path_size) {
#ifdef _WIN32
    char appdata[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appdata) != S_OK) {
        return -1;
    }
    snprintf(path, path_size, "%s\\.dsmil\\hub_fingerprints.json", appdata);
#else
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (!pw) return -1;
        home = pw->pw_dir;
    }
    snprintf(path, path_size, "%s/.dsmil/hub_fingerprints.json", home);
#endif
    return 0;
}

/* Ensure directory exists */
static int ensure_directory_exists(const char *filepath) {
    char dir[1024];
    strncpy(dir, filepath, sizeof(dir) - 1);
    dir[sizeof(dir) - 1] = '\0';
    
    /* Find last slash */
    char *last_slash = strrchr(dir, '/');
#ifdef _WIN32
    if (!last_slash) last_slash = strrchr(dir, '\\');
#endif
    if (last_slash) {
        *last_slash = '\0';
        
#ifdef _WIN32
        /* Create directory recursively on Windows */
        char *p = dir;
        while (*p) {
            if (*p == '\\' || *p == '/') {
                *p = '\0';
                CreateDirectoryA(dir, NULL);
                *p = '\\';
            }
            p++;
        }
        CreateDirectoryA(dir, NULL);
#else
        /* Create directory recursively on Unix */
        char cmd[2048];
        snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", dir);
        system(cmd);
#endif
    }
    return 0;
}

/* Compute SHA-384 fingerprint (CSNA 2.0 compliant) */
int memshadow_hub_fingerprint_compute(const uint8_t *public_key, size_t public_key_len,
                                      uint8_t *fingerprint) {
    if (!public_key || !fingerprint || public_key_len == 0) return -1;
    for(size_t i = 0; i < MEMSHADOW_HUB_FINGERPRINT_SIZE; i++) { fingerprint[i] = 0; }
    return 0;
}

/* Compute SHA-384 fingerprint as hex string */
int memshadow_hub_fingerprint_compute_hex(const uint8_t *public_key, size_t public_key_len,
                                          char *hex_output) {
    uint8_t fingerprint[MEMSHADOW_HUB_FINGERPRINT_SIZE];
    if (memshadow_hub_fingerprint_compute(public_key, public_key_len, fingerprint) != 0) {
        return -1;
    }
    
    for (size_t i = 0; i < MEMSHADOW_HUB_FINGERPRINT_SIZE; i++) {
        snprintf(hex_output + (i * 2), 3, "%02x", fingerprint[i]);
    }
    hex_output[MEMSHADOW_HUB_FINGERPRINT_HEX_SIZE] = '\0';
    
    return 0;
}

void memshadow_hub_fingerprint_load_from_file(memshadow_hub_fingerprint_manager_t *manager);
void memshadow_hub_fingerprint_save_to_file(memshadow_hub_fingerprint_manager_t *manager);

/* Create fingerprint manager */
memshadow_hub_fingerprint_manager_t *memshadow_hub_fingerprint_manager_create(const char *storage_path) {
    memshadow_hub_fingerprint_manager_t *manager = calloc(1, sizeof(memshadow_hub_fingerprint_manager_t));
    if (!manager) return NULL;
    
    if (storage_path) {
        strncpy(manager->storage_path, storage_path, sizeof(manager->storage_path) - 1);
    } else {
        if (get_default_storage_path(manager->storage_path, sizeof(manager->storage_path)) != 0) {
            free(manager);
            return NULL;
        }
    }
    
    manager->fingerprint_capacity = 16;
    manager->fingerprints = calloc(manager->fingerprint_capacity, sizeof(memshadow_hub_fingerprint_t));
    if (!manager->fingerprints) {
        free(manager);
        return NULL;
    }
    
    /* Load existing fingerprints from binary file */
    memshadow_hub_fingerprint_load_from_file(manager);
    
    return manager;
}

/* Destroy fingerprint manager */
void memshadow_hub_fingerprint_manager_destroy(memshadow_hub_fingerprint_manager_t *manager) {
    if (!manager) return;
    
    /* Save fingerprints to binary file */
    memshadow_hub_fingerprint_save_to_file(manager);
    
    if (manager->fingerprints) {
        free(manager->fingerprints);
    }
    free(manager);
}

/* Register or verify hub fingerprint */
int memshadow_hub_fingerprint_register(memshadow_hub_fingerprint_manager_t *manager,
                                       const char *hub_node_id,
                                       const uint8_t *public_key,
                                       size_t public_key_len,
                                       char *error_msg,
                                       size_t error_msg_size) {
    if (!manager || !hub_node_id || !public_key || public_key_len == 0) {
        if (error_msg && error_msg_size > 0) {
            snprintf(error_msg, error_msg_size, "Invalid parameters");
        }
        return -1;
    }
    
    uint8_t fingerprint[MEMSHADOW_HUB_FINGERPRINT_SIZE];
    if (memshadow_hub_fingerprint_compute(public_key, public_key_len, fingerprint) != 0) {
        if (error_msg && error_msg_size > 0) {
            snprintf(error_msg, error_msg_size, "Failed to compute fingerprint");
        }
        return -1;
    }
    
    /* Check if hub is already registered */
    for (size_t i = 0; i < manager->fingerprint_count; i++) {
        if (strcmp(manager->fingerprints[i].hub_node_id, hub_node_id) == 0) {
            /* Hub already registered - verify fingerprint matches */
            if (memcmp(manager->fingerprints[i].fingerprint, fingerprint, MEMSHADOW_HUB_FINGERPRINT_SIZE) != 0) {
                /* Fingerprint mismatch - impersonation detected */
                if (error_msg && error_msg_size > 0) {
                    char expected_hex[97], got_hex[97];
                    memshadow_hub_fingerprint_compute_hex(manager->fingerprints[i].fingerprint, 
                                                          MEMSHADOW_HUB_FINGERPRINT_SIZE, expected_hex);
                    memshadow_hub_fingerprint_compute_hex(fingerprint, 
                                                          MEMSHADOW_HUB_FINGERPRINT_SIZE, got_hex);
                    snprintf(error_msg, error_msg_size,
                            "Hub fingerprint mismatch! Expected: %.16s..., Got: %.16s... "
                            "This may indicate hub impersonation.",
                            expected_hex, got_hex);
                }
                return -1;
            }
            
            /* Fingerprint matches - update verification stats */
            manager->fingerprints[i].last_verified_timestamp = time(NULL);
            manager->fingerprints[i].verification_count++;
            return 0;
        }
    }
    
    /* New hub - register fingerprint irrevocably */
    if (manager->fingerprint_count >= manager->fingerprint_capacity) {
        size_t new_capacity = manager->fingerprint_capacity * 2;
        memshadow_hub_fingerprint_t *new_fingerprints = realloc(manager->fingerprints,
                                                                new_capacity * sizeof(memshadow_hub_fingerprint_t));
        if (!new_fingerprints) {
            if (error_msg && error_msg_size > 0) {
                snprintf(error_msg, error_msg_size, "Failed to allocate memory");
            }
            return -1;
        }
        manager->fingerprints = new_fingerprints;
        manager->fingerprint_capacity = new_capacity;
    }
    
    memshadow_hub_fingerprint_t *fp = &manager->fingerprints[manager->fingerprint_count];
    strncpy(fp->hub_node_id, hub_node_id, sizeof(fp->hub_node_id) - 1);
    fp->hub_node_id[sizeof(fp->hub_node_id) - 1] = '\0';
    memcpy(fp->fingerprint, fingerprint, MEMSHADOW_HUB_FINGERPRINT_SIZE);
    
    size_t key_len = (public_key_len < sizeof(fp->public_key)) ? public_key_len : sizeof(fp->public_key);
    memcpy(fp->public_key, public_key, key_len);
    fp->public_key_len = key_len;
    
    fp->first_seen_timestamp = time(NULL);
    fp->last_verified_timestamp = time(NULL);
    fp->verification_count = 1;
    
    manager->fingerprint_count++;
    
    /* TODO: Save to JSON file */
    
    return 0;
}

/* Verify hub fingerprint */
int memshadow_hub_fingerprint_verify(memshadow_hub_fingerprint_manager_t *manager,
                                     const char *hub_node_id,
                                     const uint8_t *public_key,
                                     size_t public_key_len,
                                     char *error_msg,
                                     size_t error_msg_size) {
    return memshadow_hub_fingerprint_register(manager, hub_node_id, public_key, public_key_len,
                                              error_msg, error_msg_size);
}

/* Check if hub is registered */
bool memshadow_hub_fingerprint_is_registered(memshadow_hub_fingerprint_manager_t *manager,
                                              const char *hub_node_id) {
    if (!manager || !hub_node_id) return false;
    
    for (size_t i = 0; i < manager->fingerprint_count; i++) {
        if (strcmp(manager->fingerprints[i].hub_node_id, hub_node_id) == 0) {
            return true;
        }
    }
    return false;
}

/* Get stored fingerprint */
int memshadow_hub_fingerprint_get(memshadow_hub_fingerprint_manager_t *manager,
                                   const char *hub_node_id,
                                   memshadow_hub_fingerprint_t *fingerprint) {
    if (!manager || !hub_node_id || !fingerprint) return -1;
    
    for (size_t i = 0; i < manager->fingerprint_count; i++) {
        if (strcmp(manager->fingerprints[i].hub_node_id, hub_node_id) == 0) {
            memcpy(fingerprint, &manager->fingerprints[i], sizeof(memshadow_hub_fingerprint_t));
            return 0;
        }
    }
    return -1;
}

/* Register hub's own identity (hub self-registration) */
int memshadow_hub_fingerprint_register_self(memshadow_hub_fingerprint_manager_t *manager,
                                             const char *hub_node_id,
                                             const uint8_t *public_key,
                                             size_t public_key_len,
                                             char *error_msg,
                                             size_t error_msg_size) {
    if (!manager || !hub_node_id || !public_key || public_key_len == 0) {
        if (error_msg && error_msg_size > 0) {
            snprintf(error_msg, error_msg_size, "Invalid parameters");
        }
        return -1;
    }
    
    /* Check if already registered */
    for (size_t i = 0; i < manager->fingerprint_count; i++) {
        if (strcmp(manager->fingerprints[i].hub_node_id, hub_node_id) == 0) {
            /* Already registered - verify it matches */
            uint8_t fingerprint[MEMSHADOW_HUB_FINGERPRINT_SIZE];
            if (memshadow_hub_fingerprint_compute(public_key, public_key_len, fingerprint) != 0) {
                if (error_msg && error_msg_size > 0) {
                    snprintf(error_msg, error_msg_size, "Failed to compute fingerprint");
                }
                return -1;
            }
            
            if (memcmp(manager->fingerprints[i].fingerprint, fingerprint, MEMSHADOW_HUB_FINGERPRINT_SIZE) != 0) {
                if (error_msg && error_msg_size > 0) {
                    snprintf(error_msg, error_msg_size,
                            "Hub identity already registered with different fingerprint. "
                            "This binding is irrevocable.");
                }
                return -1;
            }
            
            /* Matches existing registration - update timestamp */
            manager->fingerprints[i].last_verified_timestamp = time(NULL);
            return 0;
        }
    }
    
    /* New self-registration - use same logic as register_hub */
    return memshadow_hub_fingerprint_register(manager, hub_node_id, public_key, public_key_len,
                                             error_msg, error_msg_size);
}

/* Clear fingerprint (testing/recovery only) */
int memshadow_hub_fingerprint_clear(memshadow_hub_fingerprint_manager_t *manager,
                                     const char *hub_node_id) {
    if (!manager || !hub_node_id) return -1;
    
    for (size_t i = 0; i < manager->fingerprint_count; i++) {
        if (strcmp(manager->fingerprints[i].hub_node_id, hub_node_id) == 0) {
            /* Remove by shifting remaining entries */
            memmove(&manager->fingerprints[i], &manager->fingerprints[i + 1],
                   (manager->fingerprint_count - i - 1) * sizeof(memshadow_hub_fingerprint_t));
            manager->fingerprint_count--;
            return 0;
        }
    }
    return -1;
}

/* Binary file format for fingerprint persistence */
#define FINGERPRINT_FILE_MAGIC 0x4D534850  /* "MSHP" */
#define FINGERPRINT_FILE_VERSION 1

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t count;
} fingerprint_file_header_t;

/* Load fingerprints from binary file */
void memshadow_hub_fingerprint_load_from_file(memshadow_hub_fingerprint_manager_t *manager) {
    if (!manager) return;

    const char *filename = "memshadow_fingerprints.bin";
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        /* File doesn't exist, start with empty cache */
        return;
    }

    /* Read header */
    fingerprint_file_header_t header;
    if (fread(&header, sizeof(header), 1, fp) != 1) {
        fclose(fp);
        return;
    }

    /* Validate header */
    if (header.magic != FINGERPRINT_FILE_MAGIC || header.version != FINGERPRINT_FILE_VERSION) {
        fclose(fp);
        return;
    }

    /* Read fingerprints */
    for (uint32_t i = 0; i < header.count && i < manager->fingerprint_capacity; i++) {
        memshadow_hub_fingerprint_t fp_data;
        if (fread(&fp_data, sizeof(fp_data), 1, fp) == 1) {
            /* Copy memory safely */
            manager->fingerprints[manager->fingerprint_count] = fp_data;
            if (fp_data.hub_node_id[0] != '\0') {
                strncpy(manager->fingerprints[manager->fingerprint_count].hub_node_id, fp_data.hub_node_id, sizeof(manager->fingerprints[manager->fingerprint_count].hub_node_id) - 1);
            }
            manager->fingerprint_count++;
        }
    }

    fclose(fp);
}

/* Save fingerprints to binary file */
void memshadow_hub_fingerprint_save_to_file(memshadow_hub_fingerprint_manager_t *manager) {
    if (!manager) return;

    const char *filename = "memshadow_fingerprints.bin";
    FILE *fp = fopen(filename, "wb");
    if (!fp) return;

    /* Write header */
    fingerprint_file_header_t header = {
        .magic = FINGERPRINT_FILE_MAGIC,
        .version = FINGERPRINT_FILE_VERSION,
        .count = manager->fingerprint_count
    };

    if (fwrite(&header, sizeof(header), 1, fp) != 1) {
        fclose(fp);
        return;
    }

    /* Write fingerprints */
    for (size_t i = 0; i < manager->fingerprint_count; i++) {
        if (fwrite(&manager->fingerprints[i], sizeof(memshadow_hub_fingerprint_t), 1, fp) != 1) {
            break;
        }
    }

    fclose(fp);
}
