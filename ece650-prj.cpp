#include <minisat/core/SolverTypes.h>
#include <minisat/core/Solver.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex>
#include <vector>
#include <list>
#include <iostream>
#include <fstream>
#include "cnfsatvc.hpp"
#include "approxvcs.hpp"

using namespace std;

#define handle_error(msg) \
do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error_en(en, msg) \
do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

std::string NAME = "0";
bool OUT_TO_FILE = false;
bool LOG_EN = false;

struct task {
  int vertices;
  vector< pair<int,int> > edges;
};


pthread_mutex_t taskq1mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t taskq2mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t taskq3mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t resultq1mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t resultq2mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t resultq3mutex = PTHREAD_MUTEX_INITIALIZER;

list<task*> taskq1;
list<task*> taskq2;
list<task*> taskq3;

list<string> resultq1;
list<string> resultq2;
list<string> resultq3;


vector<string>ExtractEdgeValues(string str, string del){
  vector<string> edges;

  auto start = 0U;
  auto end = str.find(del);
  while (end != string::npos)
  {
    edges.push_back(str.substr(start, end - start));
    start = end + del.length();
    end = str.find(del, start);
  }

  edges.push_back(str.substr(start, end));
  return edges;
}

vector< pair<int,int> > GetEdges(string s){

  pair<int, int> edge;
  vector< pair<int,int> > result;

  try {
    vector<string> edge_list = ExtractEdgeValues(s, "{<");
    edge_list = ExtractEdgeValues(edge_list[1], ">}");
    edge_list = ExtractEdgeValues(edge_list[0], ">,<");

    for (unsigned int i = 0; i < edge_list.size(); i++) {
      vector<string> edgeVector = ExtractEdgeValues(edge_list[i], ",");
      int v1 = stoi(edgeVector[0]);
      int v2 = stoi(edgeVector[1]);
      edge.first = v1;
      // cout << edge.first;
      edge.second = v2;
      // cout << edge.second;

      result.push_back(edge);
    }

  }catch (...){
    // std::cerr << "Error: Invalid Input" << '\n';
    result.clear();
  }
  return result;
}


string CNFSATVC(int vertices, vector< pair<int,int> > edges) {
  string result;
  VC vc = VC(vertices, edges);
  result = vc.LinSearchVC();

  return result;
}

void AddToQ1(int vertices, vector< pair<int,int> > parsed_edges){

  struct task* intask;
  intask = new task;
  intask->vertices = vertices;
  intask->edges = parsed_edges;
  pthread_mutex_lock (&taskq1mutex);
  taskq1.push_back(intask);
  pthread_mutex_unlock (&taskq1mutex);

  return;

}

void AddToQ2(int vertices, vector< pair<int,int> > parsed_edges){

  struct task* intask;
  intask = new task;
  intask->vertices = vertices;
  intask->edges = parsed_edges;
  pthread_mutex_lock (&taskq2mutex);
  taskq2.push_back(intask);
  pthread_mutex_unlock (&taskq2mutex);

  return;

}

void AddToQ3(int vertices, vector< pair<int,int> > parsed_edges){

  struct task* intask;
  intask = new task;
  intask->vertices = vertices;
  intask->edges = parsed_edges;
  pthread_mutex_lock (&taskq3mutex);
  taskq3.push_back(intask);
  pthread_mutex_unlock (&taskq3mutex);

  return;

}


void* HandlerIO(void* args) {
  char cmd;
  int vertices = 0;
  string edge_string;
  vector< pair<int,int> > parsed_edges;

  while(cin >> cmd){

    if(cmd == 'V' || cmd == 'v'){
      cin >> vertices;
      // cout << "V " << vertices << endl;
      if (vertices < 0) {
        std::cerr << "Error: Invalid Vertex" << endl;
        vertices = 0;
      }
      cin.clear();
      cin.ignore(numeric_limits<streamsize>::max(), '\n');

    }
    else if(cmd == 'E' || cmd == 'e'){
      cin >> edge_string;
      // cout << "E " << edge_string << endl;
      parsed_edges = GetEdges(edge_string);
      // cout << "cnfsatvc: " << CNFSATVC(vertices, parsed_edges) << endl;
      // cout << "APPROX-VC-1: " << ApproxVC1(vertices, parsed_edges) << endl;
      // cout << "APPROX-VC-2: " << ApproxVC2(vertices, parsed_edges) << endl;

      AddToQ1(vertices, parsed_edges);
      AddToQ2(vertices, parsed_edges);
      AddToQ3(vertices, parsed_edges);

      cin.clear();
      cin.ignore(numeric_limits<streamsize>::max(), '\n');

    }

    else{
      cin.clear();
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
      std::cerr << "Error: Invalid Command" << endl;

    }
  }
  return NULL;
}


void* HandlerOP(void* args) {

  bool termination = false;

  while(1) {
    pthread_mutex_lock (&resultq1mutex);
    pthread_mutex_lock (&resultq2mutex);
    pthread_mutex_lock (&resultq3mutex);

    // write results to console in required order
    while(!resultq1.empty() && !resultq2.empty() && !resultq3.empty()) {
      string result;

      result = resultq1.front();
      resultq1.pop_front();
      cout << result << endl;

      result = resultq2.front();
      resultq2.pop_front();
      cout << result << endl;

      result = resultq3.front();
      resultq3.pop_front();
      cout << result << endl;
    }
    if ( (*((bool*)args+1) && resultq1.empty() && resultq2.empty() && resultq3.empty())
    || *((bool*)args+2) ) { // check if cnf_sat ignore flag was passed as argument
      termination = true;
    }

    pthread_mutex_unlock (&resultq1mutex);
    pthread_mutex_unlock (&resultq2mutex);
    pthread_mutex_unlock (&resultq3mutex);

    if (termination)
    break;
  }

  return NULL;
}


void* CalcCNFSATVC(void* args) {

  clockid_t clock_id;
  int taskno = 1;
  bool first = true;
  bool termination = false;
  ofstream outfile;
  string file_name = NAME + "_cnfsatvc.csv";
  if (OUT_TO_FILE) { outfile.open(file_name, ios_base::app); }

  while (1) {
    struct task* retrieved_task = NULL;
    string result;

    // get task from queue, use mutex for thread safety
    pthread_mutex_lock (&taskq1mutex);
    if (!taskq1.empty()) {
      retrieved_task = taskq1.front();
      taskq1.pop_front();

      // time start
      pthread_getcpuclockid(pthread_self(), &clock_id);
      struct timespec ts_start;
      clock_gettime(clock_id, &ts_start);

      // compute result
      result = "cnfsatvc: " + CNFSATVC(retrieved_task->vertices, retrieved_task->edges);

      //time end
      struct timespec ts_end;
      clock_gettime(clock_id, &ts_end);
      long double elapsed_time_us = ((long double)ts_end.tv_sec*1000000 + (long double)ts_end.tv_nsec/1000.0) - ((long double)ts_start.tv_sec*1000000 + (long double)ts_start.tv_nsec/1000.0);

      // output result to terminal if LOG_EN option
      if (LOG_EN) {
        clog << "cnfsatvc,task_num," << taskno << ","
        << "time_us," << elapsed_time_us << endl;
      }

      // write to file
      if (OUT_TO_FILE) {
        if (first) {
          outfile << elapsed_time_us;
          first = false;
        }
        else {
          outfile << "," << elapsed_time_us;
        }
      }

      //write result to result queue, use mutex for thread safety
      pthread_mutex_lock (&resultq1mutex);
      resultq1.push_back(result);
      pthread_mutex_unlock (&resultq1mutex);

      taskno++;

    }
    // exit loop when HandlerIO see EOF (flag is set in int main after thread joins)
    if(*(bool*)args && taskq1.empty()) {
      termination = true;
      outfile << endl;
    }
    pthread_mutex_unlock (&taskq1mutex);

    // cleanup memory taken by task
    delete retrieved_task;
    if (termination) {break;}
  }

  return NULL;
}


void* CalcApproxVC1(void* args) {

  clockid_t clock_id;
  int taskno = 1;
  bool first = true;
  bool termination = false;
  ofstream outfile;
  string file_name = NAME + "_APPROX-VC-1.csv";
  if (OUT_TO_FILE) { outfile.open(file_name, ios_base::app); }

  while (1) {
    struct task* retrieved_task = NULL;
    string result;

    // get task from queue, use mutex for thread safety
    pthread_mutex_lock (&taskq2mutex);
    if (!taskq2.empty()) {
      retrieved_task = taskq2.front();
      taskq2.pop_front();

      // time start
      pthread_getcpuclockid(pthread_self(), &clock_id);
      struct timespec ts_start;
      clock_gettime(clock_id, &ts_start);

      // compute result
      result = "APPROX-VC-1: " + ApproxVC1(retrieved_task->vertices, retrieved_task->edges);

      //time end
      struct timespec ts_end;
      clock_gettime(clock_id, &ts_end);
      long double elapsed_time_us = ((long double)ts_end.tv_sec*1000000 + (long double)ts_end.tv_nsec/1000.0) - ((long double)ts_start.tv_sec*1000000 + (long double)ts_start.tv_nsec/1000.0);

      // output result to terminal if LOG_EN option
      if (LOG_EN) {
        clog << "APPROX-VC-1,task_num," << taskno << ","
        << "time_us," << elapsed_time_us << endl;
      }

      // write to file
      if (OUT_TO_FILE) {
        if (first) {
          outfile << elapsed_time_us;
          first = false;
        }
        else {
          outfile << "," << elapsed_time_us;
        }
      }

      //write result to result queue, use mutex for thread safety
      pthread_mutex_lock (&resultq2mutex);
      resultq2.push_back(result);
      pthread_mutex_unlock (&resultq2mutex);

      taskno++;
    }
    // exit loop when HandlerIO see EOF (flag is set in int main after thread joins)
    if(*(bool*)args && taskq2.empty()) {
      termination = true;
      outfile << endl;
    }
    pthread_mutex_unlock (&taskq2mutex);

    // cleanup memory taken by task
    delete retrieved_task;
    if (termination) {break;}

  }

  return NULL;
}


void* CalcApproxVC2(void* args) {

  clockid_t clock_id;
  int taskno = 1;
  bool first = true;
  bool termination = false;
  ofstream outfile;
  string file_name = NAME + "_APPROX-VC-2.csv";
  if (OUT_TO_FILE) { outfile.open(file_name, ios_base::app); }

  while (1) {
    struct task* retrieved_task = NULL;
    string result;

    // get task from queue, use mutex for thread safety
    pthread_mutex_lock (&taskq3mutex);
    if (!taskq3.empty()) {
      retrieved_task = taskq3.front();
      taskq3.pop_front();

      // time start
      pthread_getcpuclockid(pthread_self(), &clock_id);
      struct timespec ts_start;
      clock_gettime(clock_id, &ts_start);

      // compute result
      result = "APPROX-VC-2: " + ApproxVC2(retrieved_task->vertices, retrieved_task->edges);

      // time end
      struct timespec ts_end;
      clock_gettime(clock_id, &ts_end);
      long double elapsed_time_us = ((long double)ts_end.tv_sec*1000000 + (long double)ts_end.tv_nsec/1000.0) - ((long double)ts_start.tv_sec*1000000 + (long double)ts_start.tv_nsec/1000.0);

      // output result to terminal if LOG_EN option
      if (LOG_EN) {
        clog << "APPROX-VC-2,task_num," << taskno << ","
        << "time_us," << elapsed_time_us << endl;
      }

      // write to file if option selected
      if (OUT_TO_FILE) {
        if (first) {
          outfile << elapsed_time_us;
          first = false;
        }
        else {
          outfile << "," << elapsed_time_us;
        }
      }

      //write result to result queue, use mutex for thread safety
      pthread_mutex_lock (&resultq3mutex);
      resultq3.push_back(result);
      pthread_mutex_unlock (&resultq3mutex);


      taskno++;
    }
    // exit loop when HandlerIO see EOF (flag is set in int main after thread joins)
    if(*(bool*)args && taskq3.empty()) {
      termination = true;
      outfile << endl;
    }

    pthread_mutex_unlock (&taskq3mutex);

    // cleanup memory taken by task
    delete retrieved_task;
    if (termination) {break;}
  }

  return NULL;
}

void DoThreading(bool endflag[3], bool isnotSAT, char **argv){

  pthread_t ThreadIO;
  pthread_t ThreadOut;
  pthread_t CNFSATThread;
  pthread_t ApproxVC1Thread;
  pthread_t ApproxVC2Thread;
  pthread_create (&ThreadIO, NULL, &HandlerIO, (void *) endflag);
  pthread_create (&ThreadOut, NULL, &HandlerOP, (void *) endflag);

  if (!isnotSAT) {
    pthread_create (&CNFSATThread, NULL, &CalcCNFSATVC, (void *) endflag);
  }
  pthread_create (&ApproxVC1Thread, NULL, &CalcApproxVC1, (void *) endflag);
  pthread_create (&ApproxVC2Thread, NULL, &CalcApproxVC2, (void *) endflag);

  pthread_join (ThreadIO, NULL);
  endflag[0] = true;

  if (!isnotSAT) {
    pthread_join (CNFSATThread, NULL);
  }
  pthread_join (ApproxVC1Thread, NULL);
  pthread_join (ApproxVC2Thread, NULL);
  endflag[1] = true;
  pthread_join (ThreadOut, NULL);

  return;
}
int main(int argc, char **argv) {
  bool endflag[3] = {false};

  opterr = 0;
  string tmp;
  bool isnotSAT = false;
  int c;

  // expected options are '-o' [out to file], '-n value' [name to prepend filename], '-i' [ignore cnfsat]
  while ((c = getopt (argc, argv, "on:il?")) != -1)
  if(c == 'o'){
    OUT_TO_FILE = true;
  }
  else if(c == 'n'){
    NAME = optarg;
  }
  else if(c == 'i'){
    isnotSAT = true;
    endflag[2] = true;

  }
  else if(c == 'l'){
    LOG_EN = true;
  }
  else{
    return 0;
  }
  // cout << "r=" << NAME << "o=" << OUT_TO_FILE << "i=" << isnotSAT << "l=" << LOG_EN << endl;
  DoThreading(endflag, isnotSAT, argv);
  return 0;
}
