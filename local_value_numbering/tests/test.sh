[ -f ./curr_diff ] && rm curr_diff
cd ../
./compile_apply_lvn.sh
cd tests/
gcc test.c -o test -ggdb && ./test
rm test
