#!/bin/sh

# $Id: bootstrap,v 1.3 2002/02/11 16:54:42 eichholz Exp $

test Makefile && make distclean

rm aux/*
rm *.tar.gz
rm -rf g4tool-*/

for PAT in "*~" stamp-h.in aclocal.m4 Makefile Makefile.in configure "t.*" "tmp.*" "*.bak" ; do
  find -name "$PAT" -exec rm "{}" \;
done

rm config.* configure.scan src/config.h 2>/dev/null

test "$1" = "-c" && exit

autoscan
aclocal
autoconf
automake --add-missing --copy

test "$1" = "-a" || exit 0

./configure
make
make check
