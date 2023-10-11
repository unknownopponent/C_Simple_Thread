#!/bin/sh

gcc -std=c11 -I ../src/ ../test/test.c -o test -pthread -g

if [ $? -ne 0 ]
then
	echo "failled to build"
	exit 1
fi

./test

if [ $? -ne 0 ]
then
	echo "test failled"
	exit 1
fi

exit 1
