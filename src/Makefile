GXX=g++
OPTIMIZATION_FLAGS=-g3
OBJS=main.o CircuitNetList.o SpefNet.o LibertyLibrary.o WireDelayModel.o  TimingAnalysis.o Parser.o

all: $(OBJS)
	$(GXX) -o TimingAnalysis $(OBJS)

main.o:
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o main.o main.cpp

TimingAnalysis.o:
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o TimingAnalysis.o TimingAnalysis.cpp

Parser.o:
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o Parser.o Parser.cpp

CircuitNetList.o:
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o CircuitNetList.o CircuitNetList.cpp

WireDelayModel.o:
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o WireDelayModel.o WireDelayModel.cpp

LibertyLibrary.o:
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o LibertyLibrary.o LibertyLibrary.cpp

SpefNet.o:
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o SpefNet.o SpefNet.cpp


clean:
	rm -rf *.o TimingAnalysis