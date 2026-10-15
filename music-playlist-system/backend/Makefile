# Music & Playlist Management System - C backend
CC      ?= gcc
CFLAGS  ?= -std=gnu11 -Wall -Wextra -O2
SRC     := $(filter-out src/main.c,$(wildcard src/*.c))
ifeq ($(OS),Windows_NT)
  LIBS := -lws2_32 -lm
  EXE  := musicms.exe
else
  LIBS := -lpthread -lm
  EXE  := musicms
endif

all: $(EXE)

$(EXE): $(SRC) src/main.c
	$(CC) $(CFLAGS) -o $@ src/main.c $(SRC) $(LIBS)

test: tests/test_ds.c $(SRC)
	$(CC) $(CFLAGS) -Isrc -o tests/test_ds tests/test_ds.c $(SRC) $(LIBS)
	./tests/test_ds

run: $(EXE)
	./$(EXE) -w ../frontend -d data

clean:
	rm -f musicms musicms.exe tests/test_ds tests/test_ds.exe

.PHONY: all test run clean
