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
 
the initial assignemnt specifies the set of data that a node has at the beginning of computation. by default, ExtMem has all input variables and the other components have nothing. after a line ".assign", a name of component and a set of data are written per line.
```
.assign
_extmem a b c
mem d e
one a b c d
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
```

a temporary component cannot keep data more than one cycle. this is mainly used to express a buffer, a Mem just relaying data. names of such components are listed in a line starting with ".temp".
```
.temp nexT_mem another
```

## commandline option
the list of commandline options will be shown with commandline option "-h":
```
usage : dfgmap <options>
        -h       : show this usage
        -o       : toggle generating output image files [default = 0]
        -e <str> : the name of environment file [default = "e.txt"]
        -f <str> : the name of formula file [default = "f.txt"]
        -g <str> : the name of option file [default = "g.txt"]
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
generate images to visualize the result. the files are named "_image(cycle).png" where (cycle) is an integer starting from 0.
 
this uses graphviz to generate images. please make sure that "dot" runs graphviz.

### -e, -f, -g
specify the names of input files: -e for array processor, -f for data-flow, and -g for synthesis option. it is allowed that the synthesis option file doesn't exist. 

### -n
specifies the number of cycles. 0 (default value) just verifies the input files.

### -r, -u
specify the number of regiters and the nubmer of processors in each PE. all PEs are uniform, where each PE has the same number of registers and the same number of processors.

### -t
specifies the number of contexts, which is assumed to be equal to the initiation interval in piepelining. although the result is not shown in the form of contexts, the mapping is done under modulo constraints and it can be wrapped into the number of contexts without violating the resource limitation.

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
sets time limit on solving each problem. this internally uses a linux command "timeout".

### -d
switches encodings of At most one constraint, where only one variable in a set of variables can be one.

### -s
switches SAT solvers. make sure that a solver name runs the solver.

### -v
switches verbosing levels.

## benchmark
we prepared three experiments. in the directory "benchmark", codes to generate problems are located. please compile one of them and run the generated executable, then "e.txt" and "f.txt" (and "g.txt" in some cases) are generated.

