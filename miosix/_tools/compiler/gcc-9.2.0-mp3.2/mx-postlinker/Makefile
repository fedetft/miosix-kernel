CXX:= g++
CXXFLAGS:= -MMD -MP -O2 -c
OBJ:= postlinker.o main.o

#create program target

mx-postlinker: $(OBJ)
	$(CXX) -o $@${SUFFIX} $(OBJ)

install: mx-postlinker
	cp mx-postlinker${SUFFIX} $(INSTALL_DIR)

clean:
	-rm mx-postlinker${SUFFIX} *.o *.d

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $? -o $@

#pull in dependecy info for existing .o files
-include $(OBJ:.o=.d)
