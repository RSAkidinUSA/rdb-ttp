#include "ttp.h"

enum {ERR_USAGE = 1, ERR_NTEAM, ERR_RSEED};

const char *const TEAM_NAMES[] = {
		"ATL","NYM","PHI","MON",\
		"FLA","PIT","CIN","CHI",\
		"STL","MIL","HOU","COL",\
		"SF", "SD", "LA", "ARI", 0
};

int main(int argc, char **argv) {
	int num_teams, valid;
	unsigned int seed = 0;
	char *temp, *filename;
	Schedule *s;
	if (argc < 2) {
		printf("Usage: %s (# of teams) <seed>\n", argv[0]);
		return ERR_USAGE;
	}
	num_teams = strtol(argv[1], &temp, 10);
	if (temp == argv[1] || num_teams < 3 || num_teams % 2) {
		printf("Error: Number of teams must be even and greater than 3\n");
		return ERR_NTEAM;
	}
	if (argc > 2) {
		seed = strtoul(argv[2], &temp, 10);
		if (temp == argv[2]) {
			printf("Error: Seed must be a positive integer\n");
			return ERR_RSEED;
		}
	}

	srand(seed);

	printf("Building random schedule for %d teams with seed %d\n", num_teams, seed);

	filename = calloc(snprintf(NULL, 0, "data/NL%d.data", num_teams) + 1, sizeof(*filename));

	sprintf(filename, "data/NL%d.data", num_teams);

	s = CreateSchedule(num_teams);
	int cost = InitCost(s, filename);
	if (cost == 0) {
		printf("Unable to read file %s\n", filename);
	} else {
		printf("Total Cost: %d\n", cost);
	}

	free(filename);

	PrintSchedule(s, TEAM_NAMES);
	valid = CheckHardReq(s);
	valid |= CheckSoftReq(s);
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
	}

	DeleteSchedule(s);

	return 0;
}