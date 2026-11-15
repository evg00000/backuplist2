/* volume.h — identifying the inserted optical disc */
#ifndef BL_VOLUME_H
#define BL_VOLUME_H

#include <windows.h>

typedef struct {
    wchar_t letter;
    wchar_t label[MAX_PATH + 1];
    DWORD   serial;
    wchar_t serialStr[16];   /* "1A2B-3C4D", as the vol command shows it */
} VolumeInfo;

/* Reads the volume label and serial. FALSE = no readable disc in the drive. */
BOOL volume_query(wchar_t letter, VolumeInfo *vi);

/* Finds the first optical drive that holds a disc. */
BOOL volume_first_optical(VolumeInfo *vi);

/* Parses WM_DEVICECHANGE. Returns a drive-letter bit mask (bit 0 = A:)
   for media inserted/removed in a drive, otherwise 0. */
DWORD volume_media_mask(WPARAM wp, LPARAM lp, BOOL *arrived);

#endif
