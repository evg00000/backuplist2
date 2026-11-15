/* verify.c — compare a scanned disc with the reference hashes and write a text report */
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include "verify.h"
#include "db.h"
#include "lang.h"

void verify_compare(FileList *exp, FileList *act, VerifyStats *st)
{
    int i;
    ZeroMemory(st, sizeof *st);
    st->expected = exp->n;
    fl_sort(act);

    for (i = 0; i < exp->n; i++) {
        ScanFile *e = &exp->v[i];
        ScanFile *a = fl_find(act, e->path);
        if (!a) {
            e->state = FS_MISSING;
            st->missing++;
        } else if (a->state == FS_UNREADABLE) {
            e->state = FS_UNREADABLE;
            st->unreadable++;
        } else if (_wcsicmp(a->sha, e->sha)) {
            e->state = FS_MISMATCH;
            st->mismatch++;
        } else {
            e->state = FS_OK;
            st->ok++;
            st->okBytes += a->size;
        }
    }
    for (i = 0; i < act->n; i++) {
        ScanFile *a = &act->v[i];
        if (!fl_find(exp, a->path)) {
            if (a->state == FS_OK) a->state = FS_EXTRA;
            st->extra++;
        }
    }
}

int verify_status(const VerifyStats *st)
{
    int bad = st->mismatch + st->unreadable + st->missing;
    if (!bad) return ST_OK;
    if (!st->ok) return ST_DEAD;
    return ST_PARTIAL;
}

/* ---------- report ---------- */

static void put(HANDLE h, const wchar_t *fmt, ...)
{
    wchar_t w[2048];
    char u[6144];
    va_list ap;
    int n;
    DWORD wr;

    va_start(ap, fmt);
    _vsnwprintf(w, 2047, fmt, ap);
    w[2047] = 0;
    va_end(ap);
    n = WideCharToMultiByte(CP_UTF8, 0, w, -1, u, sizeof u, NULL, NULL);
    if (n > 1) WriteFile(h, u, n - 1, &wr, NULL);
}

static void put_path(HANDLE h, const wchar_t *path)
{
    wchar_t p[1024], *q;
    wcsncpy(p, path, 1023);
    p[1023] = 0;
    for (q = p; *q; q++) if (*q == L'/') *q = L'\\';
    put(h, L"  %ls\r\n", p);
}

static void section(HANDLE h, int title, const FileList *fl, int state, int count)
{
    int i;
    if (!count) return;
    put(h, L"\r\n%ls (%d):\r\n", T(title), count);
    for (i = 0; i < fl->n; i++)
        if (fl->v[i].state == state) put_path(h, fl->v[i].path);
}

BOOL verify_write_report(const wchar_t *path, int discId, const wchar_t *label,
                         const wchar_t *serial, const wchar_t *date,
                         const FileList *exp, const FileList *act, const VerifyStats *st,
                         int enumErrors)
{
    static const int NAMES[] = { S_RES_OK, S_RES_PARTIAL, S_RES_DEAD };
    HANDLE h = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD wr;
    int i, shown = 0;

    if (h == INVALID_HANDLE_VALUE) return FALSE;
    WriteFile(h, "\xEF\xBB\xBF", 3, &wr, NULL);     /* UTF-8 BOM: Notepad on XP needs it */

    put(h, T(S_R_HEAD), discId);
    put(h, T(S_R_META), date, label, serial);
    put(h, T(S_R_RESULT), T(NAMES[verify_status(st)]));
    put(h, T(S_R_EXPECTED), st->expected);
    put(h, T(S_R_OK), st->ok);
    put(h, T(S_R_MISMATCH), st->mismatch);
    put(h, T(S_R_UNREADABLE), st->unreadable);
    put(h, T(S_R_MISSING), st->missing);
    put(h, T(S_R_EXTRA), st->extra);
    if (enumErrors) put(h, T(S_R_ENUMERR), enumErrors);

    section(h, S_R_SEC_UNREADABLE, exp, FS_UNREADABLE, st->unreadable);
    section(h, S_R_SEC_MISMATCH, exp, FS_MISMATCH, st->mismatch);
    section(h, S_R_SEC_MISSING, exp, FS_MISSING, st->missing);

    /* extra files: both read ones (FS_EXTRA) and unreadable ones that are not in the database */
    for (i = 0; i < act->n; i++) {
        const ScanFile *a = &act->v[i];
        if (a->state != FS_EXTRA && !(a->state == FS_UNREADABLE && !fl_find((FileList *)exp, a->path)))
            continue;
        if (!shown++) put(h, T(S_R_SEC_EXTRA), st->extra);
        put_path(h, a->path);
    }
    CloseHandle(h);
    return TRUE;
}
