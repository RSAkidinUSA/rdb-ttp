CC = gcc
CFLAGS += -Wall -Werror -std=c99
SRC	= $(wildcard *.c)
OBJ = $(patsubst %.c,%.o, $(SRC))

APP = rdb-ttp

DEPDIR := .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(TARGET_ARCH) -c

POSTCOMPILE = @mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

%.o : %.c
%.o : %.c $(DEPDIR)/%.d
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

.PHONY: clean

all: CFLAGS += -g
all: build

O1: CFLAGS += -O1
O1:	build

O2: CFLAGS += -O2
O2:	build

O3: CFLAGS += -O3
O3:	build

build: $(OBJ)
	$(CC) -o $(APP) $(OBJ)

clean:
	rm -f $(OBJ) $(APP)

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRC))))
