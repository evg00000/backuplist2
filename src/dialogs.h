/* dialogs.h — the disc card and the progress window */
#ifndef BL_DIALOGS_H
#define BL_DIALOGS_H

#include <windows.h>
#include "db.h"
#include "scan.h"

/* Disc card; isNew allows editing the number. TRUE if OK was pressed. */
BOOL dlg_disc(HWND parent, DiscInfo *d, BOOL isNew);

/* Scans the drive and shows progress.
   TRUE = finished (result in job->files), FALSE = cancelled.
   The caller frees job->files with fl_free afterwards. */
BOOL dlg_scan(HWND parent, ScanJob *job, wchar_t letter, const wchar_t *title);

void fmt_size(unsigned long long bytes, wchar_t *out, int cch);

#endif
