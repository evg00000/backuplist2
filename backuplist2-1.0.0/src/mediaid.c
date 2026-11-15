/* mediaid.c — SPTI (IOCTL_SCSI_PASS_THROUGH): works from XP to Windows 11.
   A 32-bit exe on 64-bit Windows is served by the driver's WOW64 thunk. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mediaid.h"
#include "mediaid_parse.h"
#include "db.h"
#include "lang.h"

#define IOCTL_SCSI_PASS_THROUGH_BL 0x0004D004   /* CTL_CODE(4, 0x401, BUFFERED, R|W) */
#define SCSI_DATA_IN 1
#define REPLY_MAX    4100                          /* BD DI: 4 + 4096 */

typedef struct {
    USHORT    Length;
    UCHAR     ScsiStatus;
    UCHAR     PathId;
    UCHAR     TargetId;
    UCHAR     Lun;
    UCHAR     CdbLength;
    UCHAR     SenseInfoLength;
    UCHAR     DataIn;
    ULONG     DataTransferLength;
    ULONG     TimeOutValue;
    ULONG_PTR DataBufferOffset;
    ULONG     SenseInfoOffset;
    UCHAR     Cdb[16];
} SptHeader;

typedef struct {
    SptHeader spt;
    ULONG     filler;               /* alignment */
    UCHAR     sense[32];
    UCHAR     data[REPLY_MAX + 60];
} SptBuffer;

/* Returns: >=0 data length; -1 IOCTL failed (GetLastError); -2 the drive rejected the command */
static int scsi_in(HANDLE h, const BYTE *cdb, int cdbLen, BYTE *out, int outLen, DWORD *err)
{
    SptBuffer *b = (SptBuffer *)calloc(1, sizeof *b);
    DWORD ret = 0;
    int n;

    *err = 0;
    if (!b) return -1;
    b->spt.Length = sizeof(SptHeader);
    b->spt.CdbLength = (UCHAR)cdbLen;
    b->spt.SenseInfoLength = sizeof b->sense;
    b->spt.DataIn = SCSI_DATA_IN;
    b->spt.DataTransferLength = outLen;
    b->spt.TimeOutValue = 15;
    b->spt.DataBufferOffset = (ULONG_PTR)((BYTE *)b->data - (BYTE *)b);
    b->spt.SenseInfoOffset = (ULONG)((BYTE *)b->sense - (BYTE *)b);
    memcpy(b->spt.Cdb, cdb, cdbLen);

    if (!DeviceIoControl(h, IOCTL_SCSI_PASS_THROUGH_BL, b, sizeof *b, b, sizeof *b, &ret, NULL)) {
        *err = GetLastError();
        free(b);
        return -1;
    }
    if (b->spt.ScsiStatus != 0) {
        *err = b->sense[2] & 0x0F;        /* sense key */
        free(b);
        return -2;
    }
    n = (int)b->spt.DataTransferLength;
    if (n > outLen) n = outLen;
    memcpy(out, b->data, n);
    free(b);
    return n;
}

static int read_structure(HANDLE h, int mediaType, int format, BYTE *out, int alloc, DWORD *err)
{
    BYTE cdb[12] = { 0xAD };
    cdb[1] = (BYTE)(mediaType & 0x0F);
    cdb[7] = (BYTE)format;
    cdb[8] = (BYTE)(alloc >> 8);
    cdb[9] = (BYTE)(alloc & 0xFF);
    return scsi_in(h, cdb, 12, out, alloc, err);
}

static void a2w(const char *s, wchar_t *w, int cch)
{
    MultiByteToWideChar(CP_ACP, 0, s, -1, w, cch);
    w[cch - 1] = 0;
}

static int kind_for(const wchar_t *type, int capacity)
{
    int i;
    for (i = 0; i < MEDIA_KIND_COUNT; i++)
        if (!wcscmp(MEDIA_KINDS[i].type, type) && MEDIA_KINDS[i].capacity == capacity) return i;
    return -1;
}

BOOL mediaid_read(wchar_t letter, MediaInfo *mi)
{
    wchar_t dev[8] = L"\\\\.\\A:";
    HANDLE h;
    BYTE *r;
    BYTE cfg[8], cdb[10] = { 0x46, 0, 0, 0, 0, 0, 0, 0, 8, 0 };   /* GET CONFIGURATION */
    DWORD err;
    int n;
    char id[40] = "", base[40] = "";
    const char *pn;

    ZeroMemory(mi, sizeof *mi);
    mi->kind = -1;
    dev[4] = letter;

    h = CreateFileW(dev, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        err = GetLastError();
        if (err == ERROR_ACCESS_DENIED)
            wcscpy(mi->error, T(S_E_ACCESS));
        else
            _snwprintf(mi->error, 199, T(S_E_OPEN), err);
        return FALSE;
    }

    n = scsi_in(h, cdb, 10, cfg, sizeof cfg, &err);
    if (n < 8) {
        if (n == -1 && err == ERROR_ACCESS_DENIED)
            wcscpy(mi->error, T(S_E_SPTI_DENIED));
        else if (n == -1)
            _snwprintf(mi->error, 199, T(S_E_SPTI), err);
        else
            wcscpy(mi->error, T(S_E_NOPROFILE));
        CloseHandle(h);
        return FALSE;
    }
    mi->profile = (cfg[6] << 8) | cfg[7];
    pn = mid_profile_name(mi->profile);
    if (pn) a2w(pn, mi->profileName, 24);
    else _snwprintf(mi->profileName, 23, T(S_E_PROFILE_FMT), mi->profile);

    if (mi->profile == 0) {
        wcscpy(mi->error, T(S_E_NODISC));
        CloseHandle(h);
        return FALSE;
    }

    r = (BYTE *)calloc(1, REPLY_MAX);
    if (!r) { CloseHandle(h); return FALSE; }

    if (mid_profile_is_bd(mi->profile)) {
        n = read_structure(h, 1, 0x00, r, REPLY_MAX, &err);
        if (n > 0 && mid_parse_bd_di(r, n, id, base, sizeof id, &mi->layers)) {
            if (mi->layers) mi->kind = kind_for(L"BD-R", mi->layers == 1 ? 25 : mi->layers == 2 ? 50 : 100);
        } else {
            wcscpy(mi->error, T(S_E_BD));
        }
    } else if (mid_profile_is_dvd_plus(mi->profile)) {
        mi->layers = mid_profile_dvd_layers(mi->profile);
        mi->kind = kind_for(L"DVD", mi->layers == 2 ? 9 : 5);
        n = read_structure(h, 0, 0x11, r, 4 + 256, &err);            /* ADIP */
        if (!(n > 0 && mid_parse_dvd_plus(r, n, id, base, sizeof id))) {
            n = read_structure(h, 0, 0x00, r, 2052, &err);          /* ADIP copy inside PFI */
            if (!(n > 0 && mid_parse_dvd_plus(r, n, id, base, sizeof id)))
                wcscpy(mi->error, T(S_E_ADIP));
        }
    } else if (mid_profile_is_dvd_minus(mi->profile)) {
        mi->layers = mid_profile_dvd_layers(mi->profile);
        mi->kind = kind_for(L"DVD", mi->layers == 2 ? 9 : 5);
        n = read_structure(h, 0, 0x0E, r, 4 + 256, &err);            /* pre-pit */
        if (n > 0 && mid_parse_dvd_minus(r, n, id, sizeof id))
            strcpy(base, id);
        else
            wcscpy(mi->error, T(S_E_PREPIT));
    } else if (mid_profile_is_cd(mi->profile)) {
        mi->layers = 1;
        mi->kind = kind_for(L"CD", 1);
        wcscpy(mi->error, T(S_E_CD));
    } else {
        wcscpy(mi->error, T(S_E_NOMID));
    }

    free(r);
    CloseHandle(h);
    if (!id[0]) return FALSE;
    a2w(id, mi->mediaId, 40);
    a2w(base, mi->mediaBase, 40);
    return TRUE;
}

BOOL mediaid_same(const wchar_t *a, const wchar_t *b)
{
    char x[64], y[64];
    WideCharToMultiByte(CP_ACP, 0, a, -1, x, sizeof x, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, b, -1, y, sizeof y, NULL, NULL);
    x[63] = y[63] = 0;
    return mid_same(x, y);
}

typedef struct { const MediaInfo *mi; wchar_t *out; int cch; int found; } PrefCtx;

static void pref_cb(void *ctx, const wchar_t *s)
{
    PrefCtx *p = (PrefCtx *)ctx;
    if (p->found) return;
    if (mediaid_same(s, p->mi->mediaId) || (p->mi->mediaBase[0] && mediaid_same(s, p->mi->mediaBase))) {
        wcsncpy(p->out, s, p->cch - 1);
        p->out[p->cch - 1] = 0;
        p->found = 1;
    }
}

void mediaid_preferred(const MediaInfo *mi, wchar_t *out, int cch)
{
    PrefCtx p;
    p.mi = mi; p.out = out; p.cch = cch; p.found = 0;
    wcsncpy(out, mi->mediaId, cch - 1);
    out[cch - 1] = 0;
    db_distinct("media_id", pref_cb, &p);
}
