bin_PROGRAMS=g4tool tiff2bacon
lib_LIBRARIES=libg4tool.a

g4tool_SOURCES=main.c global.h
# g4tool_HEADERS=global.h
g4tool_LDADD=libg4tool.a

libg4tool_a_SOURCES= encode.c decode.c pcl.c tiff.c tables.c 
# libg4tool_a_HEADERS=

tiff2bacon_SOURCES=tiff2bacon.c
tiff2bacon_LDADD=@TIFF_LIB@

# AM_CFLAGS=
