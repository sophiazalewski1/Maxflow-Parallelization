//############################################################################//
//##                                                                        ##//
//##        TEMPLATE FOR CREATING A GRAPH LAB LITE 'VERTEX PROBLEM'         ##//
//##                                                                        ##//
//##   Example use:                                                         ##//
//##       - myVertexProblem P;                                             ##//
//##       - P.initializeGraph(edge_info);                                  ##//
//##       - P.solveMyProblem();                                            ##//
//##                                                                        ##//
//############################################################################//

#include "../GraphLabLite/graph.h"

class myVertexProblem : public tGraph {

  struct vertex_datatype {

    // TO DO: Define me
  };
  struct edge_datatype {

    // TO DO: Define me
  };

public:
  void initializeGraph(
      std::vector<vertex_datatype *> vertex_data,
      std::vector<int *> edges,                 // [u,v] pairs
      std::vector<edge_datatype *> edge_data) { // Example input fields

    // Optionally add preprocessing here

    populateGraph(*this, (void *)vertex_data, edges, (void *)edge_data);
  }

  // Set Options:
  // - Gather/Scatter Contexts
  // - Work Schedule
  // - Consistency Model

  PushRelabelGraph(){
    tGraph::gather_context = OUTGOING; 
    tGraph::scatter_context = BIDIRECTIONAL; 
    tGraph::consist = FULL;
    tGraph::schedule = PARTITIONED;
  };

  //************************ Scatter Apply Gather **************************//

  float gather(tVertex v_n, tEdge v_e) override{

      // Your implementation here
  };

  void check_and_init(void *&accum) {
      if (accum == nullptr) {

        // Your base value for accumulation here
        // ex. int* init_val = new int{0} 
        accum = static_cast<void*>(init_val);}};
  };

  bool apply(void *&accum, void *&data) override {
    check_and_init(accum); // do not remove this line

    // Your implementation here
    // * do not remove the "check_and_init" line above
    // * Make sure to return weather or not the value for data changed

    return true; // Return weather or not the value for data changed
  };

  void scatter(void *&new_data, tVertex &v_n, tEdge &v_e) override {
    check_and_init(accum); // do not remove this line

    // Your implementation here
    // * do not remove the "check_and_init" line above 
  };

public:
  void solveMyProblem() {
    solve(*this);
    // Optionally add postprocessing
  };
};