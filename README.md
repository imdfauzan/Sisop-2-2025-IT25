# Anggota Kelompok
5027241013. Tiara Putri Prasetya
5027241037. Danuja Prasasta Bastu
5027241100. Imam Mahmud Dalil Fauzan

# ========================= SOAL 1 =========================
Cara kerja dan isi dari program ini:

1. Download dan Ekstrak File ZIP
Jika di-run tanpa argumen, program akan mengecek apakah folder Clues/ sudah ada, jika tdk ada maka akan mendownload file Clues.zip dari URL Google Drive menggunakan libcurl lalu mengekstraknya menggunakan libzip lalu menghapus file ZIP setelah berhasil diekstrak.

2. Filter File .txt Valid
program ini juga bisa mengecek folder Clues/ClueA, ClueB, ClueC, ClueD lalu menyaring file .txt valid (namanya harus terdiri dari 1 karakter [huruf/angka] diikuti .txt, contoh: 1.txt, A.txt), lalu memindahkan file valid ke folder Filtered/.

3. Gabungkan Isi File .txt
program juga membaca file dari folder Filtered/, mengelompokkan nama file menjadi dua yaitu yang diawali angka dan diawali huruf, mengurutkan keduanya (secara alfabet) serta menggabungkan isi file yaitu dengan mengambil satu karakter pertama dari file angka lalu dari file huruf secara bergantian, dan menyimpannya ke Combined.txt.

4. Dekripsi ROT13
yang terakhir program akan membuka Combined.txt, mendekripsinya dengan algoritma ROT13 (setiap huruf digeser 13 posisi), Menyimpan hasil dekripsi ke Decoded.txt.

CARA PENGGUNAAN
1. Compile Program
Pastikan kamu sudah install libcurl dan libzip, lalu gunakan:

gcc last.c -o action -lcurl -lzip

2. Menjalankan Program
Perintah : 		|| Fungsi
./action 		|| Download dan ekstrak Clues.zip jika belum ada.
./action -m Filter	||  	Filter file .txt valid ke folder Filtered/.
./action -m Combine	|| Gabungkan isi file Filtered/ ke Combined.txt.
./action -m Decode	|| Dekripsi Combined.txt menjadi Decoded.txt.

ada beberapa revisi yang telah dilakukan,
1. menghapus isi dari folder ClueA, ClueB, ClueC, dan ClueD setelah melakukan filterisasi
2. menghapus isi folder filtered setelah melakukan command combine

# ========================= SOAL 2 =========================
1. ./starterkit --decrypt

Fitur decrypt digunakan untuk mendecrypt nama-nama file yang telah dienkripsi menggunakan algoritma Base64. File ini ada di dalam folder quarantine, dan saat didecrypt, nama file-nya jadi bisa dibaca oleh manusia.
Didalamnya ada fungsi 
-is_valid_base64()
yaitu untuk mengecek apakah nama file valid base64. jika iya, berarti file ini perlu didecrypt.
-base64_decode()
yaitu untuk men decode nama file dari base64 ke nama asli.
-decrypt_filenames()
yaitu untuk membaca semua file di folder quarantine, mengecek file nya va;id atau tidak, me decode nama file dan menuliskan log aktivitas decrypt di activity.log.

2. ./starterkit --quarantine
Fitur quarantine digunakan untuk memindahkan file dari folder starter_kit ke quarantine. 
Didalamnya ada fungsi 
move_files("starter_kit", "quarantine", "[QUARANTINE]", "%s -> %s\n")
yaitu untuk membuka folder starter_kit dan membaca semua file di dalamnya lalu memindahkan ke folder quarantine dan mencatat activity.log 

3. ./starterkit --return
Fitur return digunakan untuk mengembalikan file dari folder quarantine ke starter_kit
Didalamnya ada fungsi
move_files("quarantine", "starter_kit", "[RETURN]", "%s -> %s\n");
yaitu untuk membuka folder quarantine dan membaca semua file di dalamnya lalu memindahkan ke folder starter_kit lagi dan mencatat activity.log 

4. ./starterkit --eradicate
Fitur eradicate digunakan untuk menghapus semua file yang ada di folder quarantine
Didalamnya ada fungsi
delete_quarantine_files() 
yaitu untuk membuka folder, membaca file dan menghapus semua file lalu menulis activity.log

5. ./starterkit --shutdown
Fitur shutdown digunakan untuk menghentikan proses daemon decrypt yang sedang berjalan. 
Didalamnya ada fungsi
-kill(pid, SIGTERM);
yaiufungsi untuk mengirim sinyal ke proses ID (PID) yang disimpan di file decrypt_daemon.pid.
-decrypt_filenames() 
yaitu fungsi agar pid disimpan 

6. ZIP download and extract
Fungsi utama yang dipakai:
-download_zip()
yaitu mengguakan libcurl, mendownload file dari URL ZIP_URL,  dan disimpan sebagai starter_kit.zip.
-unzip_file()
yaitu ntuk membuka file ZIP, dan mengekstrak isi ke folder starter_kit.
-remove("starter_kit.zip");
yaitu menghapus file ZIP setelah ekstrak selesai.
-gcc starterkit.c -o starterkit -lzip -lcurl
Digunakan untuk meng-compile file C (starterkit.c) menjadi program executable (starterkit), dengan bantuan dua library eksternal: libzip(download file dari internet lewat link Google Drive) dan libcurl(mengekstrak isi file ZIP yang sudah didownload).

# ========================= SOAL 3 =========================
-

# ========================= SOAL 4 =========================
A. LIST PROSES USER
./debugmon list <username>

Menampilkan semua proses (username) yg sedang berjalan, isinya:
- PID
- nama proses (command)
- cpu usage
- memori usage
sumber data dari /proc, dan CPU/memory dihitung dari file /proc/[pid]/stat serta /proc/[pid]/status.

B. MODE DAEMON ON
./debugmon daemon <username>
Me-run program sbg. daemon (latar belakang) untuk memantau proses milik <username> terus-menerus setiap 10 detik. Jika user diblokir, maka semua prosesnya langsung dihentikan.
PID dari daemon disimpan ke file debugmon_<username>.pid.

C. STOP MODE DAEMON
./debugmon stop <username>
Menghentikan proses daemon yang memantau <username>. Cara kerja:
- Membaca PID dari file debugmon_<username>.pid
- Mengirim sinyal SIGTERM ke proses tersebut
- Menghapus file .pid

D. FAIL USER (MEMBLOKIR USER)
./debugmon fail <username>
Cara kerja:
- Membunuh semua proses milik user <username> (kecuali debugmon)
- Mencatat semua proses yang dibunuh dengan status FAILED ke file log
- Menandai user sebagai "diblokir" dengan membuat file /tmp/.debugmon_blocked_<username>
- Saat daemon mendeteksi user dalam kondisi diblokir, semua prosesnya akan terus dibunuh setiap 10 detik.

E. REVERT USER (MEMBUKA BLOKIR USER)
./debugmon revert <username>
Cara kerja:
- Menghapus file /tmp/.debugmon_blocked_<username>
- Mengizinkan user <username> kembali menjalankan proses
- Mencatat aktivitas ini ke log dengan status RUNNING





