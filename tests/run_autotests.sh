#!/bin/bash

NUM_FAILED=0

# Determine path of the current script
HERE=$(cd `dirname $0` && pwd)

RESOURCES="${HERE}/.."

# Determine paths to endless-sky executable and other relevant data
ES_EXEC_PATH="${RESOURCES}/endless-sky"
ES_CONFIG_TEMPLATE_PATH="${RESOURCES}/tests/config"

ES_CONFIG_PATH=$(mktemp --directory)
ES_SAVES_PATH="${ES_CONFIG_PATH}/saves"

if [ ! $? ]
then
	echo "Error: couldn't create temporary directory"
	exit 1
fi
cp "${ES_CONFIG_TEMPLATE_PATH}/*" "${ES_CONFIG_PATH}"


echo "***********************************************"
echo "***         ES Autotest-runner              ***"
echo "***********************************************"

if [ ! -f "${ES_EXEC_PATH}" ] || [ ! -x "${ES_EXEC_PATH}" ]
then
	echo "Endless sky executable not found. Returning failure."
	exit 1
fi

echo " Setting up environment and retrieving test-case list"
mkdir -p "${ES_CONFIG_PATH}"
mkdir -p "${ES_SAVES_PATH}"
TESTS=$("${ES_EXEC_PATH}" --tests --resources "${RESOURCES}" --config "${ES_CONFIG_PATH}")
TESTS_OK=$(echo "${TESTS}" | grep -e "ACTIVE$" | cut -d$'\t' -f1)
TESTS_NOK=$(echo "${TESTS}" | grep -e "KNOWN FAILURE$" -e "MISSING FEATURE$" | cut -d$'\t' -f1)
echo ""

#TODO: Allow running known-failures by default as well (to check if they accidentally got solved)
echo "Tests not to execute (known failure or missing feature):"
echo "${TESTS_NOK}"
echo ""
echo "Tests to execute:"
echo "${TESTS_OK}"
echo "***********************************************"

# Set separator to newline (in case tests have spaces in their name)
IFS_OLD=${IFS}
IFS="
"

# Run all the tests
for TEST in ${TESTS_OK}
do
	TEST_RESULT="PASS"
	echo "Running test ${TEST}"
	"$ES_EXEC_PATH" --resources "${RESOURCES}" --test "${TEST}" --config "${ES_CONFIG_PATH}"
	if [ $? -ne 0 ]
	then
		TEST_RESULT="FAIL"
		NUM_FAILED=$((NUM_FAILED + 1))
	fi
	echo "Test ${TEST}: ${TEST_RESULT}"
	echo ""
done

IFS=${IFS_OLD}

if [ ${NUM_FAILED} -ne 0 ]
then
	echo "${NUM_FAILED} tests failed"
	exit 1
fi
echo "All executed tests passed"
exit 0
