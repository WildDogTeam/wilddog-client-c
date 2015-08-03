#!/bin/sh

#ilcov -z -d ./
find ../../ -name "*.gcda" |xargs rm -f
find ../../ -name "*.gcno" |xargs rm -f
find ../../ -name "outputfile.info" |xargs rm -f
rm -rf ../../cov
lcov -z -d ./
