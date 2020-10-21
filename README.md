# *Multithreaded approach for efficiency analysis of three Minimum Vertex Cover algorithms*
## ECE 650 [Methods and Tools for Software Engineering] Project

## Overview

-   Implemented three algorithms to solve MINIMUM-VERTEX-COVER problem
    - **CNF-SAT-VC** - Polynomial reduction of the decision version of VERTEX COVER to CNF-SAT.
    - **APPROX-VC-1** - Picked a vertex of highest degree (most incident edges). Added it to the vertex cover and threw away all edges incident on that vertex. Repeated till no edges remain. This algorithm is APPROX-VC-1.
    - **APPROX-VC-2** - Picked an edge <u, v>, and added both u and v to the vertex cover. Threw away all edges attached to u and v. Repeated till no edges remain. This algorithm is APPROX- VC-2.

## CMake to build the project:

    cd PROJECT && mkdir build && cd build && cmake ../ && make

where PROJECT is the top level directory.
