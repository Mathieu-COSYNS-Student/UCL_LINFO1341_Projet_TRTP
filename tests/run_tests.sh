echo "Tests in tests/test.c"
./test

echo "Test over a perfect network"
FILE_SIZE=20 LINK_SIM_ARGS="-l 0 -d 0 -R" ./tests/simple_test.sh

# echo "Test over a perfect network"
# FILE_SIZE=512 LINK_SIM_ARGS="-l 0 -d 0 -R" ./tests/simple_test.sh

# echo "Test over a perfect network"
# FILE_SIZE=2000 LINK_SIM_ARGS="-l 0 -d 0 -R" ./tests/simple_test.sh

# echo "Test over a perfect network with 50ms latency"
# FILE_SIZE=512 LINK_SIM_ARGS="-l 0 -d 50 -R" ./tests/simple_test.sh

# echo "A very simple test, with Valgrind"
# VALGRIND=1 ./tests/simple_test.sh