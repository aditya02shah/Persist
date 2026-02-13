CC = gcc
CFLAGS = -Wall -Iinclude

SRC = src/persist.c src/input_handling.c src/dir.c src/robust.c src/hashmap.c
OBJ = $(SRC:.c=.o)  # converts src/db.c -> src/db.o

TARGET = persist

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)