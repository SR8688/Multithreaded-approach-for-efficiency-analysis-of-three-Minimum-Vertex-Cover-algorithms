#include <iostream>
#include <vector>
#include <list>
#include <climits>
#include <algorithm>
#include "approxvcs.hpp"

using namespace std;

bool isInputValid(int v, vector< pair<int,int> > edges) {
  for ( auto& e : edges) {
    if (v <= e.first || v <= e.second || e.first < 0 || e.second < 0) {
      std::cerr << "Error: vertex does not exist" << endl;
      return false;
    }
    if (e.first == e.second) {
      std::cerr << "Error: Self loop not allowed" << endl;
      return false;
    }
  }
  return true;
}


vector< vector<int> > CreateAdjacencyList(int v, vector< pair<int,int> > edges) {
  vector< vector<int> > graph(v);
  for ( auto& e : edges) {
    graph[e.first].push_back(e.second);
    graph[e.second].push_back(e.first);
  }
  return graph;
}


vector< vector<int> > InitializeMatrix(int v){

  vector< vector<int> > graph(v);
  int i = 0;
  while (i < v){

    graph[i].resize(v);
    i++;
  }

  return graph;
}

vector< vector<int> > MakeEdgeConnection(vector< vector<int> > graph, vector< pair<int,int> > edges){
  for ( auto& e : edges) {
    graph[e.first][e.second] = 1;
    graph[e.second][e.first] = 1;
  }

  return graph;
}

vector< vector<int> > CreateAdjacencyMat(int v, vector< pair<int,int> > edges) {

  vector< vector<int> > graph(v);
  graph = InitializeMatrix(v);
  graph = MakeEdgeConnection(graph, edges);
  return graph;
}

int *PickVertexWithMaxDeg(int vertex, int maxdeg, vector< vector<int> > graph){

  static int vp[2];
  unsigned int r = 0;
  while (r < graph.size()) {
    int vertexdeg = 0;
    for (auto& n : graph[r])
    vertexdeg += n;

    if (vertexdeg > maxdeg) {
      vertex = r;
      maxdeg = vertexdeg;
    }
    r++;
  }

  vp[0] = vertex;
  vp[1] = maxdeg;

  return vp;

}

bool isEdgeLeft(int vertex){
  return vertex < 0 ? false : true;
}

vector< vector<int> >RemoveEdgeIncidentAppVC1(vector< vector<int> > graph, int vertex){
  for (unsigned int i=0; i < graph.size(); i++) {
    graph[i][vertex] = 0;
    graph[vertex][i] = 0;
  }

  return graph;

}

vector< vector<int> >RemoveEdgeIncidentAppVC2(vector< vector<int> > graph, int v1, int v2){
  for (unsigned int i=0; i < graph.size(); i++) {
    graph[v1][i] = 0;
    graph[v2][i] = 0;
    graph[i][v1] = 0;
    graph[i][v2] = 0;

  }
  return graph;

}


string ApproxVC1(int v, vector< pair<int,int> > edges) {
  if (!isInputValid(v, edges)) {
    string blank = "";
    return blank;
  }

  if (edges.empty()) {
    string blank = "";
    return blank;
  }

  vector<int> vc;
  vector< vector<int> > graph = CreateAdjacencyMat(v, edges);

  while(1) {

    int vertex = -1;
    int maxdeg = 0;
    int *vp;
    vp = PickVertexWithMaxDeg(vertex, maxdeg, graph);

    vertex = vp[0];
    maxdeg = vp[1];

    if(!isEdgeLeft(vertex))
    break;

    graph = RemoveEdgeIncidentAppVC1(graph, vertex);
    vc.push_back(vertex);

  }
  sort(vc.begin(), vc.end());

  return PrintVertex(vc);
}


string ApproxVC2(int v, vector< pair<int,int> > edges) {
  if (!isInputValid(v, edges)) {
    string blank = "";
    return blank;
  }

  if (edges.empty()) {
    string blank = "";
    return blank;
  }

  vector<int> vc;
  vector< vector<int> > graph = CreateAdjacencyMat(v, edges);

  while(1) {

    int v1;
    int v2;
    bool found = false;
    int r = 0;
    while (r < v) {
      int c = 0;
      while (c < v) {
        if (graph[r][c] == 1) {
          v1 = r;
          v2 = c;
          found = true;
          break;
        }
        c++;
      }
      if(found)
      break;
      r++;
    }

    // exit while loop if no edges remain
    if (!found)
    break;

    // add vertices to vertex cover
    vc.push_back(v1);
    vc.push_back(v2);

    graph = RemoveEdgeIncidentAppVC2(graph, v1, v2);

  }

  sort(vc.begin(), vc.end());

  return PrintVertex(vc);
}
