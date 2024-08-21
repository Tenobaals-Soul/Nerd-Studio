#
# Configurations
#
EXE = nerd-studio-$(shell uname -m)
VERSION = 0.0.1

#
# Directories
#
SRC_DIR = src/c
INC_DIR = src/header
BUILDDIR = build

#
# Compiler flags
#
CC     = gcc
CFLAGS = -Wall -Wextra -Werror -DVERSION='"$(VERSION)"'

#
# Project files
#
SRCS = $(shell pushd $(SRC_DIR) >/dev/null && find . -name '*.c' && popd >/dev/null )
OBJS = $(SRCS:.c=.o)

#
# Test build settings
#
TSTDIR = $(BUILDDIR)/test
TSTOBJS = $(addprefix $(TSTDIR)/, $(OBJS))
TSTCFLAGS = -g -O0 -DDEBUG -DTEST

#
# Debug build settings
#
DBGDIR = $(BUILDDIR)/debug
DBGEXE = $(DBGDIR)/$(EXE)
DBGOBJS = $(addprefix $(DBGDIR)/, $(OBJS))
DBGCFLAGS = -g -O0 -DDEBUG

#
# Release build settings
#
RELDIR = $(BUILDDIR)/release
RELEXE = $(RELDIR)/$(EXE)
RELOBJS = $(addprefix $(RELDIR)/, $(OBJS))
RELCFLAGS = -O3 -DNDEBUG

#
# Linker and macro Settings
#
LFLAGS    = m GL glfw GLU
DEFINES   =
METAFLAGS = $(addprefix -I, $(INC_DIR)) \
			$(addprefix -D, $(DEFINES)) \
			$(addprefix -l, $(LFLAGS))

.PHONY: all clean debug prep release remake install

# Default build
all: prep release

#
# Test rules
#
$(TSTDIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(METAFLAGS) $(TSTCFLAGS) -o $@ $<

#
# Debug rules
#
debug: $(DBGEXE)

$(DBGEXE): $(DBGOBJS)
	$(CC) $(CFLAGS) $(METAFLAGS) $(DBGCFLAGS) -o $(DBGEXE) $^
	@mv $(DBGEXE) ./$(EXE)

$(DBGDIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(METAFLAGS) $(DBGCFLAGS) -o $@ $<

#
# Release rules
#
release: $(RELEXE)

$(RELEXE): $(RELOBJS)
	$(CC) $(CFLAGS) $(METAFLAGS) $(RELCFLAGS) -o $(RELEXE) $^
	@mv $(RELEXE) ./$(EXE)

$(RELDIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(METAFLAGS) $(RELCFLAGS) -o $@ $<

#
# Other rules
#
prep:
	@mkdir -p $(DBGDIR) $(RELDIR) $(TSTDIR) $(SRC_DIR) $(INC_DIR)

#
# test rules
#

TEST_CASE_DIR = tests
TEST_CORE_DIR = test_core

$(TEST_CASE_DIR)/%.elf: $(TEST_CASE_DIR)/%.c $(TSTOBJS) $(TEST_CORE_DIR)/test_core.c
	$(CC) $(CFLAGS) $(METAFLAGS) $(DBGCFLAGS) $(TSTOBJS) $(TEST_CORE_DIR)/test_core.c -I$(TEST_CORE_DIR) $< -o $@

test: $(TSTOBJS)
	@for file in $(TEST_CASE_DIR)/*.c ; do \
		target="$${file%%.*}".elf ; \
		make $${target} && \
		./$${target} ; \
		rm -f /$${target} ; \
	done

test-valgrind: $(TSTOBJS)
	@for file in $(TEST_CASE_DIR)/*.c ; do \
		target="$${file%%.*}".elf ; \
		make $${target} && \
		valgrind ./$${target} ; \
		rm -f /$${target} ; \
	done

remake: clean all

clean:
	rm -f $(RELEXE) $(RELOBJS) $(DBGEXE) $(TSTOBJS) $(DBGOBJS) $(TEST_CASE_DIR)/*.elf
	find $(BUILDDIR) -mindepth 1 -type d -empty -delete