CXX:= g++
CXXFLAGS:= -O2 -c
OBJS:= postlinker.o main.o
DFLAGS:= -MMD -MP

#create program target

mx-postlinker: $(OBJS)
	$(CXX) -o $@${SUFFIX} $?

install: mx-postlinker
	cp mx-postlinker${SUFFIX} $(INSTALL_DIR)

clean:
	-rm mx-postlinker${SUFFIX} *.o *.d

%.o: %.cpp
	$(CXX) $(DFLAGS) $(CXXFLAGS) $? -o $@

#pull in dependecy info for existing .o files
-include $(OBJ:.o=.d)
