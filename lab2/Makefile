CC=g++
CFLAGS=-c
LFLAGS=
OBJS=sched.o 

lab2: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o sched

sched.o: sched.cpp
	$(CC) $(CFLAGS) sched.cpp

clean:
	rm -rf *.o 
