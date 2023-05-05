#ifndef GRAPH_H
#define GRAPH_H

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <queue>
#include <omp.h>
using namespace std;

// Edge contexts
#define INGOING 1
#define OUTGOING 0
#define BIDIRECTIONAL 2

// Consistency models
#define FULL 0
#define EDGE 1
#define VERTEX 2

// Work schedule
#define SIMULTANEOUS 0
#define FIFO 1
#define PARTITIONED_SIMULTANEOUS 2
#define PARTITIONED 3

typedef int context; // gather/scatter context
typedef int conistency_model;
typedef int schedule_type;
typedef int lock_t;

// Lock functions
void lockg(lock_t x);
void unlockg(lock_t x);
bool testg(lock_t x);

//######### STRUCTS ##########//

// Queue for storing work
struct localQ{ 
  omp_lock_t qlock;
  queue<int> q;
};

// Edge struct
struct tEdge {
  int u;
  int v;
  void *data;
  lock_t elock;
};

// Vertex struct
struct tVertex {
  void *data;
  int vid; // vertex id
  int processor_id = -1; // mapping of threads to cores
  std::vector<int> boundary_edges_outgoing;
  std::vector<int> boundary_edges_ingoing;
  lock_t vlock;
};

// Graph - abstract class
class tGraph { 
public:
  int num_nodes;
  std::vector<tVertex> vertices;
  std::vector<tEdge> edges;
  std::vector<int> *in_edges;  // Stores index of each in edge
  std::vector<int> *out_edges; // Stores index of each out edge
  std::vector<int> *partitions; // for simultaneous + partitioned scheduling
  void *global_data; // read-only

  // Gather / Scatter Context
  context gather_context = INGOING;
  context scatter_context = OUTGOING;
  conistency_model consist = VERTEX;
  schedule_type schedule = SIMULTANEOUS;

  // Debugging (implement optionally)
  virtual void print_vertex(tVertex &V) = 0;
  virtual void print_edge(tEdge &E) = 0;

  // User-defined Functions
  virtual void gather(void *&accum, tVertex v_n,tEdge &v_e) = 0; // Pure virtual
  virtual bool apply(void *&accum,tVertex v_n) = 0; // True if value changed, false otherwise
  virtual void scatter(void *&new_data, tVertex &v_n, tEdge &v_e) = 0;

};

//######### CLASS FUNCTIONS ##########//

// Populates a tGraph
void populateGraph(tGraph &G, std::vector<void *> &vertex_data,
                   std::vector<int *> &edges, std::vector<void *> &edge_data);

tGraph *solve(tGraph &G);

//Dynamic scheduler
void signal(tVertex V);
void signal_all(tGraph &G);
void signal_by_id(int vid);

//partitioned scheduler
void signal_partitioned(tVertex V);
void signal_all_partitioned(tGraph &G);
void signal_id_partitioned(tGraph &G, int vid);

// Queue debugging
void print_queues();

#endif
