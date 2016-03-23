# compile and link flags
CC = g++ -std=c++11
CCFLAGS = -Wall -g
LDFLAGS = -Wall -g

# make rules
TARGET = dsdv
OBJECTS = dsdv_table.o dsdv_transfer.o dsdv.o

all: $(TARGET)

.cc.o: 
	$(CC) $(CCFLAGS) -c -o $@ $<

dsdv_table.o: dsdv_table.h

dsdv_transfer.o: dsdv_transfer.h

dsdv.o: dsdv_table.h dsdv_transfer.h

dsdv: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

.PHONY: clean
clean:
	-rm -f *~ *.o $(TARGET)
