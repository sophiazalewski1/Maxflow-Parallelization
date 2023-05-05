#include "../GraphLabLite/graph.h"

const float d = 0.85;

class PageRankGraph : public tGraph {

  PageRankGraph(){
    tGraph::gather_context = INGOING; 
    tGraph::consist = VERTEX;
    tGraph::schedule = PARTITIONED_SIMULTANEOUS;
  };
  
  // Define data struct here
  struct data {
    float rank;
    int c; // Outgoing edges
  };

public:
  void initializeGraph(std::vector<int *> edges) {
    // Count number of outgoing edges to determine 'c' for each vertex
    int n = edges.size();
    int cs[n] = {0};
    for (int i = 0; i < edges.size(); i++) {
      int u = edges[i][0]; // node going out from u
      cs[u]++;
    }
    // Set initial 'rank' value for each node to be 1/n
    std::vector<void *> vertex_data; //
    for (int i = 0; i < n; i++) {
      data *D = new data;
      D->rank = 1.0 / n;
      D->c = cs[i];
      vertex_data.push_back((void *)D);
    }
    // Edge data is "empty" / not needed for this problem
    std::vector<void *> edge_data(edges.size());

    // Populate the graph with these initial conidtions
    populateGraph(*this, vertex_data, edges, edge_data);
  };

  void check_and_init(void *&accum) {
    if (accum == nullptr) {
      float *init_val = new float(0.0f);
      accum = static_cast<void *>(init_val);
    }
  }

  // This gather function accumulates a float that is the total of all of the
  // edge ranks
  void gather(void *&accum, tVertex v_n, tEdge &v_e) override {
    check_and_init(accum); // Set initial value of accumulator

    data *page_data = (data *)(this->vertices[v_e.u]).data;
    float rank = page_data->rank;
    int c = page_data->c; // outgoing links
    *((float *)accum) += (c == 0 ? 0 : rank / c);
  }; // c == 0 --> dangling node

  bool apply(void *&accum, tVertex v_n) override {
    check_and_init(accum);

    float accum_val = *((float *)accum);
    data *page_data = (data *)(v_n.data);
    float old_page_rank = page_data->rank;
    float new_page_rank = (1 - d) + d * accum_val;
    float c = page_data->c;
    page_data->rank = new_page_rank;
    return new_page_rank != old_page_rank;
  }; // Did the value change?

  void scatter(void *&new_data, tVertex &v_n, tEdge &v_e) override{
      // Nothing needs to be done at this step
  };

  void print_vertex(tVertex &V) override {
    data *d = (data *)(V.data);
    printf("rank: %f, outgoing: %d\n", d->rank, d->c);
  };

  void print_edge(tEdge &E) override { fprintf(stdout, "\n"); };

public:
  void PageRank() { solve(*this); };
};