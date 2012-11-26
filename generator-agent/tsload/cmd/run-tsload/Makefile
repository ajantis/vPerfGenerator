TYPE = EXEC
TARGET = run-tsload

include ../../Makefile.master

SRC = main.c \
	  info.c \
	  load.c

OBJ = $(SRC:.c=.o)

CFLAGS += $(INCTSLOAD) $(INCJSON)
LDFLAGS += -lc $(LINKTSLOAD) $(LINKJSON)
EXTRAS = test.tss test.tsw profile.sh
	
include ../../Makefile.build