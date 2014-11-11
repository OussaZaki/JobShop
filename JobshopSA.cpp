#include <cstdio>
#include <cctype>
#include <list>
#include "Graph.h"
#include "Schedule.h"
#define PB push_back
#define INST_BEASLEY 0
#define INST_TAILLARD 1

using namespace std;

int main(int argc, char* argv[])
{
    double modulation = 0.85;
    double initial_temperature = 50;
    double alpha_warming = 2;
    double alpha_cooling = 0.85;
    int cooling_age_length = 300;
    double warming_threshold = 0.95;
    int max_moves_without_improvement = 2000;

	int instance_format = INST_BEASLEY;
	int limit = 0;
	double totaltime;
	//Interprétation des arguments de ligne de commande
	/*
		SELECT TAILLE INSTANCE
		jobshop inst_file - Beasley (default)
		jobshop inst_file {T|B} - Taillard/Beasley
		jobshop inst_file {T|B} n - Beasley ou Taillard limitées aux premières tâches de n
	*/
	if (argc == 1)
	{
		printf("usage: jobshop inst_file [ {T|B} {0..infinity} ]\n");
		return 0;
	}
	else
	{
		if (argc >= 3)
		{
			if (tolower(argv[2][0]) == 't') instance_format = INST_TAILLARD;
			else if (tolower(argv[2][0]) == 'b') instance_format = INST_BEASLEY;
			else printf("Format de paramètre non valide, par exemple - par défaut Beasley\n");
			if (argc >= 4)
            {
				if (atoi(argv[3]) != 0) limit = atoi(argv[3]);
            }
		}

	}

	// Ouverture du fichier énoncées dans un argument
	FILE *source;
	if((source = fopen(argv[1], "r")) == NULL)
	{
		printf("Erreur d'ouverture du fichier!\n");
		return 1;
	}

	 // NOMBRE DE TRAVAIL ET ÉQUIPEMENT - lire la 1ère ligne
	int noJobs, noMachines;
	long int escape;
	int lower_bound = 0, upper_bound = 0;
        if (instance_format == INST_TAILLARD)
                fscanf(source, "%d %d %ld %ld %d %d", &noJobs, &noMachines, &escape, &escape, &lower_bound, &upper_bound);
        else
            fscanf(source, "%d %d", &noJobs, &noMachines);

	//s - rang
	Schedule s(noMachines);
	s.lower_bound = lower_bound;
	s.upper_bound = upper_bound;

	//Vecteurs m - machines, t - temps -> pour l'emploi
	vector<int> m, t;
 	// Beasley
 	if (instance_format == INST_BEASLEY)
 	{
 		// Si il existe une limite sur le nombre de tâches programmées:
 		if (limit > 0 && limit <= noJobs)
 			noJobs = limit;
 		int i, j;
		for(int i = 0; i < noJobs; i++)
		{
			m.clear();
			t.clear();
			//parce que nous lisons en meme temps la machine et le temps,
			//alors nous pouvons utiliser deux vecteurs auxiliaires pour chaque tâche.
			for(int j = 0; j < noMachines; j++)
			{
				int machine_tmp, time_tmp;
				fscanf(source, "%d %d ", &machine_tmp, &time_tmp);
				m.push_back(machine_tmp);
				t.push_back(time_tmp);
			}
			s.add_job(t, m);
		}
	}

	// Taillard
	else if (instance_format == INST_TAILLARD)
	{
		// le nombre de tâches est d'omissions
		int redundant_tasks =
			(limit > 0 && limit <= noJobs)
			? noJobs - limit
			: 0;

		noJobs -= redundant_tasks;

		//tableau 2D (vecteur de vecteur) pour stocker les temps nécessaires
		//uniquement aux instances de Taillard (nous avons besoin à la fois
        //de se rappeler de tous les temps, contrairement à Beasley)
		vector<vector<int> > tailard_times;
		//redimensionner: le nombre de lignes est le nombre de job,
        // Nombre de colonnes n'est pas défini (parce que nous faisons des push_back)
		tailard_times.resize(noJobs);
		for(int i = 0; i < noJobs; i++)
		{
			tailard_times[i].resize(0);
		}

		for(int i = 0; i < noJobs; i++)
		{
			tailard_times[i].clear();
		}
		// noJobs et noMachines déjà proclamés, maintenant sauter la première ligne
		char skip[256];
		void *result = fgets(skip, 256, source);
		// On saute la line "Times"
		result = fgets(skip, 100, source);

		for(int i = 0; i < noJobs; i++)
		{
			for(int j = 0; j < noMachines; j++)
			{
				int time_tmp;
				fscanf(source, "%d ", &time_tmp);
				tailard_times[i].push_back(time_tmp);
			}
		}
		// ignorer le reste des tâches
		for (int i = 0; i < redundant_tasks; i++)
		{
			result = fgets(skip, 256, source);
		}
		// On saute la line "Machines"
		result = fgets(skip, 100, source);
		for(int i = 0; i < noJobs; i++)
		{
			m.clear();
			//Voici vecteur m est également utilisé de nombreuses fois
			for(int j = 0; j < noMachines; j++)
			{
				int machine_tmp;
				fscanf(source, "%d ", &machine_tmp);
				m.push_back(machine_tmp-1);
				// -1 Parce que dans les cas des machines de tailard sont numérotées de 1
			}
			s.add_job(tailard_times[i], m);
		}
		// ignorer le reste des tâches
		for (int i = 0; i < redundant_tasks; i++)
		{
			result = fgets(skip, 256, source);
		}
		printf("etape 4!\n");
	}

	if(fclose(source) != 0)
		printf("Erreur de fichier de clôture %s!\n", argv[1]);

	//s'il ya une seule réponse, la solution est simple
	if(noJobs == 1)
	{
		s.create_graph();
		printf("%d\n", s.get_cmax());
		s.print_start_times();
		printf("\n");
	}
	//Procédé d'une solution de recuit simulé
	else{
        printf("etape 5!\n");
        totaltime = s.solve_using_SA(modulation, initial_temperature, alpha_warming, alpha_cooling, cooling_age_length, warming_threshold, max_moves_without_improvement);
        printf("etape 6!\n");
	}
	//enregistrer les résultats dans un fichier qualitative
	char buffer[] = "resultats.txt";
	FILE *times;
	times = fopen(buffer, "a");
	//fprintf(stderr, "\nFichier ouvert: %s\n", buffer);
	fprintf(times, "%s\t%d\t%d\t%f\t%d", argv[1], noJobs, noMachines, totaltime, s.get_cmax());
	if(instance_format == INST_TAILLARD)
		fprintf(times, "\t%d\t%d\n", lower_bound, upper_bound);
	else
		fprintf(times, "\n");

	if(fclose(times) != 0)
		printf("Erreur de fichier de clôture %s!\n", buffer);

	//fprintf(stderr, "traité %s\n", argv[1]);

	return 0;
}

