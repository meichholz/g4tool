#!/bin/sh

# $Id: bootstrap.sh,v 1.1 2002/01/05 22:10:48 eichholz Exp $

test Makefile && make distclean

rm aux/*

for PAT in "*~" stamp-h.in aclocal.m4 Makefile Makefile.in configure "t.*" "tmp.*" "*.bak" ; do
  find -name "$PAT" -exec rm "{}" \;
done

rm config.* configure.scan src/config.h 2>/dev/null

test "$1" = "-c" && exit

autoscan
aclocal
autoconf
automake --add-missing --copy
