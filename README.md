# Backuplist 2

**English** · [Русский](README.ru.md)

A small catalog for backups on optical discs (Blu-ray, DVD). It keeps track of
what is on every disc, finds files across all of them, and checks with SHA-256
whether an old disc is still readable.

One portable 32-bit `.exe` with SQLite built in. Runs on **Windows XP SP2 through
Windows 11**, needs no installer and no runtime. Created with Claude Opus 5.5.

<img width="1008" height="691" alt="{E3D2028F-1599-49B0-AE27-20B5E0A8EFB0}" src="https://github.com/user-attachments/assets/587515a9-c9d2-4215-b009-a9c423b4a22e" />


## Features

- **Disc catalog**: number, media type, burn date, content type, contents,
  priority, brand, Media ID. Dead discs are highlighted red, partially readable ones yellow.
- **Full-text search** across disc contents *and* file paths, by word prefix,
  case-insensitive, Cyrillic included.
- **Automatic disc detection.** Insert a disc and the program finds it by the volume
  serial number. Unknown discs can be added or linked to an existing entry.
- **Adding a disc** reads it completely and stores SHA-256 of every file.
  The card is pre-filled: next number, today's date, label, serial, top-level folders.
- **Verification** (F6) re-reads the disc and compares it with the stored hashes:
  OK / partially readable / dead, a check history in the database and a text report
  listing unreadable, corrupted, missing and extra files.
- **Media ID from the drive** (e.g. `VERBAT-IMe-000`, `CMCMAG-DI6-000`,
  `MBIPG101R05`), read with SCSI commands like ImgBurn does. The media kind
  (BD-R 25/50/100, DVD 5/9) is taken from the drive as well.
- Reading runs in a background thread with progress, speed and time left.
  Cancel works even on a disc stuck at a bad sector, and nothing is written then.
- **English and Russian UI**, switchable at runtime (File → Language / Язык).

<img width="479" height="375" alt="{89DC6F55-18B2-4D77-AD6A-2C108B3A45CD}" src="https://github.com/user-attachments/assets/2cfa8033-e00d-4f71-8452-c2d39094add7" />

## Download

Get `backuplist.exe` from the [Releases](https://github.com/evg00000/backuplist2/releases)
page and put it into **a folder you can write to** (not `Program Files`).
On the first start it creates an empty `backuplist.db` next to itself.

## Quick start

1. Insert a disc. Backuplist asks whether to add it — answer **Yes**,
   check the card and press **OK**. The disc is read and its hashes are stored.
2. Later, insert any catalogued disc: it is found and selected automatically.
3. Press **F6** to verify it. The result goes to the status column, the check history
   and to `reports\disk_NNN_YYYY-MM-DD.txt`.

Discs you catalogued before without hashes: select the entry, insert the disc,
press **F6** — the serial number is linked and the hashes are stored as the reference.

| Key | Action |
|---|---|
| F5 | Detect the inserted disc |
| F6 | Verify the inserted disc against stored hashes |
| Ctrl+N | Add a new disc |
| Ctrl+I | Media information (type, layers, Media ID) |
| Enter / double click | Disc card |
| Del | Delete the entry (the disc itself is not touched) |
| Ctrl+F | Search |
| Ctrl+O | Open another database |

**Priority** is yours to set: 1 — irreplaceable (personal archive), 2 — hard to find,
3 — replaceable. Together with the status it tells you which discs to re-burn first.

## Media ID notes

- Works for BD-R/RE, DVD±R/RW. CDs have only an ATIP code, which is not read.
- On Windows XP with a limited account Windows blocks SCSI commands — run as administrator.
- Some USB-to-SATA adapters do not pass SCSI commands; the program says so
  and everything else keeps working.

## Files next to the exe

| File | What it is |
|---|---|
| `backuplist.db` | the database (SQLite) — back it up too |
| `backuplist.ini` | the chosen UI language |
| `reports\` | verification reports, UTF-8 text |

The database is plain SQLite, so you can query it with any SQLite tool.
The schema is in [`docs/schema.sql`](docs/schema.sql). Handy views:

```sql
SELECT * FROM v_media_stats;              -- survival by Media ID: ok / partial / dead
SELECT * FROM v_attention;                -- what needs care first
SELECT * FROM v_copies WHERE copies = 1;  -- files that exist on one disc only
```

## Building

The code is plain C (gnu99) with the Win32 API and no other dependencies.

**Linux** (Debian/Ubuntu):

```sh
sudo apt install gcc-mingw-w64-i686 make
make          # build/backuplist.exe
make test     # Media ID parser tests (native gcc)
```

**Windows**: [w64devkit](https://github.com/skeeto/w64devkit) **i686** or MSYS2 **MINGW32**
in `PATH`, then `build.bat`. Use a toolchain with the **msvcrt** runtime:
UCRT builds do not start on XP.

GitHub Actions builds every push; a `v*` tag publishes a release with the exe.

### Layout

```
src/main.c            main window, commands, menus
src/dialogs.c         disc card, progress window
src/db.c              every SQL query; schema for a new database
src/scan.c            walking the disc and hashing in a worker thread
src/sha256.c          SHA-256 (CryptoAPI on XP SP2 has none)
src/verify.c          comparison with the reference, text report
src/volume.c          volume label/serial, WM_DEVICECHANGE
src/mediaid.c         SCSI pass-through to the drive
src/mediaid_parse.c   parsing drive replies (unit-tested)
src/lang_strings.h    all UI strings, English and Russian
third_party/sqlite/   SQLite 3.49.2 amalgamation (public domain)
```

### Adding a translation

Every UI string lives in `src/lang_strings.h` as `X(id, english, russian)`.
Add a column there, a table and a menu item in `src/lang.c` / `src/main.c`, and you are done.

## License

[Unlicense](LICENSE) — public domain. SQLite is public domain too.
