#include <iostream>
#include <vector>
#include <climits>
#include <algorithm>
#include <chrono>
#include "cnfsatvc.hpp"
#include <minisat/core/SolverTypes.h>
#include <minisat/core/Solver.h>

bool LOGGING_EN = false;


string PrintVertex(vector<int> vcpath) {
  string result;
  bool first = true;
  for (auto& v : vcpath) {
    if (first) {
      first = false;
    }
    else {
      result = result + ",";
    }
    result = result + to_string(v);
  }
  return result;
}

VC::VC( int v, vector< pair<int,int> > e):
vertices(v), edges(e) {

}

Minisat::Var VC::ConvertToVar(int r, int c, int x) {
  int columns = x;
  return r * columns + c;
}

bool VC::isInputValid(vector< pair<int,int> > edges) {
  for ( auto& e : edges) {
    if (this->vertices <= e.first || this->vertices <= e.second || e.first < 0 || e.second < 0) {
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


void VC::AddClauseforMinCover(Minisat::Solver& solver, int x) {
  int i = 0;
  while( i < x){
    Minisat::vec<Minisat::Lit> clause;
    int n = 0;
    while( n < this->vertices){
      clause.push(Minisat::mkLit(ConvertToVar(n,i,x)));
      n++;
    }
    solver.addClause(clause);
    i++;
  }
}


void VC::AddClauseforSeparateVertices(Minisat::Solver& solver, int x) {

  int i = 0;
  while(i < this->vertices){
    int z = 0;
    while(z < x){
      int y = 0;
      while(y < z){
        solver.addClause( ~Minisat::mkLit(ConvertToVar(i,y,x)), ~Minisat::mkLit(ConvertToVar(i,z,x)));
        y++;
      }
      z++;
    }
    i++;
  }
}


void VC::AddClauseforSinglePosCover(Minisat::Solver& solver, int x) {
  int i = 0;
  while(i < x){
    int z = 0;
    while(z < this->vertices){
      int y = 0;
      while(y < z){
        solver.addClause( ~Minisat::mkLit(ConvertToVar(y,i,x)), ~Minisat::mkLit(ConvertToVar(z,i,x)));
        y++;
      }
      z++;
    }
    i++;
  }
}


void VC::AddClauseforEachEdge(Minisat::Solver& solver, int x) {
  for(auto& e : edges) {
    Minisat::vec<Minisat::Lit> lits;
    int i = 0;
    while(i < x){
      lits.push(Minisat::mkLit(ConvertToVar(e.first, i, x)));
      lits.push(Minisat::mkLit(ConvertToVar(e.second, i, x)));
      i++;
    }
    solver.addClause(lits);
  }
}
void InitializeVars(int vertices, int x, Minisat::Solver& solver){
  for (int r = 0; r < vertices; r++) {
    for (int c = 0; c < x; c++) {
      solver.newVar();
    }
  }
}

void VC::AddClauses(Minisat::Solver& solver, int x) {

  AddClauseforMinCover(solver, x);
  AddClauseforSeparateVertices(solver, x);
  AddClauseforSinglePosCover(solver, x);
  AddClauseforEachEdge(solver, x);

  return;
}

bool VC::SolveGraph(Minisat::Solver& solver, int x) {

  InitializeVars(vertices,x, solver);

  AddClauses(solver, x);
  auto sat = solver.solve();

  return sat;
}

vector<int> VC::GetVertexPath(Minisat::Solver& solver, int x) {

  vector<int> path;

  int r = 0;
  while(r < this->vertices){
    int c = 0;
    while(c < x){
      if (solver.modelValue(ConvertToVar(r,c,x)) == Minisat::l_True) {
        path.push_back(r);
      }
      c++;
    }
    r++;
  }
  sort(path.begin(), path.end());
  return path;
}


int VC::GetVertices() const {
  return vertices;
}

void VC::AddEdges(vector< pair<int,int> > e) {
  if ( isInputValid(e) ) {
    edges = e;
  }
  return;
}


string VC::LinSearchVC() {
  if (!isInputValid(edges)) {
    string blank = "";
    return blank;
  }

  if (edges.empty()) {
    string blank = "";
    return blank;
  }

  int results[vertices];  //0 is UNSAT, 1 is SAT, -1 is undefined where index is x or vertex cover length
  vector<int> vcpath[vertices];
  fill_n(results, vertices, -1);

  int i = 0;
  while (i <vertices) {
    Minisat::Solver solver;

    if (LOGGING_EN) {
      clog << "Trying K=" << i;
    }
    auto start = chrono::system_clock::now(); // start measuring time

    results[i] = SolveGraph(solver, i);


    auto end = chrono::system_clock::now();   // stop measuring time

    chrono::duration<double> diff = end-start;

    if (LOGGING_EN) {
      clog << " Result: " << results[i] << " Duration=" << diff.count() << endl;
    }

    if (results[i] and !results[i-1]) {
      vcpath[i] = GetVertexPath(solver, i);
      return PrintVertex(vcpath[i]);
    }
    i++;
  }

  std::cerr << "Error: UNSAT" << endl;

  string blank = "";
  return blank;
}

string VC::BinSearchVC() {
  if (!isInputValid(edges)) {
    string blank = "";
    return blank;
  }

  if (edges.empty()) {
    string blank = "";
    return blank;
  }

  int l = 0;
  int r = vertices;
  int m;

  int results[vertices];  //0 is UNSAT, 1 is SAT, -1 is undefined where index is x or vertex cover length
  vector<int> vcpath[vertices];
  fill_n(results, vertices, -1);

  while (l == r || r > l) {
    m = (r+l)/2;
    Minisat::Solver solver;

    if (LOGGING_EN) {
      clog << "Trying K=" << m;
    }
    auto start = chrono::system_clock::now();  // start measuring time

    results[m] = SolveGraph(solver, m);

    auto end = chrono::system_clock::now();    // stop measuring time
    chrono::duration<double> diff = end-start;
    if (LOGGING_EN) {
      clog << " Result: " << results[m] << " Duration=" << diff.count() << endl;
    }

    if (results[m]) {
      vcpath[m] = GetVertexPath(solver, m);
    }

    // If SAT and result[x-1] are UNSAT, the minimum is found
    if (results[m] == 1 && results[m-1] == 0 && m != 0) {
      return PrintVertex(vcpath[m]);
    }

    // If UNSAT and result[x+1] are SAT, the minimum is found
    if (results[m] == 0 && results[m+1] == 1 && m != vertices) {
      return PrintVertex(vcpath[m+1]);
    }

    if (results[m]) {
      r = m - 1;
    }
    else {
      l = m + 1;
    }
  }

  std::cerr << "Error: UNSAT" << endl;
  string blank = "";
  return blank;
}
