GXX=g++
OPTIMIZATION_FLAGS=-g3 -gdwarf-2
OBJS=timer_interface.o main.o timing_point.o timing_net.o timing_arc.o timing_analysis.o circuit_netlist.o configuration.o design_constraints.o liberty_library.o parser.o spef_net.o wire_delay_model.o

all: include/transitions.h $(OBJS)
	$(GXX) $(OPTIMIZATION_FLAGS) -o TimingAnalysis $(OBJS)

main.o: main.cpp
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o main.o main.cpp

timer_interface.o: timer_interface.cpp
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o timer_interface.o timer_interface.cpp

timing_point.o: timing_point.cpp
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o timing_point.o timing_point.cpp

timing_net.o: timing_net.cpp
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o timing_net.o timing_net.cpp

timing_arc.o: timing_arc.cpp
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o timing_arc.o timing_arc.cpp

timing_analysis.o: timing_analysis.cpp
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o timing_analysis.o timing_analysis.cpp

circuit_netlist.o: circuit_netlist.cpp
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o circuit_netlist.o circuit_netlist.cpp

configuration.o: configuration.cpp
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o configuration.o configuration.cpp

design_constraints.o: design_constraints.cpp
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o design_constraints.o design_constraints.cpp	

liberty_library.o: liberty_library.cpp
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o liberty_library.o liberty_library.cpp	

parser.o: parser.cpp
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o parser.o parser.cpp	

spef_net.o: spef_net.cpp
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o spef_net.o spef_net.cpp

wire_delay_model.o: wire_delay_model.cpp
	$(GXX) $(OPTIMIZATION_FLAGS) -c -o wire_delay_model.o wire_delay_model.cpp

clean:
	rm -rf *.o TimingAnalysis
