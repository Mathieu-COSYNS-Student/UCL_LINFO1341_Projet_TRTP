# LINFO1341 - Projet TRTP
#
# Authors :
#		Groupe 110
#		- Mathieu Cosyns - 86032100
#		- Brandon Ngoran Ntam - 41481800

CC								:= gcc
CFLAGS						:= -std=gnu99 -pedantic -Wvla -Wall -Werror -Wextra -Wshadow -D_COLOR
LDFLAGS						:= -lz

# Adapt these as you want to fit with your project
COMMON_SOURCES		:= $(wildcard src/log.c src/packet.c src/xxd.c src/statistics.c src/real_address.c src/create_socket.c)
SENDER_SOURCES		:= $(wildcard src/sender.c)
RECEIVER_SOURCES	:= $(wildcard src/receiver.c)
TEST_SOURCES			:= $(wildcard tests/test.c tests/packet_tests.c tests/testBrandon.c)

COMMON_OBJECTS		:= $(COMMON_SOURCES:.c=.o)
SENDER_OBJECTS		:= $(SENDER_SOURCES:.c=.o)
RECEIVER_OBJECTS	:= $(RECEIVER_SOURCES:.c=.o)
TEST_OBJECTS			:= $(TEST_SOURCES:.c=.o)

SENDER						:= sender
RECEIVER					:= receiver
TEST							:= test

PHONY							:= all clean debug zip $(SENDER) $(RECEIVER) $(TEST) tests

all: $(SENDER) $(RECEIVER)

$(SENDER): $(SENDER_OBJECTS) $(COMMON_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(RECEIVER): $(RECEIVER_OBJECTS) $(COMMON_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST): $(TEST_OBJECTS) $(COMMON_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

tests: all $(TEST)
	./test
	./tests/run_tests.sh

*.o: *.c
	$(CC) $(CFLAGS) -c -o $@ $^

# By default, logs are disabled. But you can enable them with the debug target.
debug: CFLAGS += -D_DEBUG -g
debug: clean all $(TEST)

# Place the zip in the parent repository of the project
ZIP_NAME="../projet1_Cosyns_NgoranNtam.zip"

# A zip target, to help you have a proper zip file. You probably need to adapt this code.
zip:
	# Generate the log file stat now. Try to keep the repository clean.
	git log --stat > gitlog.stat
	zip -r $(ZIP_NAME) Makefile README.md src tests rapport.pdf gitlog.stat
	# We remove it now, but you can leave it if you want.
	rm gitlog.stat


clean:
	@$(RM) -rv src/*.o tests/*.o  $(SENDER) $(RECEIVER) $(TEST) input_file *.log

.PHONY = $(PHONY)