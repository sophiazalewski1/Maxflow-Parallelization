import random
import tqdm
import math

MAX_CAP = 100
MIN_CAP = 50

'''  Dense           Sparse         1-Line     Exclusive      Unbalanced
  Layer Graph      Layer Graph      Graph     Paths Graph        Graph
       s                s             s            s               s
     / | \            / | \           |          / | \            / \ 
    1  2  3          1  2  3          1         1  2  3          1   2
    |\/|\/|           \    |          |         |  |  |         /   /|\ 
    4  5  6          4  5  6          2         4  5  6         3  4 5 6  
    |\/|\/|            / \            |         |  |  |          \ | //  
    7  8  9          7  8  9          3         7  8  9            t
     \ | /            \ | /           |          \ | /
       t                t             t            t                
'''
def writeUnbalancedGraph(n, fo, testcase_name):
    edges = []
    # Edge from source to nodes 1 and 2
    for i in range(1,3):
        c = random.randint(MIN_CAP,MAX_CAP)
        edges.append((0,i,c))

    # Edge from node 1 to node 3 only
    edges.append((1,3,random.randint(MIN_CAP,MAX_CAP)))

    # Edge from node 2 to nodes 4 through n-2
    for i in range(4,n-2):
        c = random.randint(MIN_CAP,MAX_CAP)
        edges.append((2,i,c))

    # Edge from nodes 3 through n-2 to sink (n-1)
    for u in range (3,n-1):
        c = random.randint(MIN_CAP,MAX_CAP)
        edges.append((u,n-1,c))

    # Write graph info to file
    fo.write('c '+testcase_name+' \n')
    fo.write('p max ' + str(n) + " " + str(len(edges)) + "\n")
    for edge in edges:
        fo.write('a ' + str(edge[0]) + ' ' + str(edge[1]) + ' ' + str(edge[2]) + '\n')
    fo.write('\n')

def writeExclusivePathGraph(n, nodes_per_layer, filename,testcase_name):
    edges = []
    num_layers = math.ceil((n-2) / nodes_per_layer)

    # Edges from source to all nodes in layer 1
    for v in range(1, nodes_per_layer + 1):
        c = random.randint(MIN_CAP,MAX_CAP)
        edges.append((0,v,c))

    # Fill in all middle layers
    for layer in range(num_layers-1):
        layer_start = 1 + layer * nodes_per_layer
        layer_end = layer_start + nodes_per_layer
        next_layer_start = layer_end
        next_layer_end = layer_end + nodes_per_layer
        if(layer == num_layers - 2):
            next_layer_end = n-1
        for u in range(layer_start,layer_end):
            v = min(u + nodes_per_layer, n-1)
            c = random.randint(MIN_CAP,MAX_CAP)
            edges.append((u,v,c))

    # Edges from all nodes in last layer to sink
    last_layer_start = 1 + (num_layers - 1) * nodes_per_layer
    last_layer_end = n-1
    for u in range(last_layer_start, last_layer_end):
        c = random.randint(MIN_CAP,MAX_CAP)
        edges.append((u,n-1,c))
    
    # Write graph info to file
    fo.write('c '+testcase_name+' \n')
    fo.write('p max ' + str(n) + " " + str(len(edges)) + "\n")
    fo.write('n 0 s \nn ' + str(n-1) + ' t\n')
    for edge in edges:
        fo.write('a ' + str(edge[0]) + ' ' + str(edge[1]) + ' ' + str(edge[2]) + '\n')
    fo.write('\n')

def writeLineGraph(n, filename,testcase_name):
    edges = []
    for u in range(n-1):
        c = random.randint(MIN_CAP,MAX_CAP)
        edges.append((u,u+1,c))

    # Write graph info to file
    fo.write('c '+testcase_name+' \n')
    fo.write('p max ' + str(n) + " " + str(len(edges)) + "\n")
    fo.write('n 0 s \nn ' + str(n-1) + ' t\n')
    for edge in edges:
        fo.write('a ' + str(edge[0]) + ' ' + str(edge[1]) + ' ' + str(edge[2]) + '\n')
    fo.write('\n')

def writeDenseLayerGraph(n, num_layers, fo, testcase_name):
    edges = []
    nodes_per_layer = math.floor((n - 2) / num_layers)

    # Edge from source to all nodes in layer 1
    for v in range(1,nodes_per_layer + 1):
        c = random.randint(MIN_CAP,MAX_CAP)
        edges.append((0,v,c))

    # Fill in all middle layers
    for layer in range(num_layers-1):
        layer_start = 1 + layer * nodes_per_layer
        layer_end = layer_start + nodes_per_layer
        next_layer_start = layer_end
        next_layer_end = layer_end + nodes_per_layer
        if(layer == num_layers - 2):
            next_layer_end = n-1
        for u in range(layer_start,layer_end):
            for v in range(next_layer_start, next_layer_end):
                c = random.randint(MIN_CAP,MAX_CAP)
                edges.append((u,v,c))

    # Write all nodes in last layer to sink
    last_layer_start = 1 + (num_layers - 1) * nodes_per_layer
    last_layer_end = n-1
    for u in range(last_layer_start, last_layer_end):
        c = random.randint(MIN_CAP,MAX_CAP)
        edges.append((u,n-1,c))
    
    # Write graph info to file
    fo.write('c '+testcase_name+' \n')
    fo.write('n 0 s \nn ' + str(n-1) + ' t\n')
    fo.write('p max ' + str(n) + " " + str(len(edges)) + "\n")
    for edge in edges:
        fo.write('a ' + str(edge[0]) + ' ' + str(edge[1]) + ' ' + str(edge[2]) + '\n')
    fo.write('\n')

def writeSparseLayerGraph(n, num_layers, fo, testcase_name):
    edges = []
    nodes_per_layer = math.floor((n - 2) / num_layers)
    active_lanes = [0 for i in range(n)]

    # Edges from source to all nodes in layer 1
    for v in range(1,nodes_per_layer + 1):
        c = random.randint(MIN_CAP,MAX_CAP)
        edges.append((0,v,c))
        active_lanes[v] = 1

    # Determine paths in middle num_layers
    for layer in range(num_layers-1):
        layer_start = 1 + layer * nodes_per_layer 
        layer_end = layer_start + nodes_per_layer 
        next_layer_start = layer_end
        next_layer_end = layer_end + nodes_per_layer
        if(layer == num_layers - 2):
            next_layer_end = n-1
        for u in range(layer_start,layer_end):
            # Each "active" node adds between 0 and 2 edges, but is most
            # likely to add 1 edge to directly continue the current path
            if(active_lanes[u] == 1):
                branching_factor = 1
                r = random.uniform(0, 1)
                # if(r <= 0.05):
                #     branching_factor = 0
                if(r <= 0.2):
                    branching_factor = 2
                next_nodes = random.sample(range(next_layer_start, next_layer_end), min(branching_factor,nodes_per_layer))
                for v in next_nodes:
                    active_lanes[v] = 1
                    c = random.randint(MIN_CAP,MAX_CAP)
                    edges.append((u,v,c))

    # Edges from all nodes in last layer to sink
    last_layer_start = 1 + (num_layers - 1) * nodes_per_layer
    last_layer_end = n-1
    for u in range(last_layer_start, last_layer_end):
        c = random.randint(MIN_CAP,MAX_CAP)
        edges.append((u,n-1,c))
    
    # Write graph info to file
    fo.write('c '+testcase_name+' \n')
    fo.write('p max ' + str(n) + " " + str(len(edges)) + "\n")
    fo.write('n 0 s \nn ' + str(n-1) + ' t\n')
    for edge in edges:
        fo.write('a ' + str(edge[0]) + ' ' + str(edge[1]) + ' ' + str(edge[2]) + '\n')
    fo.write('\n')

file = 'partition.txt'
fo = open(file, "a")
for size in [1000,2000,4000]:
    sz = (int)(size/1)
    num_layers = int(math.sqrt(size))

    # Which graphs to generate
    writeLineGraph(size,fo,"line{}k".format(sz)) # Line graph
    writeExclusivePathGraph(size, num_layers, fo,"excl_path{}k".format(sz)) # Exclusive-paths graph
    writeSparseLayerGraph(size, num_layers, fo,"sparse_layer{}k".format(sz)) # Sparse layer graph
    writeDenseLayerGraph(size, num_layers, fo,"dense_layer{}k".format(sz)) # Dense layer graph
    writeUnbalancedGraph(size, fo, "Unbalanced")

fo.close()
