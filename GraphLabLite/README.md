# Graph lab Lite

GraphLabLite is a compact framework we developed for solving graph vertex problems based off CMU's GraphLab implementation. In a GLL Vertex Problem, 
each vertex and edge stores some form of "data" defined by the user. The user must also provide a "gather" function, which describes how a given vertex will
gather data / information from each of its neighbors, an "apply" function, where each vertex updates their data based on the gathered information, and 
a "scatter" function, where each vertex scatters their new information to neighboring nodes. In a GLL Vertex Problem, the program will iteratively call the gather,
apply, and scatter functions on each vertex until the graph converges.

## Defining a Graph Problem

In order to set your problem up as a GraphLabLite Vertex Problem, first copy and paste the "template.h" file inside this folder to
use this as a guide. You can replace all instances of "myVertexProblem" with the name of your specific implementation (ex. PageRank, PushRelabel).

#### 1. Define Data Types
The first fields you must specify when filling out this template are the fields for "vertex_data" and "edge_data". 
The vertex_data struct holds the data that your problem requires to be associated with each vertex (ex. height, excess flow), and the
edge_data struct holds the data that your problem requires to be associated with each edge (ex. flow, capacity). 

```cpp
struct vertex_data{

    // Fill in with data fields 
    // stored at each vertex

    // ex. int height;
    //     int excess_flow;
};
struct edge_data{

    // Fill in with data fields 
    // stored at each edge

    // ex. int flow;
    //     int capacity;
};    
```
#### 2. Set Scatter / Gather Contexts
GraphLabLite allows users to specify which edges a problem gathers and scatters from. "INGOING" means that each vertex
will gather or scatter to/from edges <b>leading to</b> that vertex. "OUTGOING" means the the vertex with gather/scatter to/from
edges <b>going out from</b> that vertex. And "BIDRECTIONAL" means a vertex with gather/scatter to/from <b>both ingoing and outgoing 
edges</b> (all neighbors regardless of edge direction). 
```cpp
myVertexProblem(){
        tGraph::gather_context = INGOING;  // Set these options
        tGraph::scatter_context = OUTGOING;  
        tGraph::gather_context = OUTGOING; 
        tGraph::scatter_context = BIDIRECTIONAL; 
        tGraph::consist = FULL;
        tGraph::schedule = PARTITIONED;
};
```
#### 3. Set Initial Graph / Problem State
```graph.h``` contains a function ```PopulateGraph``` that takes in a vector of ```void *```s containing the data stored at each vertex,
a vector of ```int *```s containing each edge as (u,v) pairs, and another vector of ```void *```s containg the data stored at each edge. 
If your problem requires any preprocessing, you can set this up in the ```initializeGraph``` function, and specify the input fields
you would like to pass in.
```cpp
void initializeGraph(std::vector<vertex_data *> v_data, 
                     std::vector<int *> edges,  // [u,v] pairs
                     std::vector<edge_data *> e_data){ // Example input fields

      // Optionally add preprocessing here

      populateGraph(*this, (void *) v_data, edges, (void *) e_data);
}
```
#### 4. Define Gather, Apply, and Scatter Functions
Fill in the Gather, Apply, and Scatter functions in the template header file. Make sure to not remove the ```check_and_init(accum)``` 
lines at the beginning of the apply and scatter functions, as this makes sure you are not dereferencing a null accumulator value. 

You will also need to define a "type" for the value you wish to accumulate in the apply function, as well as a "base" value for this type.
For example, if you are accumulating an "int" that is summing a particular data field on all neighboring edges, a reasonable base value
would be 0. 

Finally, in the apply function, you will need to return weather or not the value of data changed.

```cpp
float gather(tVertex v_n, tEdge v_e) override{

    // Your implementation here
};                        

void check_and_init(void* &accum){
    if (accum == nullptr){  

        // Your base value for accumulation here
        // ex. int* init_val = new int{0} 
        accum = static_cast<void*>(init_val);}};


bool apply(void* &accum, void* &data) override { 
    check_and_init(accum); 

    // Your implementation here
    // * do not remove the "check_and_init" line above
    // * Make sure to return weather or not the value for data changed

    return true; 
}; 

void scatter(void* &new_data, tVertex &v_n, tEdge &v_e) override{
    check_and_init(accum); // do not remove this line        

    // Your implementation here
    // * do not remove the "check_and_init" line above 
};  
```

#### 5. Optionally Add Postprocessing 
The graph.h function ```solve``` runs iterations of gather, apply, scatter on each vertex until the graph converges. If you would like to add
posprocessing before the returning the final answer to your problem (ex. adding the flow of edges leading to the sink on the congered graph
to return the resulting max flow), you can do so in the ```solveMyProblem()``` function:

```cpp
void solveMyProblem(){
        solve(*this); 
        // Optionally add postprocessing
};
```
## Calling the Problem

To solve an instance of your vertex problem in your main method, first declare an instance of your problem, then call populate the graph on whatever data 
this instance of your problem contains, and then call your problem's solve function. 

```cpp
std::vector<int *> edges = {new int[2]{0,1},
                            new int[2]{0,2}, 
                            new int[2]{2,3}};

myVertexProblem P; 
P.initializeGraph(edges, other params);       
P.solveMyProblem(); // Runs GAS until convergence + your added postprocessing
```

Make sure that if you are running a solver with multiple threads, you edit the value for "NUM WORKERS" defined in ```graph.cpp``` to be equal to the number of threads you wish to run the code on:

```cpp
#define NUM_WORKERS 4 // IMPORTANT! Set this equal to 
                      // the number of threads being run
```

## Debugging

If you choose to write functions to print vertex and edge data, you can call the ```printGraph``` function defined in graph.cpp which
relies on these functions to help debug your implementation.
```cpp
void print_vertex(tVertex &V) override{} // Optionally implement these
void print_edge(tEdge &E) override{} 
```
printGraph uses these functions to output the data for each vertex, and show all of the ingoing (<-) and outoing (->) edges
from each vertex, along with the data stored at those edges. Some example output from the ```printGraph``` function is below.

```cpp
Vertex 0 - Excess Flow: 0, Height: 3, Pushing -1 to -1
     -> 1 - Flow: 5, Capacity: 5
     -> 2 - Flow: 3, Capacity: 3
     <- 1 - Flow: 0, Capacity: 0
     <- 2 - Flow: 0, Capacity: 0
Vertex 1 - Excess Flow: 5, Height: 0, Pushing -1 to -1
     -> 0 - Flow: 0, Capacity: 0
     <- 0 - Flow: 5, Capacity: 5
Vertex 2 - Excess Flow: 3, Height: 0, Pushing -1 to -1
     -> 3 - Flow: 0, Capacity: 3
     -> 0 - Flow: 0, Capacity: 0
     <- 0 - Flow: 3, Capacity: 3
     <- 3 - Flow: 0, Capacity: 0
```
Some other common issues to check for when facining compiling errors or segfaults:

* If you rename your vertex problem, make sure that the name of your class matches the name of the constructor (ex. ```class myVertexProblem : public tGraph``` matches the constructor ```myVertexProblem() { ... }```)
* Check that you have initalized your accumulator type for the apply function, and have not accidentally deleted the ```check_and_init``` lines at the beginning of the apply and scatter function templates.
* Check that you are returning weather or not the value of data was updated/changed in the apply function, instead of just leaving the ```return true``` statement placeholder in the template.
