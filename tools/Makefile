HOST_ENV := $(shell uname 2>/dev/null || echo Unknown)

CC := gcc
CXX := g++
CFLAGS := -I. -O2 -s
LDFLAGS := -lstdc++ -lstdc++fs
ALL_PROGRAMS := makefs

BUILD_PROGRAMS := $(ALL_PROGRAMS)

default: all

makefs_SOURCES	:= makefs.cpp

all: $(BUILD_PROGRAMS)

clean:
	$(RM) $(ALL_PROGRAMS)

define COMPILE
$(1): $($1_SOURCES)
	$$(CXX) $(CFLAGS) $($1_CFLAGS) $$^ -o $$@ $($1_LDFLAGS) $(LDFLAGS)
endef

$(foreach p,$(BUILD_PROGRAMS),$(eval $(call COMPILE,$(p))))

.PHONY: all clean default
