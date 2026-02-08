CC = gcc
CFLAGS = -Wall -Iinclude

SRC = src/bitcask.c src/input_handling.c src/dir.c src/robust.c 
OBJ = $(SRC:.c=.o)  # converts src/db.c -> src/db.o

TARGET = main

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)