#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>

#define STARTER_KIT_DIR "./starter_kit"
#define QUARANTINE_DIR "./quarantine"
#define LOG_FILE "activity.log"
#define PID_FILE "decryptor.pid"

void write_log(const char *header, const char *message) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[100];
    strftime(timestamp, sizeof(timestamp), "[%d-%m-%Y][%H:%M:%S]", t);

    fprintf(log, "%s\n%s - %s\n", header, timestamp, message);
    fclose(log);
}

void base64_decode(const char *input, char *output) {
    const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int val = 0, valb = -8;
    int len = strlen(input);
    int pos = 0;

    for (int i = 0; i < len; i++) {
        char *p = strchr(table, input[i]);
        if (p) {
            val = (val << 6) + (p - table);
            valb += 6;
            if (valb >= 0) {
                output[pos++] = (char)((val >> valb) & 0xFF);
                valb -= 8;
            }
        }
    }
    output[pos] = '\0';
}

void decrypt_filenames_daemon() {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) {
        FILE *f = fopen(PID_FILE, "w");
        if (f) {
            fprintf(f, "%d\n", pid);
            fclose(f);

            char msg[100];
            snprintf(msg, sizeof(msg), "Successfully started decryption process with PID %d.", pid);
            write_log("Decrypt:", msg);
        }
        exit(EXIT_SUCCESS);
    }

    umask(0);
    if (setsid() < 0) exit(EXIT_FAILURE);
    if (chdir("/") < 0) exit(EXIT_FAILURE);
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    while (1) {
        DIR *dir = opendir(QUARANTINE_DIR);
        struct dirent *entry;
        if (dir) {
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_type == DT_REG) {
                    char oldpath[512], newpath[512], decoded[256];
                    snprintf(oldpath, sizeof(oldpath), "%s/%s", QUARANTINE_DIR, entry->d_name);
                    base64_decode(entry->d_name, decoded);
                    snprintf(newpath, sizeof(newpath), "%s/%s", QUARANTINE_DIR, decoded);
                    rename(oldpath, newpath);
                }
            }
            closedir(dir);
        }
        sleep(5);
    }
}

void move_files(const char *from, const char *to, const char *header, const char *log_format) {
    DIR *dir = opendir(from);
    struct dirent *entry;
    if (!dir) return;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char src[512], dest[512];
            snprintf(src, sizeof(src), "%s/%s", from, entry->d_name);
            snprintf(dest, sizeof(dest), "%s/%s", to, entry->d_name);
            rename(src, dest);

            char msg[512];
            snprintf(msg, sizeof(msg), log_format, entry->d_name);
            write_log(header, msg);
        }
    }
    closedir(dir);
}

void delete_quarantine_files() {
    DIR *dir = opendir(QUARANTINE_DIR);
    struct dirent *entry;
    if (!dir) return;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", QUARANTINE_DIR, entry->d_name);
            unlink(filepath);

            char msg[512];
            snprintf(msg, sizeof(msg), "%s - Successfully deleted.", entry->d_name);
            write_log("Eradicate:", msg);
        }
    }
    closedir(dir);
}

void shutdown_daemon() {
    FILE *f = fopen(PID_FILE, "r");
    if (!f) {
        fprintf(stderr, "Daemon is not running.\n");
        return;
    }

    int pid;
    fscanf(f, "%d", &pid);
    fclose(f);

    if (kill(pid, SIGTERM) == 0) {
        char msg[100];
        snprintf(msg, sizeof(msg), "Successfully shut off decryption process with PID %d.", pid);
        write_log("Shutdown:", msg);
        remove(PID_FILE);
    } else {
        perror("Failed to shutdown daemon");
    }
}

int main(int argc, char *argv[]) {
    mkdir(STARTER_KIT_DIR, 0755);
    mkdir(QUARANTINE_DIR, 0755);

    if (argc != 2) {
        printf("Usage:\n");
        printf("./starterkit --decrypt\n");
        printf("./starterkit --quarantine\n");
        printf("./starterkit --return\n");
        printf("./starterkit --eradicate\n");
        printf("./starterkit --shutdown\n");
        return 1;
    }

    if (strcmp(argv[1], "--decrypt") == 0) {
        decrypt_filenames_daemon();
    } else if (strcmp(argv[1], "--quarantine") == 0) {
        move_files(STARTER_KIT_DIR, QUARANTINE_DIR, "Quarantine:", "%s - Successfully moved to quarantine directory.");
    } else if (strcmp(argv[1], "--return") == 0) {
        move_files(QUARANTINE_DIR, STARTER_KIT_DIR, "Return:", "%s - Successfully returned to starter kit directory.");
    } else if (strcmp(argv[1], "--eradicate") == 0) {
        delete_quarantine_files();
    } else if (strcmp(argv[1], "--shutdown") == 0) {
        shutdown_daemon();
    } else {
        return 1;
    }

    return 0;
}
