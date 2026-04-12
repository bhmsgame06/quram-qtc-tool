SRC_DIR := src
BUILD_DIR := bin

OBJS := $(SRC_DIR)/main.o \
		$(SRC_DIR)/qtc_decode.o \
		$(SRC_DIR)/qtc_encode.o \
		$(SRC_DIR)/qtc_tables.o \
		$(SRC_DIR)/build_map.o
LIBS := 

CFLAGS := -O2 -g
LDFLAGS := $(LIBS)

ifeq ($(OS),Windows_NT)
	CC := x86_64-w64-mingw32-gcc
	LD := x86_64-w64-mingw32-gcc
else
	CC := gcc
	LD := gcc
endif

PREFIX := /usr/local

all: qtc-tool

qtc-tool: $(OBJS)
	mkdir -p $(BUILD_DIR)
	$(LD) -o $(BUILD_DIR)/$@ $^ $(LDFLAGS)

%.o: %.c %.h
	$(CC) -c -o $@ $< $(CFLAGS)

install:
	install -Dm755 $(BUILD_DIR)/qtc-tool $(PREFIX)/bin

clean:
	rm -f $(OBJS)
