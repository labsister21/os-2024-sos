# Create an 32-bit x86 Architecture Operating System
> *Source Code* ini dibuat oleh kami, Kelompok ApaGaKeOS, untuk memenuhi Tugas Besar Sistem Operasi yaitu membuat
> Sistem operasi yang akan dibuat akan berjalan pada arsitektur x86 32-bit

## Daftar Isi
- [Author](#author)
- [Deskripsi Singkat](#deskripsi-singkat)
- [Sistematika File](#sistematika-file)
- [Requirements](#requirements)
- [Cara Mengkompilasi dan Menjalankan Program](#cara-mengkompilasi-dan-menjalankan-program)

## Author
| NIM      | Nama                       | 
| -------- | -------------------------- |
| 13522052 | Haikal Assyauqi            | 
| 13522054 | Benjamin Sihombing         | 
| 13522086 | Muhammad Atpur Rafif       | 
| 13522088 | Muhammda Rafli Rasyiidin   |

## Deskripsi Singkat
Tugas ini akan membuat sebuah program sistem operasi. Sistem operasi yang akan dibuat akan berjalan pada arsitektur x86 32-bit yang nanti akan dijalankan dengan emulator QEMU. Tugas ini akan dibagi menjadi beberapa milestone.

### Milestone 0 - Pembuatan Sistem Operasi x86 Toolchain, Kernel, GDT
Waktu implementasi : Senin, 26 Februari 2024 - Sabtu, 9 Maret 2024
1. Menyiapkan repository dan tools
2. Kernel dasar
3. Otomatisasi build
4. Menjalankan sistem operasi
5. Pembuatan struktur data GDT
6. Load GDT

### Milestone 1 - Pembuatan Sistem Operasi x86, Interrupt, Driver, File System

Waktu implementasi : Minggu, 10 Maret 2024 - Sabtu, 6 April 2024 
1. Text Framebuffer 
2. Interrupt
3. Keyboard Driver
4. Filesystem
5. [BONUS] Dukungan CMOS time untuk *file system*

### Milestone 2 - Pembuatan Sistem Operasi x86 Paging, User Mode, Shell

Waktu implementasi : Minggu, 7 April 2024 - Sabtu, 27 April 2024
1. Manajemen Memori
2. Separasi Kernel-User Space
3. Shell

### Milestone 3 - Pembuatan Sistem Operasi x86 Process, Scheduler, Multitasking

Waktu implementasi : Minggu, 28 April 2024 - Senin, 20 Mei 2024
1. Menyiapkan struktur untuk proses
2. Membuat task scheduler & context switch
3. Membuat perintah shell
4. Menjalankan Multitasking


## Sistematika File
```bash
.
├─── bin
├─── other
├─── src
│   ├─── helper
│   ├─── kernel
│   ├    ├─── asm
│   ├    ├─── c
│   ├    ├    ├─── boot
│   ├    ├    ├─── cpu
│   ├    ├    ├─── driver
│   ├    ├    ├─── filesystem
│   ├    ├    ├─── memory
│   ├    ├    ├─── process
│   ├    ├    ├─── text   
│   ├    ├─── header
│   ├    ├    ├─── boot
│   ├    ├    ├─── cpu
│   ├    ├    ├─── driver
│   ├    ├    ├─── filesystem
│   ├    ├    ├─── memory
│   ├    ├    ├─── process
│   ├    ├    ├─── text
│   ├─── program
│   ├    ├─── clock
│   ├    ├─── ping
│   ├    ├─── shell
│   ├─── shared
│   ├    ├─── code
│   ├    ├─── header
│   ├    ├    ├─── std
│   ├─── linker.ld
│   └─── menu.lst
├─── makefile
└─── README.md
```

## Requirements
- GCC compiler (versi 11.2.0 atau yang lebih baru)
- Visual Studio Code
- Windows Subsystem for Linux (WSL2) dengan distribusi minimal Ubuntu 20.04
- Emulator QEMU

## Cara Mengkompilasi dan Menjalankan Program
1. Lakukan *clone repository* melalui terminal dengan *command* berikut
    ``` bash
    $ git clone https://github.com/Sister20/if2230-2023-apagakeos.git
    ```
2. Lakukan eksekusi pada makefile dengan memasukkan *command* `make all` pada terminal. Jika berhasil maka akan tercipta beberapa file pada folder `bin`
3. Jalan sistem oprerasi dengan membuka Visual Studio Code dan jalankan `Shift + F5`. Pastikan QEMU yang digunakan sudah aktif sebelumnya. Jika proses aktivasi tidak berhasil, maka gunakan [Panduan Debugger dan WSL](https://docs.google.com/document/d/1Zt3yzP_OEiFz8g2lHlpBNNr9qUyXghFNeQlAeQpAaII/edit#). 

Jika berhasil, maka sistem operasi akan muncul pada layar.