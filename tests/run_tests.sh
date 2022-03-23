echo "Tests in tests/test.c"
./test

echo "1 Test over a perfect network"
FILE_SIZE=20 LINK_SIM_ARGS="-l 0 -d 0 -R" ./tests/simple_test.sh

echo "2 Test over a perfect network"
FILE_SIZE=512 LINK_SIM_ARGS="-l 0 -d 0 -R" ./tests/simple_test.sh

echo "3 Test over a perfect network"
FILE_SIZE=2000 LINK_SIM_ARGS="-l 0 -d 0 -R" ./tests/simple_test.sh

echo "4 Test over a perfect network"
FILE_SIZE=4096 LINK_SIM_ARGS="-l 0 -d 0 -R" ./tests/simple_test.sh

echo "5 Test over a perfect network with 50ms latency"
FILE_SIZE=20 LINK_SIM_ARGS="-l 0 -d 50 -R" ./tests/simple_test.sh

echo "6 Test over a perfect network with 1000ms latency"
FILE_SIZE=20 LINK_SIM_ARGS="-l 0 -d 1000 -R" ./tests/simple_test.sh

echo "7 Test over a perfect network with last packet loss"
FILE_SIZE=20 LINK_SIM_ARGS="-l 5 -R -s 1648045140" ./tests/simple_test.sh

echo "8 Test over a perfect network with packet loss"
FILE_SIZE=2000 LINK_SIM_ARGS="-l 5 -R -s 1648045140" ./tests/simple_test.sh

echo "9 Test over a perfect network with 50ms latency and packet loss"
FILE_SIZE=2000 LINK_SIM_ARGS="-l 5 -d 50 -R -s 1648045140" ./tests/simple_test.sh


# echo "A very simple test, with Valgrind"
# VALGRIND=1 ./tests/simple_test.sh