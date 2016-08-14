CC=g++
CFLAGS=-c
LFLAGS=
OBJS=iosched.o 

lab4: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o iosched

iosched.o: iosched.cpp
	$(CC) $(CFLAGS) iosched.cpp

clean:
	rm -rf *.o 
