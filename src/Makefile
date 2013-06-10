GXX=g++
OPTIMIZATION_FLAGS=-g3
OBJS=main.o  CircuitNetList.o TimingAnalysis.o Parser.o

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


clean:
	rm -rf *.o TimingAnalysis