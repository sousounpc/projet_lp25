CC = gcc
CFLAGS = -Wall -Werror -std=c99 -I.
LDFLAGS = -lm -lncurses

TARGET = htop_lp25


SRC_FILES = main.c manager/manager.c process/process.c ui/ui.c network/network.c
OBJ_FILES = $(patsubst %.c, %.o, $(SRC_FILES))

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: all clean

clean:
	$(RM) $(TARGET)
	$(RM) *.o manager/*.o process/*.o ui/*.o network/*.o
