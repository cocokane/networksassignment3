# Computer Networks Assignment 3
# This repository contains the code and documentation for the Computer Networks Assignment 3, which includes network simulation and analysis using Mininet and custom routing algorithms.
By Yash Kokane - 20110237

## Execution Instructions

### Question 1: Network Loops
This section implements a network topology using Mininet and examines network behavior with loop detection and spanning tree protocol.

**To execute the code for part a (We get an error, due to Loop-induced ping failures):**
```bash
sudo python3 q1_a.py
```
**To execute the code for part b (Fixed error using STP):**
```bash
sudo python3 q1_a.py
```

### Question 2: Configure Host-based NAT
Here, we configure a host-based NAT (Network Address Translation) to allow communication between two hosts in different subnets.

**To execute the code:**
```bash
sudo python3 q2.py
```

### Question 3: Distance Vector Routing Simulation
This section implements and analyzes the Distance Vector Routing Algorithm on a simulated network with asymmetric link costs.

**To compile the simulation code:**
```bash
gcc distance_vector.c node0.c node1.c node2.c node3.c -o dvsim
```

**To execute the code:**
```bash
./dvsim 2
```

**Analysis Procedure:**
1. Observe the distance table convergence through printed output and debug logs.
2. Validate the simulation results with the provided analysis in the report.

## Requirements
- Linux OS (Ubuntu preferred)
- Mininet installed
- Python 3.x
- GCC compiler for C code compilation
- Ensure you have root privileges to run Mininet commands.

