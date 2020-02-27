gen:op.o cnf.o main.o 
	g++ -g $^ -o gen
op.o:op.cpp
	g++ -g -c op.cpp
main.o:main.cpp
	g++ -g -c main.cpp
cnf.o:cnf.cpp
	g++ -g -c cnf.cpp
clean:
	rm -f *.o gen
