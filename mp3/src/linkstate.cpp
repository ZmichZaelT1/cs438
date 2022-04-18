#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <list>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <map>
#include <set>
#include <limits.h>
#include <algorithm>
#include <iterator>
#include <sstream>

using namespace std;

class Graph {
    int V; // No. of vertices
    vector<int> vertices;
 
    // Pointer to an array containing adjacency lists
    

public:
    unordered_map<int, unordered_map<int, int>> adj;
    Graph(); // Constructor
    // ~Graph();
    void addEdge(int a, int b, int weight);
    void deleteEdge(int a, int b);
    int getNumVertices();
    vector<int> getVertices();
    int getWeight(int a, int b);
};

Graph::Graph()
{
    this->V = 0;
    // adj = new unordered_map<string, unordered_map<string, int>>;
}

int Graph::getNumVertices() {
    return vertices.size();
}

vector<int> Graph::getVertices() {
    return vertices;
}

int Graph::getWeight(int a, int b) {
    if (adj.count(a)) {
        if (adj[a].count(b)) {
            return adj[a][b];
        }
    } else if (adj.count(b)) {
        if (adj[b].count(a)) {
            return adj[b][a];
        }
    }  
    return 0;
}

void Graph::addEdge(int a, int b, int weight)
{
    adj[a][b] = weight;
    adj[b][a] = weight;

    if (find(vertices.begin(), vertices.end(), a) == vertices.end()) {
        vertices.push_back(a);
    }
    if (find(vertices.begin(), vertices.end(), b) == vertices.end()) {
        vertices.push_back(b);
    }
    sort(vertices.begin(), vertices.end());
}

void Graph::deleteEdge(int a, int b) {
    adj.erase(a);
    adj.erase(b);
}

void loadTopoToGraph(char*);
void sendMessage(char*, FILE*);
map<int, int> dijkstra(int, map<int, int>&);
int minDistance(map<int, int>, unordered_map<int, bool>);
void printAllTopo(FILE*);
void printPath(map<int, int>, int, vector<int>&);
void applyChanges(int, int, int);

Graph g;

int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        printf("Usage: ./linkstate topofile messagefile changesfile\n");
        return -1;
    }

    char *topofile = argv[1];
    char *messagefile = argv[2];
    char *changesfile = argv[3];

    FILE *fpOut;
    fpOut = fopen("output.txt", "w");

    loadTopoToGraph(topofile);

    printAllTopo(fpOut);

    sendMessage(messagefile, fpOut);

    // sendMessage(messagefile, fpOut);

    FILE *fpChanges;
    fpChanges = fopen(changesfile, "r");
    char *line;
    // char a[5], b[5];
    int a, b, weight;
    size_t len = 0;
    while (getline(&line, &len, fpChanges) != -1) {
        sscanf(line, "%d %d %d", &a, &b, &weight);
        // printf("%s %s %d", a, b, weight);
        applyChanges(a, b, weight);
        printAllTopo(fpOut);
        sendMessage(messagefile, fpOut);
    }

    fclose(fpOut);
    

    return 0;
}

void applyChanges(int src, int dest, int weight) {
    if (weight == -999) {
        g.deleteEdge(src, dest);
        return;
    }
    g.addEdge(src, dest, weight);
}

map<int, int> dijkstra(int src, map<int, int> &parent) {
    map<int, int> dist;
    unordered_map<int, bool> sptSet;
    vector<int> vs = g.getVertices();
    int num_v = g.getNumVertices();

    for (int v : vs) {
        parent[0] = -1;
        dist[v] = INT_MAX,
        sptSet[v] = false;
    }

    dist[src] = 0;

    for (int count = 0; count < num_v - 1; count++) {
        int u = minDistance(dist, sptSet);
        sptSet[u] = true;
        for (int v : vs) {
            if (!sptSet[v] && g.getWeight(u, v) && dist[u] != INT_MAX && dist[u] + g.getWeight(u, v) < dist[u] != INT_MAX)  {
                dist[v] = dist[u] + g.getWeight(u, v);
                parent[v] = u;
            }
        }
    }
    return dist;
}

int minDistance(map<int, int> dist, unordered_map<int, bool> sptSet) {
    // Initialize min value
    int min = INT_MAX;
    int min_index;
    vector<int> vs = g.getVertices();
    for (int v : vs) {
        if (sptSet[v] == false && dist[v] <= min) {
            if (dist[v] == min && min != INT_MAX) {
                if (v > min_index) continue;
            }
            min = dist[v], min_index = v;
        }
    }
    return min_index;
}

void sendMessage(char * messageFile, FILE* out) {
    // parse message
    int src, dest;
    char message[200];

    FILE *mf;
    mf = fopen(messageFile, "r");

    char *line;
    size_t len = 0;
    while (getline(&line, &len, mf) != -1) {
        sscanf(line, "%d %d %[^\n]", &src, &dest, message);
        map<int, int> parent;
        map<int, int> tmp = dijkstra(src, parent);
        
        vector<int> path;
        printPath(parent, dest, path);

        path.pop_back();
        stringstream path_str;
        copy(path.begin(), path.end(), ostream_iterator<int>(path_str, " "));
        const char *path_c = path_str.str().c_str();

        fprintf(out, "from %d to %d cost %d hops %smessage %s\n", src, dest, tmp[dest], path_c, message);
    }
}
// “from <x> to <y> cost <path_cost> hops <hop1> <hop2> <...> message <message>”

void printPath(map<int, int> parent, int j, vector<int> &path) {
    if (parent[j] == -1) return;
    printPath(parent, parent[j], path);
    path.push_back(j);
    printf("%d ", j);
}

void printAllTopo(FILE *fpOut) {
    vector<int> vertices = g.getVertices();
    for (int i = 0; i < vertices.size(); i++) {
        int curr = vertices[i];

        map<int, int> parent;
        map<int, int> tmp = dijkstra(curr, parent);
        
        for (int j = 0; j < vertices.size(); j++) {
            int c = vertices[j];
            printf("%d -> %d    %d  ", curr, c, tmp[c]);
            vector<int> path;
            printPath(parent, c, path);
            puts("\n");
            if (path.size() == 1) {
                fprintf(fpOut, "%d %d %d\n", c, path[0], tmp[c]);
            } else {
                fprintf(fpOut, "%d %d %d\n", c, path[1], tmp[c]);
            }
        }
        fprintf(fpOut, "\n");
    }
}

void loadTopoToGraph(char *topofile) {
    FILE *tp;
    tp = fopen(topofile, "r");
    if (!tp) {
        perror("fopen");
        exit(1);
    }

    g = Graph();

    char *line;
    int a, b, weight;
    size_t len = 0;
    while (getline(&line, &len, tp) != -1) {
        sscanf(line, "%d %d %d", &a, &b, &weight);
        g.addEdge(a, b, weight);
    }
}
