# Anggota Kelompok
5027241013. Tiara Putri Prasetya
5027241037. Danuja Prasasta Bastu
5027241100. Imam Mahmud Dalil Fauzan

# ========================= SOAL 1 =========================
Isi dan cara kerja kode ini:

1. mendownload dan Ekstrak File ZIP
Jika di run, program akan mengecek apakah folder Clues/ ada atau tidak, jika belum ada maka program akan Mendownload file Clues.zip dari URL Google Drive menggunakan libcurl, lalu mengekstraknya menggunakan libzip, dan yang terakhir menghapus file ZIP setelah berhasil diekstrak.

2. Filter File .txt Valid
program ini juga bisa mengecek folder Clues/ClueA, ClueB, ClueC, ClueD lalu memfilter file .txt valid (namanya harus terdiri dari 1 char [huruf/angka] difollow dengan .txt, contoh: 1.txt, A.txt), lalu memindahkan file yang memnuhi ke folder Filtered/.

3. Combine Isi File .txt
program ini juga membaca file dari folder Filtered/, mengelompokkan nama file menjadi dua yaitu yang awalnya berupa angka dan huruf, keduanya (secara alfabet) akan diurutkan setelah itu menggabungkan isi file yaitu dengan mengambil karakter pertama dari file angka lalu dari file huruf secara bergantian, dan di save di file Combined.txt.

4. Dekripsi ROT13
yang terakhir program akan membuka Combined.txt, decode file itu dengan algoritma ROT13 (setiap huruf digeser 13 posisi), lalu disimpan hasil decoded file ke Decoded.txt.

CARA PENGGUNAAN
1. Compile Program
Pastikan sudah install libcurl dan libzip, lalu compile file dengan:

gcc last.c -o action -lcurl -lzip

2. jalankan Program
Perintah : 		|| Fungsi
./action 		|| Download dan ekstrak Clues.zip jika belum ada.
./action -m Filter	|| Filter file .txt valid ke folder Filtered/.
./action -m Combine	|| Gabungkan isi file Filtered/ ke Combined.txt.
./action -m Decode	|| Dekripsi Combined.txt menjadi Decoded.txt.

# ========================= SOAL 2 =========================
A. Mendownload link, unzip dan menghapus file zip

B. ./starterkit --decrypt
Meminta proses berjalan otomatis di background dan mengubah nama file dari hasil decode. 
daemonize() – membuat proses jadi daemon biar jalan di background.
fork() → agar program dapat mengkloning dirinya.
setsid() → agar putus hubungan dari terminal.
chdir("/") → memindah ke direktori root agak tidak mengunci direktori lama.
umask(0) → reset permission.
close() → menutup input/output agar tidak mengganggu terminal
base64_decode() – decode nama file dari Base64 ke string asli.
rename() – mengganti nama file hasil decode.
log_decrypt() – catat PID dan waktu mulai ke activity.log.

C. ./starterkit --quarantine 
Memindahkan file ke quarantine
move_to_quarantine() – memindah file dari folder utama ke folder quarantine.

./starterkit --return
Memindahkan file ke semula
move_to_starterkit() – memindah file dari folder quarantine balik ke folder utama (starter_kit)

D. ./starterkit --eradicate 
Membersihkan semua file karantina
delete_quarantine_files() – menghapus semua file dalam folder quarantine

E. ./starterkit --shutdown
Menghentikan proses daemon
read_pid() – mengambil PID daemon dari file.
kill(pid, SIGTERM) – mematikan daemon.
log_shutdown() – catat waktu dan hasil shutdown

# ========================= SOAL 3 =========================
-

# ========================= SOAL 4 =========================
A. LIST PROSES USER
./debugmon list <username>

Menampilkan semua proses (username) yg sedang berjalan, isinya:
- PID
- Nama proses (command)
- cpu usage
- memori usage
Data diambil dari /proc, dan CPU/memory dihitung dari file /proc/[pid]/stat serta /proc/[pid]/status.

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





