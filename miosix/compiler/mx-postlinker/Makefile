CXX:= g++ -m32
CXXFLAGS:= -O2 -c
OBJS:= postlinker.o main.o
DFLAGS:= -MMD -MP

#create program target

mx-postlinker: $(OBJS)
	$(CXX) -o $@ $?

install: mx-postlinker
	cp mx-postlinker $(INSTALL_DIR)

clean:
	-rm mx-postlinker *.o *.d

%.o: %.cpp
	$(CXX) $(DFLAGS) $(CXXFLAGS) $? -o $@

#pull in dependecy info for existing .o files
-include $(OBJ:.o=.d)
