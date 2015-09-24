#!/bin/sh
export TOPDIR=$(pwd)/../../
if [ ! -n  "$1" ]
then
	echo "please use like: cov.sh nosec | tinydtls | dtls "
	exit
fi
export APP_SEC_TYPE=$1

cd ${TOPDIR} ; make cover APP_SEC_TYPE=${APP_SEC_TYPE}
${TOPDIR}./bin/test_limit
${TOPDIR}./bin/test_node
${TOPDIR}./bin/test_wdProperty

cd ${TOPDIR}/example/linux; lcov -d ./ -c -o outputfile.info

cd ${TOPDIR}/platform/linux; lcov -d ./ -c -o outputfile.info

cd ${TOPDIR}/src; lcov -d ./ -c -o outputfile.info

cd ${TOPDIR}/src/networking/coap; lcov -d ./ -c -o outputfile.info

cd ${TOPDIR}/src/secure/nosec; lcov -d ./ -c -o outputfile.info

cd ${TOPDIR}/src/serialize/cbor; lcov -d ./ -c -o outputfile.info

cd ${TOPDIR}; mkdir -p cov
cd ${TOPDIR}/cov; \
touch total.info; \
lcov -add-tracefile ../platform/linux/outputfile.info -a ../src/outputfile.info -a ../src/networking/coap/outputfile.info -a ../src/secure/nosec/outputfile.info -a ../src/serialize/cbor/outputfile.info -o total.info; \
genhtml total.info
