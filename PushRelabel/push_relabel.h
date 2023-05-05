#include "../GraphLabLite/graph.h"
#include<tuple>

class PushRelabelGraph : public tGraph{

  struct vertex_data{
    int excess_flow;
    int height;
    int pushing_to = -1; // Vertex we push flow to
    int pushing_amount = -1; // Amount of flow being pushed
  };
  struct edge_data{
    int flow;
    int capacity;
    int residual_capacity;
  };
  
  public:
  void print_vertex(tVertex &V) override{
    vertex_data *data = (vertex_data *)V.data;
    fprintf(stdout,"Excess Flow: %d, Height: %d, Pushing %d to %d\n",data->excess_flow, data->height,data->pushing_amount,data->pushing_to);
  }
  void print_edge(tEdge &E) override{
    edge_data *data = (edge_data *)E.data;
    fprintf(stdout,"Flow: %d, Capacity: %d Residual Capacity: %d\n",data->flow, data->capacity, data->residual_capacity);
  }

  // Set Gather/Scatter Contexts
  PushRelabelGraph(){
    tGraph::gather_context = OUTGOING; 
    tGraph::scatter_context = BIDIRECTIONAL; 
    tGraph::consist = FULL;
    tGraph::schedule = PARTITIONED;
  };
  
  void initializeGraph(int n, std::vector<int *> edges, // [u,v] pairs
    std::vector<int> edge_capacities, int source){  

    // Set preflow values - source pushes maximum flow to each
    // of its outgoing edges and has height = n, all else
    // height = 0

    int m = edges.size();
    std::vector<void *> edge_info(2*m); // Contains reverse edges
    std::vector<void *> vertex_info(n);
    std::vector<int> signaled_ids;
    for(int i = 0; i < n; i++){
      vertex_info[i] = new vertex_data{0,(i == 0 ? n : (i == n-1 ? -1 : 0)),-1,-1};
    }
    for(int i = 0; i < m; i++){
      int u = edges[i][0];
      int v = edges[i][1];
      edges.push_back(new int[2]{v,u}); // Add reverse edge
      
      // Set edge data
      int cap = edge_capacities[i];
      edge_data *D = new edge_data;
      edge_data *D_rev = new edge_data;

      // Forward edge (u,v) in graph
      D->capacity = cap;
      D->residual_capacity = (u == 0 ? 0 : cap); 
      D->flow = (u == 0 ? cap : 0); // (u,v) edge
      
      // Backwards edge (v,u), where (u,v) in graph
      D_rev->capacity = 0;
      D_rev->residual_capacity = (u == 0 ? cap : 0); 
      D_rev->flow = 0;

      edge_info[i] = D;
      edge_info[i+m] = D_rev;

      if(u == 0){
        ((vertex_data *)(vertex_info[v]))->excess_flow = cap;
        //printf("%d\n", v);
        // schedule nodes if using a queue-based policy
        if(this->schedule == PARTITIONED) signaled_ids.push_back(v);
        else if(this->schedule == FIFO) signal_by_id(v);
      }
    }
    
    // Populate the graph with these initial conidtions
    populateGraph(*this, vertex_info, edges, edge_info);
    if(this->schedule == PARTITIONED){
      for(int i = 0; i < signaled_ids.size(); i++){
        signal_id_partitioned(*this, signaled_ids[i]);
      }
    }
  };

  //********************** Scatter Apply Gather ************************//

  typedef tuple<int,tEdge> accum_type;
  void check_and_init(void* &accum){
    if (accum == nullptr){
      // Accumulate a tuple of (min height, edge leading to node of that height)
      accum_type* init_val = new tuple<int,tEdge>(999,tEdge{-1,-1,nullptr});
      accum = static_cast<void*>(init_val);
    }
  }
  
  void gather(void* &accum, tVertex v_n, tEdge &e_n) override{ 
    check_and_init(accum);  

    // Accumulate the "min height" edge (edge leading to the vertex 
    // of smallest height) that still has some residual capacity 

    int curr_height = std::max(((vertex_data*)v_n.data)->height,0);
    int lowest_height = get<0>(*((accum_type*)accum));

    edge_data *e_data = (edge_data *)e_n.data;
    if (curr_height < lowest_height && e_data->residual_capacity > 0){
      get<0>(*((accum_type*)accum)) = curr_height;
      get<1>(*((accum_type*)accum)) = e_n;
    }
  };

  bool apply(void* &accum, tVertex v_n) override {
    check_and_init(accum);  
    vertex_data *v_data = (vertex_data *)(v_n.data);
    v_data->pushing_amount = -1;
    v_data->pushing_to = -1;

    // If there is no neigbhoring edge with residual capacity,
    // do not apply on this vertex

    tEdge min_edge = get<1>(*((tuple<int,tEdge>*)accum));
    edge_data *e_data = (edge_data *) min_edge.data;
    if(e_data == nullptr){
      return false;
    }
    
    // If this vertex has excess flow to push...

    int min_height = get<0>(*((tuple<int,tEdge>*)accum));
    int curr_height = v_data->height;
    int excess_flow = v_data->excess_flow;
    if (excess_flow > 0){

      // Push flow
      if (min_height < curr_height){
        int pushing = std::min(excess_flow,e_data->residual_capacity);
        v_data->pushing_amount = pushing;
        v_data->pushing_to = min_edge.v;
        v_data->excess_flow -= pushing;
        // if there is still more flow left at this node, 
        // add to the queue
        if(v_data->excess_flow > 0) {
          //queue based schedules
          if(this->schedule == PARTITIONED) signal_partitioned(v_n); 
          else if (this->schedule == FIFO) signal(v_n);
          return true;
        }
      }
      // Relabel
      else if (v_data->height != -1){
        v_data->height = min_height + 1;
        //queue based schedules
        if(this->schedule == PARTITIONED) signal_partitioned(v_n);
        else if(this->schedule == FIFO) signal(v_n);
        return true;
      }
    }
    return false;
  };
  
  void scatter(void* &new_data, tVertex &v_n, tEdge &e_n) override{
    int pushing = ((vertex_data *)new_data)->pushing_amount;
    int pushing_to = ((vertex_data *)new_data)->pushing_to;

    // If flow is being pushed forward on this edge
    if(pushing_to == e_n.v){
      edge_data *e_data = (edge_data *)e_n.data;
      vertex_data *v_data = (vertex_data *)v_n.data;

      e_data->flow += pushing;
      e_data->residual_capacity -= pushing;
      v_data->excess_flow += pushing; // Push flow to the vertex
    }

    // If flow is being pushed away on this edge
    else if(pushing_to == e_n.u){
      edge_data *e_data = (edge_data *)e_n.data;
      vertex_data *v_data = (vertex_data *)v_n.data;

      e_data->flow -= pushing;
      e_data->residual_capacity += pushing;
   
      if(this->schedule == PARTITIONED) signal_id_partitioned(*this, e_n.u);
      else if(this->schedule == FIFO) signal_by_id(e_n.u);
    }
  };  

  public:
    void PushRelabel(){
      solve(*this);
      tVertex sink = this->vertices[this->num_nodes-1];
      int flow = ((vertex_data *)sink.data)->excess_flow;
      printf("RESULT: %d\n",flow);
    };
};