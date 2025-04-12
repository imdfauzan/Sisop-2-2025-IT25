#include <sys/sysinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <signal.h>
#include <pwd.h>
#include <errno.h>
#include <unistd.h>

#define MAX_LINE 1024
#define LOG_FILE "debugmon.log"

void fail_user(const char *username);
void block_user(const char *username);
int is_user_blocked(const char *username);

// SOAL A
void get_mem_usage(int pid, float *usage) {
    char statm_path[MAX_LINE];
    snprintf(statm_path, sizeof(statm_path), "/proc/%d/statm", pid);

    FILE *fp = fopen(statm_path, "r");
    if (!fp) {
        *usage = 0.0;
        return;
    }

    long size, resident;
    fscanf(fp, "%ld %ld", &size, &resident);
    fclose(fp);

    *usage = (resident * sysconf(_SC_PAGESIZE)) / (1024.0 * 1024.0); // MB
}

void get_cpu_usage(int pid, float *usage) {
    *usage = 0.0;   //default
}

void log_activity(const char *process_name, const char *status) {
    time_t now = time(NULL); //mengambil detik waktu sejak epoch
    char timestr[50];
    strftime(timestr, sizeof(timestr), "%d:%m:%Y-%H:%M:%S", localtime(&now)); //mengubah format waktu epoch 

    FILE *log = fopen(LOG_FILE, "a"); // a = membuka file&menambahkan data ke akhir file 
    if (log) {
        fprintf(log, "[%s]_%s_STATUS(%s)\n", timestr, process_name, status);
        fclose(log);
    }
}

void list_processes(const char *username) {
    DIR *dir;
    struct dirent *entry;
    FILE *fp;
    char path[MAX_LINE];
    char line[MAX_LINE];
    char command[MAX_LINE];
    int pid;
    char user[50];
    float cpu_usage, mem_usage;

    printf("%-10s %-40s %-10s %-10s\n", "PID", "COMMAND", "CPU%", "MEM(MB)");
    printf("--------------------------------------------------------------------------------\n");

    dir = opendir("/proc"); //buka direktori /proc
    if (!dir) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
            pid = atoi(entry->d_name);

            snprintf(path, sizeof(path), "/proc/%d/status", pid);
            fp = fopen(path, "r");
            if (!fp) continue;

            strcpy(user, "");
            strcpy(command, "");

            while (fgets(line, sizeof(line), fp)) {
                if (strncmp(line, "Name:", 5) == 0) {
                    sscanf(line, "Name:\t%s", command);
                } else if (strncmp(line, "Uid:", 4) == 0) {
                    int uid;
                    sscanf(line, "Uid:\t%d", &uid);
                    struct passwd *pw = getpwuid(uid);
                    if (pw != NULL) {
                        strcpy(user, pw->pw_name);
                    } else {
                        strcpy(user, "unknown");
                    }
                }
            }
            fclose(fp);

            if (strcmp(user, username) == 0) {
                get_cpu_usage(pid, &cpu_usage);
                get_mem_usage(pid, &mem_usage);

                printf("%-10d %-40s %-10.2f %-10.2f\n", pid, command, cpu_usage, mem_usage);
                log_activity(command, "RUNNING");
            }
        }
    }
    closedir(dir);
}

// SOAL B
void daemon_mode(const char *username) { //start daemon
    pid_t pid = fork();

    if (pid < 0) { // <0 (negatif) brati kalo gagal
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) { // >0 (positip) brati fork dipanggil di proses induk 
        FILE *fp;
        char pidfile[100];
        snprintf(pidfile, sizeof(pidfile), "debugmon_%s.pid", username);

        fp = fopen(pidfile, "w");
        if (fp) {
            fprintf(fp, "%d\n", pid);
            fclose(fp);
        }
        printf("Debugmon daemon started for user %s (PID: %d)\n", username, pid);
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        perror("setsid");
        exit(EXIT_FAILURE);
    }

    umask(0); // mengubah akses jd 777
    chdir("/");

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    while (1) {
        // cek apkh user diblokir lewat file blokir
        char block_path[128];
        snprintf(block_path, sizeof(block_path), "/tmp/.debugmon_blocked_%s", username);
    
        if (access(block_path, F_OK) == 0) {
            fail_user(username); //memastikan semua proses user dibunuh
        } else {
            list_processes(username);
        }
    
        sleep(10);
    }    
}

// SOAL C
void stop_daemon(const char *username) { //stop daemon
    char pidfile[100];
    snprintf(pidfile, sizeof(pidfile), "debugmon_%s.pid", username);

    FILE *fp = fopen(pidfile, "r");
    if (!fp) {
        printf("Tidak ditemukan daemon dengan username %s.\n", username);
        return;
    }

    int pid;
    fscanf(fp, "%d", &pid);
    fclose(fp);

    if (kill(pid, SIGTERM) == 0) {
        printf("Debugmon daemon dihentikan untuk username %s (PID: %d)\n", username, pid);
        remove(pidfile);
        log_activity("debugmon", "RUNNING");  // Status RUNNING saat stop
    } else {
        perror("Failed to stop daemon");
    }
}

// SOAL D
void block_user(const char *username) { //mbuat file blokir di /tmp
    char path[128];
    snprintf(path, sizeof(path), "/tmp/.debugmon_blocked_%s", username);
    FILE *fp = fopen(path, "w");
    if (fp) {
        fprintf(fp, "blocked");
        fclose(fp);
    }
}

int is_user_blocked(const char *username) { //ngecek apkh user sdg diblokir (dipake di daemon_mode)
    char path[128];
    snprintf(path, sizeof(path), "/tmp/.debugmon_blocked_%s", username);
    return access(path, F_OK) == 0;
}


void fail_user(const char *username) {
    // 1. Buat file blokir TERLEBIH DAHULU
    char block_path[256];
    snprintf(block_path, sizeof(block_path), "/tmp/.debugmon_blocked_%s", username);
    
    FILE *block_file = fopen(block_path, "w");
    if (!block_file) {
        perror("Gagal membuat file blokir");
        exit(EXIT_FAILURE);
    }
    fprintf(block_file, "blocked");
    fclose(block_file);
    printf("[SUCCESS] File blokir dibuat: %s\n", block_path);

    // 2. Hentikan semua proses user
    DIR *dir = opendir("/proc");
    if (!dir) {
        perror("Gagal buka /proc");
        return;
    }

    struct dirent *entry;
    int processes_killed = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
            int pid = atoi(entry->d_name);
            char path[256], line[256], command[256], user[50] = "";
            
            // Baca info proses
            snprintf(path, sizeof(path), "/proc/%d/status", pid);
            FILE *fp = fopen(path, "r");
            if (!fp) continue;

            while (fgets(line, sizeof(line), fp)) {
                if (strncmp(line, "Name:", 5) == 0) {
                    sscanf(line, "Name:\t%s", command);
                } else if (strncmp(line, "Uid:", 4) == 0) {
                    int uid;
                    sscanf(line, "Uid:\t%d", &uid);
                    struct passwd *pw = getpwuid(uid);
                    if (pw) strncpy(user, pw->pw_name, sizeof(user));
                }
            }
            fclose(fp);

            // Hentikan proses jika milik user dan bukan proses debugmon sendiri
            if (strcmp(user, username) == 0 && strcmp(command, "debugmon") != 0) {
                printf("Menghentikan PID %d (%s)... ", pid, command);
                
                if (kill(pid, SIGKILL) == 0) {
                    printf("Berhasil\n");
                    log_activity(command, "FAILED");
                    processes_killed++;
                    
                    // Beri waktu untuk proses critical (seperti terminal)
                    if (strstr(command, "terminal") || strstr(command, "bash") || strstr(command, "zsh")) {
                        sleep(1);
                    }
                } else {
                    printf("Gagal (Error: %s)\n", strerror(errno));
                }
            }
        }
    }
    closedir(dir);

    printf("\nTotal proses dihentikan: %d\n", processes_killed);
    printf("User %s telah diblokir dan semua proses dihentikan.\n", username);
}

// SOAL E
void revert_user(const char *username) { //mencabut Blokir
    char path[128];
    snprintf(path, sizeof(path), "/tmp/.debugmon_blocked_%s", username);

    if (access(path, F_OK) != -1) { //cek apkh ada username yg diblokir
        if (remove(path) == 0) {
            printf("Proses user %s diizinkan kembali.\n", username);
            log_activity("debugmon_revert", "RUNNING");
        } else {
            perror("Gagal menghapus file blokir");
        }
    } else {
        printf("User %s sedang tidak diblokir.\n", username);
    }
}


int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Sintaks yang benar: %s <fungsi> <username>\nPilihan fungsi: list, daemon, stop, fail, revert\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "list") == 0) {
        list_processes(argv[2]);
    } 
    else if (strcmp(argv[1], "daemon") == 0) {
        daemon_mode(argv[2]);
    } 
    else if (strcmp(argv[1], "stop") == 0) {
        stop_daemon(argv[2]);
    } 
    else if (strcmp(argv[1], "fail") == 0) {
        fail_user(argv[2]);
    }
    else if (strcmp(argv[1], "revert") == 0) {
        revert_user(argv[2]);
    }
    else {
        fprintf(stderr, "Pilihan Fungsi: list, daemon, stop, fail, revert %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
