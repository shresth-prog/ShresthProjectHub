

main: main.c base.c tools.c sensor_node.c
	mpicc -lpthread -o Assignment2_exe main.c base.c tools.c sensor_node.c -lm
	
run:
	mpirun -oversubscribe -np 21 Assignment2_exe 4 5
	
clean:
	rm -f Assignment2_exe
