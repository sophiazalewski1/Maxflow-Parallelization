#include <vector>
#include <atomic>
#include "graph.h"
#include <cstdlib>
#include <cstdio>
#include <tuple>
#include <queue>
#include <omp.h>
#include <random>
#include "../timing.h"

#define NUM_WORKERS 4 // IMPORTANT! Set this equal to 
                      // the number of threads being run

// Global queue variables
queue<int> workQ;
localQ *workQs; 
static omp_lock_t qlock;
static omp_lock_t qlocks[NUM_WORKERS * 16];

//############################################################################//
//#########################|  HELPER FUNCTIONS |##############################//
//############################################################################//

//===================== WORK STEALING QUEUES (RELAXED) =======================//

// Create a queue
void initialize_relaxed_q(int num_workers){
    queue<int> q;
    workQs = new localQ[num_workers];
    for(int i = 0; i<num_workers; i++){
        // omp_lock_t lock;
        omp_init_lock(&qlocks[i*16]);
        queue<int> q;
        workQs[i].q = q;
    }
}
// Clear memeory
void destroy_relaxed_q(int num_workers){
    for(int i = 0; i<num_workers; i++){
        omp_destroy_lock(&(workQs[i].qlock));
    }
    free(workQs);
}
// Signal to this queue
void signal_relaxed(tVertex V){
    int tid = omp_get_thread_num();
    workQs[tid].q.push(V.vid);
}
// Steal from the next neighbor
int steal(int num_workers){
    int tid = omp_get_thread_num();
    int steal_from = (tid + 1) % num_workers;
    while(steal_from != tid) {
        steal_from = (steal_from + 1) % num_workers;
        omp_set_lock(&(workQs[steal_from].qlock));
        if(!(workQs[steal_from].q.empty())){
            int vid = workQs[steal_from].q.front();
            workQs[steal_from].q.pop();
            omp_unset_lock(&(workQs[steal_from].qlock));
            return vid;
        }
       omp_unset_lock(&(workQs[steal_from].qlock));
    }
    return -1;
}
//============================ FIFO CENTRAL QUEUE ============================//

// Signal by tVertex
void signal(tVertex V){
    omp_set_lock(&qlock);
    workQ.push(V.vid);
    omp_unset_lock(&qlock);
}

// Signal by vertex_id 
void signal_by_id(int vid){
    omp_set_lock(&qlock);
    workQ.push(vid);
    omp_unset_lock(&qlock);
}

// Add all nodes to the work queue
void signal_all(tGraph &G){
    for (int i = 0; i<G.num_nodes; i++){
        workQ.push(i);
    }
}

//=========================== PARTITIONED QUEUE ==============================//

// shares same initialization functions as the RELAXED queues. 

void signal_id_partitioned(tGraph &G, int vid){
    int pid = G.vertices[vid].processor_id;
    #pragma omp critical (queue)
    {
        workQs[pid].q.push(vid);
    }
}

void signal_partitioned(tVertex V){
    int pid = V.processor_id;
    #pragma omp critical (queue)
    {
        workQs[pid].q.push(V.vid);
    }
}
// ASSUMPTION: this takes place outside an omp parallel
void signal_all_partitioned(tGraph &G){
    for(int i = 0; i < G.num_nodes; i++){
        tVertex V = G.vertices[i];
        int pid = V.processor_id;
        workQs[pid].q.push(i);
    }
}
// Remove an element from local queue
int pop_paritioned(int tid){
    int vid = -1;
    #pragma omp critical (queue)
    {  
    if(!workQs[tid].q.empty()){
        vid = workQs[tid].q.front();
        workQs[tid].q.pop();
    }
    }
    return vid;
}
// Debugging
void print_queues(){
    for(int i = 0; i < NUM_WORKERS; i ++){
        printf("printing queue %d: ", i);
        queue<int> tmp;
        int count = 0;
        while(!workQs[i].q.empty()){
            int elem = workQs[i].q.front();
            workQs[i].q.pop();
            tmp.push(elem);
            count++;
            printf("%d", elem);
        }
        for(int j = 0; j < count; j++){
            int elem = tmp.front();
            tmp.pop();
            workQs[i].q.push(elem);
        }
        printf("\n"); 
    }
    printf("done printing queues\n");
}


//============================ LOCKING FUNCTIONS =============================//

// Locks to Enforce Consistency Model
void lockg(lock_t *x){*x = 1;}
void unlockg(lock_t *x){*x = 0;}
bool testg(lock_t x){return x;}

// Lock's necessary vertices and edges in neighborhood based on consistency model
bool try_lock_neighbors(tGraph &G, int vid){
    // Test in edges
    for(auto &e_i : G.in_edges[vid]){
        if(testg(G.edges[e_i].elock)) {
            return false;}
        if(G.consist == FULL && testg(G.vertices[G.edges[e_i].u].vlock)) {
             return false;}}
    // Test out edges
    for(auto &e_i : G.out_edges[vid]){
        if(testg(G.edges[e_i].elock)) return false;
        if(G.consist == FULL && testg(G.vertices[G.edges[e_i].v].vlock)) 
            return false;}
    // Lock in edges
    for(auto &e_i : G.in_edges[vid]){
        lockg(&G.edges[e_i].elock);
        if(G.consist == FULL) 
            lockg(&(G.vertices[G.edges[e_i].u].vlock));}
    // Lock out edges
    for(auto &e_i : G.out_edges[vid]){
        lockg(&G.edges[e_i].elock);
        if(G.consist == FULL) 
            lockg(&(G.vertices[G.edges[e_i].v].vlock));}
    return true;
}

// Lock all adjacent graph_out_edges
void lock_neighborhood(tGraph &G, int vid){
    while(true){
        bool all_unlocked = true;
        #pragma omp critical (vlock)
        {
            lock_t *vlock = &(G.vertices[vid].vlock);
            if(testg(*vlock)){
                all_unlocked = false;}
            else if(G.consist != VERTEX && !(try_lock_neighbors(G,vid))){
                all_unlocked = false;}
            else{
                lockg(vlock);}
        }
        if(all_unlocked){
            return;
        }    
    }
}
// Unlocks a vertex's neighborhood
void unlock_neighborhood(tGraph &G, int vid){
    // does this need to be in a critical section?
    #pragma omp critical (vlock)
    {
        unlockg(&(G.vertices[vid].vlock));
        if (G.consist != VERTEX){
            for(auto &e_i : G.out_edges[vid]){
                unlockg(&(G.edges[e_i].elock));
                if(G.consist == FULL){
                    unlockg(&(G.vertices[G.edges[e_i].v].vlock));
                }
            }
            for(auto &e_i : G.in_edges[vid]){
                unlockg(&(G.edges[e_i].elock));
                if(G.consist == FULL){
                    unlockg(&(G.vertices[G.edges[e_i].u].vlock));
                }
            }
        }
        unlockg(&G.vertices[vid].vlock);
    }
}

//############################################################################//
//#######################| GRAPHLAB LITE FUNCTIONS |##########################//
//############################################################################//

// Debugging
void printGraph(tGraph &G){
    int n = G.num_nodes;
    fprintf(stdout, "printing graph");
    for(int i = 0; i < n; i++){
        tVertex V = G.vertices[i];
        fprintf(stdout,"Vertex %d - ",i);
        fprintf(stdout, "vlock: %d | ", V.vlock);
        if(G.schedule == PARTITIONED){
            fprintf(stdout, "partition: %d | ", V.processor_id);
        }
        G.print_vertex(V);
        for (auto & e_i : G.out_edges[i]){
            tEdge E = G.edges[e_i];
            fprintf(stdout,"     -> %d - ",E.v);
            fprintf(stdout," elock: %d | ", E.elock);
            G.print_edge(E);
        }
    }
    fprintf(stdout,"\n");
}

//============================ CREATING THE GRAPH ============================//

// Preprocess graph assign vertices to different partitions
// Assumes the graph is connected
void partitionVertices(tGraph &G){
    Timer partition_timer;
    double start = partition_timer.elapsed();
    int n = G.num_nodes;
    int width = n/NUM_WORKERS;
    std::uniform_int_distribution<> random_node(0, G.num_nodes); 
    
    if(n < NUM_WORKERS){ // For small graphs, use one processor
        for(int i = 0; i < G.num_nodes; i++){
            G.vertices[i].processor_id = 0;}}

    else{
        // Evenly distribute "start" vertices
        queue<int> Qs[NUM_WORKERS];
        for(int proc = 0; proc < NUM_WORKERS; proc ++){
            int v_i = proc * width;
            Qs[proc].push(v_i);
        }

        bool done = false;
        while(!done){
            for(int proc = 0; proc < NUM_WORKERS; proc ++){
                int v_i; // The current vertex

                // If this queue is empty, check if we are done processing
                if(Qs[proc].empty()){
                    bool all_empty = true;
                    for (int proc_ = 0; proc_ < NUM_WORKERS; proc_ ++){
                        if(!Qs[proc_].empty()){ all_empty = false;}}
                    if(all_empty){
                        done = true;
                        break;
                    }
                    // Still more work to be done, pick random vertex
                    else{
                        std::random_device rd; // obtain a random number from hardware
                        std::mt19937 gen(rd()); // seed the generator
                        v_i = random_node(gen); // Get random vertex
                        if(G.vertices[v_i].processor_id == -1){ // Add to this proc
                            G.vertices[v_i].processor_id = proc;
                            G.partitions[proc].push_back(v_i);
                        }
                        else{continue;}
                    }
                }
                // Queue not empty, pop vertex from current proc's queue
                else{
                    v_i = Qs[proc].front();
                    Qs[proc].pop();
                }
                // Add all vertex neighbors to this proc's queue
                std::vector<int> boundary_edges_outgoing;
                for(auto & e_i : G.out_edges[v_i]){
                    int next_i = G.edges[e_i].v;
                    tVertex next = G.vertices[next_i];
                    if(next.processor_id == -1){ // Neighbor not yet assigned
                        G.vertices[next_i].processor_id = proc; // Assign to this proc
                        G.partitions[proc].push_back(next_i);
                        Qs[proc].push(next_i);
                    }
                    else if(next.processor_id != proc){ // This edge has already been asigned to another proc
                        boundary_edges_outgoing.push_back(e_i); 
                    }
                }
                std::vector<int> boundary_edges_ingoing;
                for(auto & e_i : G.in_edges[v_i]){
                    int next_i = G.edges[e_i].u;
                    tVertex next = G.vertices[next_i];
                    if(next.processor_id == -1){  // Neighbor not yet assigned
                        Qs[proc].push(next_i);
                        G.vertices[next_i].processor_id = proc; // Assign to this proc
                        G.partitions[proc].push_back(next_i);
                    }
                    else if(next.processor_id != proc){ // This edge has already been asigned to another proc
                        boundary_edges_ingoing.push_back(e_i); 
                    }
                }
                G.vertices[v_i].boundary_edges_outgoing = boundary_edges_outgoing;
                G.vertices[v_i].boundary_edges_ingoing = boundary_edges_ingoing;
            }
        }
    }
    double total_part = partition_timer.elapsed() - start;
    printf("time spent partitioning %f\n", total_part);
}

// Populates a tGraph with vertex and edge data
void populateGraph(tGraph &G, std::vector<void*> &vertex_data, std::vector<int *> &uv_pairs, std::vector<void*> &edge_data){
    int n = vertex_data.size();
    int m = uv_pairs.size();
    G.num_nodes = n;
    std::vector<tVertex> vertices;
    std::vector<tEdge> edges;
    std::vector<int> *out_edges = (std::vector<int> *) calloc(n, sizeof(std::vector<int>));
    std::vector<int> *in_edges = (std::vector<int> *) calloc(n, sizeof(std::vector<int>));
    // Populate vertices
    for (int i = 0; i < n; i++){
        tVertex V;
        V.data = (void*) vertex_data[i];
        V.vid = i;
        unlockg(&(V.vlock));
        vertices.push_back(V);
    }
    // Populate edges
    for (int i = 0; i < edge_data.size(); i++){
        int u = uv_pairs[i][0];
        int v = uv_pairs[i][1];
        void* data = edge_data[i];

        tEdge E = tEdge{u,v,data}; 
        int index = edges.size();
        unlockg(&(E.elock));
        edges.push_back(E);

        in_edges[v].push_back(index);
        out_edges[u].push_back(index);
    }
    G.vertices = vertices;
    G.edges = edges;
    G.in_edges = in_edges;
    G.out_edges = out_edges;
    if((G.schedule == PARTITIONED || G.schedule == PARTITIONED)){
        G.partitions = new std::vector<int>[NUM_WORKERS];
        partitionVertices(G); // assign vertices to processor
        if(G.schedule == PARTITIONED) initialize_relaxed_q(NUM_WORKERS);
    }
    printf("graph populated\n");
};

//============================ UPDATING THE GRAPH ============================//

bool update(tGraph &G, int vid){
    tVertex V = G.vertices[vid];
    // UNCOMMENT THIS LINE TO USE "LOCKS" TO ENFORCE CONSISTENCY
    // TESTING FOUND NO SIGNIFICANT PERFORMANCE IMPACT VS USING CRITICAL SECTION TO DO UPDATE
    // lock_neighborhood(G, vid);
    
    // Gather
    void* accum = nullptr;
    for (auto & e_i : (G.gather_context == INGOING ? G.in_edges[vid] : G.out_edges[vid])){
        // If not scattering INGOING, scatter outgoing edges
        tEdge E = G.edges[e_i];
        tVertex V_n = G.vertices[E.v];
        G.gather(accum, V_n, E);}
    if (G.gather_context == BIDIRECTIONAL){
        // If BIDIRECTIONAL, also scatter ingoing
        for (auto & e_i : G.in_edges[vid]){
            tEdge E = G.edges[e_i];
            tVertex V_n = G.vertices[E.v];
            G.gather(accum, V_n, E);}
    }
    // Apply
    bool value_changed = G.apply(accum, V);

    // Scatter
    for (auto & e_i : (G.scatter_context == INGOING ? G.in_edges[vid] : G.out_edges[vid])){
        // If not scattering INGOING, scatter outgoing edges
        tEdge E = G.edges[e_i];
        tVertex V_n = G.vertices[E.v];
        G.scatter(V.data, V_n, E);}
    if (G.scatter_context == BIDIRECTIONAL){
        // If BIDIRECTIONAL, also scatter ingoing
        for (auto & e_i : G.in_edges[vid]){
            tEdge E = G.edges[e_i];
            tVertex V_n = G.vertices[E.v];
            G.scatter(V.data, V_n, E);}
    }

    // UNCOMMENT THIS LINE TO USE "LOCKS" TO ENFORCE CONSISTENCY
    // TESTING FOUND NO SIGNIFICANT PERFORMANCE IMPACT VS USING CRITICAL SECTION TO DO UPDATE
    // unlock_neighborhood(G, vid);
    return value_changed;
}

// checks if locking is needed under full consistency
bool has_boundary_neighbors(tGraph &G, int vid){
    for (auto &e_i : G.in_edges[vid]){
        if(G.vertices[G.edges[e_i].u].boundary_edges_ingoing.size() > 0 
        || G.vertices[G.edges[e_i].u].boundary_edges_outgoing.size() > 0){
            return true;
        }
    }
    for (auto &e_i : G.out_edges[vid]){
        if(G.vertices[G.edges[e_i].v].boundary_edges_ingoing.size() > 0 
        || G.vertices[G.edges[e_i].v].boundary_edges_outgoing.size() > 0){
            return true;
        }
    }
    return false;
}

// checks the consistency model of the graph G
// and executes the update function with the 
// appropriate protections
bool consistent_update(tGraph &G, int vid, double *critical_time){
    bool res = false;
    if(G.consist == EDGE 
        && (G.schedule == FIFO
        || G.schedule == SIMULTANEOUS
        || G.vertices[vid].boundary_edges_ingoing.size() >= 0
        || G.vertices[vid].boundary_edges_outgoing.size() >= 0)){
        #pragma omp critical (update)
        {
        Timer critical_timer;
        double start = critical_timer.elapsed();
        res = update(G,vid);
        *critical_time += critical_timer.elapsed() - start;
        }
    }
    else if(G.consist == FULL
        && (G.schedule == FIFO 
        || G.schedule == SIMULTANEOUS 
        || has_boundary_neighbors(G, vid))){
        #pragma omp critical (update)
        {
        Timer critical_timer;
        double start = critical_timer.elapsed();
        res = update(G,vid);
        *critical_time += critical_timer.elapsed() - start;
        }
    }
    else{
        res = update(G,vid);
    }
    return res;
}

bool done_working(){
    for(int i = 0; i< NUM_WORKERS; i++){
        if(!workQs[i].q.empty()){
            return true;
        }
    }
    return false;
}

//=========================== RETURNING A SOLUTION ===========================//

tGraph *solve(tGraph &G){
    bool converged = false;
    double critical_time = 0.0;
    printf("schedule %d\n", G.schedule);
    if(G.schedule == SIMULTANEOUS){
        while(!converged){
            converged = true;
            // simulataneous (everything scheduled at once)
            #pragma omp parallel for schedule(dynamic,8) reduction (&& : converged)
            for (int i = 0; i < G.num_nodes; i++){
                if(consistent_update(G,i,&critical_time)){converged = false;} // see if this value has converged
            }
        } 
    } 
    else if(G.schedule == FIFO){
        omp_init_lock(&qlock);
        converged = true;
        #pragma omp parallel num_threads(NUM_WORKERS)
        {
            int vid = -1;
            while(!workQ.empty()){
                omp_set_lock(&qlock);
                if(!workQ.empty()){
                    vid = workQ.front();
                    workQ.pop();
                }
                omp_unset_lock(&qlock);
                if(vid >= 0 && consistent_update(G, vid, &critical_time)){converged = false;}
            }
        }
        omp_destroy_lock(&qlock);
    }

    else if (G.schedule == PARTITIONED) {
        printf("solving...\n");
        int num_empty=0;
        #pragma omp parallel num_threads(NUM_WORKERS)
        {   
            Timer mytime;
            int tid = omp_get_thread_num();
            bool empty = false;
            while(done_working()){
                double start = mytime.elapsed();
                #pragma omp barrier
                int next_v = pop_paritioned(tid);
                if(next_v != -1){
                    consistent_update(G, next_v, &critical_time);
                }
                #pragma omp barrier
            }   
        }
    }
    else if(G.schedule == PARTITIONED_SIMULTANEOUS){
        bool converged = false;
        while(!converged){
            converged = true;
            #pragma omp parallel num_threads(NUM_WORKERS) reduction(&& : converged)
            {
                converged = true;
                int tid = omp_get_thread_num();
                for(int i = 0; i < G.partitions[tid].size(); i++){
                    if(consistent_update(G, G.partitions[tid][i], &critical_time)){
                        converged = false;
                    }
                }
            }
        }   
    }
    printf("critical time %f \n", critical_time);
    return &G;
}
