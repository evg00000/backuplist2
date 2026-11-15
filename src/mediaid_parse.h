/* mediaid_parse.h — parsing of drive replies (no Windows API, unit-tested on its own).
   Offsets from: MMC-5 6.23 READ DISC STRUCTURE, ECMA-279/337/349,
   dvd+rw-mediainfo and libburn doc/mediainfo.txt. */
#ifndef BL_MEDIAID_PARSE_H
#define BL_MEDIAID_PARSE_H

#include <stddef.h>

/* MMC profiles (GET CONFIGURATION, Current Profile) */
const char *mid_profile_name(int profile);   /* "BD-R", "DVD+R DL", ... or NULL */
int  mid_profile_is_bd(int profile);
int  mid_profile_is_dvd_plus(int profile);
int  mid_profile_is_dvd_minus(int profile);
int  mid_profile_is_cd(int profile);
int  mid_profile_dvd_layers(int profile);     /* 2 for DL profiles, otherwise 1 */

/* Each function takes the whole reply including the 4-byte header.
   id     — full ID, ImgBurn style ("VERBAT-IMe-000", "MKM-003-00", "MBIPG101R05")
   base   — without the revision ("VERBAT-IMe", "MKM-003"); same as id for DVD-R
   layers — number of layers, or 0 if unknown.
   Returns 1 on success, 0 if the reply lacks the data. */
int mid_parse_bd_di(const unsigned char *r, size_t len, char *id, char *base, size_t cap, int *layers);
int mid_parse_dvd_plus(const unsigned char *r, size_t len, char *id, char *base, size_t cap);
int mid_parse_dvd_minus(const unsigned char *r, size_t len, char *id, size_t cap);

/* Compares Media IDs ignoring case, spaces, dashes and dots:
   "MBIPG101-R05" == "MBIPG101R05" */
int mid_same(const char *a, const char *b);

#endif
