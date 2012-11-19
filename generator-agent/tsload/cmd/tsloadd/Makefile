TYPE = EXEC
TARGET = tsloadd

include ../../Makefile.master

SRC = main.c \
	  cfgfile.c
OBJ = $(SRC:.c=.o)

CFLAGS += $(INCTSLOAD) $(INCJSON)
LDFLAGS += -lc $(LINKTSLOAD) $(LINKJSON)
	
include ../../Makefile.build