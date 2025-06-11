CC = gcc
CFLAGS = -Wall -Wextra -std=c99
EXECUTABLE = test_printf

all: $(EXECUTABLE) output.txt

$(EXECUTABLE): myprintf.c
	$(CC) $(CFLAGS) myprintf.c -o $@

output.txt: $(EXECUTABLE)
	./$(EXECUTABLE) > $@

run: $(EXECUTABLE)
	./$(EXECUTABLE)

test: $(EXECUTABLE)
	./$(EXECUTABLE) > output.txt

clean:
	@rm -f $(EXECUTABLE) output.txt
	@rm -rf $(EXECUTABLE).dSYM

.PHONY: all run clean test