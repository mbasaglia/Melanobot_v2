#!/bin/bash

# NOTE: This script compiles and tests everything,
# better use the build instructions on README to compile.

COMPILER=/usr/bin/clang++

if [ ! -d build ]
then
	mkdir build
	cd build
	cmake -DMODULE_POSIX=ON -DCMAKE_CXX_COMPILER=$COMPILER ..
else
	cd build
fi

if [ ! -d log ]
then
	mkdir log
fi

COMMIT=$(git describe --always)
LOG_FILENAME=$(date "+log/build_$COMMIT_%FT%T.log")
ERROR_LOG=error.log
ERROR=false

if ! ( make && make coverage ) &>>"$LOG_FILENAME"
then
	ln -s -f -T "$LOG_FILENAME" "$ERROR_LOG"
	ERROR=true
else
	rm -f "$ERROR_LOG"
fi

make doc

$ERROR && exit 1
