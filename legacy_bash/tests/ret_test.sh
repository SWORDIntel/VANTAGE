#!/bin/bash
# ret_test.sh just returns with whatever option is passed on the command line

RETURN=$1
if [ "${RETURN}x" == "x" ]; then
	RETURN=0
fi

exit ${RETURN}
