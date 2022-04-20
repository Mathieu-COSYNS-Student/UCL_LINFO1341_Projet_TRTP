# LINFO1341 - Projet TRTP
#
# Authors :
#		Groupe 110
#		- Mathieu Cosyns - 86032100
#		- Brandon Ngoran Ntam - 41481800

CC								:= gcc
DEBUG_FLAGS				:= -D_DEBUG -g
COLOR_FLAGS				:= -D_COLOR
CFLAGS						:= -std=gnu99 -pedantic -Wvla -Wall -Werror -Wextra -Wshadow $(COLOR_FLAGS) $(DEBUG_FLAGS)
LDFLAGS						:= -lz -lm

# Adapt these as you want to fit with your project
COMMON_SOURCES		:= $(wildcard src/log.c src/packet.c src/xxd.c src/statistics.c src/address.c \
																src/create_socket.c src/read_write_loop.c src/wait_for_client.c \
																src/exchange_trtp.c src/window.c src/utils.c src/queue.c)
SENDER_SOURCES		:= $(wildcard src/sender.c)
RECEIVER_SOURCES	:= $(wildcard src/receiver.c)
TEST_SOURCES			:= $(wildcard tests/test.c tests/packet_tests.c tests/fec_tests.c tests/address_tests.c tests/queue_tests.c)

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

tests: all install_dependencies $(TEST)
	./tests/run_tests.sh

*.o: *.c
	$(CC) $(CFLAGS) -c -o $@ $^

# By default, logs are disabled. But you can enable them with the debug target.
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean all $(TEST)

install_dependencies:
	./install_dependencies.sh

# Place the zip in the parent repository of the project
ZIP_NAME="../projet1_Cosyns_NgoranNtam.zip"

# A zip target, to help you have a proper zip file. You probably need to adapt this code.
zip: clean
	# Generate the log file stat now. Try to keep the repository clean.
	git log --stat > gitlog.stat
	zip -r $(ZIP_NAME) Makefile README.md src tests rapport.pdf gitlog.stat
	# We remove it now, but you can leave it if you want.
	rm gitlog.stat


clean:
	@$(RM) -rv src/*.o tests/*.o $(SENDER) $(RECEIVER) $(TEST) logs/

.PHONY = $(PHONY)

# Wireshark filter
# (udp.port == 8888 || udp.port == 2456 || udp.port == 1341) && !(udp.srcport == 1341 && udp.dstport == 2456) && !(udp.srcport == 2456 && udp.dstport == 1341)