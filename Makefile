CC = gcc
CFLAGS = -Wall -Iinclude

SRC = src/persist.c src/input_handling.c src/dir.c src/robust.c src/hashmap.c
OBJ = $(SRC:.c=.o)

TARGET = persist

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# debug build with AddressSanitizer
DEBUG_CC = clang
DEBUG_CFLAGS = -Wall -Iinclude -g -O0 \
	-fsanitize=address,undefined \
	-fno-omit-frame-pointer \
	-fno-optimize-sibling-calls

DEBUG_OBJ = $(SRC:.c=.debug.o)
DEBUG_TARGET = persist_debug

debug: $(DEBUG_TARGET)

$(DEBUG_TARGET): $(DEBUG_OBJ)
	$(DEBUG_CC) $(DEBUG_CFLAGS) $(DEBUG_OBJ) -o $(DEBUG_TARGET)

src/%.debug.o: src/%.c
	$(DEBUG_CC) $(DEBUG_CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o src/*.debug.o $(TARGET) $(DEBUG_TARGET)