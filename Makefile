gen:op.o gen.o main.o 
	g++ $^ ~/glucose-syrup/simp/lib.a -o gen
op.o:op.cpp
	g++ -g -c op.cpp
main.o:main.cpp
	g++ -g -I ~/glucose-syrup -c main.cpp
gen.o:gen.cpp
	g++ -g -I ~/glucose-syrup -c gen.cpp
clean:
	rm -f *.o gen
