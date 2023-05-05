#include "../timing.h"
#include "dinics_graph.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits.h>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace std;
// Modified from C# Dinic's impoementation found here:
// https://www.geeksforgeeks.org/dinics-algorithm-maximum-flow/#

bool BFS(Graph &G, int source, int sink) {
  // Initalize all distances to be -1
  for (int i = 0; i < G.num_nodes; i++) {
    G.levels[i] = -1;
  }
  G.levels[source] = 0;
  queue<int> q;
  q.push(source);

  // Loop through fronteir until empty
  while (!q.empty()) {
    int u = q.front();
    q.pop();
    for (auto &edge : G.edges[u]) {
      // Neighbor we have not yet seen before that has more flow
      if (G.levels[edge.v] == -1 && edge.flow < edge.capacity) {
        q.push(edge.v); // Add to new fronteir
        G.levels[edge.v] = G.levels[u] + 1;
      }
    } // Update distance
  }
  // Return bool indicating if more flow can be sent
  return (G.levels[sink] >= 0);
}

int sendFlow(Graph &G, int u, int flow, int sink, int *start) {
  if (u == sink) {
    return flow;
  }
  for (; start[u] < G.edges[u].size(); start[u]++) {
    Edge &edge = G.edges[u][start[u]];
    if (G.levels[edge.v] == G.levels[u] + 1 && edge.flow < edge.capacity) {
      int curr_flow = std::min(flow, edge.capacity - edge.flow);
      int temp_flow = sendFlow(G, edge.v, curr_flow, sink, start);
      if (temp_flow > 0) {
        edge.flow += temp_flow;
        G.edges[edge.v][edge.rev].flow -= temp_flow;
        return temp_flow;
      }
    }
  }
  return 0;
}

int dinics(Graph &G, int source, int sink) {
  if (source == sink) {
    return 0;
  }
  int total = 0;
  Timer BFS_Timer, flowTimer;
  double BFS_start = BFS_Timer.elapsed();
  double flowStart;
  double flowTotal = 0;
  double BFS_total = 0;
  while (BFS(G, source, sink)) {
    BFS_total += BFS_Timer.elapsed() - BFS_start;
    int *start = new int[G.num_nodes + 1]{0};
    // while flow is not zero in graph from S to D
    flowStart = flowTimer.elapsed();
    while (int flow = sendFlow(G, source, INT_MAX, sink, start)) {
      flowTotal += flowTimer.elapsed() - flowStart;
      // Add path flow to overall flow
      total += flow;
      flowStart = flowTimer.elapsed();
    }
    // Remove allocated array
    delete[] start;
    BFS_start = BFS_Timer.elapsed();
  }
  fprintf(stdout, "BFS Time: %.7lfs, Push Flow Time %.7lfs \n", BFS_total,
          flowTotal);
  return total;
}