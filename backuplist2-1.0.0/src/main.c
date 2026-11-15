/* Backuplist 2 — a catalog of backups on optical discs.
   Win32 API + SQLite, runs on Windows XP SP2 .. Windows 11. */
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdarg.h>
#include "sqlite3.h"
#include "version.h"
#include "lang.h"
#include "db.h"
#include "volume.h"
#include "scan.h"
#include "dialogs.h"
#include "verify.h"
#include "mediaid.h"

#define WIDEN2(x) L##x
#define WIDEN(x)  WIDEN2(x)

#ifndef EM_SETCUEBANNER
#define EM_SETCUEBANNER 0x1501
#endif

enum {
    IDC_SEARCH = 100, IDC_SEARCHLBL, IDC_DISCS, IDC_FILESLBL, IDC_FILES, IDC_STATUS,
    IDM_NEWDB = 200, IDM_OPEN, IDM_EXIT, IDM_LANG_EN, IDM_LANG_RU,
    IDM_DETECT, IDM_ADD, IDM_VERIFY, IDM_EDIT, IDM_DELETE, IDM_UNBIND, IDM_MEDIAINFO,
    IDM_FIND, IDM_ABOUT,
    TIMER_SEARCH = 1,
    WM_APP_VOLUME = WM_APP + 1      /* wParam = drive letter */
};

/* Disc list columns, in display order (matches SORT_* in db.h) */
static const struct { int title, width; } DISC_COLS[] = {
    { S_COL_NUM, 40 }, { S_COL_TYPE, 70 }, { S_COL_DATA, 80 }, { S_COL_STATUS, 70 },
    { S_COL_PRIO, 36 }, { S_COL_CHECKED, 78 }, { S_COL_MEDIA, 96 }, { S_COL_SERIAL, 80 },
    { S_COL_CONTENTS, 520 },
};
static const struct { int title, width; } FILE_COLS[] = {
    { S_COL_PATH, 560 }, { S_COL_SHA, 460 },
};

static HINSTANCE g_inst;
static HWND g_main, g_search, g_searchLbl, g_discs, g_filesLbl, g_files, g_status;
static HFONT g_font;
static int  g_fontH = 16;
static int  g_sortCol = SORT_ID, g_sortDesc = 0;
static int  g_selDisc = 0;
static int  g_busy = 0;            /* a dialog or scan is running: ignore inserted discs */
static wchar_t g_dbPath[MAX_PATH];
static wchar_t g_fts[512];          /* current FTS5 query ("" = no filter) */

/* ---------- small helpers ---------- */

static void status_text(int part, const wchar_t *fmt, ...)
{
    wchar_t buf[512];
    va_list ap;
    va_start(ap, fmt);
    _vsnwprintf(buf, 511, fmt, ap);
    buf[511] = 0;
    va_end(ap);
    SendMessageW(g_status, SB_SETTEXTW, part, (LPARAM)buf);
}

/* printf into a fixed buffer, always terminated */
static void fmt(wchar_t *out, int cch, const wchar_t *f, ...)
{
    va_list ap;
    va_start(ap, f);
    _vsnwprintf(out, cch - 1, f, ap);
    out[cch - 1] = 0;
    va_end(ap);
}

static int msg_box(UINT flags, const wchar_t *title, const wchar_t *f, ...)
{
    wchar_t buf[1536];
    va_list ap;
    va_start(ap, f);
    _vsnwprintf(buf, 1535, f, ap);
    buf[1535] = 0;
    va_end(ap);
    return MessageBoxW(g_main, buf, title ? title : APP_NAME, flags);
}

static const wchar_t *status_name(int st)
{
    switch (st) {
    case ST_OK:      return T(S_ST_OK);
    case ST_PARTIAL: return T(S_ST_PARTIAL);
    case ST_DEAD:    return T(S_ST_DEAD);
    default:         return T(S_ST_UNKNOWN);
    }
}

static const wchar_t *result_name(int st)
{
    return st == ST_OK ? T(S_RES_OK) : st == ST_PARTIAL ? T(S_RES_PARTIAL) : T(S_RES_DEAD);
}

static void lv_add_column(HWND lv, int idx, const wchar_t *title, int width)
{
    LVCOLUMNW c = {0};
    c.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    c.pszText = (wchar_t *)title;
    c.cx = MulDiv(width, g_fontH, 13);     /* scale with the system font / DPI */
    c.iSubItem = idx;
    SendMessageW(lv, LVM_INSERTCOLUMNW, idx, (LPARAM)&c);
}

static void lv_set_column_title(HWND lv, int idx, const wchar_t *title)
{
    LVCOLUMNW c = {0};
    c.mask = LVCF_TEXT;
    c.pszText = (wchar_t *)title;
    SendMessageW(lv, LVM_SETCOLUMNW, idx, (LPARAM)&c);
}

static void lv_set(HWND lv, int item, int sub, const wchar_t *text)
{
    LVITEMW it = {0};
    it.iSubItem = sub;
    it.pszText = (wchar_t *)text;
    SendMessageW(lv, LVM_SETITEMTEXTW, item, (LPARAM)&it);
}

static HWND make_listview(int id)
{
    HWND lv = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
        0, 0, 0, 0, g_main, (HMENU)(INT_PTR)id, g_inst, NULL);
    SendMessageW(lv, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                 LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    SendMessageW(lv, WM_SETFONT, (WPARAM)g_font, TRUE);
    return lv;
}

static void exe_dir(wchar_t *out, int cch)
{
    wchar_t *slash;
    GetModuleFileNameW(NULL, out, cch);
    slash = wcsrchr(out, L'\\');
    if (slash) slash[1] = 0;
}

static void today(wchar_t out[16])
{
    SYSTEMTIME t;
    GetLocalTime(&t);
    _snwprintf(out, 16, L"%04u-%02u-%02u", t.wYear, t.wMonth, t.wDay);
    out[15] = 0;
}

/* ---------- disc list ---------- */

static void add_disc_cb(void *ctx, const DiscRow *r)
{
    HWND lv = (HWND)ctx;
    wchar_t num[16], prio[8];
    LVITEMW it = {0};
    int i;

    fmt(num, 16, L"%d", r->id);
    fmt(prio, 8, L"%d", r->priority);

    it.mask = LVIF_TEXT | LVIF_PARAM;
    it.iItem = 0x7fffffff;
    it.pszText = num;
    it.lParam = MAKELPARAM(r->id, r->status);   /* id + status for row colouring */
    i = (int)SendMessageW(lv, LVM_INSERTITEMW, 0, (LPARAM)&it);

    lv_set(lv, i, 1, r->type);
    lv_set(lv, i, 2, r->ctype);
    lv_set(lv, i, 3, status_name(r->status));
    lv_set(lv, i, 4, prio);
    lv_set(lv, i, 5, r->checked);
    lv_set(lv, i, 6, r->media);
    lv_set(lv, i, 7, r->serial);
    lv_set(lv, i, 8, r->contents);
}

static int disc_at(int item)
{
    LVITEMW it = {0};
    it.mask = LVIF_PARAM;
    it.iItem = item;
    if (!SendMessageW(g_discs, LVM_GETITEMW, 0, (LPARAM)&it)) return 0;
    return LOWORD(it.lParam);
}

static int select_disc(int id)
{
    int n = (int)SendMessageW(g_discs, LVM_GETITEMCOUNT, 0, 0), i;
    for (i = 0; i < n; i++) {
        if (disc_at(i) == id) {
            LVITEMW it = {0};
            it.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
            it.state = LVIS_SELECTED | LVIS_FOCUSED;
            SendMessageW(g_discs, LVM_SETITEMSTATE, i, (LPARAM)&it);
            SendMessageW(g_discs, LVM_ENSUREVISIBLE, i, FALSE);
            return 1;
        }
    }
    return 0;
}

static void load_files(int discId);

static void load_discs(void)
{
    int keep = g_selDisc, n;

    SendMessageW(g_discs, WM_SETREDRAW, FALSE, 0);
    SendMessageW(g_discs, LVM_DELETEALLITEMS, 0, 0);
    n = db_list_discs(g_fts, g_sortCol, g_sortDesc, add_disc_cb, g_discs);
    SendMessageW(g_discs, WM_SETREDRAW, TRUE, 0);

    if (n < 0) {
        status_text(0, L"%ls", T(S_QUERY_ERROR));
        status_text(1, L"%ls", db_error());
        return;
    }
    if (g_fts[0]) status_text(0, T(S_FOUND_DISCS), n);
    else {
        int d, f;
        db_counts(&d, &f);
        status_text(0, T(S_DISCS_TOTAL), d, f);
    }
    g_selDisc = 0;
    if (!(keep && select_disc(keep))) load_files(0);
}

/* ---------- file list ---------- */

static void add_file_cb(void *ctx, const FileRow *r)
{
    HWND lv = (HWND)ctx;
    wchar_t path[1024];
    LVITEMW it = {0};
    int i;

    /* paths are stored with '/', shown Windows-style */
    wcsncpy(path, r->path, 1023);
    path[1023] = 0;
    for (i = 0; path[i]; i++) if (path[i] == L'/') path[i] = L'\\';

    it.mask = LVIF_TEXT;
    it.iItem = 0x7fffffff;
    it.pszText = path;
    i = (int)SendMessageW(lv, LVM_INSERTITEMW, 0, (LPARAM)&it);
    lv_set(lv, i, 1, r->sha256);
}

static void load_files(int discId)
{
    int n = 0;
    wchar_t lbl[200];

    g_selDisc = discId;
    SendMessageW(g_files, WM_SETREDRAW, FALSE, 0);
    SendMessageW(g_files, LVM_DELETEALLITEMS, 0, 0);
    if (discId) n = db_list_files(discId, g_fts, add_file_cb, g_files);
    SendMessageW(g_files, WM_SETREDRAW, TRUE, 0);

    if (!discId)                     fmt(lbl, 200, L"%ls", T(S_FILES_NONE));
    else if (n <= 0 && !g_fts[0])    fmt(lbl, 200, T(S_FILES_NOHASH), discId);
    else if (g_fts[0])               fmt(lbl, 200, T(S_FILES_MATCH), discId, n);
    else                             fmt(lbl, 200, T(S_FILES_N), discId, n);
    SetWindowTextW(g_filesLbl, lbl);
}

/* ---------- search ---------- */

static void apply_search(void)
{
    wchar_t raw[256];
    GetWindowTextW(g_search, raw, 256);
    db_make_fts_query(raw, g_fts, 512);
    load_discs();
}

static void clear_search(void)
{
    KillTimer(g_main, TIMER_SEARCH);
    if (GetWindowTextLengthW(g_search) > 0) {
        SetWindowTextW(g_search, L"");
        KillTimer(g_main, TIMER_SEARCH);   /* EN_CHANGE has just set the timer again */
    }
    g_fts[0] = 0;
}

static void show_disc(int id)
{
    clear_search();
    load_discs();
    select_disc(id);
    load_files(id);
}

/* ---------- inserted disc ---------- */

static void cmd_add(const VolumeInfo *given);
static void handle_volume_inner(wchar_t letter);

static void handle_volume(wchar_t letter)
{
    static int inside = 0;           /* while a question is open, ignore further insertions */
    if (inside || g_busy || !db_is_open()) return;
    inside = 1;
    handle_volume_inner(letter);
    inside = 0;
}

static void handle_volume_inner(wchar_t letter)
{
    VolumeInfo vi;
    wchar_t bound[16];
    int id;

    if (!volume_query(letter, &vi)) {
        status_text(1, T(S_NO_READABLE_IN), letter);
        return;
    }

    id = db_find_by_serial(vi.serialStr);
    if (id) {
        MediaInfo mi;
        DiscInfo card;
        wchar_t extra[200] = L"";

        show_disc(id);
        if (mediaid_read(letter, &mi) && db_get_disc(id, &card)) {
            if (!card.media[0])
                fmt(extra, 200, T(S_MID_EMPTY_CARD), mi.mediaId);
            else if (mediaid_same(card.media, mi.mediaId) || mediaid_same(card.media, mi.mediaBase))
                fmt(extra, 200, T(S_MID_MATCH), mi.mediaId);
            else
                fmt(extra, 200, T(S_MID_DIFF), mi.mediaId, card.media);
        }
        status_text(1, T(S_INSERTED), id, letter, vi.label, vi.serialStr, extra);
        FlashWindow(g_main, TRUE);
        return;
    }

    /* Unknown disc: add it as new, or link it to the selected entry */
    FlashWindow(g_main, TRUE);
    if (g_selDisc && !db_disc_serial(g_selDisc, bound, 16)) {
        int r = msg_box(MB_YESNOCANCEL | MB_ICONQUESTION, NULL, T(S_UNKNOWN_3WAY),
                        letter, vi.label, vi.serialStr, g_selDisc);
        if (r == IDYES) {
            cmd_add(&vi);
        } else if (r == IDNO) {
            int keep = g_selDisc;
            if (db_bind_serial(keep, vi.serialStr, vi.label)) {
                load_discs();
                select_disc(keep);
                status_text(1, T(S_BOUND), keep, vi.serialStr);
            } else {
                MessageBoxW(g_main, db_error(), APP_NAME, MB_ICONERROR);
            }
        }
        return;
    }
    if (msg_box(MB_YESNO | MB_ICONQUESTION, NULL, T(S_UNKNOWN_ADD),
                letter, vi.label, vi.serialStr) == IDYES)
        cmd_add(&vi);
    else
        status_text(1, T(S_UNKNOWN_STATUS), letter, vi.label, vi.serialStr);
}

static void detect_inserted(void)
{
    VolumeInfo vi;
    if (volume_first_optical(&vi)) handle_volume(vi.letter);
    else status_text(1, L"%ls", T(S_NO_DISC_ANY));
}

static BOOL pick_inserted(VolumeInfo *vi)
{
    if (volume_first_optical(vi)) return TRUE;
    MessageBoxW(g_main, T(S_NO_READABLE_ANY), APP_NAME, MB_ICONINFORMATION);
    return FALSE;
}

/* ---------- database file ---------- */

static void open_db(const wchar_t *path)
{
    if (db_open(path)) {
        wchar_t title[MAX_PATH + 64];
        wcsncpy(g_dbPath, path, MAX_PATH - 1);
        fmt(title, MAX_PATH + 64, APP_NAME L" \x2014 %ls", path);
        SetWindowTextW(g_main, title);
        clear_search();
        g_selDisc = 0;
        load_discs();
        status_text(1, L"%ls", T(S_DB_OPENED));
    } else {
        SendMessageW(g_discs, LVM_DELETEALLITEMS, 0, 0);
        load_files(0);
        status_text(0, L"%ls", T(S_DB_NOT_OPEN));
        status_text(1, L"%ls: %ls", path, db_error());
    }
}

static BOOL create_db(const wchar_t *path)
{
    if (!db_create(path)) {
        msg_box(MB_ICONERROR, NULL, T(S_DB_CREATE_FAIL), db_error());
        return FALSE;
    }
    open_db(path);
    status_text(1, T(S_DB_CREATED), path);
    return TRUE;
}

static void cmd_open(void)
{
    wchar_t path[MAX_PATH] = L"";
    OPENFILENAMEW of = {0};
    of.lStructSize = sizeof of;
    of.hwndOwner = g_main;
    of.lpstrFilter = T(S_DB_FILTER);
    of.lpstrFile = path;
    of.nMaxFile = MAX_PATH;
    of.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (GetOpenFileNameW(&of)) open_db(path);
}

static void cmd_new_db(void)
{
    wchar_t path[MAX_PATH] = L"backuplist.db";
    OPENFILENAMEW of = {0};
    of.lStructSize = sizeof of;
    of.hwndOwner = g_main;
    of.lpstrFilter = T(S_DB_FILTER);
    of.lpstrFile = path;
    of.nMaxFile = MAX_PATH;
    of.lpstrDefExt = L"db";
    of.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
    if (!GetSaveFileNameW(&of)) return;
    if (GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES) {
        MessageBoxW(g_main, T(S_DB_EXISTS), APP_NAME, MB_ICONWARNING);
        return;
    }
    create_db(path);
}

static void cmd_unbind(void)
{
    wchar_t serial[16];
    if (!g_selDisc) return;
    if (!db_disc_serial(g_selDisc, serial, 16)) {
        status_text(1, T(S_NOT_BOUND), g_selDisc);
        return;
    }
    if (msg_box(MB_YESNO | MB_ICONQUESTION, NULL, T(S_UNBIND_Q), serial, g_selDisc) == IDYES) {
        int keep = g_selDisc;
        db_unbind_serial(keep);
        load_discs();
        select_disc(keep);
    }
}

/* ---------- adding, verification, card ---------- */

static void count_states(const FileList *fl, int *ok, int *bad, unsigned long long *bytes)
{
    int i;
    *ok = *bad = 0;
    *bytes = 0;
    for (i = 0; i < fl->n; i++) {
        if (fl->v[i].state == FS_OK) { (*ok)++; *bytes += fl->v[i].size; }
        else (*bad)++;
    }
}

/* Media kind: same as the most recently burned disc, but big enough for the data */
static int guess_kind(wchar_t letter)
{
    wchar_t root[4] = { letter, L':', L'\\', 0 };
    ULARGE_INTEGER total;
    DiscInfo last;
    int kind = 0, i;
    double gb;

    if (db_get_disc(db_latest_disc(), &last)) kind = last.kind;
    if (!GetDiskFreeSpaceExW(root, NULL, &total, NULL)) return kind;
    gb = total.QuadPart / 1e9;
    if (gb <= MEDIA_KINDS[kind].capacity) return kind;
    for (i = 0; i < MEDIA_KIND_COUNT; i++)
        if (!wcscmp(MEDIA_KINDS[i].type, L"BD-R") && gb <= MEDIA_KINDS[i].capacity) return i;
    return kind;
}

static void cmd_add(const VolumeInfo *given)
{
    VolumeInfo vi;
    DiscInfo d;
    ScanJob job;
    MediaInfo mi;
    wchar_t title[96], date[16], notes[200], size[32];
    int existing, ok, bad;
    unsigned long long bytes;

    if (g_busy || !db_is_open()) return;
    if (given) vi = *given;
    else if (!pick_inserted(&vi)) return;

    existing = db_find_by_serial(vi.serialStr);
    if (existing) {
        show_disc(existing);
        msg_box(MB_ICONINFORMATION, NULL, T(S_ALREADY), existing);
        return;
    }

    g_busy++;
    ZeroMemory(&d, sizeof d);
    d.id = db_next_id();
    d.kind = guess_kind(vi.letter);
    today(d.written);
    d.priority = 2;
    d.status = ST_UNKNOWN;
    scan_top_folders(vi.letter, d.contents, 4096);
    wcsncpy(d.label, vi.label, MAX_PATH_LABEL - 1);
    wcscpy(d.serial, vi.serialStr);

    if (mediaid_read(vi.letter, &mi)) {
        mediaid_preferred(&mi, d.media, 64);
        fmt(d.hint, 200, T(S_HINT_OK), mi.profileName, mi.mediaId, mi.layers > 1 ? T(S_HINT_LAYERS) : L"");
    } else if (mi.profile) {
        fmt(d.hint, 200, T(S_HINT_PROFILE_NOID), mi.profileName, mi.error);
    } else {
        fmt(d.hint, 200, T(S_HINT_NOID), mi.error);
    }
    if (mi.kind >= 0) d.kind = mi.kind;     /* the drive knows the media kind better than a guess */

    if (!dlg_disc(g_main, &d, TRUE)) { g_busy--; return; }

    fmt(title, 96, T(S_TITLE_ADD), d.id);
    if (!dlg_scan(g_main, &job, vi.letter, title)) {
        fl_free(&job.files);
        status_text(1, T(S_ADD_CANCELLED), d.id);
        g_busy--;
        return;
    }

    count_states(&job.files, &ok, &bad, &bytes);
    d.status = bad == 0 ? ST_OK : (ok == 0 ? ST_DEAD : ST_PARTIAL);
    today(date);
    wcscpy(d.checked, date);
    fmt_size(bytes, size, 32);
    fmt(notes, 200, T(S_NOTES_ADDED), ok, size, bad);

    if (db_begin() && db_insert_disc(&d) && db_store_files(d.id, &job.files)
        && db_record_check(d.id, d.status, bad, date, notes) && db_commit()) {
        show_disc(d.id);
        status_text(1, T(S_ADDED_STATUS), d.id, notes);
        if (bad) msg_box(MB_ICONWARNING, NULL, T(S_ADDED_BAD), d.id, bad);
    } else {
        db_rollback();
        MessageBoxW(g_main, db_error(), T(S_DB_WRITE_ERR), MB_ICONERROR);
    }
    fl_free(&job.files);
    g_busy--;
}

static void run_verify(int id, const VolumeInfo *vi, BOOL bind)
{
    FileList exp;
    ScanJob job;
    wchar_t title[96], date[16], notes[256];
    BOOL baseline;

    fl_init(&exp);
    if (db_load_files(id, &exp) < 0) {
        MessageBoxW(g_main, db_error(), APP_NAME, MB_ICONERROR);
        return;
    }
    baseline = exp.n == 0;
    if (baseline && msg_box(MB_YESNO | MB_ICONQUESTION, NULL, T(S_NO_HASHES_Q), id) != IDYES)
        return;

    fmt(title, 96, T(baseline ? S_TITLE_BASELINE : S_TITLE_VERIFY), id);
    if (!dlg_scan(g_main, &job, vi->letter, title)) {
        fl_free(&job.files);
        fl_free(&exp);
        status_text(1, T(S_VERIFY_CANCELLED), id);
        return;
    }
    today(date);

    if (!db_begin()) goto dberr;
    if (bind && !db_bind_serial(id, vi->serialStr, vi->label)) goto dberr;

    if (baseline) {
        int ok, bad, st;
        unsigned long long bytes;
        wchar_t size[32];
        count_states(&job.files, &ok, &bad, &bytes);
        st = bad == 0 ? ST_OK : (ok == 0 ? ST_DEAD : ST_PARTIAL);
        fmt_size(bytes, size, 32);
        fmt(notes, 256, T(S_NOTES_BASELINE), ok, size, bad);
        if (!db_store_files(id, &job.files) || !db_record_check(id, st, bad, date, notes)
            || !db_commit()) goto dberr;
        show_disc(id);
        status_text(1, T(S_DISC_NOTES), id, notes);
        msg_box(bad ? MB_ICONWARNING : MB_ICONINFORMATION, NULL, T(S_BASELINE_DONE),
                id, ok, size, bad ? T(S_BASELINE_PARTIAL) : L"");
    } else {
        VerifyStats vs;
        int st, bad;
        wchar_t report[MAX_PATH], okSize[32];
        size_t len;

        verify_compare(&exp, &job.files, &vs);
        st = verify_status(&vs);
        bad = vs.mismatch + vs.unreadable + vs.missing;
        fmt(notes, 256, T(S_NOTES_VERIFY), vs.ok, vs.expected, vs.mismatch,
            vs.unreadable, vs.missing, vs.extra);
        if (!db_update_sizes(id, &job.files) || !db_record_check(id, st, bad, date, notes)
            || !db_commit()) goto dberr;

        exe_dir(report, MAX_PATH);
        wcscat(report, L"reports");
        CreateDirectoryW(report, NULL);
        len = wcslen(report);
        fmt(report + len, MAX_PATH - (int)len, L"\\disk_%03d_%ls.txt", id, date);
        verify_write_report(report, id, vi->label, vi->serialStr, date, &exp, &job.files,
                            &vs, job.enumErrors);

        show_disc(id);
        status_text(1, T(S_VERIFY_STATUS), id, result_name(st), notes);
        fmt_size(vs.okBytes, okSize, 32);
        if (msg_box(MB_YESNO | (st == ST_OK ? MB_ICONINFORMATION : MB_ICONWARNING), title,
                    T(S_VERIFY_RESULT), id, result_name(st), vs.ok, vs.expected, okSize,
                    vs.mismatch, vs.unreadable, vs.missing, vs.extra, report) == IDYES)
            ShellExecuteW(g_main, L"open", report, NULL, NULL, SW_SHOWNORMAL);
    }
    fl_free(&job.files);
    fl_free(&exp);
    return;

dberr:
    db_rollback();
    MessageBoxW(g_main, db_error(), T(S_DB_WRITE_ERR), MB_ICONERROR);
    fl_free(&job.files);
    fl_free(&exp);
}

static void cmd_verify(void)
{
    VolumeInfo vi;
    wchar_t bound[16];
    int id;
    BOOL bind = FALSE;

    if (g_busy || !db_is_open()) return;
    if (!pick_inserted(&vi)) return;

    id = db_find_by_serial(vi.serialStr);
    if (!id) {
        if (!g_selDisc) {
            msg_box(MB_ICONINFORMATION, NULL, T(S_NOT_LINKED_SELECT), vi.letter, vi.label, vi.serialStr);
            return;
        }
        if (db_disc_serial(g_selDisc, bound, 16)) {
            msg_box(MB_ICONWARNING, NULL, T(S_OTHER_DISC), g_selDisc, bound, vi.serialStr);
            return;
        }
        if (msg_box(MB_YESNO | MB_ICONQUESTION, NULL, T(S_VERIFY_AS_Q),
                    vi.letter, vi.label, g_selDisc, vi.serialStr) != IDYES)
            return;
        id = g_selDisc;
        bind = TRUE;
    }
    g_busy++;
    run_verify(id, &vi, bind);
    g_busy--;
}

static void cmd_media_info(void)
{
    VolumeInfo vi;
    MediaInfo mi;
    DiscInfo card;
    wchar_t msg[1024], pref[64], layers[32] = L"";
    BOOL ok;
    int id, n;

    if (g_busy || !db_is_open()) return;
    if (!pick_inserted(&vi)) return;
    ok = mediaid_read(vi.letter, &mi);
    id = db_find_by_serial(vi.serialStr);
    if (mi.layers) fmt(layers, 32, T(S_MI_LAYERS), mi.layers);

    n = _snwprintf(msg, 1023, T(S_MI_HEAD), vi.letter, vi.label, vi.serialStr,
                   mi.profileName[0] ? mi.profileName : T(S_MI_UNKNOWN), layers,
                   ok ? mi.mediaId : mi.error);
    if (n < 0) n = 0;
    msg[1023] = 0;

    if (!id) {
        fmt(msg + n, 1024 - n, L"%ls", T(S_MI_NOT_IN_DB));
        MessageBoxW(g_main, msg, T(S_MI_TITLE), MB_ICONINFORMATION);
        return;
    }
    db_get_disc(id, &card);
    show_disc(id);
    if (!ok || mediaid_same(card.media, mi.mediaId) || mediaid_same(card.media, mi.mediaBase)) {
        fmt(msg + n, 1024 - n, T(S_MI_IN_DB), id, card.media[0] ? card.media : T(S_MI_EMPTY),
            ok ? T(S_MI_MATCHES) : L"");
        MessageBoxW(g_main, msg, T(S_MI_TITLE), MB_ICONINFORMATION);
        return;
    }
    mediaid_preferred(&mi, pref, 64);
    fmt(msg + n, 1024 - n, T(S_MI_WRITE_Q), id, card.media[0] ? card.media : T(S_MI_EMPTY), pref);
    if (MessageBoxW(g_main, msg, T(S_MI_TITLE), MB_YESNO | MB_ICONQUESTION) == IDYES) {
        if (db_set_media(id, pref)) {
            load_discs();
            select_disc(id);
            status_text(1, T(S_MI_WRITTEN), id, pref);
        } else {
            MessageBoxW(g_main, db_error(), T(S_DB_WRITE_ERR), MB_ICONERROR);
        }
    }
}

static void cmd_edit(void)
{
    DiscInfo d;
    int id = g_selDisc;
    if (g_busy || !id) return;
    if (!db_get_disc(id, &d)) return;
    g_busy++;
    if (dlg_disc(g_main, &d, FALSE)) {
        if (db_begin() && db_update_disc(&d) && db_commit()) {
            load_discs();
            select_disc(id);
            status_text(1, T(S_SAVED), id);
        } else {
            db_rollback();
            MessageBoxW(g_main, db_error(), T(S_DB_WRITE_ERR), MB_ICONERROR);
        }
    }
    g_busy--;
}

static void cmd_delete(void)
{
    int id = g_selDisc;
    if (g_busy || !id) return;
    if (msg_box(MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2, NULL, T(S_DELETE_Q), id) != IDYES) return;
    if (db_begin() && db_delete_disc(id) && db_commit()) {
        g_selDisc = 0;
        load_discs();
        status_text(1, T(S_DELETED), id);
    } else {
        db_rollback();
        MessageBoxW(g_main, db_error(), T(S_DB_WRITE_ERR), MB_ICONERROR);
    }
}

static void cmd_about(void)
{
    msg_box(MB_ICONINFORMATION, T(S_ABOUT_TITLE), T(S_ABOUT_BODY),
            APP_NAME, APP_VERSION, APP_URL, WIDEN(SQLITE_VERSION));
}

/* ---------- window ---------- */

static HMENU build_menu(void)
{
    HMENU bar = CreateMenu(), m, lang;

    lang = CreatePopupMenu();
    AppendMenuW(lang, MF_STRING | (lang_get() == LNG_EN ? MF_CHECKED : 0), IDM_LANG_EN, L"English");
    AppendMenuW(lang, MF_STRING | (lang_get() == LNG_RU ? MF_CHECKED : 0), IDM_LANG_RU, L"Русский");

    m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING, IDM_NEWDB, T(S_M_NEWDB));
    AppendMenuW(m, MF_STRING, IDM_OPEN, T(S_M_OPEN));
    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m, MF_POPUP, (UINT_PTR)lang, T(S_M_LANG));
    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m, MF_STRING, IDM_EXIT, T(S_M_EXIT));
    AppendMenuW(bar, MF_POPUP, (UINT_PTR)m, T(S_M_FILE));

    m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING, IDM_DETECT, T(S_M_DETECT));
    AppendMenuW(m, MF_STRING, IDM_ADD, T(S_M_ADD));
    AppendMenuW(m, MF_STRING, IDM_VERIFY, T(S_M_VERIFY));
    AppendMenuW(m, MF_STRING, IDM_MEDIAINFO, T(S_M_MEDIAINFO));
    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m, MF_STRING, IDM_EDIT, T(S_M_EDIT));
    AppendMenuW(m, MF_STRING, IDM_DELETE, T(S_M_DELETE));
    AppendMenuW(m, MF_STRING, IDM_UNBIND, T(S_M_UNBIND));
    AppendMenuW(bar, MF_POPUP, (UINT_PTR)m, T(S_M_DISC));

    m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING, IDM_FIND, T(S_M_FIND));
    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m, MF_STRING, IDM_ABOUT, T(S_M_ABOUT));
    AppendMenuW(bar, MF_POPUP, (UINT_PTR)m, T(S_M_HELP));
    return bar;
}

/* Re-applies every visible string after a language switch */
static void apply_language(void)
{
    HMENU old = GetMenu(g_main);
    int i;

    SetMenu(g_main, build_menu());
    if (old) DestroyMenu(old);
    DrawMenuBar(g_main);

    SetWindowTextW(g_searchLbl, T(S_SEARCH_LBL));
    SendMessageW(g_search, EM_SETCUEBANNER, TRUE, (LPARAM)T(S_SEARCH_CUE));
    InvalidateRect(g_search, NULL, TRUE);       /* the cue banner is only drawn on repaint */
    for (i = 0; i < (int)(sizeof DISC_COLS / sizeof DISC_COLS[0]); i++)
        lv_set_column_title(g_discs, i, T(DISC_COLS[i].title));
    for (i = 0; i < (int)(sizeof FILE_COLS / sizeof FILE_COLS[0]); i++)
        lv_set_column_title(g_files, i, T(FILE_COLS[i].title));
    status_text(1, L"");
    if (db_is_open()) load_discs();             /* status names and counters */
    else status_text(0, L"%ls", T(S_DB_NOT_OPEN));
}

static void create_font(void)
{
    NONCLIENTMETRICSW ncm;
    HDC dc;
    TEXTMETRICW tm;
    HGDIOBJ old;

    /* built with _WIN32_WINNT=0x0501: no iPaddedBorderWidth, so the size works on XP and Win11 */
    ZeroMemory(&ncm, sizeof ncm);
    ncm.cbSize = sizeof ncm;
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof ncm, &ncm, 0))
        g_font = CreateFontIndirectW(&ncm.lfMessageFont);
    if (!g_font) g_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    dc = GetDC(NULL);
    old = SelectObject(dc, g_font);
    GetTextMetricsW(dc, &tm);
    SelectObject(dc, old);
    ReleaseDC(NULL, dc);
    g_fontH = tm.tmHeight + tm.tmExternalLeading;
}

static void on_create(HWND hw)
{
    int parts[2], i;
    wchar_t path[MAX_PATH];

    g_main = hw;
    create_font();

    g_searchLbl = CreateWindowExW(0, L"STATIC", T(S_SEARCH_LBL), WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
        0, 0, 0, 0, hw, (HMENU)IDC_SEARCHLBL, g_inst, NULL);
    g_search = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        0, 0, 0, 0, hw, (HMENU)IDC_SEARCH, g_inst, NULL);
    SendMessageW(g_search, EM_SETCUEBANNER, TRUE, (LPARAM)T(S_SEARCH_CUE));

    g_discs = make_listview(IDC_DISCS);
    for (i = 0; i < (int)(sizeof DISC_COLS / sizeof DISC_COLS[0]); i++)
        lv_add_column(g_discs, i, T(DISC_COLS[i].title), DISC_COLS[i].width);

    g_filesLbl = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
        0, 0, 0, 0, hw, (HMENU)IDC_FILESLBL, g_inst, NULL);
    g_files = make_listview(IDC_FILES);
    for (i = 0; i < (int)(sizeof FILE_COLS / sizeof FILE_COLS[0]); i++)
        lv_add_column(g_files, i, T(FILE_COLS[i].title), FILE_COLS[i].width);

    g_status = CreateWindowExW(0, STATUSCLASSNAMEW, L"", WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0, hw, (HMENU)IDC_STATUS, g_inst, NULL);
    parts[0] = MulDiv(220, g_fontH, 13);
    parts[1] = -1;
    SendMessageW(g_status, SB_SETPARTS, 2, (LPARAM)parts);

    SendMessageW(g_searchLbl, WM_SETFONT, (WPARAM)g_font, TRUE);
    SendMessageW(g_search,    WM_SETFONT, (WPARAM)g_font, TRUE);
    SendMessageW(g_filesLbl,  WM_SETFONT, (WPARAM)g_font, TRUE);
    SendMessageW(g_status,    WM_SETFONT, (WPARAM)g_font, TRUE);

    /* Default database lives next to the exe; the first run creates an empty one */
    exe_dir(path, MAX_PATH);
    wcscat(path, L"backuplist.db");
    if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) create_db(path);
    else open_db(path);
}

static void on_size(int w, int h)
{
    RECT sr;
    int pad = MulDiv(6, g_fontH, 13), rowH = g_fontH + 10, lblW = MulDiv(50, g_fontH, 13);
    int y, statusH, avail, discsH;

    SendMessageW(g_status, WM_SIZE, 0, 0);
    GetWindowRect(g_status, &sr);
    statusH = sr.bottom - sr.top;

    y = pad;
    MoveWindow(g_searchLbl, pad, y, lblW, rowH, TRUE);
    MoveWindow(g_search, pad + lblW, y, w - 2 * pad - lblW, rowH, TRUE);
    y += rowH + pad;

    avail = h - statusH - y - rowH - 2 * pad;
    if (avail < 0) avail = 0;
    discsH = avail * 55 / 100;
    MoveWindow(g_discs, pad, y, w - 2 * pad, discsH, TRUE);
    y += discsH + pad / 2;
    MoveWindow(g_filesLbl, pad, y, w - 2 * pad, rowH, TRUE);
    y += rowH;
    MoveWindow(g_files, pad, y, w - 2 * pad, h - statusH - pad - y, TRUE);
}

static LRESULT on_notify(NMHDR *nh)
{
    if (nh->hwndFrom != g_discs) return 0;

    if (nh->code == LVN_ITEMCHANGED) {
        NMLISTVIEW *nl = (NMLISTVIEW *)nh;
        if ((nl->uChanged & LVIF_STATE) && (nl->uNewState & LVIS_SELECTED)
            && !(nl->uOldState & LVIS_SELECTED))
            load_files(LOWORD(nl->lParam));
    } else if (nh->code == NM_DBLCLK) {
        cmd_edit();
    } else if (nh->code == LVN_KEYDOWN) {
        if (((NMLVKEYDOWN *)nh)->wVKey == VK_DELETE) cmd_delete();
    } else if (nh->code == LVN_COLUMNCLICK) {
        int col = ((NMLISTVIEW *)nh)->iSubItem;
        if (col == g_sortCol) g_sortDesc = !g_sortDesc;
        else { g_sortCol = col; g_sortDesc = 0; }
        load_discs();
    } else if (nh->code == NM_CUSTOMDRAW) {
        NMLVCUSTOMDRAW *cd = (NMLVCUSTOMDRAW *)nh;
        if (cd->nmcd.dwDrawStage == CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;
        if (cd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
            int st = HIWORD(cd->nmcd.lItemlParam);
            if (st == ST_DEAD)    cd->clrTextBk = RGB(255, 205, 205);
            if (st == ST_PARTIAL) cd->clrTextBk = RGB(255, 238, 185);
            return CDRF_NEWFONT;
        }
        return CDRF_DODEFAULT;
    }
    return 0;
}

static LRESULT CALLBACK wnd_proc(HWND hw, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE:
        on_create(hw);
        return 0;

    case WM_SIZE:
        on_size(LOWORD(lp), HIWORD(lp));
        return 0;

    case WM_GETMINMAXINFO:
        ((MINMAXINFO *)lp)->ptMinTrackSize.x = 600;
        ((MINMAXINFO *)lp)->ptMinTrackSize.y = 400;
        return 0;

    case WM_NOTIFY:
        return on_notify((NMHDR *)lp);

    case WM_TIMER:
        if (wp == TIMER_SEARCH) {
            KillTimer(hw, TIMER_SEARCH);
            apply_search();
        }
        return 0;

    case WM_DEVICECHANGE: {
        BOOL arrived;
        DWORD mask = volume_media_mask(wp, lp, &arrived);
        int i;
        for (i = 0; i < 26; i++) {
            if (!(mask & (1u << i))) continue;
            if (arrived)
                /* no dialogs or queries inside a broadcast message */
                PostMessageW(hw, WM_APP_VOLUME, L'A' + i, 0);
            else
                status_text(1, T(S_EJECTED), L'A' + i);
        }
        return TRUE;
    }

    case WM_APP_VOLUME:
        handle_volume((wchar_t)wp);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wp) == IDC_SEARCH && HIWORD(wp) == EN_CHANGE) {
            SetTimer(hw, TIMER_SEARCH, 300, NULL);    /* search after a pause in typing */
            return 0;
        }
        switch (LOWORD(wp)) {
        case IDM_NEWDB:     cmd_new_db(); break;
        case IDM_OPEN:      cmd_open(); break;
        case IDM_EXIT:      DestroyWindow(hw); break;
        case IDM_LANG_EN:   lang_set(LNG_EN); apply_language(); break;
        case IDM_LANG_RU:   lang_set(LNG_RU); apply_language(); break;
        case IDM_DETECT:    detect_inserted(); break;
        case IDM_ADD:       cmd_add(NULL); break;
        case IDM_VERIFY:    cmd_verify(); break;
        case IDM_EDIT:      cmd_edit(); break;
        case IDM_DELETE:    cmd_delete(); break;
        case IDM_MEDIAINFO: cmd_media_info(); break;
        case IDM_UNBIND:    cmd_unbind(); break;
        case IDOK:          /* Enter in the window (via IsDialogMessage) */
            if (GetFocus() == g_discs) cmd_edit();
            break;
        case IDM_FIND:
            SetFocus(g_search);
            SendMessageW(g_search, EM_SETSEL, 0, -1);
            break;
        case IDM_ABOUT:     cmd_about(); break;
        }
        return 0;

    case WM_DESTROY:
        db_close();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hw, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE prev, LPWSTR cmd, int show)
{
    INITCOMMONCONTROLSEX icc = { sizeof icc, ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES | ICC_PROGRESS_CLASS };
    WNDCLASSEXW wc = {0};
    ACCEL acc[] = {
        { FVIRTKEY,            VK_F5, IDM_DETECT },
        { FVIRTKEY,            VK_F6, IDM_VERIFY },
        { FVIRTKEY | FCONTROL, 'F',   IDM_FIND },
        { FVIRTKEY | FCONTROL, 'O',   IDM_OPEN },
        { FVIRTKEY | FCONTROL, 'N',   IDM_ADD },
        { FVIRTKEY | FCONTROL, 'I',   IDM_MEDIAINFO },
    };
    HACCEL hacc;
    HWND hw;
    MSG m;
    (void)prev; (void)cmd;

    g_inst = inst;
    lang_init();
    InitCommonControlsEx(&icc);

    wc.cbSize = sizeof wc;
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = inst;
    wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(1));
    if (!wc.hIcon) wc.hIcon = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"Backuplist2Main";
    RegisterClassExW(&wc);

    hw = CreateWindowExW(0, wc.lpszClassName, APP_NAME, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1024, 700, NULL, build_menu(), inst, NULL);
    ShowWindow(hw, show);
    UpdateWindow(hw);

    hacc = CreateAcceleratorTableW(acc, sizeof acc / sizeof acc[0]);
    while (GetMessageW(&m, NULL, 0, 0) > 0) {
        if (TranslateAcceleratorW(hw, hacc, &m)) continue;
        if (!IsDialogMessageW(hw, &m)) {
            TranslateMessage(&m);
            DispatchMessageW(&m);
        }
    }
    DestroyAcceleratorTable(hacc);
    return (int)m.wParam;
}
