GXX=g++
OPTIMIZATION_FLAGS=-g3 -gdwarf-2
OBJS=main.o CircuitNetList.o SpefNet.o LibertyLibrary.o WireDelayModel.o DesignConstraints.o timer_interface.o TimingAnalysis.o Parser.o

all: include/Transitions.h $(OBJS)
	$(GXX) $(OPTIMIZATION_FLAGS) -o TimingAnalysis $(OBJS)

main.o: main.cpp
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o main.o main.cpp

TimingAnalysis.o: TimingAnalysis.cpp
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o TimingAnalysis.o TimingAnalysis.cpp

Parser.o: Parser.cpp
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o Parser.o Parser.cpp

CircuitNetList.o: CircuitNetList.cpp
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o CircuitNetList.o CircuitNetList.cpp

WireDelayModel.o: WireDelayModel.cpp
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o WireDelayModel.o WireDelayModel.cpp

LibertyLibrary.o: LibertyLibrary.cpp
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o LibertyLibrary.o LibertyLibrary.cpp

SpefNet.o: SpefNet.cpp
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o SpefNet.o SpefNet.cpp

DesignConstraints.o: DesignConstraints.cpp
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o DesignConstraints.o DesignConstraints.cpp

timer_interface.o: timer_interface.cpp
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o timer_interface.o timer_interface.cpp	


clean:
	rm -rf *.o TimingAnalysis