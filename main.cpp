#include "Dinic's/Dinics_par.h"
#include "Dinic's/Dinics_seq.h"
#include "Dinic's/dinics_graph.h" // defines t_graph
#include "Ford Fulkerson/ford_fulkerson_par.h"
#include "Ford Fulkerson/ford_fulkerson_seq.h"
#include "GraphLabLite/graph.h"
#include "timing.h"
#include <queue>
#include "PageRank/pagerank.h"
#include "PushRelabel/push_relabel.h"

using namespace std;

int main() {
  // Ignore this - used for our testing and analysis purposes

  //#############################################################//
  bool RUN_DINICS = 1; // 0 if running FF, 1 if running Dinic's
  int NUM_GRAPHS = RUN_DINICS ? 5 : 12;
  //#############################################################//

  std::vector<double> seq_times, par_times;
  std::vector<string> test_cases;

  //======================= READ GRAPHS FROM TEXT FILE========================//
  
  FILE *file = RUN_DINICS ? fopen("partition_large.txt", "r") : fopen("FFTests.txt", "r");
  printf("OPENED FILE\n");
  int n, m, s, t;
  char str[10];
  int numGraphs = 0;
  char tok[20] = "";
  printf("here\n");
  while (true) {
    printf("here\n");
    char tok[20] = "";
    char name[20];
    bool obtained_name = false;
    
    while (!obtained_name){  
      fscanf(file, "%s", tok);
      if(strcmp(tok, "c") == 0){ // Comment line
        fscanf(file, "%s",&name);
        test_cases.push_back(name);
        obtained_name = true;
      }
    }
    printf("here\n");
    // Read problem info (determine n and m)
    bool obtained_problem = false;
    while (!obtained_problem){  
      fscanf(file, "%s", tok);
      if(strcmp(tok, "p") == 0){ // Problem info line
        fscanf(file, "%s %d %d",tok,&n,&m);
        obtained_problem = true;
      }
    }
    // Allocate space for graphs
    std::vector<int> graphMat(0);
    std::vector<int*> edges;
    std::vector<int> edge_capacities;
    if(!RUN_DINICS) {
      graphMat.resize(n * n);
    }
    std::vector<Edge> graphAdj[n];  // Adjacency list for Dinic's
    Graph G;
    G.num_nodes = n;
    G.edges = graphAdj;
    if(RUN_DINICS){
      G.levels = (int *)malloc(n * sizeof(int));}

    // Read edges
    int obtained_edges = 0;
    while (obtained_edges < m){  
      fscanf(file, "%s", tok);
      if(strcmp(tok, "a") == 0){ // "a" token = add edge to graph
        int u,v,c;
        fscanf(file, "%d %d %d",&u,&v,&c);
        edges.push_back(new int[2]{u,v});
        edge_capacities.push_back(c);
        if(RUN_DINICS){
          Edge forward_edge;
          Edge backward_edge;
          forward_edge.v = v;
          forward_edge.flow = 0;
          forward_edge.capacity = c;
          forward_edge.rev = graphAdj[v].size();
          backward_edge.v = u;
          backward_edge.flow = 0;
          backward_edge.capacity = 0;
          backward_edge.rev = graphAdj[u].size();
          graphAdj[u].push_back(forward_edge);
          graphAdj[v].push_back(backward_edge);
        }
        else{
          graphMat[u*n+v] = c;
        }
        obtained_edges++;
      }
    }
    printf("READ GRAPH\n");
    //====================== TIME AND CHECK CORRECTNESS ======================//

    cout << "\n------------------------\n" << endl;
    cout << name << endl;
    fprintf(stdout,"Read graph with %d nodes and %d edges\n",n,m);

    //copy graph for second Dinic's run
    Graph G_copy;
    std::vector<Edge> graphAdjCopy[n];
    G_copy.num_nodes = n;
    G_copy.levels = (int *)malloc(n * sizeof(int));
    G_copy.edges = graphAdjCopy;
    for (int i = 0; i < G_copy.num_nodes; i++) {
      G_copy.edges[i] = G.edges[i]; // deep copy vectors
    }

    // Sequential algorithm
    Timer timer;
    double start = timer.elapsed();

    int seq_res = RUN_DINICS ? dinics(G, 0, n-1) : fordFulkerson(n, graphMat, 0, n - 1); 
    double seq_time = timer.elapsed() - start;
    seq_times.push_back(seq_time);
    cout << "Sequential time: " << seq_time << "s" << endl;

    //Graph Lab
    PushRelabelGraph prG;
    
    double pr_time = 0.0;
    int NUM_RUNS = 3;
    for(int i = 0; i< NUM_RUNS; i++){
      prG.initializeGraph(n,edges,edge_capacities, 0);
      printf("initialized graph\n");
      start = timer.elapsed();
      prG.PushRelabel();
      pr_time += timer.elapsed() - start;
    }
    printf("%d TARGET | AVG TIME: %f \n",seq_res, pr_time / (double)NUM_RUNS);

    //Parallel algorithm
    start = timer.elapsed();
    int par_res = RUN_DINICS ? dinics_par(G_copy, 0, n-1) : fordFulkersonPar(n, graphMat, 0, n - 1, bfsParLockFree);
    double par_time = timer.elapsed() - start;
    par_times.push_back(par_time);
    cout << "Parallel time: " << par_time << "s" << endl;
    if (par_res != seq_res) {
      fprintf(stdout,"Error - target: %d does not match output: %d\n",seq_res, par_res);}
    else {
      fprintf(stdout,"Correctness passed\n");}

    free(G.levels);
    free(G_copy.levels);
    numGraphs++;
    
    // Print perf table
    if(numGraphs == NUM_GRAPHS){
      cout << "\n\nTest-Case        |  Seq Alg       |  Par Alg       | Speedup           " << endl;
      cout <<     "--------------------------------------------------------------------" << endl;
      for (int i = 0; i < numGraphs; i++) {
        std::string name = test_cases[i];
        std::string blanks0(16 - name.length(), ' ');
        std::string time1 = std::to_string(seq_times[i]);
        std::string blanks1(14 - time1.length(), ' ');
        std::string time2 = std::to_string(par_times[i]);
        std::string blanks2(11 - time2.length(), ' ');
        std::string speedup1 = std::to_string(seq_times[i] / par_times[i]);
        std::string blanks3(16 - speedup1.length(), ' ');
   
        cout << name << blanks0 << " | " << time1 << "s" << blanks1 << "| "
              << time2 << "s " << blanks2 << "(" << speedup1 << "x)" << blanks3
              << "| " << endl;
      }
      cout << "\n" << endl;
      fclose(file);
      exit(0);
    }
  }
}