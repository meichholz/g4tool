test Makefile && make distclean

rm -r build-aux aux m4
rm *.tar.gz
rm g4tool-*

for PAT in "*~" stamp-h.in aclocal.m4 Makefile Makefile.in configure "t.*" "tmp.*" "*.bak" ; do
  find . -name "$PAT" -exec rm "{}" \;
done

rm config.* configure.scan src/config.h 2>/dev/null

test "$1" = "-c" && exit

autoreconf --install

test "$1" = "-a" || exit 0

./configure
make
make check

