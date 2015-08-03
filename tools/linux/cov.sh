#!/bin/sh

make clean
make 
make cover
./bin/test_limit
./bin/test_node
./bin/test_wdProperty

cd example/linux
lcov -d ./ -c -o outputfile.info

cd ../../platform/linux
lcov -d ./ -c -o outputfile.info

cd ../../src
lcov -d ./ -c -o outputfile.info

cd ./networking/coap
lcov -d ./ -c -o outputfile.info

cd ../../secure/nosec
lcov -d ./ -c -o outputfile.info

cd ../../serialize/cbor
lcov -d ./ -c -o outputfile.info

cd ../../../
mkdir -p cov
cd ./cov
touch total.info
lcov -add-tracefile ../platform/linux/outputfile.info -a ../src/outputfile.info -a ../src/networking/coap/outputfile.info -a ../src/secure/nosec/outputfile.info -a ../src/serialize/cbor/outputfile.info -o total.info
genhtml total.info
