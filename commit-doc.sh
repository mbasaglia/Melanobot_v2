#!/bin/bash

if [ -n "$1" -a "$(basename "$(git -C "$1" rev-parse --show-toplevel)")" = html ]
then
    git -C "$1" commit -am "Update from Doxygen"
    git -C "$1" push origin gh-pages
else
    echo "$1 isn't a git repository"
fi
