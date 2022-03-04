# LINFO1341 - Projet TRTP
#
# Authors :
#		Groupe 110
#		- Mathieu Cosyns - 86032100
#		- Brandon Ngoran Ntam - 41481800

CC							:=	gcc
CFLAGS					:=	-pedantic -Wvla -Wall -Werror -Wextra -Wshadow
LDFLAGS					:=	-lz

PHONY						:=	all clean

OBJ							:=	packet_interface.o xxd.o

all: test

test: test.c $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

xxd.o: xxd.c
	$(CC) $(CFLAGS) -c -o $@ $^

packet_interface.o: packet_implem.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	@$(RM) -rv *.o
	@$(RM) -rv test

.PHONY = $(PHONY)