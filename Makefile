gen:op.o sat.o gen.o 
	g++ $^ ~/glucose-syrup/simp/lib.a -o gen
op.o:op.cpp
	g++ -g -c op.cpp
gen.o:gen.cpp
	g++ -g -I ~/glucose-syrup -c gen.cpp
sat.o:sat.cpp
	g++ -g -I ~/glucose-syrup -c sat.cpp
