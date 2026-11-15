/* volume.c — volume label/serial and media change notifications (XP SP2 .. Windows 11) */
#include <windows.h>
#include <dbt.h>
#include <stdio.h>
#include "volume.h"

BOOL volume_query(wchar_t letter, VolumeInfo *vi)
{
    wchar_t root[4] = { letter, L':', L'\\', 0 };
    UINT oldMode;
    BOOL ok;

    ZeroMemory(vi, sizeof *vi);
    vi->letter = letter;

    /* Without this an empty drive pops up the system "Insert a disk" box */
    oldMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    ok = GetVolumeInformationW(root, vi->label, MAX_PATH + 1, &vi->serial,
                               NULL, NULL, NULL, 0);
    SetErrorMode(oldMode);
    if (!ok) return FALSE;

    _snwprintf(vi->serialStr, 16, L"%04X-%04X",
               (unsigned)HIWORD(vi->serial), (unsigned)LOWORD(vi->serial));
    vi->serialStr[15] = 0;
    return TRUE;
}

BOOL volume_first_optical(VolumeInfo *vi)
{
    DWORD mask = GetLogicalDrives();
    int i;
    for (i = 0; i < 26; i++) {
        wchar_t root[4] = { (wchar_t)(L'A' + i), L':', L'\\', 0 };
        if (!(mask & (1u << i))) continue;
        if (GetDriveTypeW(root) != DRIVE_CDROM) continue;
        if (volume_query(root[0], vi)) return TRUE;
    }
    return FALSE;
}

DWORD volume_media_mask(WPARAM wp, LPARAM lp, BOOL *arrived)
{
    DEV_BROADCAST_HDR *hdr = (DEV_BROADCAST_HDR *)lp;
    DEV_BROADCAST_VOLUME *v;

    if (wp != DBT_DEVICEARRIVAL && wp != DBT_DEVICEREMOVECOMPLETE) return 0;
    if (!hdr || hdr->dbch_devicetype != DBT_DEVTYP_VOLUME) return 0;
    v = (DEV_BROADCAST_VOLUME *)hdr;
    if (!(v->dbcv_flags & DBTF_MEDIA)) return 0;   /* media change in a drive only */
    *arrived = (wp == DBT_DEVICEARRIVAL);
    return v->dbcv_unitmask;
}
