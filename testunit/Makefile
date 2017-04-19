
CPPSRCS:=testProjectBuffer.cpp ../src/ProjectBuffer.cpp ../src/utilites.cpp ../src/log4z.cpp ../src/commonFunc.cpp
OBJS:=$(CPPSRCS:.cpp=.o)

CXXFLAGS:= -g -Wall
testProjectBuffer: ${OBJS} gtest.a
	${CXX} -o $@ $^ -lpthread
	rm -f ${OBJS}
