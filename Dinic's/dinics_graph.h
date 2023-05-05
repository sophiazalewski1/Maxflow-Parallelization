#ifndef DGRAPH_H
#define DGRAPH_H
#include <cstdlib>
#include <vector>

class Edge {
public:
  int v;
  int rev;
  int flow;
  int capacity;
};

class Graph {
public:
  int num_nodes;
  int *levels;
  std::vector<Edge> *edges;

  // Graph copy_graph(Graph &G){
  //   int n = G.num_nodes;
  //   std::vector<Edge> graphAdjCopy[n];
  //   Graph G_copy;
  //   G_copy.num_nodes = n;
  //   G_copy.levels = (int *)malloc(n * sizeof(int));
  //   G_copy.edges = graphAdjCopy;
  //   for (int i = 0; i < G_copy.num_nodes; i++) {
  //     G_copy.edges[i] = G.edges[i]; // deep copy vectors
  //   }
  //   return G;
  // }
};

#endif