# to use Makefile variables later in the Makefile: $(<var>)
#
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
#
# Example usage
# matan@matanm:~/cpp/matan$ clang++ -g -std=c++1z -fsanitize=address -fno-omit-frame-pointer junk.cc -o bin/junk 
# matan@matanm:~/cpp/matan$ ASAN_SYMBOLIZER_PATH=/usr/lib/llvm-3.8/bin/llvm-symbolizer bin/junk 
#
SANITIZER?=address
SANITIZER_FLAGS = -fsanitize=$(SANITIZER) -fsanitize=undefined -fno-omit-frame-pointer
CC = clang++-4.0
CFLAGS = -g -Wall -std=c++1z -pthread $(SANITIZER_FLAGS)
BINDIR = bin

bigmap: timsort.hh BigMap.hh bigmap.cc
				$(CC) $(CFLAGS) bigmap.cc -o $(BINDIR)/bigmap

threadpool: ThreadPool.hh threadpool.cc
				$(CC) $(CFLAGS) threadpool.cc -o $(BINDIR)/threadpool

bunchq: BunchQueue.hh bunch_queue.cc
				$(CC) $(CFLAGS) bunch_queue.cc -o $(BINDIR)/bunchq

logger: Logger.o logger.cc
				$(CC) $(CFLAGS) Logger.o logger.cc -o $(BINDIR)/logger

Logger.o: BunchQueue.hh AsyncWorker.hh Logger.hh Logger.cc
				$(CC) $(CFLAGS) -c Logger.cc

# .PHONY is so that make doesn't confuse clean with a file
.PHONY clean: 
clean:
				rm -r $(BINDIR) && mkdir $(BINDIR)
				rm -f *.o *~
