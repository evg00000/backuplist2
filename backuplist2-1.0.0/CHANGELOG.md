# Changelog

## 1.0.0 — 2026-11-15

First public release.

- English and Russian interface, switchable at runtime (File → Language / Язык);
  the choice is kept in `backuplist.ini`, the default follows the Windows UI language.
- A new empty database is created on the first start; File → New database.
- Media ID and media kind read from the drive (BD via Disc Information,
  DVD+R via ADIP, DVD-R via pre-pit data); Ctrl+I shows media information.
- Adding a disc reads it completely and stores SHA-256 of every file.
- Verification against the stored hashes with a check history and a text report.
- Disc card editing, entry deletion, unlinking a serial number.
- Disc auto-detection by the volume serial number.
- Full-text search over disc contents and file paths (SQLite FTS5).
