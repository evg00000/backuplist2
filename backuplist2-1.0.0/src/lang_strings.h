/* lang_strings.h — every UI string, English and Russian side by side.
   X(id, english, russian). Adding a language = adding a column here and in lang.c. */

/* ---- statuses ---- */
X(S_ST_OK,            L"OK",                       L"живой")
X(S_ST_PARTIAL,       L"partial",                  L"частично")
X(S_ST_DEAD,          L"dead",                     L"мёртв")
X(S_ST_UNKNOWN,       L"\x2014",                   L"\x2014")
X(S_ST_NOTCHECKED,    L"not checked",              L"не проверялся")
X(S_RES_OK,           L"OK",                       L"живой")
X(S_RES_PARTIAL,      L"partially readable",       L"частично читается")
X(S_RES_DEAD,         L"dead",                     L"мёртв")

/* ---- main window ---- */
X(S_SEARCH_LBL,       L"Search:",                  L"Поиск:")
X(S_SEARCH_CUE,       L"artist, movie, folder or file \x2014 searches contents and file paths",
                      L"исполнитель, фильм, папка или файл \x2014 по содержимому и путям")
X(S_COL_NUM,          L"#",                        L"№")
X(S_COL_TYPE,         L"Type",                     L"Тип")
X(S_COL_DATA,         L"Content",                  L"Данные")
X(S_COL_STATUS,       L"Status",                   L"Статус")
X(S_COL_PRIO,         L"Pr.",                      L"Пр.")
X(S_COL_CHECKED,      L"Checked",                  L"Проверен")
X(S_COL_MEDIA,        L"Media ID",                 L"Media ID")
X(S_COL_SERIAL,       L"Serial",                   L"Серийник")
X(S_COL_CONTENTS,     L"Contents",                 L"Содержимое")
X(S_COL_PATH,         L"Path",                     L"Путь")
X(S_COL_SHA,          L"SHA-256",                  L"SHA-256")
X(S_DISCS_TOTAL,      L"Discs: %d, files: %d",     L"Дисков: %d, файлов: %d")
X(S_FOUND_DISCS,      L"Discs found: %d",          L"Найдено дисков: %d")
X(S_QUERY_ERROR,      L"Query error",              L"Ошибка запроса")
X(S_FILES_NONE,       L"Files: select a disc",     L"Файлы: выберите диск")
X(S_FILES_NOHASH,     L"Files of disc #%d: no hashes (disc not read yet?)",
                      L"Файлы диска №%d: хешей нет (диск не прочитан?)")
X(S_FILES_MATCH,      L"Files of disc #%d matching the search: %d",
                      L"Файлы диска №%d, совпадающие с поиском: %d")
X(S_FILES_N,          L"Files of disc #%d: %d",    L"Файлы диска №%d: %d")
X(S_EJECTED,          L"Disc ejected from %lc:",   L"Диск извлечён из %lc:")

/* ---- menu ---- */
X(S_M_FILE,           L"&File",                    L"&Файл")
X(S_M_NEWDB,          L"&New database...",         L"&Новая база...")
X(S_M_OPEN,           L"&Open database...\tCtrl+O", L"&Открыть базу...\tCtrl+O")
X(S_M_LANG,           L"&Language / Язык",         L"&Language / Язык")
X(S_M_EXIT,           L"E&xit",                    L"В&ыход")
X(S_M_DISC,           L"&Disc",                    L"&Диск")
X(S_M_DETECT,         L"&Detect inserted disc\tF5", L"&Определить вставленный\tF5")
X(S_M_ADD,            L"&Add new disc...\tCtrl+N", L"&Добавить новый...\tCtrl+N")
X(S_M_VERIFY,         L"&Verify hashes...\tF6",    L"&Проверить по хешам...\tF6")
X(S_M_MEDIAINFO,      L"&Media information...\tCtrl+I", L"Сведения о &носителе...\tCtrl+I")
X(S_M_EDIT,           L"Disc &card...\tEnter",     L"&Карточка диска...\tEnter")
X(S_M_DELETE,         L"De&lete entry...\tDel",    L"&Удалить запись...\tDel")
X(S_M_UNBIND,         L"&Unlink serial number",    L"Отвязать &серийный номер")
X(S_M_HELP,           L"&Help",                    L"&Справка")
X(S_M_FIND,           L"&Search\tCtrl+F",          L"&Поиск\tCtrl+F")
X(S_M_ABOUT,          L"&About",                   L"&О программе")

/* ---- database ---- */
X(S_DB_OPENED,        L"Database opened",          L"База открыта")
X(S_DB_NOT_OPEN,      L"No database open",         L"База не открыта")
X(S_DB_CREATED,       L"New database created: %ls", L"Создана новая база: %ls")
X(S_DB_CREATE_FAIL,   L"Could not create the database:\n%ls", L"Не удалось создать базу:\n%ls")
X(S_DB_EXISTS,        L"This file already exists. Choose another name \x2014 an existing database is never overwritten.",
                      L"Такой файл уже есть. Выберите другое имя \x2014 существующая база не перезаписывается.")
X(S_DB_FILTER,        L"Backuplist database (*.db)\0*.db\0All files\0*.*\0",
                      L"База Backuplist (*.db)\0*.db\0Все файлы\0*.*\0")
X(S_DB_WRITE_ERR,     L"Database write error",     L"Ошибка записи в базу")
X(S_E_DB_NOT_OPEN,    L"no database open",         L"база не открыта")

/* ---- inserted disc ---- */
X(S_NO_READABLE_IN,   L"No readable disc in drive %lc:", L"В приводе %lc: нет читаемого диска")
X(S_NO_DISC_ANY,      L"No disc in any drive",     L"Ни в одном приводе нет диска")
X(S_NO_READABLE_ANY,  L"There is no readable disc in any drive.", L"Ни в одном приводе нет читаемого диска.")
X(S_INSERTED,         L"Disc #%d inserted  (%lc: \"%ls\", %ls)%ls",
                      L"Вставлен диск №%d  (%lc: «%ls», %ls)%ls")
X(S_MID_EMPTY_CARD,   L" \x00B7 Media ID %ls (empty in the card \x2014 Ctrl+I)",
                      L" \x00B7 Media ID %ls (в карточке пусто \x2014 Ctrl+I)")
X(S_MID_MATCH,        L" \x00B7 Media ID %ls matches", L" \x00B7 Media ID %ls совпадает")
X(S_MID_DIFF,         L" \x00B7 Media ID %ls, the card says %ls \x2014 Ctrl+I",
                      L" \x00B7 Media ID %ls, в карточке %ls \x2014 Ctrl+I")
X(S_UNKNOWN_3WAY,     L"The disc in drive %lc: is not in the database.\n\n"
                      L"Label: \"%ls\"\nSerial number: %ls\n\n"
                      L"Yes \x2014 add it as a new disc\n"
                      L"No \x2014 it is disc #%d (selected in the list), link it\n"
                      L"Cancel \x2014 do nothing",
                      L"Диск в приводе %lc: не найден в базе.\n\n"
                      L"Метка: «%ls»\nСерийный номер: %ls\n\n"
                      L"Да \x2014 добавить как новый диск\n"
                      L"Нет \x2014 это диск №%d (выбран в списке), привязать к нему\n"
                      L"Отмена \x2014 ничего не делать")
X(S_UNKNOWN_ADD,      L"The disc in drive %lc: is not in the database.\n\n"
                      L"Label: \"%ls\"\nSerial number: %ls\n\n"
                      L"Add it as a new disc?\n\n"
                      L"If this disc is already in the list, click No, select its entry "
                      L"and press F5 \x2014 the serial number will be linked to it.",
                      L"Диск в приводе %lc: не найден в базе.\n\n"
                      L"Метка: «%ls»\nСерийный номер: %ls\n\n"
                      L"Добавить его как новый диск?\n\n"
                      L"Если это диск, который уже есть в списке, нажмите «Нет», "
                      L"выделите его запись и нажмите F5 \x2014 серийник привяжется к ней.")
X(S_UNKNOWN_STATUS,   L"Unknown disc %lc: \"%ls\" (%ls)", L"Неизвестный диск %lc: «%ls» (%ls)")
X(S_BOUND,            L"Disc #%d linked to serial %ls", L"Диск №%d привязан к серийнику %ls")
X(S_NOT_BOUND,        L"Disc #%d is not linked to a serial number", L"Диск №%d ни к чему не привязан")
X(S_UNBIND_Q,         L"Unlink serial number %ls from disc #%d?", L"Отвязать серийный номер %ls от диска №%d?")

/* ---- adding ---- */
X(S_ALREADY,          L"This disc is already in the database as #%d.", L"Этот диск уже в базе под №%d.")
X(S_HINT_OK,          L"From the disc: %ls, %ls%ls", L"С диска: %ls, %ls%ls")
X(S_HINT_LAYERS,      L", 2+ layers",              L", 2+ слоя")
X(S_HINT_PROFILE_NOID,L"From the disc: %ls; Media ID not read \x2014 %ls",
                      L"С диска: %ls; Media ID не прочитан \x2014 %ls")
X(S_HINT_NOID,        L"Media ID not read \x2014 %ls", L"Media ID не прочитан \x2014 %ls")
X(S_TITLE_ADD,        L"Adding disc #%d",          L"Добавление диска №%d")
X(S_ADD_CANCELLED,    L"Adding disc #%d cancelled \x2014 nothing was written to the database",
                      L"Добавление диска №%d отменено \x2014 в базу ничего не записано")
X(S_NOTES_ADDED,      L"added: %d files (%ls), %d unreadable", L"добавлен: файлов %d (%ls), не читаются %d")
X(S_ADDED_STATUS,     L"Disc #%d added: %ls",      L"Диск №%d добавлен: %ls")
X(S_ADDED_BAD,        L"Disc #%d was added, but %d files could not be read \x2014 their hashes "
                      L"are not in the database. Consider burning this disc again.",
                      L"Диск №%d добавлен, но %d файлов не прочитались \x2014 их хешей в базе нет. "
                      L"Запись на этот диск стоит повторить.")

/* ---- verification ---- */
X(S_NO_HASHES_Q,      L"Disc #%d has no hashes in the database \x2014 there is nothing to compare with.\n\n"
                      L"Read the disc and store its hashes as the reference?",
                      L"У диска №%d в базе нет хешей \x2014 сверять не с чем.\n\n"
                      L"Прочитать диск и сохранить хеши как эталон?")
X(S_TITLE_BASELINE,   L"Reference hashes for disc #%d", L"Эталон диска №%d")
X(S_TITLE_VERIFY,     L"Verifying disc #%d",       L"Проверка диска №%d")
X(S_VERIFY_CANCELLED, L"Verification of disc #%d cancelled \x2014 nothing was written to the database",
                      L"Проверка диска №%d отменена \x2014 в базу ничего не записано")
X(S_NOTES_BASELINE,   L"reference: %d files (%ls), %d unreadable", L"эталон: файлов %d (%ls), не читаются %d")
X(S_DISC_NOTES,       L"Disc #%d: %ls",            L"Диск №%d: %ls")
X(S_BASELINE_DONE,    L"Disc #%d: %d hashes stored (%ls).%ls", L"Диск №%d: сохранено хешей %d (%ls).%ls")
X(S_BASELINE_PARTIAL, L"\n\nSome files could not be read \x2014 the disc is marked as partially readable.",
                      L"\n\nЧасть файлов не прочиталась \x2014 диск отмечен как читающийся частично.")
X(S_NOTES_VERIFY,     L"matched %d of %d; mismatched %d; unreadable %d; missing %d; extra %d",
                      L"совпали %d из %d; не совпали %d; не читаются %d; нет %d; лишние %d")
X(S_VERIFY_STATUS,    L"Verification of #%d: %ls \x2014 %ls", L"Проверка №%d: %ls \x2014 %ls")
X(S_VERIFY_RESULT,    L"Disc #%d: %ls\n\nMatched: %d of %d (%ls)\nHash mismatch: %d\n"
                      L"Unreadable: %d\nMissing from disc: %d\nExtra: %d\n\nReport: %ls\n\nOpen the report?",
                      L"Диск №%d: %ls\n\nСовпали: %d из %d (%ls)\nХеш не совпал: %d\n"
                      L"Не читаются: %d\nНет на диске: %d\nЛишние: %d\n\nОтчёт: %ls\n\nОткрыть отчёт?")
X(S_NOT_LINKED_SELECT,L"The disc in drive %lc: (\"%ls\", %ls) is not linked to an entry yet.\n\n"
                      L"Select its entry in the list and start the verification again.",
                      L"Диск в приводе %lc: («%ls», %ls) ещё не привязан к записи.\n\n"
                      L"Выделите его в списке и запустите проверку снова.")
X(S_OTHER_DISC,       L"Disc #%d is linked to serial %ls, but the drive holds %ls.\n\n"
                      L"Looks like a different disc is inserted.",
                      L"Диск №%d привязан к серийнику %ls, а в приводе \x2014 %ls.\n\n"
                      L"Похоже, вставлен другой диск.")
X(S_VERIFY_AS_Q,      L"Verify the disc in drive %lc: (\"%ls\") as #%d?\n\n"
                      L"Its serial number %ls will be linked to this entry.",
                      L"Проверить диск из привода %lc: («%ls») как №%d?\n\n"
                      L"Его серийный номер %ls будет привязан к этой записи.")

/* ---- media information ---- */
X(S_MI_TITLE,         L"Media information",        L"Сведения о носителе")
X(S_MI_LAYERS,        L", layers: %d",             L", слоёв: %d")
X(S_MI_HEAD,          L"Drive %lc:\nLabel: \"%ls\"\nSerial number: %ls\n\nMedia: %ls%ls\nMedia ID: %ls\n",
                      L"Привод %lc:\nМетка: «%ls»\nСерийный номер: %ls\n\nНоситель: %ls%ls\nMedia ID: %ls\n")
X(S_MI_UNKNOWN,       L"unknown",                  L"не определён")
X(S_MI_NOT_IN_DB,     L"\nThis disc is not in the database.", L"\nВ базе этого диска нет.")
X(S_MI_IN_DB,         L"\nIn the database: disc #%d, Media ID in the card: %ls%ls",
                      L"\nВ базе: диск №%d, Media ID в карточке: %ls%ls")
X(S_MI_EMPTY,         L"(empty)",                  L"(пусто)")
X(S_MI_MATCHES,       L" \x2014 matches",          L" \x2014 совпадает")
X(S_MI_WRITE_Q,       L"\nIn the database: disc #%d, Media ID in the card: %ls\n\nWrite \"%ls\" to the card?",
                      L"\nВ базе: диск №%d, Media ID в карточке: %ls\n\nЗаписать в карточку «%ls»?")
X(S_MI_WRITTEN,       L"Disc #%d: Media ID saved \x2014 %ls", L"Диск №%d: Media ID записан \x2014 %ls")

/* ---- card, delete, about ---- */
X(S_SAVED,            L"Disc #%d saved",           L"Диск №%d сохранён")
X(S_DELETE_Q,         L"Delete the entry for disc #%d together with its hashes and check history?\n\n"
                      L"The disc itself is not affected.",
                      L"Удалить запись о диске №%d вместе с хешами и историей проверок?\n\n"
                      L"На сам диск это не влияет.")
X(S_DELETED,          L"Entry #%d deleted",        L"Запись №%d удалена")
X(S_ABOUT_TITLE,      L"About",                    L"О программе")
X(S_ABOUT_BODY,       L"%ls %ls\n\nCatalog of backups on optical discs:\nSHA-256 verification, disc auto-detection, Media ID.\n\n"
                      L"%ls\n\nLicense: Unlicense (public domain)\nWin32 API + SQLite %ls\nWindows XP SP2 \x2014 Windows 11",
                      L"%ls %ls\n\nУчёт резервных копий на оптических дисках:\nпроверка по SHA-256, автоопределение дисков, Media ID.\n\n"
                      L"%ls\n\nЛицензия: Unlicense (общественное достояние)\nWin32 API + SQLite %ls\nWindows XP SP2 \x2014 Windows 11")

/* ---- disc card dialog ---- */
X(S_CARD_NEW,         L"New disc",                 L"Новый диск")
X(S_CARD_TITLE,       L"Disc #%d",                 L"Диск №%d")
X(S_CARD_CAPTION,     L"Disc",                     L"Диск")
X(S_NOT_LINKED,       L"not linked",               L"не привязан")
X(S_CARD_WILL_READ,   L"After OK the whole disc will be read.", L"После OK диск будет прочитан целиком.")
X(S_CARD_STATUS,      L"Status: %ls%ls%ls",        L"Статус: %ls%ls%ls")
X(S_CARD_CHECKED,     L", checked ",               L", проверен ")
X(S_ERR_ID,           L"The disc number must be a positive integer.", L"Номер диска \x2014 целое число больше нуля.")
X(S_ERR_ID_EXISTS,    L"A disc with this number is already in the database.", L"Диск с таким номером уже есть в базе.")
X(S_ERR_DATE,         L"Enter the burn date as YYYY-MM-DD, for example 2026-11-13.",
                      L"Дата записи в виде ГГГГ-ММ-ДД, например 2026-11-13.")
X(S_L_ID,             L"Number:",                  L"Номер:")
X(S_L_KIND,           L"Media:",                   L"Носитель:")
X(S_L_WRITTEN,        L"Burned on:",               L"Дата записи:")
X(S_L_CTYPE,          L"Content:",                 L"Данные:")
X(S_L_PRIO,           L"Priority:",                L"Приоритет:")
X(S_L_BRAND,          L"Brand:",                   L"Бренд:")
X(S_L_MEDIA,          L"Media ID:",                L"Media ID:")
X(S_L_LABEL,          L"Volume label:",            L"Метка тома:")
X(S_L_SERIAL,         L"Serial:",                  L"Серийник:")
X(S_L_CONTENTS,       L"Contents (separate items with \";\"):", L"Содержимое (через «;»):")
X(S_CANCEL,           L"Cancel",                   L"Отмена")
X(S_CT_VIDEO,         L"Video",                    L"Видео")
X(S_CT_AUDIO,         L"Audio",                    L"Аудио")
X(S_CT_SOFTWARE,      L"Software",                 L"ПО")
X(S_CT_MIXED,         L"Mixed",                    L"Смешанный")
X(S_CT_DOCS,          L"Documents",                L"Документы")
X(S_PRIO_1,           L"1 \x2014 irreplaceable",   L"1 \x2014 незаменимое")
X(S_PRIO_2,           L"2 \x2014 hard to find",    L"2 \x2014 редкое")
X(S_PRIO_3,           L"3 \x2014 replaceable",     L"3 \x2014 заменимое")

/* ---- progress dialog ---- */
X(S_P_TITLE,          L"Scanning",                 L"Сканирование")
X(S_P_LISTING,        L"Listing files\x2026 %d found", L"Составляю список файлов\x2026 найдено %d")
X(S_P_STOPPING,       L"Stopping\x2026",           L"Останавливаю\x2026")
X(S_P_STRUCT,         L"Reading the disc structure", L"Чтение структуры диска")
X(S_P_HASHING,        L"Reading and computing SHA-256", L"Чтение и подсчёт SHA-256")
X(S_P_ETA,            L" \x00B7 ~%u min left",     L" \x00B7 осталось ~%u мин")
X(S_P_STATS,          L"%ls of %ls \x00B7 files %d of %d \x00B7 %.1f MB/s%ls%ls",
                      L"%ls из %ls \x00B7 файлов %d из %d \x00B7 %.1f МБ/с%ls%ls")
X(S_P_ERRORS,         L" \x00B7 read errors",      L" \x00B7 есть ошибки чтения")
X(S_P_START_FAIL,     L"Could not start reading.", L"Не удалось запустить чтение.")
X(S_SIZE_GB,          L"%.1f GB",                  L"%.1f ГБ")
X(S_SIZE_MB,          L"%.1f MB",                  L"%.1f МБ")
X(S_SIZE_KB,          L"%u KB",                    L"%u КБ")

/* ---- verification report ---- */
X(S_R_HEAD,           L"Backuplist 2 \x2014 verification of disc #%d\r\n", L"Backuplist 2 \x2014 проверка диска №%d\r\n")
X(S_R_META,           L"Date: %ls\r\nVolume label: %ls\r\nSerial number: %ls\r\n\r\n",
                      L"Дата: %ls\r\nМетка тома: %ls\r\nСерийный номер: %ls\r\n\r\n")
X(S_R_RESULT,         L"Result: %ls\r\n",          L"Итог: %ls\r\n")
X(S_R_EXPECTED,       L"  files in database:   %d\r\n", L"  в базе файлов:       %d\r\n")
X(S_R_OK,             L"  matched:             %d\r\n", L"  совпали:             %d\r\n")
X(S_R_MISMATCH,       L"  hash mismatch:       %d\r\n", L"  хеш не совпал:       %d\r\n")
X(S_R_UNREADABLE,     L"  unreadable:          %d\r\n", L"  не читаются:         %d\r\n")
X(S_R_MISSING,        L"  missing from disc:   %d\r\n", L"  нет на диске:        %d\r\n")
X(S_R_EXTRA,          L"  extra (not in db):   %d\r\n", L"  лишние (нет в базе): %d\r\n")
X(S_R_ENUMERR,        L"  unreadable folders:  %d \x2014 some files may be missing from the list\r\n",
                      L"  папок не прочитано:  %d \x2014 часть файлов могла не попасть в список\r\n")
X(S_R_SEC_UNREADABLE, L"Unreadable",               L"Не читаются")
X(S_R_SEC_MISMATCH,   L"Hash mismatch (data is corrupted)", L"Хеш не совпал (данные искажены)")
X(S_R_SEC_MISSING,    L"Missing from disc",        L"Нет на диске")
X(S_R_SEC_EXTRA,      L"\r\nExtra \x2014 on the disc but not in the database (%d).\r\n"
                      L"(for discs imported from PowerShell hashes these are usually hidden files it skipped)\r\n",
                      L"\r\nЛишние \x2014 есть на диске, нет в базе (%d).\r\n"
                      L"(у дисков, импортированных из PowerShell, это обычно скрытые файлы \x2014 он их не видел)\r\n")

/* ---- Media ID errors ---- */
X(S_E_ACCESS,         L"no access to the drive \x2014 on Windows XP run the program as administrator",
                      L"нет доступа к приводу \x2014 на Windows XP запустите программу от имени администратора")
X(S_E_OPEN,           L"could not open the drive (error %lu)", L"не удалось открыть привод (ошибка %lu)")
X(S_E_SPTI_DENIED,    L"Windows refused to pass the command to the drive \x2014 try running as administrator",
                      L"Windows не дала отправить команду приводу \x2014 попробуйте запуск от имени администратора")
X(S_E_SPTI,           L"the drive does not accept SCSI commands (error %lu) \x2014 happens with some USB adapters",
                      L"привод не принимает SCSI-команды (ошибка %lu) \x2014 бывает с USB-переходниками")
X(S_E_NOPROFILE,      L"the drive did not report the media type", L"привод не сообщил тип носителя")
X(S_E_PROFILE_FMT,    L"profile %04Xh",            L"профиль %04Xh")
X(S_E_NODISC,         L"no disc in the drive",     L"в приводе нет диска")
X(S_E_BD,             L"the drive did not return BD Disc Information", L"привод не отдал Disc Information для BD")
X(S_E_ADIP,           L"the drive did not return DVD+R ADIP data", L"привод не отдал ADIP для DVD+R")
X(S_E_PREPIT,         L"the drive did not return DVD-R pre-pit data", L"привод не отдал pre-pit данные DVD-R")
X(S_E_CD,             L"CDs have no Media ID of this kind, only an ATIP code", L"у CD нет Media ID в этом виде \x2014 только код ATIP")
X(S_E_NOMID,          L"this media has no Media ID (pressed disc?)", L"у этого носителя Media ID нет (штампованный диск?)")
