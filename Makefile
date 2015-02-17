PROG:=c2f
QASMS:=c2f.qasm
SRCS:=main.c mailbox/xmailbox.c mailbox/mailbox.c mailbox/pagesize.c
ALLDEPS:=
CFLAGS:=-Wall -Wextra -Imailbox/
CC:=gcc
RM:=rm -f
SUDO:=sudo
QBIN2HEX:=./qbin2hex.py
QCC=./qtc
TOCLEAN:=

ALLDEPS+=$(QBIN2HEX) $(QCC)

TARGETS:=all $(PROG) %.c.o %.c.d run clean
TARGETS_NO_NEED_DEPS:=clean
BINS:=$(QASMS:%.qasm=%.qasm.bin)
HEXS:=$(BINS:%.bin=%.bin.hex)
OBJS:=$(SRCS:%.c=%.c.o)
DEPS:=$(SRCS:%.c=%.c.d)

COMPILE.c=$(CC) $(CFLAGS) -c
COMPILE.d=$(CC) $(CFLAGS) -M -MP -MT $<.o -MF $@
LINK.o=$(CC) $(LDFLAGS)

all: $(PROG)

EXTRA_TARGETS:=$(filter-out $(TARGETS), $(MAKECMDGOALS))
ifneq '$(EXTRA_TARGETS)' ''
 $(error No rule to make target `$(word 1, $(EXTRA_TARGETS))')
else
 ifeq '$(filter-out $(TARGETS_NO_NEED_DEPS), $(MAKECMDGOALS))' '$(MAKECMDGOALS)'
  sinclude $(DEPS)
 else
  ifneq '$(words $(MAKECMDGOALS))' '1'
   $(error Specify only one target if you want to make a target that needs no dependency file)
  endif
 endif
endif

MAKEFILE_LIST_SANS_DEPS:=$(filter-out %.c.d, $(MAKEFILE_LIST))
ALLDEPS+=$(MAKEFILE_LIST_SANS_DEPS)

.PRECIOUS: $(BINS)

main.c.d: c2f.qasm.bin.hex

$(PROG): $(HEXS) $(OBJS) $(ALLDEPS)
	$(LINK.o) $(OUTPUT_OPTION) $(OBJS)

%.c.o: %.c $(ALLDEPS)
	$(COMPILE.c) $(OUTPUT_OPTION) $<

%.c.d: %.c $(ALLDEPS)
	$(COMPILE.d) $(OUTPUT_OPTION) $<

%.bin.hex: %.bin $(ALLDEPS)
	$(QBIN2HEX) <$< >$@

%.qasm.bin: %.qasm $(ALLDEPS)
	$(QCC) <$< >$@

./qtc:
	make -C qpu-trivial-assembler/
	rm -f $@
	cp qpu-trivial-assembler/qtc ./

.PHONY: run
run: $(PROG)
	sudo ./$<

.PHONY: clean
clean:
	$(RM) $(PROG)
	$(RM) $(OBJS)
	$(RM) $(DEPS)
	$(RM) $(HEXS)
	$(RM) $(BINS)
	$(RM) $(TOCLEAN)
