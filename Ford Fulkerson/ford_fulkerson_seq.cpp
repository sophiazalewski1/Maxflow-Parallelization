// Ford-Fulkerson algorith in C++
// adapted from https://www.programiz.com/dsa/ford-fulkerson-algorithm

#include <cmath>
#include <fstream>
#include <iostream>
#include <limits.h>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace std;

// Using BFS as a searching algorithm
bool bfs(int n, std::vector<int> &rGraph, int s, int t, int parent[]) {
  // printf("calling bfs sequentially\n");
  bool visited[n];
  memset(visited, 0, sizeof(visited));

  queue<int> q;
  q.push(s);
  visited[s] = true;
  parent[s] = -1;

  while (!q.empty()) {
    int u = q.front();
    q.pop();

    for (int v = 0; v < n; v++) {
      if (visited[v] == false && rGraph[u * n + v] > 0) {
        q.push(v);
        parent[v] = u;
        visited[v] = true;
      }
    }
  }
  return (visited[t] == true);
}

// Applying fordfulkerson algorithm
int fordFulkerson(int n, std::vector<int> &graph, int s, int t) {
  int u, v;

  std::vector<int> rGraph(n * n); // Create the residual graph
  for (u = 0; u < n; u++)
    for (v = 0; v < n; v++)
      rGraph.at(u * n + v) = graph.at(u * n + v);

  int parent[n];
  int max_flow = 0;

  // Updating the residual values of edges
  while (bfs(n, rGraph, s, t, parent)) {
    int path_flow = INT_MAX;
    for (v = t; v != s; v = parent[v]) {
      u = parent[v];
      path_flow = min(path_flow, rGraph[u * n + v]);
    }

    for (v = t; v != s; v = parent[v]) {
      u = parent[v];
      rGraph[u * n + v] -= path_flow;
      rGraph[v * n + u] += path_flow;
    }

    // Adding the path flows
    max_flow += path_flow;
  }
  return max_flow;
}