/* mediaid_parse.c */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "mediaid_parse.h"

static const struct { int code; const char *name; } PROFILES[] = {
    { 0x08, "CD-ROM" },   { 0x09, "CD-R" },     { 0x0A, "CD-RW" },
    { 0x10, "DVD-ROM" },  { 0x11, "DVD-R" },    { 0x12, "DVD-RAM" },
    { 0x13, "DVD-RW" },   { 0x14, "DVD-RW" },   { 0x15, "DVD-R DL" },
    { 0x16, "DVD-R DL" }, { 0x1A, "DVD+RW" },   { 0x1B, "DVD+R" },
    { 0x2A, "DVD+RW DL" },{ 0x2B, "DVD+R DL" },
    { 0x40, "BD-ROM" },   { 0x41, "BD-R" },     { 0x42, "BD-R" },
    { 0x43, "BD-RE" },
};

const char *mid_profile_name(int p)
{
    size_t i;
    for (i = 0; i < sizeof PROFILES / sizeof PROFILES[0]; i++)
        if (PROFILES[i].code == p) return PROFILES[i].name;
    return NULL;
}

int mid_profile_is_bd(int p)        { return p >= 0x40 && p <= 0x43; }
int mid_profile_is_dvd_plus(int p)  { return p == 0x1A || p == 0x1B || p == 0x2A || p == 0x2B; }
int mid_profile_is_dvd_minus(int p) { return p == 0x11 || p == 0x13 || p == 0x14 || p == 0x15 || p == 0x16; }
int mid_profile_is_cd(int p)        { return p >= 0x08 && p <= 0x0A; }
int mid_profile_dvd_layers(int p)   { return (p == 0x15 || p == 0x16 || p == 0x2A || p == 0x2B) ? 2 : 1; }

/* Copies a fixed-width field: trims NULs/spaces at both ends,
   replaces non-printable bytes with '?' */
static void field(const unsigned char *src, size_t n, char *dst, size_t cap)
{
    size_t a = 0, b = n, o = 0, i;
    while (a < b && (src[a] == 0 || src[a] == ' ')) a++;
    while (b > a && (src[b - 1] == 0 || src[b - 1] == ' ')) b--;
    for (i = a; i < b && o + 1 < cap; i++)
        dst[o++] = (src[i] >= 0x20 && src[i] < 0x7F) ? (char)src[i] : '?';
    dst[o] = 0;
}

int mid_parse_bd_di(const unsigned char *r, size_t len, char *id, char *base, size_t cap, int *layers)
{
    const unsigned char *di = r + 4;           /* the first DI unit follows the header */
    char mfr[8], typ[4];
    int nl;

    *layers = 0;
    if (len < 4 + 112) return 0;
    if (di[0] != 'D' || di[1] != 'I') return 0;   /* a DVD drive may ignore the BD media type */

    field(di + 100, 6, mfr, sizeof mfr);
    field(di + 106, 3, typ, sizeof typ);
    if (!mfr[0]) return 0;

    snprintf(base, cap, "%s-%s", mfr, typ);
    snprintf(id, cap, "%s-%s-%03u", mfr, typ, (unsigned)di[111]);

    /* Byte 12, high nibble: number of layers (1..4). Range-checked. */
    nl = (di[12] >> 4) & 0x0F;
    if (nl >= 1 && nl <= 4) *layers = nl;
    return 1;
}

int mid_parse_dvd_plus(const unsigned char *r, size_t len, char *id, char *base, size_t cap)
{
    char mfr[12], typ[4];
    if (len < 4 + 31) return 0;
    field(r + 4 + 19, 8, mfr, sizeof mfr);     /* ADIP: Disc Manufacturer ID */
    field(r + 4 + 27, 3, typ, sizeof typ);     /* Media Type ID */
    if (!mfr[0]) return 0;
    snprintf(base, cap, "%s-%s", mfr, typ);
    snprintf(id, cap, "%s-%s-%02u", mfr, typ, (unsigned)r[4 + 30]);
    return 1;
}

int mid_parse_dvd_minus(const unsigned char *r, size_t len, char *id, size_t cap)
{
    char a[8], b[8];
    if (len < 4 + 31) return 0;
    /* Pre-pit: fields 3 and 4, 6 bytes of the manufacturer ID in each */
    if (r[4 + 16] != 3 || r[4 + 24] != 4) return 0;
    field(r + 4 + 17, 6, a, sizeof a);
    field(r + 4 + 25, 6, b, sizeof b);
    if (!a[0]) return 0;
    /* the first part may be shorter than 6 characters: join without gaps */
    snprintf(id, cap, "%s%s", a, b);
    return 1;
}

static void squash(const char *s, char *out, size_t cap)
{
    size_t o = 0;
    for (; *s && o + 1 < cap; s++)
        if (isalnum((unsigned char)*s)) out[o++] = (char)tolower((unsigned char)*s);
    out[o] = 0;
}

int mid_same(const char *a, const char *b)
{
    char x[64], y[64];
    squash(a, x, sizeof x);
    squash(b, y, sizeof y);
    return x[0] && !strcmp(x, y);
}
