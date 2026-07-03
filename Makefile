CC = clang
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wno-unused-parameter -I include -O2
SRCS = $(shell find src/ -name "*.c" | sort)
OBJS = $(SRCS:.c=.o)
LIB = libvaultlog.a

all: $(LIB)

$(LIB): $(OBJS)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(LIB)

.PHONY: all clean
