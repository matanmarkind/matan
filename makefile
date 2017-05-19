# to use Makefile variables later in the Makefile: $(<var>)
#
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CC = clang++
CFLAGS = -g -Wall -std=c++1z -pthread
BINDIR = bin

bigmap: timsort.hh BigMap.hh bigmap.cc
				$(CC) $(CFLAGS) bigmap.cc -o $(BINDIR)/bigmap

threadpool: ThreadPool.hh threadpool.cc
				$(CC) $(CFLAGS) threadpool.cc -o $(BINDIR)/threadpool

msgq: MessageQueue.hh message_queue.cc
				$(CC) $(CFLAGS) message_queue.cc -o $(BINDIR)/msgq


# .PHONY is so that make doesn't confuse clean with a file
.PHONY clean: 
clean:
				rm -r $(BINDIR) && mkdir $(BINDIR)
				rm -f *.o *~
