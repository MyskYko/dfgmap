gen:op.o sat.o main.o 
	g++ $^ ~/glucose-syrup/simp/lib.a -o gen
op.o:op.cpp
	g++ -g -c op.cpp
main.o:main.cpp
	g++ -g -I ~/glucose-syrup -c main.cpp
sat.o:sat.cpp
	g++ -g -I ~/glucose-syrup -c sat.cpp
clean:
	rm -f *.o gen
