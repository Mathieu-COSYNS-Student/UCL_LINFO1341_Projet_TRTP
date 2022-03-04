# LINFO1341 - Projet TRTP
#
# Authors :
#		Groupe 110
#		- Mathieu Cosyns - 86032100
#		- Brandon Ngoran Ntam - 41481800



CC							:=	gcc
CFLAGS					:=	-pedantic -Wvla -Wall -Werror -Wextra -Wshadow
LDFLAGS					:=	

PHONY						:=	all clean

all: packet_interface.o

packet_interface.o: packet_implem.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	@$(RM) -rv *.o

.PHONY = $(PHONY)