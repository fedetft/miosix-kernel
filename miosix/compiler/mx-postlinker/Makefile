CXX:= g++
CXXFLAGS:= -O2 -c
OBJS:= postlinker.o main.o

#create program target

mx-postlinker: $(OBJS)
	$(CXX) -o $@ $?

install: mx-postlinker
	cp mx-postlinker $(INSTALL_DIR)

clean:
	rm mx-postlinker *.o

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $? -o $@
