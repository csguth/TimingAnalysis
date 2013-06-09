GXX=g++
OPTIMIZATION_FLAGS=-O3
OBJS=main.o TimingAnalysis.o

all: $(OBJS)
	$(GXX) -o TimingAnalysis $(OBJS)

main.o:
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o main.o main.cpp

TimingAnalysis.o:
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o TimingAnalysis.o TimingAnalysis.cpp

clean:
	rm -rf *.o TimingAnalysis