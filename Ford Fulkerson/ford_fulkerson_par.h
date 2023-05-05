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

bool bfsParLockFree(int n, std::vector<int> &rGraph, int s, int t,
                    int parent[]);

bool bfsParCritical(int n, std::vector<int> &rGraph, int s, int t,
                    int parent[]);

bool bfsParList(int n, std::vector<int> &rGraph, int s, int t, int parent[]);

int fordFulkersonPar(int n, std::vector<int> &graph, int s, int t,
                     bool (*bfsPar)(int, std::vector<int> &, int, int, int[]));