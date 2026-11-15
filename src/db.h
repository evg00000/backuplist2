/* db.h — access to backuplist.db (SQLite). All strings are UTF-16. */
#ifndef BL_DB_H
#define BL_DB_H

#include <wchar.h>

#define MAX_PATH_LABEL 64

enum { ST_OK = 0, ST_PARTIAL = 1, ST_DEAD = 2, ST_UNKNOWN = 3 };

typedef struct {
    int id;
    int priority;
    int status;                 /* ST_* */
    const wchar_t *type;        /* "BD-R 25" */
    const wchar_t *ctype;       /* Video / Audio / ... */
    const wchar_t *checked;
    const wchar_t *media;
    const wchar_t *serial;
    const wchar_t *contents;
} DiscRow;

typedef struct {
    const wchar_t *path;
    const wchar_t *sha256;
} FileRow;

typedef void (*DiscCb)(void *ctx, const DiscRow *r);
typedef void (*FileCb)(void *ctx, const FileRow *r);

/* Sort columns, same order as the disc list columns */
enum { SORT_ID, SORT_TYPE, SORT_CTYPE, SORT_STATUS, SORT_PRIO,
       SORT_CHECKED, SORT_MEDIA, SORT_SERIAL, SORT_CONTENTS, SORT_COUNT };

int  db_open(const wchar_t *path);
int  db_create(const wchar_t *path);              /* new empty database with the full schema */
void db_close(void);
int  db_is_open(void);
const wchar_t *db_error(void);

/* fts == NULL or "" means no filter. Returns the row count or -1. */
int  db_list_discs(const wchar_t *fts, int sortCol, int desc, DiscCb cb, void *ctx);
int  db_list_files(int discId, const wchar_t *fts, FileCb cb, void *ctx);

int  db_find_by_serial(const wchar_t *serial);            /* id or 0 */
int  db_disc_serial(int discId, wchar_t *out, int cch);   /* 1 if linked */
int  db_bind_serial(int discId, const wchar_t *serial, const wchar_t *label);
int  db_unbind_serial(int discId);
void db_counts(int *discs, int *files);

/* Turns user input into a safe FTS5 query:
   tokyo shoeg -> "tokyo"* "shoeg"*  (all words, prefix match) */
void db_make_fts_query(const wchar_t *in, wchar_t *out, int cch);

/* ---------- disc card ---------- */

typedef struct {
    const wchar_t *title;    /* shown in the list: "BD-R 25" */
    const wchar_t *type;     /* disc.disc_type */
    int capacity, layers;
} MediaKind;

extern const MediaKind MEDIA_KINDS[];
extern const int MEDIA_KIND_COUNT;
int media_kind_find(const wchar_t *type, int capacity);   /* index, or 0 */

typedef struct {
    int  id;
    int  kind;                     /* index into MEDIA_KINDS */
    wchar_t written[16];           /* YYYY-MM-DD */
    wchar_t ctype[64];
    wchar_t contents[4096];
    int  priority;                 /* 1..3 */
    wchar_t brand[64];
    wchar_t media[64];
    wchar_t label[MAX_PATH_LABEL];
    wchar_t serial[16];
    int  status;                   /* ST_* (read-only in the card) */
    wchar_t checked[16];
    wchar_t hint[200];             /* hint shown in the card (not stored) */
} DiscInfo;

int  db_get_disc(int id, DiscInfo *d);
int  db_id_exists(int id);
int  db_next_id(void);
int  db_latest_disc(void);                     /* most recently burned */
int  db_insert_disc(const DiscInfo *d);       /* new disc; status and check date come from d */
int  db_update_disc(const DiscInfo *d);       /* edit the card (status untouched) */
int  db_delete_disc(int id);
int  db_set_media(int id, const wchar_t *media);

/* Distinct values of a column for the drop-down lists:
   "content_type", "brand" or "media_id" */
typedef void (*StrCb)(void *ctx, const wchar_t *s);
void db_distinct(const char *column, StrCb cb, void *ctx);

/* ---------- files and checks ---------- */

#include "scan.h"

int  db_load_files(int discId, FileList *fl);              /* reference from the database, sorted */
int  db_store_files(int discId, const FileList *fl);       /* replace the disc's file list */
int  db_update_sizes(int discId, const FileList *fl);      /* sizes of the files that were read */
int  db_record_check(int discId, int status, int badFiles,
                     const wchar_t *date, const wchar_t *notes);

int  db_begin(void);
int  db_commit(void);
void db_rollback(void);

#endif
