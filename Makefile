# Backuplist 2 — MinGW build (i686, msvcrt CRT, so the exe runs on XP SP2)
# Linux:    make
# Windows:  mingw32-make CROSS=     (w64devkit-i686 / MSYS2 mingw32)

CROSS   ?= i686-w64-mingw32-
CC      := $(CROSS)gcc
WINDRES := $(CROSS)windres

WINVER  := -D_WIN32_WINNT=0x0501 -DWINVER=0x0501 -D_WIN32_IE=0x0600
ARCH    := -march=i686 -mtune=generic
CFLAGS  := -O2 -std=gnu99 -Wall -Wextra -Wno-unused-parameter $(ARCH) $(WINVER) \
           -municode -DUNICODE -D_UNICODE -Ithird_party/sqlite

SQLITE_FLAGS := -O2 $(ARCH) $(WINVER) \
           -DSQLITE_ENABLE_FTS5 -DSQLITE_THREADSAFE=0 -DSQLITE_DQS=0 \
           -DSQLITE_DEFAULT_FOREIGN_KEYS=1 -DSQLITE_OMIT_LOAD_EXTENSION \
           -DSQLITE_OMIT_DEPRECATED -DSQLITE_DEFAULT_MEMSTATUS=0 \
           -DSQLITE_WIN32_MALLOC

LDFLAGS := -mwindows -municode -static -static-libgcc -s
LIBS    := -lcomctl32 -lcomdlg32 -lshell32

OBJ := build/main.o build/db.o build/volume.o build/scan.o build/sha256.o \
       build/dialogs.o build/verify.o build/mediaid.o build/mediaid_parse.o build/lang.o build/sqlite3.o build/app.res.o
HDR := $(wildcard src/*.h)

all: build/backuplist.exe

build:
	mkdir -p build

build/%.o: src/%.c $(HDR) | build
	$(CC) $(CFLAGS) -c $< -o $@

build/sqlite3.o: third_party/sqlite/sqlite3.c | build
	$(CC) $(SQLITE_FLAGS) -c $< -o $@

build/app.res.o: res/app.rc res/app.manifest res/app.ico src/resource.h | build
	$(WINDRES) -I res -O coff $< -o $@

build/backuplist.exe: $(OBJ)
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

test:
	gcc -Wall -Wextra -Isrc tests/test_mediaid.c src/mediaid_parse.c -o build/test_mediaid && build/test_mediaid

clean:
	rm -rf build

.PHONY: all clean test
