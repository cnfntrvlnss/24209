CC=gcc#icc
CCC=g++#icpc
CXX=g++#icpc
TESTDIR:=test09
CPPFLAGS=-g -DTESTDIR=${TESTDIR}
LDFLAGS:=-Wl,-rpath,${IOADIR}
SRCCS:=$(addprefix src/,apue.c myiofuncs.c)
SRCCPPS:= $(addprefix src/,comm_struct.cpp test242.cpp waveinfo.cpp)
OBJCS:=$(SRCCS:.c=.o)
OBJCPPS:=$(SRCCPPS:.cpp=.o)
testmain: $(OBJCS) $(OBJCPPS)
	${CCC} ${LDFLAGS} -o $@ $^ -Lioacas -ldl -lpthread
	rm -f $^
	make deploy

.PHONY: DEPLOY
deploy:
	cp -t ../${TESTDIR} testmain
