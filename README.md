JobShop
=======

Simulated annealing heuristic for JobShop scheduler problem <br>
After compiling: Graph.cpp, Graph.h, Schedule.cpp, Schedule.h, JobshopSA.cpp into executable file. <br>
Use this command line to execute the SA algo: <br>

JobshopSA instance-file [ {T|B} {0..infinity} ]<br>
T|B - Taillard's or Beasley's instance; <br>

Simple exemple: <br>
JobshopSA ./instances/tailard/tai01.txt	T <br>
Output: 15	15	498390099.583784	9760	1231	1005 <br>

JobshopSA ./instances/tailard/tai02.txt	T <br>
Output: 15	15	498390099.583789	9248	1244	953 <br>

