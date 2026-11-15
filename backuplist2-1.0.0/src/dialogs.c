/* dialogs.c — the disc card and the scan progress window */
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "dialogs.h"
#include "resource.h"
#include "lang.h"

void fmt_size(unsigned long long b, wchar_t *out, int cch)
{
    if (b >= 1024ULL * 1024 * 1024)
        _snwprintf(out, cch, T(S_SIZE_GB), b / (1024.0 * 1024 * 1024));
    else if (b >= 1024ULL * 1024)
        _snwprintf(out, cch, T(S_SIZE_MB), b / (1024.0 * 1024));
    else
        _snwprintf(out, cch, T(S_SIZE_KB), (unsigned)((b + 1023) / 1024));
    out[cch - 1] = 0;
}

/* ---------- disc card ---------- */

typedef struct { DiscInfo *d; BOOL isNew; } DiscDlg;

static const struct { int ctl, str; } CARD_LABELS[] = {
    { IDC_DL_ID, S_L_ID }, { IDC_DL_KIND, S_L_KIND }, { IDC_DL_WRITTEN, S_L_WRITTEN },
    { IDC_DL_CTYPE, S_L_CTYPE }, { IDC_DL_PRIO, S_L_PRIO }, { IDC_DL_BRAND, S_L_BRAND },
    { IDC_DL_MEDIA, S_L_MEDIA }, { IDC_DL_LABEL, S_L_LABEL }, { IDC_DL_SERIAL, S_L_SERIAL },
    { IDC_DL_CONTENTS, S_L_CONTENTS }, { IDCANCEL, S_CANCEL },
};

static void combo_add_unique(void *ctx, const wchar_t *s)
{
    HWND cb = (HWND)ctx;
    if (SendMessageW(cb, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)s) == CB_ERR)
        SendMessageW(cb, CB_ADDSTRING, 0, (LPARAM)s);
}

/* Reads a field and trims spaces at both ends */
static void get_text(HWND dlg, int id, wchar_t *out, int cch)
{
    wchar_t *p, *q;
    GetDlgItemTextW(dlg, id, out, cch);
    for (p = out; *p == L' '; p++) ;
    if (p != out) memmove(out, p, (wcslen(p) + 1) * sizeof(wchar_t));
    for (q = out + wcslen(out); q > out && q[-1] == L' '; q--) ;
    *q = 0;
}

static BOOL valid_date(const wchar_t *s)
{
    int y, m, d;
    if (!*s) return TRUE;
    if (wcslen(s) != 10 || swscanf(s, L"%4d-%2d-%2d", &y, &m, &d) != 3) return FALSE;
    return y >= 1990 && y <= 2100 && m >= 1 && m <= 12 && d >= 1 && d <= 31;
}

static void card_init(HWND dlg, DiscDlg *dd)
{
    static const int CTYPES[] = { S_CT_VIDEO, S_CT_AUDIO, S_CT_SOFTWARE, S_CT_MIXED, S_CT_DOCS };
    static const int PRIOS[] = { S_PRIO_1, S_PRIO_2, S_PRIO_3 };
    DiscInfo *d = dd->d;
    wchar_t buf[320];
    HWND cb;
    int i;

    for (i = 0; i < (int)(sizeof CARD_LABELS / sizeof CARD_LABELS[0]); i++)
        SetDlgItemTextW(dlg, CARD_LABELS[i].ctl, T(CARD_LABELS[i].str));

    if (dd->isNew) wcscpy(buf, T(S_CARD_NEW));
    else _snwprintf(buf, 320, T(S_CARD_TITLE), d->id);
    buf[319] = 0;
    SetWindowTextW(dlg, buf);

    SetDlgItemInt(dlg, IDC_D_ID, d->id, FALSE);
    if (!dd->isNew) SendDlgItemMessageW(dlg, IDC_D_ID, EM_SETREADONLY, TRUE, 0);

    cb = GetDlgItem(dlg, IDC_D_KIND);
    for (i = 0; i < MEDIA_KIND_COUNT; i++)
        SendMessageW(cb, CB_ADDSTRING, 0, (LPARAM)MEDIA_KINDS[i].title);
    SendMessageW(cb, CB_SETCURSEL, d->kind, 0);

    SetDlgItemTextW(dlg, IDC_D_WRITTEN, d->written);

    /* values already used in the database first, then the default suggestions */
    cb = GetDlgItem(dlg, IDC_D_CTYPE);
    db_distinct("content_type", combo_add_unique, cb);
    for (i = 0; i < 5; i++) combo_add_unique(cb, T(CTYPES[i]));
    SetWindowTextW(cb, d->ctype);

    cb = GetDlgItem(dlg, IDC_D_PRIO);
    for (i = 0; i < 3; i++) SendMessageW(cb, CB_ADDSTRING, 0, (LPARAM)T(PRIOS[i]));
    SendMessageW(cb, CB_SETCURSEL, (d->priority >= 1 && d->priority <= 3) ? d->priority - 1 : 1, 0);

    cb = GetDlgItem(dlg, IDC_D_BRAND);
    db_distinct("brand", combo_add_unique, cb);
    SetWindowTextW(cb, d->brand);

    cb = GetDlgItem(dlg, IDC_D_MEDIA);
    db_distinct("media_id", combo_add_unique, cb);
    SetWindowTextW(cb, d->media);

    SetDlgItemTextW(dlg, IDC_D_LABEL, d->label);
    SetDlgItemTextW(dlg, IDC_D_SERIAL, d->serial[0] ? d->serial : T(S_NOT_LINKED));
    SetDlgItemTextW(dlg, IDC_D_CONTENTS, d->contents);

    if (dd->isNew) {
        _snwprintf(buf, 320, L"%ls%ls%ls", d->hint, d->hint[0] ? L"\n" : L"", T(S_CARD_WILL_READ));
    } else {
        static const int NAMES[] = { S_ST_OK, S_ST_PARTIAL, S_ST_DEAD, S_ST_NOTCHECKED };
        _snwprintf(buf, 320, T(S_CARD_STATUS), T(NAMES[d->status]),
                   d->checked[0] ? T(S_CARD_CHECKED) : L"", d->checked);
    }
    buf[319] = 0;
    SetDlgItemTextW(dlg, IDC_D_STATUS, buf);
    SetFocus(GetDlgItem(dlg, dd->isNew ? IDC_D_CTYPE : IDC_D_CONTENTS));
}

static BOOL card_save(HWND dlg, DiscDlg *dd)
{
    DiscInfo *d = dd->d;
    BOOL ok;
    wchar_t date[32], *p;
    UINT id = GetDlgItemInt(dlg, IDC_D_ID, &ok, FALSE);

    if (!ok || id == 0 || id > 99999) {
        MessageBoxW(dlg, T(S_ERR_ID), T(S_CARD_CAPTION), MB_ICONWARNING);
        SetFocus(GetDlgItem(dlg, IDC_D_ID));
        return FALSE;
    }
    if (dd->isNew && db_id_exists((int)id)) {
        MessageBoxW(dlg, T(S_ERR_ID_EXISTS), T(S_CARD_CAPTION), MB_ICONWARNING);
        SetFocus(GetDlgItem(dlg, IDC_D_ID));
        return FALSE;
    }
    get_text(dlg, IDC_D_WRITTEN, date, 32);
    if (!valid_date(date)) {
        MessageBoxW(dlg, T(S_ERR_DATE), T(S_CARD_CAPTION), MB_ICONWARNING);
        SetFocus(GetDlgItem(dlg, IDC_D_WRITTEN));
        return FALSE;
    }
    d->id = (int)id;
    wcsncpy(d->written, date, 15);
    d->written[15] = 0;
    d->kind = (int)SendDlgItemMessageW(dlg, IDC_D_KIND, CB_GETCURSEL, 0, 0);
    if (d->kind < 0) d->kind = 0;
    d->priority = (int)SendDlgItemMessageW(dlg, IDC_D_PRIO, CB_GETCURSEL, 0, 0) + 1;
    get_text(dlg, IDC_D_CTYPE, d->ctype, 64);
    get_text(dlg, IDC_D_BRAND, d->brand, 64);
    get_text(dlg, IDC_D_MEDIA, d->media, 64);
    get_text(dlg, IDC_D_CONTENTS, d->contents, 4096);
    /* line breaks -> spaces: they look like garbage in the list */
    for (p = d->contents; *p; p++) if (*p == L'\r' || *p == L'\n') *p = L' ';
    return TRUE;
}

static INT_PTR CALLBACK disc_proc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
    DiscDlg *dd = (DiscDlg *)GetWindowLongPtrW(dlg, DWLP_USER);

    switch (msg) {
    case WM_INITDIALOG:
        dd = (DiscDlg *)lp;
        SetWindowLongPtrW(dlg, DWLP_USER, (LONG_PTR)dd);
        card_init(dlg, dd);
        return FALSE;               /* focus was set by card_init */

    case WM_COMMAND:
        if (LOWORD(wp) == IDOK) {
            if (card_save(dlg, dd)) EndDialog(dlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wp) == IDCANCEL) {
            EndDialog(dlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL dlg_disc(HWND parent, DiscInfo *d, BOOL isNew)
{
    DiscDlg dd;
    dd.d = d;
    dd.isNew = isNew;
    return DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_DISC), parent,
                           disc_proc, (LPARAM)&dd) == IDOK;
}

/* ---------- progress ---------- */

typedef struct {
    ScanJob *job;
    wchar_t letter;
    const wchar_t *title;
    DWORD started;
    BOOL cancelling;
} ScanDlg;

#define TIMER_PROGRESS 1

static void progress_update(HWND dlg, ScanDlg *sd)
{
    ScanProgress p;
    wchar_t done[32], total[32], stats[300], caption[200], eta[64] = L"";
    DWORD elapsed = GetTickCount() - sd->started;
    double secs, speed;
    int permille = 0;

    scan_get_progress(sd->job, &p);

    if (p.phase == 0) {
        _snwprintf(stats, 300, T(S_P_LISTING), p.filesTotal);
        stats[299] = 0;
        SetDlgItemTextW(dlg, IDC_P_PHASE, T(sd->cancelling ? S_P_STOPPING : S_P_STRUCT));
        SetDlgItemTextW(dlg, IDC_P_STATS, stats);
        SendDlgItemMessageW(dlg, IDC_P_BAR, PBM_SETPOS, 0, 0);
        return;
    }

    if (p.bytesTotal) permille = (int)(p.bytesDone * 1000 / p.bytesTotal);
    else if (p.filesTotal) permille = p.filesDone * 1000 / p.filesTotal;
    SendDlgItemMessageW(dlg, IDC_P_BAR, PBM_SETPOS, permille, 0);

    SetDlgItemTextW(dlg, IDC_P_PHASE, T(sd->cancelling ? S_P_STOPPING : S_P_HASHING));
    SetDlgItemTextW(dlg, IDC_P_FILE, p.current);

    fmt_size(p.bytesDone, done, 32);
    fmt_size(p.bytesTotal, total, 32);
    secs = elapsed / 1000.0;
    speed = secs > 1 ? p.bytesDone / secs / (1024.0 * 1024) : 0;
    if (speed > 0.1 && p.bytesTotal > p.bytesDone) {
        unsigned mins = (unsigned)((p.bytesTotal - p.bytesDone) / (speed * 1024 * 1024) / 60) + 1;
        _snwprintf(eta, 64, T(S_P_ETA), mins);
        eta[63] = 0;
    }
    _snwprintf(stats, 300, T(S_P_STATS), done, total, p.filesDone, p.filesTotal, speed, eta,
               p.unreadable ? T(S_P_ERRORS) : L"");
    stats[299] = 0;
    SetDlgItemTextW(dlg, IDC_P_STATS, stats);

    _snwprintf(caption, 200, L"%d%% \x2014 %ls", permille / 10, sd->title);
    caption[199] = 0;
    SetWindowTextW(dlg, caption);
}

static INT_PTR CALLBACK scan_proc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
    ScanDlg *sd = (ScanDlg *)GetWindowLongPtrW(dlg, DWLP_USER);

    switch (msg) {
    case WM_INITDIALOG:
        sd = (ScanDlg *)lp;
        SetWindowLongPtrW(dlg, DWLP_USER, (LONG_PTR)sd);
        SetWindowTextW(dlg, sd->title);
        SetDlgItemTextW(dlg, IDCANCEL, T(S_CANCEL));
        SendDlgItemMessageW(dlg, IDC_P_BAR, PBM_SETRANGE32, 0, 1000);
        sd->started = GetTickCount();
        if (!scan_start(sd->job, sd->letter)) {
            MessageBoxW(dlg, T(S_P_START_FAIL), sd->title, MB_ICONERROR);
            EndDialog(dlg, 0);
            return TRUE;
        }
        SetTimer(dlg, TIMER_PROGRESS, 250, NULL);
        progress_update(dlg, sd);
        return TRUE;

    case WM_TIMER:
        progress_update(dlg, sd);
        if (scan_finished(sd->job)) {
            KillTimer(dlg, TIMER_PROGRESS);
            EndDialog(dlg, sd->cancelling ? 0 : 1);
        }
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wp) != IDCANCEL) break;
        /* fallthrough */
    case WM_CLOSE:
        if (!sd->cancelling) {
            /* Do not wait for the thread here: it may be stuck on a bad sector.
               Raise the flag; the window closes once the thread exits. */
            sd->cancelling = TRUE;
            InterlockedExchange(&sd->job->cancel, 1);
            EnableWindow(GetDlgItem(dlg, IDCANCEL), FALSE);
            progress_update(dlg, sd);
        }
        return TRUE;
    }
    return FALSE;
}

BOOL dlg_scan(HWND parent, ScanJob *job, wchar_t letter, const wchar_t *title)
{
    ScanDlg sd;
    INT_PTR r;
    ZeroMemory(&sd, sizeof sd);
    sd.job = job;
    sd.letter = letter;
    sd.title = title;
    r = DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_PROGRESS), parent,
                        scan_proc, (LPARAM)&sd);
    scan_free(job);
    return r == 1;
}
