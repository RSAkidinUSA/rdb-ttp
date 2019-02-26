#include <stdio.h>
#include <stdlib.h>

enum {ERR_USAGE = 1, ERR_NTEAM, ERR_RSEED};

int main(int argc, char **argv) {
	int numTeams;
	unsigned int seed = 0;
	char *temp;
	if (argc < 2) {
		printf("Usage: %s (# of teams) <seed>\n", argv[0]);
		return ERR_USAGE;
	}
	numTeams = strtol(argv[1], &temp, 10);
	if (temp == argv[1] || numTeams < 3) {
		printf("Error: Number of teams must be greater than 2\n");
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

	printf("Building random schedule for %d teams with seed %d: %d\n", numTeams, seed, rand());
	return 0;
}