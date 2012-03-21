CXX:= g++
CXXFLAGS:= -O2 -c
OBJS:= strip.o main.o

#create program target

mx-strip: $(OBJS)
	$(CXX) -o $@ $?

install: mx-strip
	sudo cp mx-strip /usr/bin

clean:
	rm mx-strip *.o

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $? -o $@
