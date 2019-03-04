#include "ttp.h"

enum {ERR_USAGE = 1, ERR_NTEAM, ERR_RSEED, ERR_FILENAME, ERR_GENSCHED};

int main(int argc, char **argv) {
	int num_teams, valid;
	unsigned int seed = 0;
	char *tmp, *filename;
	Schedule *s;
	Settings settings;
	if (argc < 2) {
		printf("Usage: %s (# of teams) <seed>\n", argv[0]);
		return ERR_USAGE;
	}
	num_teams = strtol(argv[1], &tmp, 10);
	if (tmp == argv[1] || num_teams < 3 || num_teams % 2) {
		printf("Error: Number of teams must be even and greater than 3\n");
		return ERR_NTEAM;
	}
	if (argc > 2) {
		seed = strtoul(argv[2], &tmp, 10);
		if (tmp == argv[2]) {
			printf("Error: Seed must be a positive integer\n");
			return ERR_RSEED;
		}
	}

	settings.temp = 100;
	settings.beta = 0.9; 
	settings.weight = 1;
	settings.theta = settings.delta = 3; 
	settings.max_reheat = 5;
	settings.max_phase = 100;
	settings.max_counter = 100;

	srand(seed);

	printf("Building random schedule for %d teams with seed %d\n", num_teams, seed);

	filename = calloc(snprintf(NULL, 0, "data/NL%d.data", num_teams) + 1, sizeof(*filename));

	sprintf(filename, "data/NL%d.data", num_teams);

	s = CreateSchedule(num_teams);
	GenerateSchedule(s);
	if (!InitCost(s, filename)) {
		printf("Unable to read file %s\n", filename);
		free(filename);
		DeleteSchedule(s);
		return ERR_FILENAME;
	} 


	free(filename);
	if (CheckHardReq(s)) {
		printf("Invalid Schedule Generated\n");
		DeleteSchedule(s);
		return ERR_GENSCHED;
	}

	Anneal(s, settings);
	valid = CheckHardReq(s);
	valid |= CheckSoftReq(s, NULL);
	if (valid) {
		if (valid & SCHED_INVALID) {
			printf("Schedule is invalid\n");
		}
		if (valid & SCHED_ATMOST) {
			printf("Schedule violates atmost contraint.\n");
		}
		if (valid & SCHED_REPEAT) {
			printf("Schedule violates repeat contraint.\n");
		}
	} else {
		printf("Valid Schedule!\n");
		printf("Cost: %lu\n", s->cost.total_cost);
		PrintSchedule(s, TEAM_NAMES);
	}

	DeleteSchedule(s);

	return 0;
}