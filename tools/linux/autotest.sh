#!/bin/sh

if [ ! -n  "$1" ]
then
	echo "please use like: autotest.sh nosec | tinydtls | dtls "
	exit
fi
cd ../../

export APP_SEC_TYPE=$1

make test APP_SEC_TYPE=${APP_SEC_TYPE}

./bin/test_wdProperty

WD_PROPERTY=$?

./bin/test_limit

WD_LIMIT=$?

./bin/test_node

WD_NODE=$?

./bin/test_multipleHost

WD_MULTIPLEHOST=$?

./bin/test_mts

WD_MTS=$?

./bin/test_step

WD_STEP=$?

make clean APP_SEC_TYPE=${APP_SEC_TYPE}
echo "\n*************************************************************************\n"

if [ ${WD_PROPERTY} -ne 0 ]
then
	echo "wilddog property test failed, please run test_wdProperty to find more information!"
else
	echo "wilddog property test pass!"
fi

if [ ${WD_LIMIT} -ne 0 ]
then
	echo "wilddog limit test failed, please run test_limit to find more information!"
else
	echo "wilddog limit test pass!"
fi

if [ ${WD_NODE} -ne 0 ]
then
	echo "wilddog node test failed, please run test_node to find more information!"
else
	echo "wilddog node test pass!"
fi
if [ "${WD_MULTIPLEHOST}" -ne "0" ]
then
	echo "wilddog test_multipleHost test failed, please run test_multipleHost to find more information!"
else
	echo "wilddog test_multipleHost test pass!"
fi
if [ ${WD_MTS} -ne 0 ]
then
	echo "wilddog test_mts test failed, please run test_mts to find more information!"
else
	echo "wilddog test_mts test pass!"
fi
if [ ${WD_STEP} -ne 0 ]
then
	echo "wilddog test_step test failed, please run test_step to find more information!"
else
	echo "wilddog test_step test pass!"
fi
echo "\n*************************************************************************\n"
