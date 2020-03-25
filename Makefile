CC      = g++
CFLAGS  = -g -Wall
SOURCES = $(shell ls *.cpp)
OBJS    = $(SOURCES:.cpp=.o)
TARGET  = dfgmap

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS)

%.o: %.cpp 
	$(CC) $(CFLAGS) -o $@ -c $< 

clean:
	$(RM) $(OBJS) $(TARGET)
