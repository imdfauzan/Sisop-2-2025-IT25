#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <curl/curl.h>
#include <zip.h>

#define ZIP_URL "https://drive.usercontent.google.com/u/0/uc?id=1xFn1OBJUuSdnApDseEczKhtNzyGekauK&export=download"
#define ZIP_NAME "Clues.zip"
#define CLUES_FOLDER "Clues"
#define FILTERED_FOLDER "Filtered"
#define COMBINED_FILE "Combined.txt"
#define DECODED_FILE "Decoded.txt"

// ========== Download File ==========
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
}

void download_file(const char *url, const char *output_file) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "CURL init failed\n");
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

        if (name[strlen(name)-1] == '/') {
            mkdir(full_path, 0755);
        } else {
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

int is_valid_txt_file(const char *filename) {
    size_t len = strlen(filename);
    if (len != 5) return 0;
    if (strcmp(filename + 1, ".txt") != 0) return 0;
    if (!isalpha(filename[0]) && !isdigit(filename[0])) return 0;
    return 1;
}

void copy_file(const char *src_path, const char *dest_path) {
    FILE *src = fopen(src_path, "rb");
    FILE *dest = fopen(dest_path, "wb");

    if (!src || !dest) {
        perror("File error");
        return;
    }

    char buffer[1024];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, n, dest);
    }

    fclose(src);
    fclose(dest);
}

void filter_txt_files() {
    const char *folders[] = {
        "Clues/ClueA", "Clues/ClueB", "Clues/ClueC", "Clues/ClueD"
    };

    mkdir(FILTERED_FOLDER, 0755);

    for (int i = 0; i < 4; i++) {
        DIR *dir = opendir(folders[i]);
        if (!dir) continue;

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (!is_valid_txt_file(entry->d_name)) continue;

            char src[512], dest[512];
            snprintf(src, sizeof(src), "%s/%s", folders[i], entry->d_name);
            snprintf(dest, sizeof(dest), "%s/%s", FILTERED_FOLDER, entry->d_name);

            copy_file(src, dest);
        }
        closedir(dir);
    }

    printf("Filtered done.\n");
}

int compare(const void *a, const void *b) {
    const char *fa = *(const char **)a;
    const char *fb = *(const char **)b;
    return strcmp(fa, fb);
}

void combine_filtered() {
    DIR *dir = opendir(FILTERED_FOLDER);
    if (!dir) return;

    char *digit_files[100], *alpha_files[100];
    int d_count = 0, a_count = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!is_valid_txt_file(entry->d_name)) continue;

        if (isdigit(entry->d_name[0])) digit_files[d_count++] = strdup(entry->d_name);
        else if (isalpha(entry->d_name[0])) alpha_files[a_count++] = strdup(entry->d_name);
    }

    closedir(dir);
    qsort(digit_files, d_count, sizeof(char*), compare);
    qsort(alpha_files, a_count, sizeof(char*), compare);

    FILE *out = fopen(COMBINED_FILE, "w");
    int i = 0;
    while (i < d_count || i < a_count) {
        if (i < d_count) {
            char path[512];
            snprintf(path, sizeof(path), "%s/%s", FILTERED_FOLDER, digit_files[i]);
            FILE *f = fopen(path, "r");
            if (f) {
                int c = fgetc(f);
                if (c != EOF) fputc(c, out);
                fclose(f);
            }
        }
        if (i < a_count) {
            char path[512];
            snprintf(path, sizeof(path), "%s/%s", FILTERED_FOLDER, alpha_files[i]);
            FILE *f = fopen(path, "r");
            if (f) {
                int c = fgetc(f);
                if (c != EOF) fputc(c, out);
                fclose(f);
            }
        }
        i++;
    }

    fclose(out);
    printf("Combined to %s\n", COMBINED_FILE);
}

char rot13_char(char c) {
    if ('A' <= c && c <= 'Z') return ((c - 'A' + 13) % 26) + 'A';
    if ('a' <= c && c <= 'z') return ((c - 'a' + 13) % 26) + 'a';
    return c;
}

void decode_rot13_file(const char *input, const char *output) {
    FILE *in = fopen(input, "r");
    FILE *out = fopen(output, "w");

    if (!in || !out) {
        perror("Open failed");
        return;
    }

    int c;
    while ((c = fgetc(in)) != EOF) {
        fputc(rot13_char(c), out);
    }

    fclose(in);
    fclose(out);
    printf("Decoded to %s\n", output);
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        if (access(CLUES_FOLDER, F_OK) == 0) {
            printf("Clues folder already exists, skipping download.\n");
        } else {
            download_file(ZIP_URL, ZIP_NAME);
            unzip_file(ZIP_NAME, ".");
            remove(ZIP_NAME);
        }
    } else if (argc == 3 && strcmp(argv[1], "-m") == 0) {
        if (strcmp(argv[2], "Filter") == 0) {
            filter_txt_files();
        } else if (strcmp(argv[2], "Combine") == 0) {
            combine_filtered();
        } else if (strcmp(argv[2], "Decode") == 0) {
            decode_rot13_file(COMBINED_FILE, DECODED_FILE);
        } else {
            printf("Unknown mode: %s\n", argv[2]);
        }
    } else {
        printf("Usage:\n");
        printf("  ./action             -> Download & unzip if needed\n");
        printf("  ./action -m Filter   -> Filter txt to Filtered/\n");
        printf("  ./action -m Combine  -> Merge txt in order to Combined.txt\n");
        printf("  ./action -m Decode   -> Decode Combined.txt to Decoded.txt\n");
    }

    return 0;
}
