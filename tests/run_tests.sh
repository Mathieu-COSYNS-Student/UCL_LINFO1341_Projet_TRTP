echo "Tests in tests/test.c"
./test

test "$?" -ne "0" && exit 1

rm -rf logs/
mkdir logs/

echo "Tests over a perfect network"
TEST_NUMBER=01 FILE_SIZE=20 LINK_SIM_ARGS="-R" ./tests/simple_test.sh
TEST_NUMBER=02 FILE_SIZE=512 LINK_SIM_ARGS="-R" ./tests/simple_test.sh
TEST_NUMBER=03 FILE_SIZE=2000 LINK_SIM_ARGS="-R" ./tests/simple_test.sh
TEST_NUMBER=04 FILE_SIZE=4096 LINK_SIM_ARGS="-R" ./tests/simple_test.sh

echo "Tests over a network with 50ms latency"
TEST_NUMBER=05 FILE_SIZE=20 LINK_SIM_ARGS="-d 50 -R" ./tests/simple_test.sh

echo "Tests over a network with 1000ms latency"
TEST_NUMBER=06 FILE_SIZE=20 LINK_SIM_ARGS="-d 1000 -R" ./tests/simple_test.sh

echo "Tests over a network with last packet loss"
TEST_NUMBER=07 FILE_SIZE=20 LINK_SIM_ARGS="-l 5 -R -s 1648045140" ./tests/simple_test.sh

echo "Tests over a network with packet loss"
TEST_NUMBER=08 FILE_SIZE=2000 LINK_SIM_ARGS="-l 5 -R -s 1648045140" ./tests/simple_test.sh

echo "Tests over a network with 50ms latency and packet loss"
TEST_NUMBER=09 FILE_SIZE=2000 LINK_SIM_ARGS="-l 5 -d 50 -R -s 1648045140" ./tests/simple_test.sh

echo "Tests, with Valgrind"
TEST_NUMBER=10 VALGRIND=1 FILE_SIZE=4096 LINK_SIM_ARGS="-R" ./tests/simple_test.sh

echo "Tests with 0 byte file"
TEST_NUMBER=11 FILE_SIZE=0 LINK_SIM_ARGS="-R" ./tests/simple_test.sh

echo "Tests with a large file that exceeds seqnum=255"
TEST_NUMBER=12 FILE_SIZE=135168 LINK_SIM_ARGS="-R" ./tests/simple_test.sh

echo "Tests with corrupted packets"
TEST_NUMBER=13 FILE=tests/files/man_man LINK_SIM_ARGS="-e 5 -R -s -707636660" ./tests/simple_test.sh
TEST_NUMBER=14 FILE_SIZE=2000 LINK_SIM_ARGS="-e 5 -R -s 1649248568" ./tests/simple_test.sh
TEST_NUMBER=15 FILE_SIZE=2000 LINK_SIM_ARGS="-e 5 -R -s 1649249706" ./tests/simple_test.sh
TEST_NUMBER=16 FILE_SIZE=5000 LINK_SIM_ARGS="-e 5 -R -s 1649249909" ./tests/simple_test.sh
TEST_NUMBER=17 FILE=tests/files/man_man LINK_SIM_ARGS="-e 5 -R -s 1649249897" ./tests/simple_test.sh
TEST_NUMBER=18 FILE_SIZE=5000 LINK_SIM_ARGS="-e 5 -R -s 1649249879" ./tests/simple_test.sh

# echo "Tests with truncated packets"
# TEST_NUMBER=19 FILE_SIZE=20 LINK_SIM_ARGS="-c 10 -R -s 1649256310" ./tests/simple_test.sh
# TEST_NUMBER=20 FILE_SIZE=512 LINK_SIM_ARGS="-c 10 -R" ./tests/simple_test.sh
# TEST_NUMBER=21 FILE_SIZE=2000 LINK_SIM_ARGS="-c 10 -R" ./tests/simple_test.sh
# TEST_NUMBER=22 FILE_SIZE=4096 LINK_SIM_ARGS="-c 10 -R" ./tests/simple_test.sh