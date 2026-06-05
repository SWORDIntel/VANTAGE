#!/bin/bash
# Last Edit: 06/04/2013
#
# Checks syntax of bash scripts

BASH=${BASH:-`which bash`}

${BASH} -n $1
