/* scan.h — walking the disc and hashing in a background thread */
#ifndef BL_SCAN_H
#define BL_SCAN_H

#include <windows.h>

/* File state, used both for scan results and for comparison with the database */
enum {
    FS_PENDING = 0,
    FS_OK,           /* read OK (when verifying: hash matched) */
    FS_UNREADABLE,   /* open/read error */
    FS_MISMATCH,     /* read, but the hash differs from the database */
    FS_MISSING,      /* in the database, not on the disc */
    FS_EXTRA         /* on the disc, not in the database */
};

typedef struct {
    wchar_t *path;                /* from the disc root, '/' separated */
    unsigned long long size;
    int  hasSize;
    wchar_t sha[65];              /* lowercase hex; "" if not computed */
    int  state;                   /* FS_* */
} ScanFile;

typedef struct {
    ScanFile *v;
    int n, cap;
} FileList;

void fl_init(FileList *fl);
ScanFile *fl_add(FileList *fl, const wchar_t *path);
void fl_free(FileList *fl);
void fl_sort(FileList *fl);                          /* by path, case-insensitive */
ScanFile *fl_find(FileList *fl, const wchar_t *path); /* after fl_sort */

/* Progress snapshot for the window */
typedef struct {
    int  phase;                    /* 0 = listing, 1 = reading, 2 = done */
    int  filesDone, filesTotal;
    unsigned long long bytesDone, bytesTotal;
    int  unreadable;
    wchar_t current[MAX_PATH];
} ScanProgress;

typedef struct {
    wchar_t letter;
    FileList files;                /* result: every file on the disc with its hash */
    int  enumErrors;               /* folders that could not be listed */
    volatile LONG cancel;

    CRITICAL_SECTION cs;
    ScanProgress prog;
    HANDLE thread;
} ScanJob;

BOOL scan_start(ScanJob *job, wchar_t letter);   /* starts the thread */
void scan_get_progress(ScanJob *job, ScanProgress *out);
BOOL scan_finished(ScanJob *job);
void scan_cancel(ScanJob *job);                  /* asks to stop and waits for the thread */
void scan_free(ScanJob *job);                    /* frees everything except files */

/* Top-level folder names joined with "; " — a starting point for Contents */
void scan_top_folders(wchar_t letter, wchar_t *out, int cch);

#endif
