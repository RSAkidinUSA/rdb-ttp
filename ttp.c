#include "ttp.h"

// randomizes an array of teams to play
static void randomize_order(int *order, int numVals) {
	for (int i = 0; i < numVals; i++) {
		unsigned index = rand() % numVals;
		int temp = order[index];
		order[index] = order[i];
		order[i] = temp;
	}
}

// Create a new schedule for N teams with random matchups
Schedule *create_schedule(int numTeams) {
	Schedule *s = calloc(1, sizeof(*s));
	s->numTeams = numTeams;
	s->numRounds = (numTeams * 2) - 2;
	s->round = calloc(s->numRounds, sizeof(*(s->round)));
	// allocate memory
	for (int i = 0; i < s->numRounds; i++) {
		s->round[i] = calloc(1, sizeof(*(s->round[i])));
		s->round[i]->team = calloc(s->numTeams + 1, sizeof(*(s->round[i]->team)));
	}
	// fill teams
	for (int i = 1; i < s->numTeams; i++) {
		int numVals = s->numRounds - (2 * (i - 1));
		int *order = calloc(numVals, sizeof(*order));
		for (int j = 0; j < numVals; j++) {
			if (j < numVals / 2) {
				order[j] = j + i + 1;
			} else {
				order[j] = (numVals / 2) - j - i - 1;
			}
		}
		randomize_order(order, numVals);
		int k = 0;
		for (int j = 0; j < s->numRounds; j++) {
			if (s->round[j]->team[i] == 0) {
				int index = abs(order[k]);
				while (s->round[j]->team[index] != 0) {
					k = (k + 1) % numVals;
					index = abs(order[k]);
				}
				s->round[j]->team[i] = order[k];
				int val = (order[k] > 0) ? -i : i;
				s->round[j]->team[index] = val;
				k = (k + 1) % numVals;
			}
		}

		free(order);
	}
	return s;
}

// Delete a schedule and free all memory
void delete_schedule(Schedule *s) {
	for (int i = 0; i < s->numRounds; i++) {
		free(s->round[i]->team);
		free(s->round[i]);
	}
	free(s->round);
	free(s);
}

// Print a schedule
void print_schedule(Schedule *s) {
	for (int i = 0; i < s->numRounds; i++) {
		printf("\t|\t%d", i + 1);
	}
	printf("\t|\n");
	for (int i = 1; i <= s->numTeams; i++) {
		printf("%d", i);
		for (int j = 0; j < s->numRounds; j++) {
			printf("\t|\t%d", s->round[j]->team[i]);
		}
		printf("\t|\n");
		fflush(stdout);
	}
}

// Determine if a schedule is valid
// returns 0 if valid, SCHED_ATMOST if the atmost contraint fails
// SCHED_REPEAT if the norepeat contraint fails. These values can OR together
int valid_schedule(Schedule *s) {
	int retval = 0;
	// keeps track of number of consecutive games
	int *atmostCount = calloc(s->numTeams, sizeof(*atmostCount));
	// keeps track of location of consectutive games
	int *atmostVal = calloc(s->numTeams, sizeof(*atmostVal));
	// keeps track of last game played
	int *repeatVal = calloc(s->numTeams, sizeof(*repeatVal));
	for (int i = 0; i < s->numRounds; i++) {
		for (int j = 1; j < s->numTeams; j++) {
			// check atmost
			if (atmostVal[j] > 0 && s->round[i]->team[j] > 0) {
				atmostCount[j]++;
			} else if (atmostVal[j] < 0 && s->round[i]->team[j] < 0) {
				atmostCount[j]++;
			} else {
				atmostCount[j] = 1;
				atmostVal[j] = (s->round[i]->team[j] > 0) ? 1 : -1;
			}
			if (atmostCount[j] > 3) {
				retval |= SCHED_ATMOST;
			}
			// check repeats
			if (repeatVal[j] == abs(s->round[i]->team[j])) {
				retval |= SCHED_REPEAT;
			}
			repeatVal[j] = abs(s->round[i]->team[j]);
		}
	}
	free(repeatVal);
	free(atmostVal);
	free(atmostCount);
	return retval;
}