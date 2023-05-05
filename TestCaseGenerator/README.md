# Maxflow Graph Generator

## How it Works

We wrote a python script to generate random flow graphs of five different "types" - 
dense layer graph, sparse layer graph, 1-line graph, exclusive paths graph, and unblanced graph.

The user can specify the number of nodes in the graph they wish to generate,
as well as the number of "layers" (the graphs below all contain 3 layers)
and the type of graph from the 5 configurations.

Below are text-art visualizations of each of the graph types this test case generator can produce.
```  
     Dense           Sparse         1-Line     Exclusive      Unbalanced
  Layer Graph      Layer Graph      Graph     Paths Graph        Graph
       s                s             s            s               s
     / | \            / | \           |          / | \            / \ 
    1  2  3          1  2  3          1         1  2  3          1   2
    |\/|\/|           \    |          |         |  |  |         /   /|\ 
    4  5  6          4  5  6          2         4  5  6         3  4 5 6  
    |\/|\/|            / \            |         |  |  |          \ | //  
    7  8  9          7  8  9          3         7  8  9            t
     \ | /            \ | /           |          \ | /
       t                t             t            t                
```
The output of the script is a DIMACS text represenation of the generated graph that is written
to a specified text file. In the DIMACS max flow format, text lines are of the following types:

- <b>c ... </b>= comment line (can be ignored)
- <b>p max n m</b> = problem statement line, indicates graph has n vertices an m edges
- <b>n 0 s</b> = node descriptor line, indicates 0 is the source node or (n-1) is the sink node
- <b>a u v c</b> = arc descriptor line, indicates an edge between u and v with capacity c

The text below is the DIMACs output of our testcase generator script after calling ``` makeLineGraph(10, file_name) ```.
To learn about DIMACS max flow graph representations, see https://lpsolve.sourceforge.net/5.5/DIMACS_maxf.htm. 

```
c Line Graph 
p max 10 9
n 0 s 
n 9 t
a 0 1 81
a 1 2 76
a 2 3 72
a 3 4 68
a 4 5 67
a 5 6 89
a 6 7 92
a 7 8 51
a 8 9 96
```
## Use
### Using makeTestCases.py to generate DIMACS max flow graphs

To generate the graphs, call any of the write graph functions and specifify the number of verties, number of levels, and the name of a text file that the output will be written to. The script will create the textfile in the same directory, and write the graph output accordingly.

Example use:
```python
writeLineGraph(20, 'myFlowGraphs.txt') # Make a line graph
writeExclusivePathGraph(20, 5, 'myFlowGraphs.txt') # Make an exclusive-paths graph
writeSparseLayerGraph(20, 5, 'myFlowGraphs.txt') # Make a sparse layer graph
writeDenseLayerGraph(20, 5, 'myFlowGraphs.txt') # Make a dense layer graph
```
### Reading DIMACS graphs to C++ (or any other language)
To use the generated text graphs, we have provided example parsing code in c++ which can be easily translated over to any
other language. To use, reproduce the code we use in the main.cpp file and parse
your corresponding DIMACs file.
