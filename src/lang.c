/* lang.c */
#include <windows.h>
#include "lang.h"

static const wchar_t *const STR_EN[S_COUNT] = {
#define X(id, en, ru) en,
#include "lang_strings.h"
#undef X
};

static const wchar_t *const STR_RU[S_COUNT] = {
#define X(id, en, ru) ru,
#include "lang_strings.h"
#undef X
};

static int g_lang = LNG_EN;

static void ini_path(wchar_t *out, int cch)
{
    wchar_t *slash;
    GetModuleFileNameW(NULL, out, cch);
    slash = wcsrchr(out, L'\\');
    if (slash) wcscpy(slash + 1, L"backuplist.ini");
}

void lang_init(void)
{
    wchar_t ini[MAX_PATH], val[8];
    ini_path(ini, MAX_PATH);
    GetPrivateProfileStringW(L"ui", L"language", L"", val, 8, ini);
    if (!lstrcmpiW(val, L"ru"))      g_lang = LNG_RU;
    else if (!lstrcmpiW(val, L"en")) g_lang = LNG_EN;
    else g_lang = PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_RUSSIAN ? LNG_RU : LNG_EN;
}

int lang_get(void) { return g_lang; }

void lang_set(int lang)
{
    wchar_t ini[MAX_PATH];
    if (lang < 0 || lang >= LNG_COUNT) return;
    g_lang = lang;
    ini_path(ini, MAX_PATH);
    WritePrivateProfileStringW(L"ui", L"language", lang == LNG_RU ? L"ru" : L"en", ini);
}

const wchar_t *T(int id)
{
    if (id < 0 || id >= S_COUNT) return L"?";
    return g_lang == LNG_RU ? STR_RU[id] : STR_EN[id];
}
