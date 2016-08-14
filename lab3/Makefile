CC=g++ -std=c++11
#CC=g++48 -std=c++11
CFLAGS=-c
LFLAGS=
OBJS=mmu.o 

lab3: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o mmu

mmu.o: mmu.cpp
	$(CC) $(CFLAGS) mmu.cpp

clean:
	rm -rf *.o 
