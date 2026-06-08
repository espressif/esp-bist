# Host-side unit tests for BIST helpers that can run without target hardware.
# Build and run all tests with:  make check

CC      ?= cc
CFLAGS  := -std=c99 -Wall -Wextra -Wpedantic -O2 \
           -I../../src/bist/core/clock/include
LDLIBS  := -lm

TESTS := test_xtal_deviation

all: $(TESTS)

test_xtal_deviation: test_xtal_deviation.c
	$(CC) $(CFLAGS) $< $(LDLIBS) -o $@

check: $(TESTS)
	@set -e; for t in $(TESTS); do echo "--- $$t ---"; ./$$t; done

clean:
	rm -f $(TESTS)

.PHONY: all check clean
