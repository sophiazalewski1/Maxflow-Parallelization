#include "../timing.h"
#include "dinics_graph.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits.h>
#include <omp.h>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace std;
// Modified from C# Dinic's impoementation found here:
// https://www.geeksforgeeks.org/dinics-algorithm-maximum-flow/#

void print_frontier(std::vector<int> fs) {
  printf("[");
  for (int i = 0; i < fs.size(); i++) {
    printf("%d,", fs[i]);
  }
  printf("]\n");
}

bool BFS_par(Graph &G, int source, int sink) {
// Initalize all distances to be -1
// printf("calling bfs parallel\n");
// have 2 containers of vertices Frontier set
// and Next Frontier
// printf("calling bfs parallel\n");
// Initalize all distances to be -1
#pragma omp parallel for schedule(static, 256)
  for (int i = 0; i < G.num_nodes; i++) {
    G.levels[i] = -1;
  }
  G.levels[source] = 0;
  std::vector<int> FS, NS;
  FS.push_back(source);
  // print_frontier(FS);
  // Loop through fronteir until empty
  while (FS.size() != 0) {
#pragma omp parallel for schedule(static, 256)
    for (int i = 0; i < FS.size(); i++) {
      int u = FS[i];
      for (auto &edge : G.edges[u]) {
        // printf("id %d,level %d,flow %d,cap %d\n", edge.v, G.levels[edge.v],
        // edge.flow, edge.capacity); Neighbor we have not yet seen before that
        // has more flow
        if (G.levels[edge.v] == -1 && edge.flow < edge.capacity) {
#pragma omp critical
          {
            NS.push_back(edge.v); // Add to new frontier
          }
          G.levels[edge.v] = G.levels[u] + 1;
        }
      } // Update distance
    }
    FS = NS;
    // print_frontier(FS);
    NS.resize(0);
  }
  // Return bool indicating if more flow can be sent
  return (G.levels[sink] >= 0);
}

void print_levels(Graph &G){
  #pragma omp critical
  {
  printf("printing level graph...\n");
  for(int i = 0; i< G.num_nodes; i++){
    printf("%d,", G.levels[i]);
  }
  printf("\n");
  }
}

// BFS with thread local variables for the frontier + neighboring set
bool BFS_par_local(Graph &G, int source, int sink) {
#pragma omp parallel for schedule(static, 256)
  for (int i = 0; i < G.num_nodes; i++) {
    G.levels[i] = -1;
  }
  
  G.levels[source] = 0;

  // all neighbors of s added to the frontier
  // int total_FS = G.edges[source].size();
  std::vector<int> first_FS;
  for (auto &edge : G.edges[source]) {
    if(edge.flow < edge.capacity){
      first_FS.push_back(edge.v);
      G.levels[edge.v] = G.levels[source] + 1;
    }
  }
int total_FS = first_FS.size();

#pragma omp parallel num_threads(128)
{
    int tid = omp_get_thread_num();
    int nproc = omp_get_num_threads();
    // printf("hello from %d\n", tid);
    std::vector<int> FS, NS;
    // each thread gets a portion of the initial FS set
    int rem = total_FS % nproc;
    int extra = (tid < rem) ? 1 : 0;
    int num_elements = (total_FS / nproc) + extra;

    int start = (tid < rem) ? tid * num_elements : tid * num_elements + rem;
    int end = start + num_elements;

    FS.insert(FS.begin(), first_FS.begin() + start,
              first_FS.begin() + end); // + end*sizeof(int));
    while (total_FS > 0) {
      for (int i = 0; i < FS.size(); i++) {
        int u = FS[i];
        for (auto &edge : G.edges[u]) {
          int v = edge.v;
          // that has more flow
          if (G.levels[edge.v] == -1 && edge.flow < edge.capacity) {
            NS.push_back(edge.v); // Add to new frontier
            G.levels[edge.v] = G.levels[u] + 1;
          }
        } // Update distance
      }
      FS = NS;
      NS.resize(0);
      #pragma omp barrier
      if(tid == 0){
        total_FS = 0;
      }
      #pragma omp barrier
      #pragma omp atomic
      total_FS += FS.size();
      #pragma omp barrier
    }
  }
 
  return (G.levels[sink] >= 0);
}

int sendFlow_par(Graph &G, int u, int flow, int sink, int *start) {
  if (u == sink) {
    return flow;
  }
  for (; start[u] < G.edges[u].size(); start[u]++) {
    Edge &edge = G.edges[u][start[u]];
    if (G.levels[edge.v] == G.levels[u] + 1 && edge.flow < edge.capacity) {
      int curr_flow = std::min(flow, edge.capacity - edge.flow);
      int temp_flow = sendFlow_par(G, edge.v, curr_flow, sink, start);
      if (temp_flow > 0) {
        edge.flow += temp_flow;
        G.edges[edge.v][edge.rev].flow -= temp_flow;
        return temp_flow;
      }
    }
  }
  return 0;
}

int dinics_par(Graph &G, int source, int sink) {
  if (source == sink) {
    return 0;
  }
  int total = 0;
  Timer BFS_Timer, flowTimer;
  double BFS_start = BFS_Timer.elapsed();
  double flowStart;
  double flowTotal = 0;
  double BFS_total = 0;
  while (BFS_par_local(G, source, sink)) {
    BFS_total += BFS_Timer.elapsed() - BFS_start;
    int *start = new int[G.num_nodes + 1]{0};
    // while flow is not zero in graph from S to D
    flowStart = flowTimer.elapsed();
    while (int flow = sendFlow_par(G, source, INT_MAX, sink, start)) {
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