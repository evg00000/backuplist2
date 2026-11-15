/* db.c — all database queries. SQLite stores text as UTF-8; we hand out UTF-16
   through the *_text16 API, so no manual conversions are needed. */
#include <stdio.h>
#include <string.h>
#include "sqlite3.h"
#include "db.h"
#include "lang.h"

static sqlite3 *g_db;
static wchar_t  g_err[512];

static void set_err(void)
{
    const wchar_t *m = g_db ? (const wchar_t *)sqlite3_errmsg16(g_db) : T(S_E_DB_NOT_OPEN);
    wcsncpy(g_err, m ? m : L"?", 511);
    g_err[511] = 0;
}

const wchar_t *db_error(void) { return g_err; }
int db_is_open(void) { return g_db != NULL; }

int db_open(const wchar_t *path)
{
    db_close();
    if (sqlite3_open16(path, &g_db) != SQLITE_OK) {
        set_err();
        sqlite3_close(g_db);
        g_db = NULL;
        return 0;
    }
    /* Make sure this is really a Backuplist database */
    if (sqlite3_exec(g_db, "PRAGMA foreign_keys=ON; SELECT 1 FROM disc LIMIT 1;",
                     NULL, NULL, NULL) != SQLITE_OK) {
        set_err();
        db_close();
        return 0;
    }
    return 1;
}

void db_close(void)
{
    if (g_db) sqlite3_close(g_db);
    g_db = NULL;
}

static const wchar_t *col16(sqlite3_stmt *st, int i)
{
    const void *p = sqlite3_column_text16(st, i);
    return p ? (const wchar_t *)p : L"";
}

static int status_code(const wchar_t *s)
{
    if (!wcscmp(s, L"ok"))      return ST_OK;
    if (!wcscmp(s, L"partial")) return ST_PARTIAL;
    if (!wcscmp(s, L"dead"))    return ST_DEAD;
    return ST_UNKNOWN;
}

static const char *SORT_SQL[SORT_COUNT] = {
    "id",
    "disc_type, capacity_gb",
    "content_type",
    "CASE status WHEN 'dead' THEN 0 WHEN 'partial' THEN 1 WHEN 'ok' THEN 2 ELSE 3 END",
    "priority",
    "last_checked",
    "media_id",
    "vol_serial",
    "contents",
};

int db_list_discs(const wchar_t *fts, int sortCol, int desc, DiscCb cb, void *ctx)
{
    char sql[1024];
    sqlite3_stmt *st;
    int n = 0, filtered = fts && *fts;

    if (!g_db) return -1;
    if (sortCol < 0 || sortCol >= SORT_COUNT) sortCol = SORT_ID;

    snprintf(sql, sizeof sql,
        "SELECT id, disc_type || ' ' || capacity_gb, content_type, status, priority,"
        "       last_checked, media_id, vol_serial, contents"
        "  FROM disc %s ORDER BY %s %s, id",
        filtered ? "WHERE id IN (SELECT disc_id FROM search WHERE search MATCH ?1)" : "",
        SORT_SQL[sortCol], desc ? "DESC" : "ASC");

    if (sqlite3_prepare_v2(g_db, sql, -1, &st, NULL) != SQLITE_OK) { set_err(); return -1; }
    if (filtered) sqlite3_bind_text16(st, 1, fts, -1, SQLITE_TRANSIENT);

    for (;;) {
        int rc = sqlite3_step(st);
        DiscRow r;
        if (rc == SQLITE_DONE) break;
        if (rc != SQLITE_ROW) { set_err(); n = -1; break; }
        r.id       = sqlite3_column_int(st, 0);
        r.type     = col16(st, 1);
        r.ctype    = col16(st, 2);
        r.status   = status_code(col16(st, 3));
        r.priority = sqlite3_column_int(st, 4);
        r.checked  = col16(st, 5);
        r.media    = col16(st, 6);
        r.serial   = col16(st, 7);
        r.contents = col16(st, 8);
        cb(ctx, &r);
        n++;
    }
    sqlite3_finalize(st);
    return n;
}

int db_list_files(int discId, const wchar_t *fts, FileCb cb, void *ctx)
{
    sqlite3_stmt *st;
    int n = 0, filtered = fts && *fts;
    const char *sql = filtered
        ? "SELECT path, sha256 FROM file WHERE disc_id = ?1 AND path IN"
          " (SELECT text FROM search WHERE search MATCH ?2 AND disc_id = ?1 AND kind = 'path')"
          " ORDER BY path"
        : "SELECT path, sha256 FROM file WHERE disc_id = ?1 ORDER BY path";

    if (!g_db) return -1;
    if (sqlite3_prepare_v2(g_db, sql, -1, &st, NULL) != SQLITE_OK) { set_err(); return -1; }
    sqlite3_bind_int(st, 1, discId);
    if (filtered) sqlite3_bind_text16(st, 2, fts, -1, SQLITE_TRANSIENT);

    for (;;) {
        int rc = sqlite3_step(st);
        FileRow r;
        if (rc == SQLITE_DONE) break;
        if (rc != SQLITE_ROW) { set_err(); n = -1; break; }
        r.path   = col16(st, 0);
        r.sha256 = col16(st, 1);
        cb(ctx, &r);
        n++;
    }
    sqlite3_finalize(st);
    return n;
}

int db_find_by_serial(const wchar_t *serial)
{
    sqlite3_stmt *st;
    int id = 0;
    if (!g_db) return 0;
    if (sqlite3_prepare_v2(g_db, "SELECT id FROM disc WHERE vol_serial = ?1 LIMIT 1",
                           -1, &st, NULL) != SQLITE_OK) { set_err(); return 0; }
    sqlite3_bind_text16(st, 1, serial, -1, SQLITE_TRANSIENT);
    if (sqlite3_step(st) == SQLITE_ROW) id = sqlite3_column_int(st, 0);
    sqlite3_finalize(st);
    return id;
}

int db_disc_serial(int discId, wchar_t *out, int cch)
{
    sqlite3_stmt *st;
    int ok = 0;
    out[0] = 0;
    if (!g_db) return 0;
    if (sqlite3_prepare_v2(g_db, "SELECT vol_serial FROM disc WHERE id = ?1",
                           -1, &st, NULL) != SQLITE_OK) { set_err(); return 0; }
    sqlite3_bind_int(st, 1, discId);
    if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) {
        wcsncpy(out, col16(st, 0), cch - 1);
        out[cch - 1] = 0;
        ok = out[0] != 0;
    }
    sqlite3_finalize(st);
    return ok;
}

int db_bind_serial(int discId, const wchar_t *serial, const wchar_t *label)
{
    sqlite3_stmt *st;
    int ok;
    if (!g_db) return 0;
    if (sqlite3_prepare_v2(g_db, "UPDATE disc SET vol_serial = ?2, vol_label = ?3 WHERE id = ?1",
                           -1, &st, NULL) != SQLITE_OK) { set_err(); return 0; }
    sqlite3_bind_int(st, 1, discId);
    sqlite3_bind_text16(st, 2, serial, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text16(st, 3, label, -1, SQLITE_TRANSIENT);
    ok = sqlite3_step(st) == SQLITE_DONE;
    if (!ok) set_err();
    sqlite3_finalize(st);
    return ok;
}

int db_unbind_serial(int discId)
{
    sqlite3_stmt *st;
    int ok;
    if (!g_db) return 0;
    if (sqlite3_prepare_v2(g_db, "UPDATE disc SET vol_serial = NULL, vol_label = NULL WHERE id = ?1",
                           -1, &st, NULL) != SQLITE_OK) { set_err(); return 0; }
    sqlite3_bind_int(st, 1, discId);
    ok = sqlite3_step(st) == SQLITE_DONE;
    if (!ok) set_err();
    sqlite3_finalize(st);
    return ok;
}

static int scalar_int(const char *sql)
{
    sqlite3_stmt *st;
    int v = 0;
    if (sqlite3_prepare_v2(g_db, sql, -1, &st, NULL) == SQLITE_OK) {
        if (sqlite3_step(st) == SQLITE_ROW) v = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
    }
    return v;
}

void db_counts(int *discs, int *files)
{
    *discs = *files = 0;
    if (!g_db) return;
    *discs = scalar_int("SELECT count(*) FROM disc");
    *files = scalar_int("SELECT count(*) FROM file");
}

void db_make_fts_query(const wchar_t *in, wchar_t *out, int cch)
{
    int o = 0;
    out[0] = 0;
    while (*in) {
        /* skip separators */
        while (*in == L' ' || *in == L'\t' || *in == L'"') in++;
        if (!*in) break;
        if (o + 4 >= cch) break;
        if (o) out[o++] = L' ';
        out[o++] = L'"';
        while (*in && *in != L' ' && *in != L'\t' && o + 3 < cch) {
            if (*in != L'"') out[o++] = *in;   /* drop quotes inside a word */
            in++;
        }
        out[o++] = L'"';
        out[o++] = L'*';
        out[o] = 0;
    }
}

/* ======================================================================
   Disc card, files, checks
   ====================================================================== */

const MediaKind MEDIA_KINDS[] = {
    { L"BD-R 25",  L"BD-R", 25,  1 },
    { L"BD-R 50",  L"BD-R", 50,  2 },
    { L"BD-R 100", L"BD-R", 100, 3 },
    { L"DVD 5",    L"DVD",  5,   1 },
    { L"DVD 9",    L"DVD",  9,   2 },
    { L"CD",       L"CD",   1,   1 },
};
const int MEDIA_KIND_COUNT = sizeof MEDIA_KINDS / sizeof MEDIA_KINDS[0];

int media_kind_find(const wchar_t *type, int capacity)
{
    int i;
    for (i = 0; i < MEDIA_KIND_COUNT; i++)
        if (!wcscmp(MEDIA_KINDS[i].type, type) && MEDIA_KINDS[i].capacity == capacity)
            return i;
    return 0;
}

static const wchar_t *status_str(int st)
{
    switch (st) {
    case ST_OK:      return L"ok";
    case ST_PARTIAL: return L"partial";
    case ST_DEAD:    return L"dead";
    default:         return NULL;
    }
}

static sqlite3_stmt *prep(const char *sql)
{
    sqlite3_stmt *st = NULL;
    if (!g_db) { set_err(); return NULL; }
    if (sqlite3_prepare_v2(g_db, sql, -1, &st, NULL) != SQLITE_OK) { set_err(); return NULL; }
    return st;
}

/* Empty string -> NULL (matters for the serial: "" must never match "") */
static void bind_opt(sqlite3_stmt *st, int i, const wchar_t *s)
{
    if (s && *s) sqlite3_bind_text16(st, i, s, -1, SQLITE_TRANSIENT);
    else sqlite3_bind_null(st, i);
}

static int step_done(sqlite3_stmt *st)
{
    int ok = sqlite3_step(st) == SQLITE_DONE;
    if (!ok) set_err();
    sqlite3_finalize(st);
    return ok;
}

static void cpy(wchar_t *dst, int cch, sqlite3_stmt *st, int i)
{
    wcsncpy(dst, col16(st, i), cch - 1);
    dst[cch - 1] = 0;
}

static int exec(const char *sql)
{
    if (!g_db) return 0;
    if (sqlite3_exec(g_db, sql, NULL, NULL, NULL) != SQLITE_OK) { set_err(); return 0; }
    return 1;
}

int db_begin(void)    { return exec("BEGIN"); }
int db_commit(void)   { return exec("COMMIT"); }
void db_rollback(void) { if (g_db) sqlite3_exec(g_db, "ROLLBACK", NULL, NULL, NULL); }

int db_get_disc(int id, DiscInfo *d)
{
    sqlite3_stmt *st = prep(
        "SELECT disc_type, capacity_gb, written, content_type, contents, priority,"
        "       brand, media_id, vol_label, vol_serial, status, last_checked"
        "  FROM disc WHERE id = ?1");
    int found = 0;
    ZeroMemory(d, sizeof *d);
    if (!st) return 0;
    sqlite3_bind_int(st, 1, id);
    if (sqlite3_step(st) == SQLITE_ROW) {
        d->id = id;
        d->kind = media_kind_find(col16(st, 0), sqlite3_column_int(st, 1));
        cpy(d->written, 16, st, 2);
        cpy(d->ctype, 64, st, 3);
        cpy(d->contents, 4096, st, 4);
        d->priority = sqlite3_column_int(st, 5);
        cpy(d->brand, 64, st, 6);
        cpy(d->media, 64, st, 7);
        cpy(d->label, MAX_PATH_LABEL, st, 8);
        cpy(d->serial, 16, st, 9);
        d->status = status_code(col16(st, 10));
        cpy(d->checked, 16, st, 11);
        found = 1;
    }
    sqlite3_finalize(st);
    return found;
}

int db_id_exists(int id)
{
    sqlite3_stmt *st = prep("SELECT 1 FROM disc WHERE id = ?1");
    int found;
    if (!st) return 0;
    sqlite3_bind_int(st, 1, id);
    found = sqlite3_step(st) == SQLITE_ROW;
    sqlite3_finalize(st);
    return found;
}

int db_next_id(void)
{
    return g_db ? scalar_int("SELECT coalesce(max(id), 0) + 1 FROM disc") : 1;
}

int db_latest_disc(void)
{
    return g_db ? scalar_int("SELECT id FROM disc ORDER BY written DESC, id DESC LIMIT 1") : 0;
}

static int replace_contents_search(int id, const wchar_t *contents)
{
    sqlite3_stmt *st = prep("DELETE FROM search WHERE disc_id = ?1 AND kind = 'contents'");
    if (!st) return 0;
    sqlite3_bind_int(st, 1, id);
    if (!step_done(st)) return 0;
    st = prep("INSERT INTO search (disc_id, kind, text) VALUES (?1, 'contents', ?2)");
    if (!st) return 0;
    sqlite3_bind_int(st, 1, id);
    sqlite3_bind_text16(st, 2, contents, -1, SQLITE_TRANSIENT);
    return step_done(st);
}

int db_insert_disc(const DiscInfo *d)
{
    const MediaKind *k = &MEDIA_KINDS[d->kind];
    sqlite3_stmt *st = prep(
        "INSERT INTO disc (id, disc_type, capacity_gb, layers, written, content_type, contents,"
        "                  priority, status, last_checked, brand, media_id, vol_label, vol_serial)"
        " VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14)");
    if (!st) return 0;
    sqlite3_bind_int(st, 1, d->id);
    sqlite3_bind_text16(st, 2, k->type, -1, SQLITE_STATIC);
    sqlite3_bind_int(st, 3, k->capacity);
    sqlite3_bind_int(st, 4, k->layers);
    bind_opt(st, 5, d->written);
    bind_opt(st, 6, d->ctype);
    bind_opt(st, 7, d->contents);
    sqlite3_bind_int(st, 8, d->priority);
    bind_opt(st, 9, status_str(d->status));
    bind_opt(st, 10, d->checked);
    bind_opt(st, 11, d->brand);
    bind_opt(st, 12, d->media);
    bind_opt(st, 13, d->label);
    bind_opt(st, 14, d->serial);
    if (!step_done(st)) return 0;
    return replace_contents_search(d->id, d->contents);
}

int db_update_disc(const DiscInfo *d)
{
    const MediaKind *k = &MEDIA_KINDS[d->kind];
    sqlite3_stmt *st = prep(
        "UPDATE disc SET disc_type = ?2, capacity_gb = ?3, layers = ?4, written = ?5,"
        "       content_type = ?6, contents = ?7, priority = ?8, brand = ?9, media_id = ?10"
        " WHERE id = ?1");
    if (!st) return 0;
    sqlite3_bind_int(st, 1, d->id);
    sqlite3_bind_text16(st, 2, k->type, -1, SQLITE_STATIC);
    sqlite3_bind_int(st, 3, k->capacity);
    sqlite3_bind_int(st, 4, k->layers);
    bind_opt(st, 5, d->written);
    bind_opt(st, 6, d->ctype);
    bind_opt(st, 7, d->contents);
    sqlite3_bind_int(st, 8, d->priority);
    bind_opt(st, 9, d->brand);
    bind_opt(st, 10, d->media);
    if (!step_done(st)) return 0;
    return replace_contents_search(d->id, d->contents);
}

int db_delete_disc(int id)
{
    sqlite3_stmt *st = prep("DELETE FROM search WHERE disc_id = ?1");
    if (!st) return 0;
    sqlite3_bind_int(st, 1, id);
    if (!step_done(st)) return 0;
    st = prep("DELETE FROM disc WHERE id = ?1");   /* file and check_log rows go by cascade */
    if (!st) return 0;
    sqlite3_bind_int(st, 1, id);
    return step_done(st);
}

void db_distinct(const char *column, StrCb cb, void *ctx)
{
    char sql[160];
    sqlite3_stmt *st;
    if (strcmp(column, "content_type") && strcmp(column, "brand") && strcmp(column, "media_id"))
        return;
    snprintf(sql, sizeof sql,
             "SELECT DISTINCT %s FROM disc WHERE coalesce(%s, '') <> '' ORDER BY 1",
             column, column);
    st = prep(sql);
    if (!st) return;
    while (sqlite3_step(st) == SQLITE_ROW) cb(ctx, col16(st, 0));
    sqlite3_finalize(st);
}

int db_load_files(int discId, FileList *fl)
{
    sqlite3_stmt *st = prep("SELECT path, sha256, size FROM file WHERE disc_id = ?1");
    int n = 0;
    if (!st) return -1;
    sqlite3_bind_int(st, 1, discId);
    while (sqlite3_step(st) == SQLITE_ROW) {
        ScanFile *f = fl_add(fl, col16(st, 0));
        if (!f) break;
        wcsncpy(f->sha, col16(st, 1), 64);
        f->sha[64] = 0;
        if (sqlite3_column_type(st, 2) != SQLITE_NULL) {
            f->size = (unsigned long long)sqlite3_column_int64(st, 2);
            f->hasSize = 1;
        }
        n++;
    }
    sqlite3_finalize(st);
    fl_sort(fl);
    return n;
}

int db_store_files(int discId, const FileList *fl)
{
    sqlite3_stmt *st, *ins, *srch;
    int i, ok = 1;

    st = prep("DELETE FROM file WHERE disc_id = ?1");
    if (!st) return 0;
    sqlite3_bind_int(st, 1, discId);
    if (!step_done(st)) return 0;
    st = prep("DELETE FROM search WHERE disc_id = ?1 AND kind = 'path'");
    if (!st) return 0;
    sqlite3_bind_int(st, 1, discId);
    if (!step_done(st)) return 0;

    ins  = prep("INSERT INTO file (disc_id, path, sha256, size) VALUES (?1, ?2, ?3, ?4)");
    srch = prep("INSERT INTO search (disc_id, kind, text) VALUES (?1, 'path', ?2)");
    if (!ins || !srch) { sqlite3_finalize(ins); sqlite3_finalize(srch); return 0; }

    for (i = 0; i < fl->n && ok; i++) {
        const ScanFile *f = &fl->v[i];
        if (f->state != FS_OK || !f->sha[0]) continue;   /* no hash, no reference */
        sqlite3_bind_int(ins, 1, discId);
        sqlite3_bind_text16(ins, 2, f->path, -1, SQLITE_STATIC);
        sqlite3_bind_text16(ins, 3, f->sha, -1, SQLITE_STATIC);
        sqlite3_bind_int64(ins, 4, (sqlite3_int64)f->size);
        if (sqlite3_step(ins) != SQLITE_DONE) { set_err(); ok = 0; }
        sqlite3_reset(ins);

        sqlite3_bind_int(srch, 1, discId);
        sqlite3_bind_text16(srch, 2, f->path, -1, SQLITE_STATIC);
        if (ok && sqlite3_step(srch) != SQLITE_DONE) { set_err(); ok = 0; }
        sqlite3_reset(srch);
    }
    sqlite3_finalize(ins);
    sqlite3_finalize(srch);
    return ok;
}

int db_update_sizes(int discId, const FileList *fl)
{
    sqlite3_stmt *st = prep("UPDATE file SET size = ?3 WHERE disc_id = ?1 AND path = ?2");
    int i, ok = 1;
    if (!st) return 0;
    for (i = 0; i < fl->n && ok; i++) {
        const ScanFile *f = &fl->v[i];
        if (f->state != FS_OK || !f->hasSize) continue;
        sqlite3_bind_int(st, 1, discId);
        sqlite3_bind_text16(st, 2, f->path, -1, SQLITE_STATIC);
        sqlite3_bind_int64(st, 3, (sqlite3_int64)f->size);
        if (sqlite3_step(st) != SQLITE_DONE) { set_err(); ok = 0; }
        sqlite3_reset(st);
    }
    sqlite3_finalize(st);
    return ok;
}

int db_record_check(int discId, int status, int badFiles,
                    const wchar_t *date, const wchar_t *notes)
{
    sqlite3_stmt *st = prep(
        "INSERT INTO check_log (disc_id, checked, status, bad_files, tool, notes)"
        " VALUES (?1, ?2, ?3, ?4, 'Backuplist 2', ?5)");
    if (!st) return 0;
    sqlite3_bind_int(st, 1, discId);
    sqlite3_bind_text16(st, 2, date, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text16(st, 3, status_str(status), -1, SQLITE_STATIC);
    sqlite3_bind_int(st, 4, badFiles);
    bind_opt(st, 5, notes);
    if (!step_done(st)) return 0;

    st = prep("UPDATE disc SET status = ?2, last_checked = ?3 WHERE id = ?1");
    if (!st) return 0;
    sqlite3_bind_int(st, 1, discId);
    sqlite3_bind_text16(st, 2, status_str(status), -1, SQLITE_STATIC);
    sqlite3_bind_text16(st, 3, date, -1, SQLITE_TRANSIENT);
    return step_done(st);
}

int db_set_media(int id, const wchar_t *media)
{
    sqlite3_stmt *st = prep("UPDATE disc SET media_id = ?2 WHERE id = ?1");
    if (!st) return 0;
    sqlite3_bind_int(st, 1, id);
    bind_opt(st, 2, media);
    return step_done(st);
}

/* ---------- schema of a new database ---------- */

static const char SCHEMA_SQL[] =
"PRAGMA user_version = 1;\n"
"CREATE TABLE disc (\n"
"    id            INTEGER PRIMARY KEY,                 -- number written on the disc\n"
"    disc_type     TEXT NOT NULL,                       -- BD-R / DVD / CD\n"
"    capacity_gb   INTEGER,                             -- 25 / 50 / 100 / 5 / 9 / 1\n"
"    layers        INTEGER,\n"
"    written       TEXT,                                -- burn date, YYYY-MM-DD\n"
"    content_type  TEXT,                                -- Video / Audio / ... (free text)\n"
"    contents      TEXT,                                -- what is on the disc, ';'-separated\n"
"    priority      INTEGER CHECK (priority IN (1,2,3)), -- 1 irreplaceable .. 3 replaceable\n"
"    status        TEXT CHECK (status IN ('ok','partial','dead')),  -- after the last check\n"
"    last_checked  TEXT,\n"
"    brand         TEXT,                                -- brand on the box\n"
"    media_id      TEXT,                                -- real manufacturer code\n"
"    vol_label     TEXT,\n"
"    vol_serial    TEXT,                                -- volume serial, identifies the disc\n"
"    hash_file     TEXT,                                -- source CSV, if imported\n"
"    notes         TEXT\n"
");\n"
"CREATE TABLE file (\n"
"    id       INTEGER PRIMARY KEY,\n"
"    disc_id  INTEGER NOT NULL REFERENCES disc(id) ON DELETE CASCADE,\n"
"    path     TEXT NOT NULL,                            -- from the disc root, '/' separated\n"
"    sha256   TEXT NOT NULL,\n"
"    size     INTEGER\n"
");\n"
"CREATE INDEX file_disc ON file(disc_id);\n"
"CREATE INDEX file_hash ON file(sha256);\n"
"CREATE TABLE check_log (\n"
"    id        INTEGER PRIMARY KEY,\n"
"    disc_id   INTEGER NOT NULL REFERENCES disc(id) ON DELETE CASCADE,\n"
"    checked   TEXT NOT NULL,\n"
"    status    TEXT NOT NULL CHECK (status IN ('ok','partial','dead')),\n"
"    bad_files INTEGER,\n"
"    tool      TEXT,\n"
"    notes     TEXT\n"
");\n"
"CREATE VIRTUAL TABLE search USING fts5(\n"
"    disc_id UNINDEXED, kind UNINDEXED, text,\n"
"    tokenize = 'unicode61 remove_diacritics 2'\n"
");\n"
"CREATE VIEW v_media_stats AS\n"
"SELECT media_id, group_concat(DISTINCT brand) AS brands,\n"
"       disc_type || ' ' || capacity_gb || 'GB' AS kind, count(*) AS discs,\n"
"       sum(status = 'ok') AS ok, sum(status = 'partial') AS partial, sum(status = 'dead') AS dead\n"
"FROM disc GROUP BY media_id, kind ORDER BY dead DESC, partial DESC;\n"
"CREATE VIEW v_copies AS\n"
"SELECT sha256, count(DISTINCT disc_id) AS copies, group_concat(DISTINCT disc_id) AS discs,\n"
"       min(path) AS sample_path\n"
"FROM file GROUP BY sha256;\n"
"CREATE VIEW v_attention AS\n"
"SELECT id, priority, disc_type, capacity_gb, layers, status, media_id, contents\n"
"FROM disc WHERE status <> 'ok' OR (priority = 1 AND layers = 2)\n"
"ORDER BY priority, CASE status WHEN 'dead' THEN 0 WHEN 'partial' THEN 1 ELSE 2 END, id;\n";

int db_create(const wchar_t *path)
{
    sqlite3 *db = NULL;
    char *err = NULL;
    int ok;

    if (sqlite3_open16(path, &db) != SQLITE_OK) {
        if (db) {
            wcsncpy(g_err, (const wchar_t *)sqlite3_errmsg16(db), 511);
            g_err[511] = 0;
            sqlite3_close(db);
        }
        return 0;
    }
    ok = sqlite3_exec(db, SCHEMA_SQL, NULL, NULL, &err) == SQLITE_OK;
    if (!ok) {
        MultiByteToWideChar(CP_UTF8, 0, err ? err : "?", -1, g_err, 512);
        g_err[511] = 0;
        sqlite3_free(err);
    }
    sqlite3_close(db);
    if (!ok) DeleteFileW(path);       /* do not leave a half-made file behind */
    return ok;
}
