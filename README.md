JobShop
=======

Simulated annealing heuristic for JobShop scheduler problem
After compiling: Graph.cpp, Graph.h, Schedule.cpp, Schedule.h, JobshopSA.cpp into executable file. 
Use this command line to execute the SA algo:

JobshopSA instance-file [ {T|B} {0..infinity} ]
T|B - Taillard's or Beasley's instance; 

Simple exemple:
JobshopSA ./instances/tailard/tai01.txt	T 
15	15	498390099.583784	9760	1231	1005
JobshopSA ./instances/tailard/tai02.txt	T
15	15	498390099.583789	9248	1244	953


