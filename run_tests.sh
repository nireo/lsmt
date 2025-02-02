tests_dir="tests"

# compile each file in the test_dir and then run each compiled binary
for test in $(ls $tests_dir); do
  echo "compiling test: $test"
  gcc -o $test $tests_dir/$test bloom.c utils.c memtable.c lsmt.c

  echo "running test: $test"
  echo "--------------------------------"
  ./$test
  echo "--------------------------------"

  rm -rf $test
done


