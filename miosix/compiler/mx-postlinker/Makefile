CXX:= g++ -m32
CXXFLAGS:= -O2 -c -m32
OBJS:= postlinker.o main.o
DFLAGS    := -MM

#create program target

mx-postlinker: $(OBJS)
	$(CXX) -o $@ $?

install: mx-postlinker
	cp mx-postlinker $(INSTALL_DIR)

clean:
	rm mx-postlinker *.o *.d

#pull in dependecy info for existing .o files
-include $(OBJ:.o=.d)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $? -o $@
	$(CXX) $(DFLAGS) $(CXXFLAGS) $*.cpp > $*.d
