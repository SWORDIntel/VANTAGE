# MEMSHADOW Protocol C Implementation Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -fPIC
LDFLAGS = -lz
TARGET = libmemshadow.so
STATIC_TARGET = libmemshadow.a
TEST_TARGET = memshadow_test

# Source files
SOURCES = memshadow.c memshadow_pqc.c memshadow_vlan_topology.c memshadow_dpi_evasion.c memshadow_traffic_analysis_resistance.c memshadow_covert_vlan.c memshadow_gossip.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = memshadow.h

# Default target
all: $(TARGET) $(STATIC_TARGET)

# Shared library
$(TARGET): $(OBJECTS)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

# Static library
$(STATIC_TARGET): $(OBJECTS)
	ar rcs $@ $^

# Object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Test program
test: $(TEST_TARGET)

$(TEST_TARGET): ../examples/c/example.c $(STATIC_TARGET)
	$(CC) $(CFLAGS) -I. -o $@ $< -L. -lmemshadow $(LDFLAGS)

# Install
install: $(TARGET)
	install -d /usr/local/lib
	install -d /usr/local/include
	install -m 644 $(TARGET) /usr/local/lib/
	install -m 644 $(HEADERS) /usr/local/include/
	ldconfig

# Clean
clean:
	rm -f $(OBJECTS) $(TARGET) $(STATIC_TARGET) $(TEST_TARGET)

.PHONY: all test install clean
