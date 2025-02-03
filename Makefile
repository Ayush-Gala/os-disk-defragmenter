# Please change the compiler in the event of any errors
CC = gcc
CFLAGS = -std=c11 -O0 -g -Wall

TARGET = defrag
OUTPUT = disk_defrag

SRCS = defrag.c main.c
HEADERS = disk.h
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	@input=$(input); \
	if [ -z "$$input" ]; then \
		echo "Usage: make run input=<frag_image_number>"; \
	else \
		./$(TARGET) images_frag/disk_frag_$$input; \
	fi

diff:
	@input=$(input); \
	if [ -z "$$input" ]; then \
		echo "Usage: make diff input=<defrag_image_number>"; \
	else \
		python2 diff.py $(OUTPUT) images_defrag/disk_defrag_$$input; \
	fi

clean:
	rm -f $(OBJS) $(TARGET) $(OUTPUT)

.PHONY: all clean run diff
