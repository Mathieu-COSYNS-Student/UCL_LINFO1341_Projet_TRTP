echo "Tests in tests/test.c"
./test

echo "A very simple test"
./tests/simple_test.sh

echo "A very simple test, with Valgrind"
VALGRIND=1 ./tests/simple_test.sh