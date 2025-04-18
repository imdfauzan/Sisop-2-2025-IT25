#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <zip.h>

#define ZIP_URL "https://drive.usercontent.google.com/u/0/uc?id=1_5GxIGfQr3mNKuavJbte_AoRkEQLXSKS&export=download"
#define ZIP_FILE "starter_kit.zip"
#define STARTER_KIT_DIR "./starter_kit"
#define QUARANTINE_DIR "./quarantine"
#define LOG_FILE "activity.log"

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
}

void download_zip(const char *url, const char *output_file) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize CURL\n");
        return;
    }

    FILE *fp = fopen(output_file, "wb");
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        fprintf(stderr, "Download error: %s\n", curl_easy_strerror(res));

    fclose(fp);
    curl_easy_cleanup(curl);
}

void unzip_file(const char *zip_path, const char *dest_path) {
    int err;
    struct zip *z = zip_open(zip_path, 0, &err);
    if (!z) {
        fprintf(stderr, "Failed to open zip\n");
        return;
    }

    zip_int64_t num_entries = zip_get_num_entries(z, 0);
    for (zip_uint64_t i = 0; i < num_entries; i++) {
        const char *name = zip_get_name(z, i, 0);
        struct zip_file *zf = zip_fopen_index(z, i, 0);

        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", dest_path, name);

        if (name[strlen(name) - 1] == '/') {
            mkdir(full_path, 0755);
        } else {
            char dir_path[512];
            strcpy(dir_path, full_path);
            char *last_slash = strrchr(dir_path, '/');
            if (last_slash) {
                *last_slash = '\0';
                mkdir(dir_path, 0755);
            }

            FILE *f = fopen(full_path, "wb");
            if (!f) continue;

            char buf[1024];
            zip_int64_t n;
            while ((n = zip_fread(zf, buf, sizeof(buf))) > 0) {
                fwrite(buf, 1, n, f);
            }
            fclose(f);
        }

        zip_fclose(zf);
    }

    zip_close(z);
}

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

int is_valid_base64(const char *str) {
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
        if (!((str[i] >= 'A' && str[i] <= 'Z') ||
              (str[i] >= 'a' && str[i] <= 'z') ||
              (str[i] >= '0' && str[i] <= '9') ||
              str[i] == '+' || str[i] == '/' || str[i] == '=')) {
            return 0;
        }
    }
    return 1;
}

void base64_decode(const char *input, char *output) {
    const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int val = 0, valb = -8;
    int len = strlen(input);
    int pos = 0;

    char cleaned_input[256];
    int ci = 0;

    for (int i = 0; i < len; i++) {
        if (input[i] != '\n' && input[i] != '\r') {
            cleaned_input[ci++] = input[i];
        }
    }
    cleaned_input[ci] = '\0';

    for (int i = 0; i < ci; i++) {
        char *p = strchr(table, cleaned_input[i]);
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

    size_t out_len = strlen(output);
    if (out_len > 0 && (output[out_len - 1] == '\n' || output[out_len - 1] == '\r')) {
        output[out_len - 1] = '\0';
    }
}

void decrypt_filenames() {
    DIR *dir = opendir(QUARANTINE_DIR);
    struct dirent *entry;
    if (!dir) return;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            if (!is_valid_base64(entry->d_name)) continue;

            char oldpath[512], newpath[512], decoded[256];
            snprintf(oldpath, sizeof(oldpath), "%s/%s", QUARANTINE_DIR, entry->d_name);
            base64_decode(entry->d_name, decoded);

            if (strlen(decoded) == 0 || strcmp(entry->d_name, decoded) == 0) continue;

            snprintf(newpath, sizeof(newpath), "%s/%s", QUARANTINE_DIR, decoded);
            rename(oldpath, newpath);

            char msg[512];
            snprintf(msg, sizeof(msg), "%.200s - Successfully decrypted to %.200s.", entry->d_name, decoded);
            write_log("Decrypt:", msg);
        }
    }

    closedir(dir);
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
            snprintf(msg, sizeof(msg) - 1, "%s - Successfully deleted.", entry->d_name);
            msg[sizeof(msg) - 1] = '\0';
            write_log("Eradicate:", msg);
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (access(STARTER_KIT_DIR, F_OK) != 0) {
        printf("Downloading and extracting starter kit...\n");
        download_zip(ZIP_URL, ZIP_FILE);
        mkdir(STARTER_KIT_DIR, 0755);
        unzip_file(ZIP_FILE, STARTER_KIT_DIR);
        remove(ZIP_FILE);
        printf("Starter kit ready.\n");
    }

    mkdir(STARTER_KIT_DIR, 0755);
    mkdir(QUARANTINE_DIR, 0755);

    if (argc != 2) {
        printf("Usage:\n");
        printf("./starterkit --decrypt\n");
        printf("./starterkit --quarantine\n");
        printf("./starterkit --return\n");
        printf("./starterkit --eradicate\n");
        return 1;
    }

    if (strcmp(argv[1], "--decrypt") == 0) {
        decrypt_filenames();
    } else if (strcmp(argv[1], "--quarantine") == 0) {
        move_files(STARTER_KIT_DIR, QUARANTINE_DIR, "Quarantine:", "%s - Successfully moved to quarantine directory.");
    } else if (strcmp(argv[1], "--return") == 0) {
        move_files(QUARANTINE_DIR, STARTER_KIT_DIR, "Return:", "%s - Successfully returned to starter kit directory.");
    } else if (strcmp(argv[1], "--eradicate") == 0) {
        delete_quarantine_files();
    } else {
        printf("=============================================\n");
        printf("Please use another argument.\n");
        printf("=============================================\n");
        return 1;
    }

    return 0;
}
