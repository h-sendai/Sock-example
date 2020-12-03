CC = g++
PROG = sample
CXXFLAGS = -g -O2 -Wall
# CXXFLAGS += -pthread
# LDFLAGS += -pthread
#LDLIBS += -L/usr/lib64/daqmw -lSock
# LDLIBS += -lrt
# LDFLAGS += -pthread

all: $(PROG)
OBJS += $(PROG).o
OBJS += logUtil.o
OBJS += Sock.o
$(PROG): $(OBJS)

clean:
	rm -f *.o $(PROG)
