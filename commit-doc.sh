#!/bin/bash
# NOTE This script is used to automatically commit changes to gh-pages
# after Doxygen has been run. Call whit via `make doc-commit`, not directly

if [ -n "$1" -a "$(basename "$(git -C "$1" rev-parse --show-toplevel)")" = html ]
then
    cd "$1"
    git add .
    git commit -am "Update from Doxygen"
    git push origin gh-pages
else
    echo "$1 isn't a git repository"
fi
