// Ford-Fulkerson algorith in C++
// adapted from https://www.programiz.com/dsa/ford-fulkerson-algorithm

#include "../timing.h"
#include <atomic>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits.h>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace std;

// lock free stack implementation from lecture notes
struct Node {
  Node *next;
  int val;
};

struct Stack {
  std::atomic<Node *> top;
  std::atomic<int> x;
  int pop_count;
};

bool stack_empty(Stack *s) { return s->top == NULL; }

void new_stack(Stack *s) {
  s->top = NULL;
  s->pop_count = 0;
}

// example CAS from
// https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange
void push(Stack *s, int val) {
  Node *n = (Node *)malloc(sizeof(Node));
  n->val = val;
  n->next = s->top.load(std::memory_order_relaxed);
  while (!s->top.compare_exchange_weak(n->next, n, std::memory_order_release,
                                       std::memory_order_relaxed))
    ;
}

int pop(Stack *s) {
  Node *top = s->top.load(std::memory_order_relaxed);
  int val = top->val;
  s->top.store(top->next);
  free(top);
  return val;
}

// BFS with lock free stack
bool bfsParLockFree(int n, std::vector<int> &rGraph, int s, int t,
                    int parent[]) {
  bool visited[n];
  memset(visited, 0, sizeof(visited));
  // printf("calling bfs parallel\n");

  // create stack
  Stack *fs = (Stack *)malloc(sizeof(Stack));
  new_stack(fs);

  push(fs, s);
  visited[s] = true;
  parent[s] = -1;

  while (!stack_empty(fs)) {
    int u = pop(fs);

#pragma omp parallel for schedule(static, 256)
    for (int v = 0; v < n; v++) {
      if (visited[v] == false && rGraph[u * n + v] > 0) {
        push(fs, v);
        parent[v] = u;
        visited[v] = true;
      }
    }
  }
  free(fs);
  return (visited[t] == true);
}

// Using BFS
bool bfsParCritical(int n, std::vector<int> &rGraph, int s, int t,
                    int parent[]) {
  bool visited[n];
  memset(visited, 0, sizeof(visited));
  // printf("calling bfs parallel\n");

  queue<int> q;
  q.push(s);
  visited[s] = true;
  parent[s] = -1;

  while (!q.empty()) {
    int u = q.front();
    q.pop();

#pragma omp parallel for schedule(static, 256)
    for (int v = 0; v < n; v++) {
      if (visited[v] == false && rGraph[u * n + v] > 0) {
#pragma omp critical
        q.push(v);
        parent[v] = u;
        visited[v] = true;
      }
    }
  }
  return (visited[t] == true);
}

int find_first(int n, int *next_list) {
  for (int i = 0; i < n; i++) {
    if (next_list[i] == 1) {
      return i;
    }
  }
  return -1;
}

void print_list(int n, int *list) {
  printf("[");
  for (int i = 0; i < n; i++) {
    if (list[i] == 0) {
      printf("X,");
    } else {
      printf("%d,", list[i]);
    }
  }
  printf("]\n");
}

// Using BFS as a searching algorithm
bool bfsParList(int n, std::vector<int> &rGraph, int s, int t, int parent[]) {
  bool visited[n];
  memset(visited, 0, sizeof(visited));

  int next_list[n] = {0};
  next_list[s] = 1;
  visited[s] = true;
  parent[s] = -1;

  int first_elt = s;
  while (first_elt != -1) { // While there are still elements in the list
    int u = first_elt;      // q.front();
    next_list[u] = 0;

#pragma omp parallel for schedule(static, 256)
    for (int v = 0; v < n; v++) {
      if (visited[v] == false && rGraph[u * n + v] > 0) {
        next_list[v] = 1;
        parent[v] = u;
        visited[v] = true;
      }
    }
    first_elt = find_first(n, next_list);
  }
  return (visited[t] == true);
}

// Applying fordfulkerson algorithm
int fordFulkersonPar(int n, std::vector<int> &graph, int s, int t,
                     bool (*bfsPar)(int, std::vector<int> &, int, int, int[])) {
  int u, v;

  std::vector<int> rGraph(n * n); // Create the residual graph
  for (u = 0; u < n; u++)
    for (v = 0; v < n; v++)
      rGraph.at(u * n + v) = graph.at(u * n + v);

  int parent[n];
  int max_flow = 0;

  // Updating the residual values of edges
  while (bfsPar(n, rGraph, s, t, parent)) {
    int path_flow = INT_MAX;
    for (v = t; v != s; v = parent[v]) {
      u = parent[v];
      path_flow = min(path_flow, rGraph[u * n + v]);
    }

    // #pragma omp parallel for schedule(static, 1)
    // invalid increment expression
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
