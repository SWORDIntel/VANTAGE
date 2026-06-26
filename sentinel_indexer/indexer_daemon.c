#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#define LAST_ACTIVE_FILE "/tmp/sentinel_last_active"
#define PENDING_DIRS_FILE "/tmp/sentinel_pending_dirs"
#define CWD_FILE "/tmp/sentinel_cwd"
#define IDLE_THRESHOLD_SEC 30

void qihse_vector_connect(const char* filepath) {
    // Mock function to simulate forming QIHSE vector relationships
    // based on file proximity and names.
    printf("QIHSE vector connect: %s\n", filepath);
}

void scan_directory(const char* dirpath) {
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(dirpath)))
        return;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", dirpath, entry->d_name);
            scan_directory(path);
        } else {
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", dirpath, entry->d_name);
            qihse_vector_connect(path);
        }
    }
    closedir(dir);
}

void process_pending_dirs() {
    FILE *file = fopen(PENDING_DIRS_FILE, "r");
    if (!file) return;

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0; // Remove newline
        if (strlen(line) > 0) {
            scan_directory(line);
        }
    }
    fclose(file);
}

long check_ram_buffer_size() {
    struct stat st;
    if (stat("/dev/shm/sentinel_active_session", &st) == 0) {
        return (long)st.st_size;
    }
    return 0;
}

void commit_to_zfs_disk() {
    printf("Flushing to ZFS disk...\n");
    // Mock system call to hybrid_db
    system("hybrid_db --flush-to-zfs 2>/dev/null || echo 'hybrid_db mock called'");
}

int main() {
    char current_working_dir[1024] = "";

    while (1) {
        long idle_time = 0;
        FILE *file = fopen(LAST_ACTIVE_FILE, "r");
        if (file) {
            long last_active = 0;
            if (fscanf(file, "%ld", &last_active) == 1) {
                time_t current_time = time(NULL);
                idle_time = current_time - last_active;
                if (idle_time > IDLE_THRESHOLD_SEC) {
                    // System is idle, start scanning
                    process_pending_dirs();
                }
            }
            fclose(file);
        }
        
        // Track current_working_dir
        char user_pwd[1024] = "";
        FILE *cwd_file = fopen(CWD_FILE, "r");
        if (cwd_file) {
            if (fgets(user_pwd, sizeof(user_pwd), cwd_file) != NULL) {
                user_pwd[strcspn(user_pwd, "\n")] = 0;
            }
            fclose(cwd_file);
        } else {
            // Fallback to daemon's own cwd if the hook file is missing
            if (getcwd(user_pwd, sizeof(user_pwd)) == NULL) {
                user_pwd[0] = '\0';
            }
        }

        int user_pwd_changed = 0;
        if (strlen(current_working_dir) == 0) {
            snprintf(current_working_dir, sizeof(current_working_dir), "%s", user_pwd);
        } else if (strcmp(current_working_dir, user_pwd) != 0) {
            user_pwd_changed = 1;
            snprintf(current_working_dir, sizeof(current_working_dir), "%s", user_pwd);
        }

        if (user_pwd_changed) {
            commit_to_zfs_disk();
        }

        long ram_db_size = check_ram_buffer_size();
        if (idle_time > 60 && ram_db_size >= 1048576) {
            commit_to_zfs_disk();
        }
        
        // Sleep to ensure almost 0% CPU usage when not idle
        // (or between idle scans)
        sleep(5); 
    }
    return 0;
}
