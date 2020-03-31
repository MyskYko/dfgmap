# dfgmap
## requirement
 - install a SAT solver you want to use: minisat, glucose, and lingeling are compatible currently
 - install an ILP solver (only compatible with CPLEX) if you want to use
 - install graphviz if you want to generate images of results
 
## get started
 - make, then an execuable "./dfgmap" will be generated
 - run the executable without commandline option, this will check the input files:
   - "e.txt" ... array processor
   - "f.txt" ... data-flow
   - "g.txt" (optional) ... synthesis option
 - run the executable with commandline option "-n 7", this will solve the mapping problem with 7 cycles, which is originally unsatisfiable
 - run the executable with commandline option "-n 8", this will solve the mapping problem with 8 cycles, which is originally satisfiable
 - run the executable with commandline option "-n 8 -v 1", this will display the mapping result
 - run the executable with commandline option "-n 8 -o", this will generate images, which visualize the result, named "_image(cycle).png" where (cycle) is an integer starting from 0
 
## input files
### array processor
the file name is "e.txt", which can be changed by commandline option.

an array processor is composed of PEs, Mems, and ExtMem.
a PE is a processing element, which has processors and registers.
a Mem is a memory, which keeps values.
ExtMem is an external device to provide input values and collect output values. its behaviour is the same as a Mem's.

each component is distingished by name. the names must be listed in the file. (ExtMem is implicitly added with a name "_extmem".) PEs are listed after ".pe" in a line, and Mems are listed in a line after ".mem" as follows:
``` 
.pe pe1 pe2 pe3 pe4 a b c
.mem mem next_mem one another
```

the components are connected by weighted directed edges (paths). a weight is the maximum number of data the path can communicate in one cycle. a path can have multiple starting points and ending points, which realizes bus communcation.

the paths must be listed after ".com", one for each line. starting points are written left side and ending points right side of "->". at the end of line, the weight of the path is written separated by ":". if the number of data communicated is not limited, it can be omitted.
```
.com
pe1 -> pe2 : 2
pe2 -> pe1 : 2
_extmem -> pe1 : 1
_extmem -> pe2 : 1
pe1 -> _extmem : 1
pe2 -> _extmem : 1
pe1 -> pe3
pe2 -> pe4
pe3 pe4 -> mem : 1
mem a b c -> mem a b c : 1
mem -> next_mem
next_mem -> _extmem
```

### data-flow
the file name is "f.txt", which can be changed by commandline option.

input variables and output variables must be named without duplication and listed in a line starting with ".i" and ".o" respectively.
(a name can conflict with a name of element in an array processor because the names of elements in an array processor are managed in a different container.) 
```
.i a b c d e
.o x y z
```

available operators must be listed after ".f" one per line. it must be followed by the number of operands. if the operator is commutative or associative, "c" or "a" should follow in order to utilize optimization methods. an associative operator must have two operands.
```
.f
+ 2 c a
- 2
* 2 a c
max3 3 c
max4 4 c
```

operations in a data-flow (nodes) must be listed from the next line of ".n". each line must start with a name of node, which cannot conflict with a name of input variable. when the result of operation corresponds to an output variable, the name of output variable is used. otherwise, a unique name must be given. an operator must be specified next, and operands must follow in order. multiple nodes can be expressed in the same line using polish notation. in the mapping phase, equivalent operations will be unified into one node.
```
.n
x + a b
i + a b
j max3 c d e
y * i j
z - * a b * c d
```

a processor must be able to process one operator per cycle. in the case that one processor can process more than one operators at the same time, MAC opeartion for example, such a multi-operator operation should be listed per line after a line ".m". the names of operands are arbitrary, where other than the order of operations will be ignored. even if the same name appears several times, the equivalency of the operands will not be ensured.
```
.m
+ * foo bar baz
- * a a a
```

### synthesis option
this file is optional one. the file name is "g.txt", which can be changed by commandline option.

currently, four options are prepared:
 - the initial assignment of data among components
 - the size of a Mem or ExtMem
 - the number of ports of a component
 - temporary component
 
the initial assignemnt specifies the set of data that a node has at the beginning of computation. by default, ExtMem has all input variables and the other components have nothing. after a line ".assign", a name of component and a set of data are written per line. if only a limited number of variables in a set can be assigned, surround the set with "{ K" and "}" where K is the limitation.
```
.assign
_extmem a b c
mem d e
one { 2 a b c d }
pe1 a
```

the size of a Mem or ExtMem sets a limit on the number of data a Mem or ExtMem keeps at the same time. it is unlimited by default. after a line ".memsize", a name of a Mem or Extmem and its size are written per line.
```
.memsize
_extmem 10
mem 5
next_mem 2
```

the number of ports of a component restricts the number of data the component receives/sends in a cycle. it is also restricted by the topology of paths, but sometimes this additional restriction is required, 2R1W memory for example. after a line ".port", a name of a component, the number of incoming data, and the number of outgoing data are written per line.
```
.port
_extmem 1 2
one 1 4
another 2 2
pe2 1 1
```

a temporary component cannot keep data more than one cycle. this is mainly used to express a buffer, a Mem just relaying data. names of such components are listed in a line starting with ".temp".
```
.temp next_mem another
```

## commandline option
the list of commandline options will be shown with commandline option "-h":
```
usage : dfgmap <options>
        -h       : show this usage
        -o       : toggle generating output image files [default = 0]
        -e <str> : the name of file for array processor [default = "e.txt"]
        -f <str> : the name of file for data-flow [default = "f.txt"]
        -g <str> : the name of file for synthesis option [default = "g.txt"]
        -n <int> : the number of cycles [default = 0]
        -r <int> : the number of registers in each PE (0 means no limit) [default = 2]
        -u <int> : the number of processors in each PE (0 means no limit) [default = 1]
        -t <int> : the number of contexts for pipeline (0 means no pipelining) [default = 0]
        -x       : toggle enabling external memory to store intermediate values [default = 0]
        -c       : toggle transforming dataflow [default = 0]
        -m       : toggle using given multi-operator operations [default = 1]
        -i       : toggle using ILP solver instead of SAT solver [default = 0]
        -y       : toggle post processing to remove redundancy [default = 1]
        -a       : toggle doing incremental synthesis [default = 0]
        -l <str> : the duration of timeout for each problem (0 means no time limit) [default = 1d]
        -d <int> : the type of at most one encoding [default = 3]
                        0 : naive
                        1 : commander
                        2 : binary
                        3 : bimander half
                        4 : bimander root
        -s <int> : SAT solver [default = 0]
                        0 : minisat
                        1 : glucose
                        2 : lingeling
                        3 : plingeling
        -v <int> : the level of verbosing information [default = 0]
                        0 : minimum
                        1 : results
                        2 : settings and results
                        3 : settings, solver outputs, and results

```

### -o
generate images to visualize the result. the files are named "_image(cycle).png" where (cycle) is an integer starting from 0. this uses graphviz to generate images. please make sure that "dot" runs graphviz.

### -e, -f, -g
specify the names of input files: -e for array processor, -f for data-flow, and -g for synthesis option. it is allowed that the synthesis option file doesn't exist. 

### -n
specifies the number of cycles. 0 (default value) just verifies the input files.

### -r, -u
specify the number of regiters and the number of processors in each PE. all PEs are uniform, where each PE has the same number of registers and the same number of processors. setting 0 imposes no limit.

### -t
specifies the number of contexts, which is assumed to be equal to the initiation interval in piepelining. although the result is not shown in the form of contexts, the mapping is done under modulo constraints and it can be wrapped into the number of contexts without violating the resource limitation. setting 0 works without the modulo constraint.

### -x
enables ExtMem to store intermediate values other than input/output variables, which is not allowed by default.

### -c
optimizes a data-flow under the associative law. the commutative law is also considered. these properties of operation should be specified in the data-flow file.

### -m
utilizes multi-operator operations. this is turned on by default.

### -i
switches the solver to the ILP solver (CPLEX).

### -y
removes redundancy in the result in a deterministic way. the necessary data are marked from the last cycle, where the output variables in ExtMem are marked, to the precedent cycles, where the data that cause the marked data in the next cycle are marked.

### -a
solves the mapping problem with incrementing the number of cycles until the problem becomes satisfiable. Note that the problem may be unsatisfiable even with an infinate number of cycles.

### -l
sets time limit on solving each problem. this internally uses a linux command "timeout". setting 0 runs the solver without time limit.

### -d
switches encodings of At most one constraint, where only one variable in a set of variables can be one.

### -s
switches SAT solvers. make sure that a solver name runs the solver.

### -v
switches verbosing levels. the level 0 shows the properties of the problem and the result with the elapsed time. the level 1 displays the detailed scheduling in the result. the level 2 shows the configurations of the problem read from the input files. the level 3 shows the solver's output to stdout.

## benchmark
three benchmarks are prepared. in the directory "benchmark", codes to generate problems are located. please compile one of them and run the generated executable with an interger parameter, then "e.txt" and "f.txt" (and "g.txt" in some cases) are generated.

### ring.cpp
generates a problem to map matrix-vector multiplication onto a ring-connected PEs. it requires one parameter, detnoted as N.

#### e.txt
the array processor generated with N=4 is shown below. N PEs are connected in one-way ring connection. Each PE has an input connection from and an output connection to ExtMem. every path is weighted by one.

<img src="https://user-images.githubusercontent.com/18373300/78001316-7eda7980-7370-11ea-8102-401240756160.png" width="200">

#### f.txt
the data-flow generated with N=4 is shown below. it calculates matrix-vector multiplication where the matrix size is N x N. the order of additions is from the first column to the last column. the operators in the data-flow are only + and *, but MAC operation, a multi-operator operation, is specified to be available in the mapping.

<img src="https://user-images.githubusercontent.com/18373300/78002362-1ab8b500-7372-11ea-8c7f-d11e87065a2e.png" width="200">

#### g.txt
this problem does not use synthesis option.

#### example
the problem generated with a parameter N=4 is the same as the problem located in the project root directory, which is unsatisfiable when the number of cycles is less than 8 and satisfiable when it is more than or equal to 8.

if the automatic transformation is turned on by a commandline option "-c", the minimum required number of cycles will become 7 with a change in the order of additions. run "../dfgmap -a -c" to see that the problem under the automatic transformation becomes satisfiable with 7 cycles.

### mmm.cpp
generates a problem to map (dense) matrix multiplicatoin onto a mesh array. it requires one parameter, detnoted as N. this problem utilizes the pipelining with the number of contexts 3.

#### e.txt
the array processor consists of N x N PEs as shown below. each PE at the left has an input connection from ExtMem. each PE at the bottom has an output connection to ExtMem. PEs are connected from left to right and from top to bottom. each of these paths is weighted by one. a Mem is located for each PE and works as a ROM to provide data to the PE.

<img src="https://user-images.githubusercontent.com/18373300/78004940-cf080a80-7375-11ea-8b92-bc281ee47460.png" width="200">

#### f.txt
the data-flow calculates matrix multiplication, A = W X, where the size of matrices, A, W, and X is N x N. the operations are performed in a typical order, from the first column to the last column of W. MAC operation is defined as well as + and *.

#### g.txt
in this problem, we assume that the elements of W are distributed among PEs before the computation. on the other hand, the pipelining is sought to be performed for different Xs with the same W. then, we need to exclude the elements of W from the modulo conatraints and a Mem without size limit and a path without weight are set for each PE. we define the initial assignment that the elements of X are supplied from ExtMem, while the elements of W are distributed among Mems.
```
.assign
_extmem X1_1 X1_2 X1_3 X2_1 X2_2 X2_3 X3_1 X3_2 X3_3
rom1_1 W1_1
rom2_1 W1_2
rom3_1 W1_3
rom1_2 W2_1
rom2_2 W2_2
rom3_2 W2_3
rom1_3 W3_1
rom2_3 W3_2
rom3_3 W3_3
```

#### example
to solve the mapping under the pipelining constraints, we need to give a commandline option "-t 3", where "-t" is followed by the number of contexts (the initiation interval). this problem is satisfiable with 9 cycles. run "../dfgmap -t 3 -a" to check it.

### sparse.cpp
generates the problem of "mmm/.cpp" with some elements in W fixed at zero. the multiplication using a zero element is skipped. this generator requires two parameters: the first one, N, is the size of matrix and PE array, and the other one, M, specifies the sparsity.

the second parameter is given as a decimal number but will be converted into a N*N-digit binary number. when N=3, M=123 will be 001111011. let i and j be integers. if (i*N + j)-th digit is 0, the elemnt at (i, j) of W is fixed at 0. otherwise, that element is regarded as an arbitrary number, where its multiplication is considered valid.

### e.txt
the same as "mmm.cpp".

### f.txt
the data-flow in "mmm.cpp" is modified to skip multiplications with zero elements.

### g.txt
the assignement is modified so that each Mem has a non-zero element of W in arbitrary order. the following example is for the problem with N=3 and M=123.
```
.assign
_extmem X1_1 X1_2 X1_3 X2_1 X2_2 X2_3 X3_1 X3_2 X3_3
rom1_1 { 1 W1_1 W1_2 W2_1 W2_2 W2_3 W3_1 }
rom2_1 { 1 W1_1 W1_2 W2_1 W2_2 W2_3 W3_1 }
rom3_1 { 1 W1_1 W1_2 W2_1 W2_2 W2_3 W3_1 }
rom1_2 { 1 W1_1 W1_2 W2_1 W2_2 W2_3 W3_1 }
rom2_2 { 1 W1_1 W1_2 W2_1 W2_2 W2_3 W3_1 }
rom3_2 { 1 W1_1 W1_2 W2_1 W2_2 W2_3 W3_1 }
rom1_3 { 1 W1_1 W1_2 W2_1 W2_2 W2_3 W3_1 }
rom2_3 { 1 W1_1 W1_2 W2_1 W2_2 W2_3 W3_1 }
rom3_3 { 1 W1_1 W1_2 W2_1 W2_2 W2_3 W3_1 }
```

#### example
when the generated is given parameters N=3 and M=123, the problem with the matrix below W will be generated.
```
1 1 0
1 1 1
1 0 0
```

the mapping with the automatic transformation under pipeling required 8 cycles, 1 cycle reduced compared to the mapping of dense matrix. run "../dfgmap -a -c -t 3" to check it. note that the mapping without the transformation also succeeds with only 8 cycles for N=3 and M=123, but the result may differ for another set of parameters.
