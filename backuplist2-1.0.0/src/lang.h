/* lang.h — UI language: string table lookup and the saved language setting */
#ifndef BL_LANG_H
#define BL_LANG_H

#include <wchar.h>

enum { LNG_EN = 0, LNG_RU = 1, LNG_COUNT };

enum {
#define X(id, en, ru) id,
#include "lang_strings.h"
#undef X
    S_COUNT
};

/* Picks the language: backuplist.ini next to the exe, otherwise the Windows UI language. */
void lang_init(void);
int  lang_get(void);
/* Switches the language and remembers it in backuplist.ini (silently skipped if read-only). */
void lang_set(int lang);
const wchar_t *T(int id);

#endif
