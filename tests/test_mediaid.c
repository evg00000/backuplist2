/* Media ID parser tests on synthetic drive replies built from the MMC-5 / ECMA offsets */
#include <stdio.h>
#include <string.h>
#include "mediaid_parse.h"

static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { printf("FAIL: %s\n", msg); fails++; } else printf("ok:   %s\n", msg); } while (0)

int main(void)
{
    unsigned char r[4200];
    char id[40], base[40];
    int layers;

    /* BD-R SL Verbatim */
    memset(r, 0, sizeof r);
    r[0] = 0x10; r[1] = 0x02;
    memcpy(r + 4, "DI", 2); memcpy(r + 4 + 8, "BDR", 3); r[4 + 12] = 0x12;
    memcpy(r + 4 + 100, "VERBAT", 6); memcpy(r + 4 + 106, "IMe", 3); r[4 + 111] = 0;
    CHECK(mid_parse_bd_di(r, 4100, id, base, 40, &layers), "BD: parsed");
    CHECK(!strcmp(id, "VERBAT-IMe-000"), "BD: id VERBAT-IMe-000");
    CHECK(!strcmp(base, "VERBAT-IMe"), "BD: base VERBAT-IMe");
    CHECK(layers == 1, "BD: 1 layer");

    /* BD-R DL Panasonic, short manufacturer padded with NULs */
    memset(r + 4 + 100, 0, 12);
    memcpy(r + 4 + 100, "MEI", 3); memcpy(r + 4 + 106, "T02", 3); r[4 + 111] = 1; r[4 + 12] = 0x22;
    CHECK(mid_parse_bd_di(r, 4100, id, base, 40, &layers) && !strcmp(id, "MEI-T02-001"), "BD DL: MEI-T02-001");
    CHECK(layers == 2, "BD DL: 2 layers");

    /* garbage in the layer byte: layers unknown, ID still read */
    r[4 + 12] = 0x92;
    CHECK(mid_parse_bd_di(r, 4100, id, base, 40, &layers) && layers == 0, "BD: implausible layers -> 0");

    /* a DVD drive ignored the BD media type: no DI signature */
    r[4] = 0; 
    CHECK(!mid_parse_bd_di(r, 4100, id, base, 40, &layers), "BD: no 'DI' -> rejected");
    CHECK(!mid_parse_bd_di(r, 50, id, base, 40, &layers), "BD: short reply -> rejected");

    /* DVD+R: ADIP */
    memset(r, 0, sizeof r);
    memcpy(r + 4 + 19, "MKM\0\0\0\0\0", 8); memcpy(r + 4 + 27, "003", 3); r[4 + 30] = 0;
    CHECK(mid_parse_dvd_plus(r, 260, id, base, 40) && !strcmp(id, "MKM-003-00"), "DVD+R: MKM-003-00");
    CHECK(!strcmp(base, "MKM-003"), "DVD+R: base MKM-003");

    /* DVD-R: pre-pit, fields 3 and 4 */
    memset(r, 0, sizeof r);
    r[4 + 16] = 3; memcpy(r + 4 + 17, "MBIPG1", 6);
    r[4 + 24] = 4; memcpy(r + 4 + 25, "01R05\0", 6);
    CHECK(mid_parse_dvd_minus(r, 260, id, 40) && !strcmp(id, "MBIPG101R05"), "DVD-R: MBIPG101R05");
    r[4 + 24] = 7;
    CHECK(!mid_parse_dvd_minus(r, 260, id, 40), "DVD-R: wrong field ids -> rejected");

    /* Spelling comparison */
    CHECK(mid_same("MBIPG101-R05", "MBIPG101R05"), "same: MBIPG101-R05 == MBIPG101R05");
    CHECK(mid_same("VERBAT-IMe", "verbat ime"), "same: case and spaces");
    CHECK(!mid_same("VERBAT-IMe", "VERBAT-IMe-000"), "same: revision differs (base is matched separately)");
    CHECK(!mid_same("", ""), "same: empty strings never match");

    /* Profiles */
    CHECK(!strcmp(mid_profile_name(0x41), "BD-R") && mid_profile_is_bd(0x41), "profile 41h = BD-R");
    CHECK(mid_profile_dvd_layers(0x2B) == 2 && mid_profile_is_dvd_plus(0x2B), "profile 2Bh = DVD+R DL");
    CHECK(mid_profile_is_dvd_minus(0x11) && mid_profile_dvd_layers(0x11) == 1, "profile 11h = DVD-R");

    printf(fails ? "\n%d FAILED\n" : "\nall tests passed\n", fails);
    return fails != 0;
}
