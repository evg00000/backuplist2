/* scan.c — walking and hashing. The worker thread only reads files and never touches
   SQLite (built without thread safety); the UI thread does all database writes. */
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "scan.h"
#include "sha256.h"

#define READ_BUF  (1024 * 1024)
#define PATH_MAXW 32768

/* ---------- FileList ---------- */

void fl_init(FileList *fl) { fl->v = NULL; fl->n = fl->cap = 0; }

ScanFile *fl_add(FileList *fl, const wchar_t *path)
{
    ScanFile *f;
    if (fl->n == fl->cap) {
        int cap = fl->cap ? fl->cap * 2 : 256;
        ScanFile *nv = (ScanFile *)realloc(fl->v, cap * sizeof *nv);
        if (!nv) return NULL;
        fl->v = nv;
        fl->cap = cap;
    }
    f = &fl->v[fl->n];
    ZeroMemory(f, sizeof *f);
    f->path = _wcsdup(path);
    if (!f->path) return NULL;
    fl->n++;
    return f;
}

void fl_free(FileList *fl)
{
    int i;
    for (i = 0; i < fl->n; i++) free(fl->v[i].path);
    free(fl->v);
    fl_init(fl);
}

static int cmp_file(const void *a, const void *b)
{
    return _wcsicmp(((const ScanFile *)a)->path, ((const ScanFile *)b)->path);
}

void fl_sort(FileList *fl)
{
    if (fl->n > 1) qsort(fl->v, fl->n, sizeof *fl->v, cmp_file);
}

ScanFile *fl_find(FileList *fl, const wchar_t *path)
{
    ScanFile key;
    key.path = (wchar_t *)path;
    return fl->n ? (ScanFile *)bsearch(&key, fl->v, fl->n, sizeof *fl->v, cmp_file) : NULL;
}

/* ---------- walking ---------- */

typedef struct {
    ScanJob *job;
    wchar_t *full;      /* \\?\E:\path\... */
    wchar_t *tmp;       /* relative path with '/' */
    int      rootLen;   /* length of "\\?\E:\" */
} Walk;

static void set_current(ScanJob *job, const wchar_t *rel)
{
    EnterCriticalSection(&job->cs);
    wcsncpy(job->prog.current, rel, MAX_PATH - 1);
    job->prog.current[MAX_PATH - 1] = 0;
    LeaveCriticalSection(&job->cs);
}

static void walk_dir(Walk *w, int len)
{
    WIN32_FIND_DATAW fd;
    HANDLE h;

    if (len + 3 >= PATH_MAXW) return;
    wcscpy(w->full + len, L"*");
    h = FindFirstFileW(w->full, &fd);
    w->full[len] = 0;
    if (h == INVALID_HANDLE_VALUE) {
        if (GetLastError() != ERROR_FILE_NOT_FOUND) w->job->enumErrors++;
        return;
    }
    do {
        int nlen;
        if (w->job->cancel) break;
        if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")) continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;
        nlen = (int)wcslen(fd.cFileName);
        if (len + nlen + 2 >= PATH_MAXW) { w->job->enumErrors++; continue; }
        wcscpy(w->full + len, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            w->full[len + nlen] = L'\\';
            w->full[len + nlen + 1] = 0;
            walk_dir(w, len + nlen + 1);
        } else {
            wchar_t *p;
            ScanFile *f;
            /* the database uses '/' as the separator */
            wcscpy(w->tmp, w->full + w->rootLen);
            for (p = w->tmp; *p; p++) if (*p == L'\\') *p = L'/';
            EnterCriticalSection(&w->job->cs);
            f = fl_add(&w->job->files, w->tmp);
            if (f) {
                f->size = ((unsigned long long)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
                f->hasSize = 1;
                w->job->prog.filesTotal++;
                w->job->prog.bytesTotal += f->size;
            }
            LeaveCriticalSection(&w->job->cs);
        }
        w->full[len] = 0;
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

/* ---------- hashing ---------- */

static void hash_file(ScanJob *job, const wchar_t *fullPath, ScanFile *f, BYTE *buf)
{
    Sha256 ctx;
    HANDLE h;
    BYTE digest[32];
    unsigned long long done = 0;
    BOOL ok = TRUE;

    h = CreateFileW(fullPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                    FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        ok = FALSE;
    } else {
        sha256_init(&ctx);
        for (;;) {
            DWORD got = 0;
            if (job->cancel) { ok = FALSE; break; }
            if (!ReadFile(h, buf, READ_BUF, &got, NULL)) { ok = FALSE; break; }
            if (!got) break;
            sha256_update(&ctx, buf, got);
            done += got;
            EnterCriticalSection(&job->cs);
            job->prog.bytesDone += got;
            LeaveCriticalSection(&job->cs);
        }
        CloseHandle(h);
    }

    EnterCriticalSection(&job->cs);
    if (ok) {
        sha256_final(&ctx, digest);
        sha256_hex(digest, f->sha);
        f->state = FS_OK;
    } else if (!job->cancel) {
        f->sha[0] = 0;
        f->state = FS_UNREADABLE;
        job->prog.unreadable++;
        /* count the rest of the file as done so the progress bar keeps moving */
        if (f->size > done) job->prog.bytesDone += f->size - done;
    }
    job->prog.filesDone++;
    LeaveCriticalSection(&job->cs);
}

static DWORD WINAPI scan_thread(LPVOID arg)
{
    ScanJob *job = (ScanJob *)arg;
    Walk w;
    BYTE *buf;
    int i, n;

    w.job = job;
    w.full = (wchar_t *)malloc(PATH_MAXW * sizeof(wchar_t));
    w.tmp  = (wchar_t *)malloc(PATH_MAXW * sizeof(wchar_t));
    buf = (BYTE *)VirtualAlloc(NULL, READ_BUF, MEM_COMMIT, PAGE_READWRITE);
    if (!w.full || !w.tmp || !buf) goto done;

    /* \\?\ lifts the MAX_PATH limit; works on XP with the W functions */
    wcscpy(w.full, L"\\\\?\\A:\\");
    w.full[4] = job->letter;
    w.rootLen = (int)wcslen(w.full);
    walk_dir(&w, w.rootLen);
    if (job->cancel) goto done;

    EnterCriticalSection(&job->cs);
    job->prog.phase = 1;
    n = job->files.n;
    LeaveCriticalSection(&job->cs);

    for (i = 0; i < n && !job->cancel; i++) {
        ScanFile *f = &job->files.v[i];
        wchar_t *p;
        int len;
        set_current(job, f->path);
        len = w.rootLen;
        wcsncpy(w.full + len, f->path, PATH_MAXW - len - 1);
        w.full[PATH_MAXW - 1] = 0;
        for (p = w.full + len; *p; p++) if (*p == L'/') *p = L'\\';
        hash_file(job, w.full, f, buf);
    }

done:
    free(w.full);
    free(w.tmp);
    if (buf) VirtualFree(buf, 0, MEM_RELEASE);
    EnterCriticalSection(&job->cs);
    job->prog.phase = 2;
    job->prog.current[0] = 0;
    LeaveCriticalSection(&job->cs);
    return 0;
}

BOOL scan_start(ScanJob *job, wchar_t letter)
{
    DWORD tid;
    ZeroMemory(job, sizeof *job);
    job->letter = letter;
    fl_init(&job->files);
    InitializeCriticalSection(&job->cs);
    job->thread = CreateThread(NULL, 0, scan_thread, job, 0, &tid);
    if (!job->thread) {
        DeleteCriticalSection(&job->cs);
        return FALSE;
    }
    /* keep the UI responsive: read at a slightly lower priority */
    SetThreadPriority(job->thread, THREAD_PRIORITY_BELOW_NORMAL);
    return TRUE;
}

void scan_get_progress(ScanJob *job, ScanProgress *out)
{
    EnterCriticalSection(&job->cs);
    *out = job->prog;
    LeaveCriticalSection(&job->cs);
}

BOOL scan_finished(ScanJob *job)
{
    return WaitForSingleObject(job->thread, 0) == WAIT_OBJECT_0;
}

void scan_cancel(ScanJob *job)
{
    InterlockedExchange(&job->cancel, 1);
    WaitForSingleObject(job->thread, INFINITE);
}

void scan_free(ScanJob *job)
{
    if (job->thread) {
        WaitForSingleObject(job->thread, INFINITE);
        CloseHandle(job->thread);
        job->thread = NULL;
        DeleteCriticalSection(&job->cs);
    }
}

void scan_top_folders(wchar_t letter, wchar_t *out, int cch)
{
    wchar_t pat[8] = L"A:\\*";
    WIN32_FIND_DATAW fd;
    HANDLE h;
    UINT oldMode;
    int o = 0;

    out[0] = 0;
    pat[0] = letter;
    oldMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    h = FindFirstFileW(pat, &fd);
    SetErrorMode(oldMode);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        int len;
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        if (fd.cFileName[0] == L'.') continue;
        if (fd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) continue;
        len = (int)wcslen(fd.cFileName);
        if (o + len + 3 >= cch) break;
        if (o) { out[o++] = L';'; out[o++] = L' '; }
        wcscpy(out + o, fd.cFileName);
        o += len;
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    if (o && o + 1 < cch) { out[o++] = L';'; out[o] = 0; }
}
