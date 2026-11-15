/* mediaid.h — reads the Media ID and media type straight from the drive (SCSI pass-through) */
#ifndef BL_MEDIAID_H
#define BL_MEDIAID_H

#include <windows.h>

typedef struct {
    int     profile;              /* current MMC profile, 0 = unknown */
    wchar_t profileName[24];      /* "BD-R", "DVD+R DL" ... */
    wchar_t mediaId[40];          /* "VERBAT-IMe-000" */
    wchar_t mediaBase[40];        /* "VERBAT-IMe", without the revision */
    int     layers;               /* 0 = unknown */
    int     kind;                 /* index into MEDIA_KINDS or -1 */
    wchar_t error[200];           /* why it failed */
} MediaInfo;

/* TRUE = Media ID read. On FALSE the profile/kind may still be set
   and error holds a message for the user. */
BOOL mediaid_read(wchar_t letter, MediaInfo *mi);

/* Picks the spelling already used in the database: if this Media ID is in some card
   (ignoring case and dashes, with or without the revision) that spelling is returned,
   otherwise mediaId as read. */
void mediaid_preferred(const MediaInfo *mi, wchar_t *out, int cch);

/* Whether two Media IDs are the same (same rules) */
BOOL mediaid_same(const wchar_t *a, const wchar_t *b);

#endif
