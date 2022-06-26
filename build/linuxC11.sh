#!/bin/sh

gcc -std=c11 -I ../ ../test/test.c -o test.exe -pthread

if [ $? -ne 0 ]
then
	echo "failled to build"
	exit 1
fi

./test.exe

if [ $? -ne 0 ]
then
	echo "test failled"
	exit 1
fi

exit 1
