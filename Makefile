CFLAGS = -std=c89 -Ofast -Wall -Wextra -Iinclude/
OUT = build

all: $(OUT)
	$(CC) $(CFLAGS) $(wildcard src/**.c) -o $(OUT)/lnstat

lib: $(OUT)
	$(CC) $(CFLAGS) -c src/lnstat.c -o $(OUT)/lnstat.o
	$(AR) rcs $(OUT)/liblnstat.a $(OUT)/lnstat.o

clean:
	rm -rf $(OUT)/

$(OUT):
	@mkdir -p $(OUT)

.PHONY: all clean
