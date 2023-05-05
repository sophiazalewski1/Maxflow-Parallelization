## Maxflow Parallelization

In this project, we compare the advantages and disadvantages of Domain Specific Languages (DSLs) for max-flow problems. We:

- Design our own compact DSL for graph vertex problems (GraphLabLite)
- Implement three different algorithms to solve max flow problems (Ford Fulkerson, Dinic’s, Push-Relabel)

And compare the difference between the OpenMP-parallelized algorithms and DSL-parallelized algorithm

### Repo Layout

- ```Test Case Generator``` - contains a python script to generate Max-flow graph problems. See [here](TestCaseGenerator/README.md).
- ```GraphLabLite ``` - contains our implementation of the GraphLabLite DSL. See [here](GraphLabLite/README.md).
- ```Dinic's``` - contains sequential and parallel implementations of Dinic's in OpenMP
- ```Ford Fulkerson's``` - contains sequential and parallel implementations of Ford Fulkerson's in OpenMP
- ```PushRelabel``` - contains Push Relabel implementation in GraphLabLite
- ```PageRank``` - contains PageRank implementation in GraphLabLite
