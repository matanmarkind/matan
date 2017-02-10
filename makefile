# to use Makefile variables later in the Makefile: $(<var>)
#
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CC = g++
CFLAGS = -g -Wall -std=c++11
BINDIR = bin

bigmap: timsort.hh BigMap.hh bigmap.cc
				$(CC) $(CFLAGS) timsort.hh BigMap.hh bigmap.cc -o $(BINDIR)/bigmap

# .PHONY is so that make doesn't confuse clean with a file
.PHONY clean: 
clean:
				rm -r $(BINDIR) && mkdir $(BINDIR)
				rm -f *.o *~
