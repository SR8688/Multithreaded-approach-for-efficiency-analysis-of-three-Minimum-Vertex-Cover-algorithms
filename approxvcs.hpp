#pragma once

#include <iostream>
#include <vector>
#include "cnfsatvc.hpp"

using namespace std;

vector< vector<int> > CreateAdjacencyList(int v, vector< pair<int,int> > edges);
vector< vector<int> > CreateAdjacencyMat(int v, vector< pair<int,int> > edges);
bool isInputValid(int v, vector< pair<int,int> > edges);
string ApproxVC1(int v, vector< pair<int,int> > edges);
string ApproxVC2(int v, vector< pair<int,int> > edges);
