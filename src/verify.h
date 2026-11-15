/* verify.h — comparing a scanned disc with the reference from the database, and the report */
#ifndef BL_VERIFY_H
#define BL_VERIFY_H

#include "scan.h"

typedef struct {
    int ok, mismatch, unreadable, missing, extra;
    int expected;                    /* files in the reference */
    unsigned long long okBytes;
} VerifyStats;

/* exp comes from the database (sorted), act from the scan.
   Sets state in both lists and counts the totals. */
void verify_compare(FileList *exp, FileList *act, VerifyStats *st);

/* ST_OK / ST_PARTIAL / ST_DEAD from the totals */
int verify_status(const VerifyStats *st);

/* Plain-text report (UTF-8 with BOM so that Notepad on XP reads it). */
BOOL verify_write_report(const wchar_t *path, int discId, const wchar_t *label,
                         const wchar_t *serial, const wchar_t *date,
                         const FileList *exp, const FileList *act, const VerifyStats *st,
                         int enumErrors);

#endif
