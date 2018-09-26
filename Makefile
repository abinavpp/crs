.SUFFIXES:
# disables implicit rules

LD = gcc
CC = gcc
override CFLAGS += -g -O2 -lpthread -std=gnu89 \
	-Wall -Wshadow -Wcast-align -Wpointer-arith \
	-Wwrite-strings -Wundef -Wredundant-decls -Wextra -Wno-sign-compare \
	-Wformat-security -Wno-pointer-sign -Werror-implicit-function-declaration \
	-Wno-unused-parameter
CPPFLAGS += -MMD

TGT = crs

SRC = $(wildcard *.c)
OBJ = $(SRC:%.c=%.o)
DEP = $(SRC:%.c=%.d)

.PHONY: all
all : $(TGT)

%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

$(TGT) : $(OBJ)
	$(LD) $(LDFLAGS) $(CFLAGS) $^ -o $@


.PHONY: clean
clean : 
	rm -f $(DEP) $(OBJ) $(TGT)

-include $(DEP)
