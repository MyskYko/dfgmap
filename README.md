# dfgmap
## requirement
 - install the sat solver you want to use: minisat, glucose, and lingeling are compatible currently.
 - install graphviz if you want to generate images of results
 
## get started
 - make, then an execuable "./dfgmap" will be generated
 - run the executable without commandline option, this will check the input files:
   - "e.txt" ... array processor
   - "f.txt" ... data-flow
   - "g.txt" (optional) ... synthesis option
 - run the executable with commandline option "-n 7", this will solve the mapping problem with 7 cycles, which is originally unsatisfiable
 - run the executable with commandline option "-n 8", this will solve the mapping problem with 8 cycles, which is originally satisfiable
 - run the executable with commandline option "-n 8 -v", this will display the mapping result
 - run the executable with commandline option "-n 8 -o", this will generate images, which visualize the result, named "_image(cycle).png" where (cycle) is an integer starting from 0
 
## input files
### array processor
the file name is "e.txt", which can be changed by commandline option.

an array processor consists of PEs and Mems.
PE is a processing element, which has processors and registers.
Mem is a memory, which keeps values.
also, there is Extmem (just one) as an external device to provide input values and collect output values.its behaviour is the same as Mem.

each element is distingished by name. a name must not start from "_". the names must be listed in the file. (ExtMem is implicitly added with name "_extmem".) PEs are listed after ".pe" in a line, and Mems are listed in a line after ".mem" as follows:
``` 
.pe pe1 pe2 pe3 pe4 a b c
.mem mem next_mem one anotherone
```

the elements are connected by weighted directed edges.
a weight is the maximum number of data the edge can communicate in one cycle.
an edge can have multiple starting points and ending points, which realizes bus communcation.

the edges must be listed after ".com", one for each line. starting points are written left side and ending points right side of "->". at the end of line, the weight of the edge is written separated by ":". if the number of data communicated is not limited, it can be omitted.
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
.i a b c d
.o e
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

under construction

a processor must be able to process one operator per cycle. in the case one processor can process more than one operator at the same time, MAC opeartion for example, such a multi-operator operation should be listed after ".m"

### synthesis option

## options
