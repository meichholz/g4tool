#
# $Author: eichholz $
# $Revision: 1.1 $
# $State: Exp $
# $Date: 2001/12/28 21:50:34 $
#
# g4tool
#
# (C) Marian Matthias Eichholz
#
################
NAME=		g4tool
################

CC=		gcc
CCFLAGS=	-g -Wall -pg		# Testing
LDFLAGS=	-L${HOME}/lib -pg
CCFLAGS=	-O3			# Production quality
LDFLAGS=	-L${HOME}/lib
INCLUDE=	-I ${HOME}/include
OFILES=		main.o tiff.o encode.o tables.o pcl.o decode.o
HFILES=		global.h

TIFFLIB=	-ltiff

DOCXX=		doc++
DOCFLAGS=	-p

DOCS=		../docs/$(NAME)

BINARIES=	$(NAME) tiff2bacon


.c.o :
	@echo -n "$< "
	@${CC} ${INCLUDE} ${CCFLAGS} -c $*.c -o $*.o

default:	test
install:	g4tool tiff2bacon
		install -s -m755 g4tool /usr/stuff/bin
		install -s -m755 tiff2bacon /usr/stuff/bin

new:		clean $(BINARIES)
all:		new
bin:		$(NAME)

##################

${OFILES}:	${HFILES}

test:	testt2b

testt2b:	tiff2bacon
		@echo "testing tiff2bacon..."
		sh dotest_t2b.sh

testg4:	$(NAME)
	@echo "test $(NAME)..."
	@dotest

docs:
	@${DOCXX} ${DOCFLAGS} -d ${DOCS} $(NAME).dxx

alldocs:	docs
	@${DOCXX} ${DOCFLAGS} -t -o doc.texi $(NAME).dxx
	@a2tex doc.texi doc.tex
	@rm doc.texi
	latex doc.tex
	latex doc.tex

clean:
	@echo "cleanup..."
	@rm -f *.o $(NAME) *~ *% t tt* doc*

tiff2bacon:	tiff2bacon.o
	@echo "linking $<..."
	@${CC} -o $@ ${LDFLAGS} $@.c ${TIFFLIB}

$(NAME):	${OFILES}
	@echo "linking $<..."
	@${CC} -o $@ ${LDFLAGS} ${OFILES}
