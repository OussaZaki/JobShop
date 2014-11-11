#include "Schedule.h"
#include <list>

//#define SCHED_DEBUG

#ifdef SCHED_DEBUG
#define sched_debug(...) fprintf(stderr, __VA_ARGS__)
#else
#define sched_debug(...) /*(__VA_ARGS__)*/
#endif

#define WARMING 1
#define COOLING 0

Schedule::Schedule(int machines)
{
	/*
		Initialisation de la queue, nous avons fixé le nombre d'opérations à zéro,
		le nombre de machines et les aretes de poids issus des sommets que nous ajoutons 0,
		à savoir le poids d'atteindre l'arete du sommet de départ.
	*/
	operations_number = 0;
	this->machines.resize(machines);
	this->vertex_weights.push_back(0);
}


Schedule::~Schedule()
{
}


void Schedule::add_job(vector<int> times, vector<int> machines)
{
	/*
		Ajout personnalisé - Vecteurs * i machines doivent être de la même longueur.
        le i-ème élément correspond * la i-ième composante de la machine.
	*/
	if (times.size() != machines.size()) // quelque chose a mal tourné:)
	{
		sched_debug("---number of times(%d) doesn't match the number of machines(%d)!\n");
		return;
	}

	int no_operations = times.size(); // le nombre d'opérations

	jobs.push_back(Job(times, machines)); // ajouter une travail à la liste des travaux
	for (int i=0; i<no_operations; i++) // pour chaque travail
	{
		operations_number++; // augmente le nombre actuel d'opérations (à noter qu'il symbolise aussi le nombre de sommets qui représente cette opération)
		this->machines[machines[i]].push_back(operations_number); // l'opération est effectuée sur la i-ième machine
		vertex_weights.push_back(times[i]); // enregistrer le poids de chaque arête provenant du sommet
	}
}


void Schedule::create_graph(void)
{
	/*
		On crée notre graphe
	*/
	Graph g(operations_number + 2); // le nombre de tache + debut + fin

	int source = 0;
	int sink = operations_number + 1; // dernier sommet

	int current_operation_vertex = 1; // à partir de sommet 1, car il contient les premières taches du premier ordre

	int jobs_number = jobs.size(); // le nombre de travaux
	for (int i=0; i<jobs_number; i++) // pour chaque travail
	{
		int job_operations = jobs[i].first.size();
		g.add_arc(0, current_operation_vertex, 0);
		for (int j=0; j<job_operations-1; j++)
		{
			g.add_arc(current_operation_vertex, current_operation_vertex+1, jobs[i].first[j]);
			current_operation_vertex++; // le sommet suivant
		}
		g.add_arc(current_operation_vertex, sink, jobs[i].first[job_operations-1]);
		current_operation_vertex++;
	}

	for (int i=0; i<machines.size(); i++) // Pour chaque machine, nous créons une clique
	{
		Machine m = machines[i]; // m = symbolisant les sommets des taches sur la i-ème machine
		int no_ops_machine = m.size(); // le nombre de travail sur la machine

		vector<int> l; // durée de fonctionnement de la i-ème machine

		for (int j=0; j<no_ops_machine; j++) // pour chaque trav,
		{
			l.push_back(vertex_weights[m[j]]);
		}

		g.create_acyclic_clique(m, l); // Créer clique acyclique

	}



	this->graph = g;
}


int Schedule::get_cmax(void)
{
	/*
		Nous calculons la longueur - le chemin critique de debut à la fin dans le graphe.
	*/
	return graph.max_distances(0).back();
}

vector<int> Schedule::get_start_times(void)
{
	/*
		Retourne l'instant de debut de tous les travaux - en fait, il s'agit simplement d'un calcul de la distance
		maximale dans le graphe à partir de la source à chaque sommet.
	*/
	vector<int> v = graph.max_distances(0);
	v.erase(v.begin(), v.begin()+1);
	v.pop_back();
	return v;
}

vector<int> Schedule::get_job_lengths(void)
{
	vector<int> v;
	for (int i=0; i<jobs.size(); i++)
		v.push_back(jobs[i].first.size());
	return v;
}

void Schedule::print_start_times(void)
{
	vector<int> times = get_start_times();
	vector<int> jlengths = get_job_lengths();
	int current_job_pos = 0;
	int current_job = 0;
	for (int i=0; i<times.size(); i++)
	{
		if (current_job_pos == jlengths[current_job])
		{
			current_job++;
			current_job_pos = 0;
			printf("\n");
		}
		printf("%d ", times[i]);
		current_job_pos++;
	}


}

bool Schedule::success_chance(int cmax, int new_cmax, double temperature, double modulation)
{
	//int chance = rand() % 2;
	double chance = ((double)rand() / (RAND_MAX));
	double result = exp((cmax - new_cmax)/(temperature * modulation));
	if(chance <= result)
		return true;
	else
		return false;

}

vector<int> Schedule::select_arc(deque<int> critpath)
{
	int dice_roll = (rand() % (critpath.size() - 2)) + 1;
	vector<int> selected_arc;
	selected_arc.resize(2);
	selected_arc[0] = critpath[dice_roll];
	selected_arc[1] = critpath[dice_roll + 1];
	return selected_arc;
}

double Schedule::solve_using_SA(double modulation, double initial_temperature, double alpha_warming, double alpha_cooling, int cooling_age_length, double warming_threshold, int max_moves_without_improvement)
{

        srand(time(NULL));
	int mode; //réchauffe // réfrigération
	int accepted_moves;
        list<bool> last_moves;

        vector<int> last_inverted_arc;
        last_inverted_arc.resize(2);

        const int last_moves_size = 500;
        last_moves.resize(last_moves_size);
        std::fill(last_moves.begin(), last_moves.end(), false);
        int accepted_moves_out_of_last_n = 0;

        bool halt = false;

	double temperature;
	int cmax;
	int best_cmax;
	Graph best_graph;
	create_graph();

	best_cmax = get_cmax();
	sched_debug("begin with cmax = %d\n", best_cmax);

	deque<int> crit_path;
	mode = WARMING;
	accepted_moves = 0;
	temperature = initial_temperature;
	vector<int> random_arc;
	random_arc.resize(2);
	int new_cmax;
	bool time_exceeded = false;
	bool cmax_is_optimal = false;
	int moves_without_improvement = 0;
	int max_temperature;

	//mesure du temps
	//struct timespec start, stop;
	time_t *start, *stop;
 	double totaltime = 0.0;
 	//clock_gettime(CLOCK_REALTIME, &start);
 	*start = time(start);
	while (moves_without_improvement < max_moves_without_improvement && !cmax_is_optimal && !time_exceeded)
	{
		//sched_debug("calculating critical path from %d to %d\n", 0, operations_number + 1);
		crit_path = graph.critical_path(0, operations_number + 1);
		cmax = get_cmax();

		int attempts = 0;
		do
		{
			random_arc = select_arc(crit_path);

			++attempts;
                        if (attempts > 100000)
                        {
                            halt = true;

                            break;
                        }
		}
		while (random_arc[0] == 0 || random_arc[1] == operations_number + 1 || (random_arc[1] - random_arc[0] == 1));

                if (halt) break;

		graph.invert_arc(random_arc[0], random_arc[1]);
                last_inverted_arc = random_arc;
		graph.set_arc_length(random_arc[1], random_arc[0], vertex_weights[random_arc[1]]);

		new_cmax = get_cmax();
		sched_debug("%d\t", new_cmax);

		if(new_cmax < cmax)
		{
                        if (mode == WARMING)
                        {
                            last_moves.push_back(true);
                            if (last_moves.front() == false)
                                accepted_moves_out_of_last_n++;
                            last_moves.pop_front();
                        }
			accepted_moves++;
			cmax = new_cmax;
			sched_debug(" +a\t");
			if (new_cmax < best_cmax)
			{
				moves_without_improvement = 0;
				best_cmax = new_cmax;
				best_graph = graph;
				if (lower_bound && upper_bound && best_cmax <= upper_bound && best_cmax >= lower_bound)
					cmax_is_optimal = true;
			}

		}
		else if (new_cmax == cmax)
		{
			accepted_moves++;
			sched_debug("==\t");
			moves_without_improvement++;
                        if (mode == WARMING)
                        {
                            last_moves.push_back(true);
                            if (last_moves.front() == false)
                                accepted_moves_out_of_last_n++;
                            last_moves.pop_front();
                        }
		}
		else
		{
			if(success_chance(cmax, new_cmax, temperature, modulation) == true)
			{
				//failed_moves = 0;
				accepted_moves++;
				cmax = new_cmax;
				sched_debug(" -a\t");
				moves_without_improvement++;
                                if (mode == WARMING)
                                {
                                    last_moves.push_back(true);
                                    if (last_moves.front() == false)
                                    {
                                        accepted_moves_out_of_last_n++;
                                    }
                                    last_moves.pop_front();
                                }
			}
			else // success_chance == false, aucune possibilité pour le trafic de succès
			{
				sched_debug(" -r\t");
				moves_without_improvement++;
				if(mode == WARMING)
				{
                                        if (accepted_moves > cooling_age_length/10)
                                        {
                                                accepted_moves = 0;
                                                temperature += alpha_warming * initial_temperature;
                                        }
                                        last_moves.push_back(false);
                                        if (last_moves.front() == true)
                                            accepted_moves_out_of_last_n--;
                                        last_moves.pop_front();
				}
				graph.invert_arc(random_arc[1], random_arc[0]);
				graph.set_arc_length(random_arc[0], random_arc[1], vertex_weights[random_arc[0]]);
			}
		}

		sched_debug("%4.2f\t", temperature);
		sched_debug(mode == WARMING ? "+" : "-");
		sched_debug("\t%d\t%d\t%4.2f\t", accepted_moves, moves_without_improvement, (double)accepted_moves_out_of_last_n / last_moves_size);


		if (mode == WARMING && (double)accepted_moves_out_of_last_n / last_moves_size >= warming_threshold)
		{
			accepted_moves = 0;
			mode = COOLING;
			max_temperature = temperature;
		}


		else if (mode == COOLING && accepted_moves >= cooling_age_length)
		{

			temperature *= alpha_cooling;
			accepted_moves = 0;
		}



        //clock_gettime(CLOCK_REALTIME, &stop);
		*stop = time(stop);
		//totaltime += (double) (stop.tv_sec - start.tv_sec)+1.e-9*(stop.tv_nsec - start.tv_nsec);
        totaltime += (double) (stop - start)+1.e-9*(stop - start);
		//si on a depassé un certain nombre de secondes
		if(totaltime > 180.0)
			time_exceeded = true;

		//clock_gettime(CLOCK_REALTIME, &start);
        *start = time(start);

		sched_debug("\n");
	}
#ifdef SCHED_DEBUG
	sched_debug("Stopped due to: ");
	if (!(moves_without_improvement < max_moves_without_improvement))
		sched_debug("moves_without_improvement >= limit\n");
	else if(time_exceeded)
		sched_debug("time exceeded\n");
	else if (cmax_is_optimal)
		sched_debug("cmax_is_optimal\n");
#endif

	graph = best_graph;

	printf("%d\n", get_cmax());
	print_start_times();
	printf("\n");
	return totaltime;
}
