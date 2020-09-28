[ -f ./curr_diff ] && rm curr_diff
cd ../
./compile_print_liveout.sh
cd tests/
gcc test.c -o test -ggdb && ./test
rm test
