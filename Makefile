gen:op.o gen.o main.o 
	g++ -g $^ -o gen
op.o:op.cpp
	g++ -g -c op.cpp
main.o:main.cpp
	g++ -g -c main.cpp
gen.o:gen.cpp
	g++ -g -c gen.cpp
clean:
	rm -f *.o gen
