gen:op.o gen.o blif.o main.o 
	g++ -g $^ -o gen
op.o:op.cpp
	g++ -g -c op.cpp
main.o:main.cpp
	g++ -g -c main.cpp
gen.o:gen.cpp
	g++ -g -c gen.cpp
blif.o:blif.cpp
	g++ -g -c blif.cpp
clean:
	rm -f *.o gen
