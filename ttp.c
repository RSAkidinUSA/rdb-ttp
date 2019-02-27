#include "ttp.h"
#include <stdbool.h>
#include <string.h>

// check if the schedule is empty
static bool ScheduleEmpty(Schedule *s) {
	if (s->set_vals == s->num_rounds * s->num_teams) {
		return true;
	} else {
		return false;
	}
}

static int SetChoices(int *order, Schedule *s, int t) {
	int n = 0;

	// positives
	for (int i = 1; i <= s->num_teams; i++) {
		if (i != t) {
			order[n++] = i;
			for (int k = 0; k < s->num_rounds; k++) {
				if (s->round[k]->team[t] == i) {
					order[--n] = 0;
					break;
				}
			}
		}
	}
	// negatives
	for (int i = 1; i <= s->num_teams; i++) {
		if (i != t) {
			order[n++] = -i;
			for (int k = 0; k < s->num_rounds; k++) {
				if (s->round[k]->team[t] == -i) {
					order[--n] = 0;
					break;
				}
			}
		}
	}

	return n;
}

// randomize the order for num_rounds for team t
static void RandomizeOrder(int *order, int num_choices) {
	for (int i = 0; i < num_choices; i++) {
		unsigned index = rand() % num_choices;
		int tmp = order[index];
		order[index] = order[i];
		order[i] = tmp;
	}
}

// generate a random schedule
static bool GenerateSchedule(Schedule *s) {
	bool retval = false;
	if (ScheduleEmpty(s)) {
		return true;
	}
	int t, w;
	// find the smallest team for the smallest week
	for (w = 0; w < s->num_rounds; w++) {
		for (t = 1; t <= s->num_teams; t++) {
			if (s->round[w]->team[t] == 0) {
				break;
			}
		}
		if (t <= s->num_teams) {
			break;
		}
	}
	int *order = calloc(s->num_rounds, sizeof(*order));
	int num_choices = SetChoices(order, s, t);
	RandomizeOrder(order, num_choices);
	for (int i = 0; i < s->num_rounds; i++) {
		int choice = order[i];
		if (choice != 0 && s->round[w]->team[abs(choice)] == 0) {
			s->round[w]->team[t] = choice;
			s->round[w]->team[abs(choice)] = (choice > 0) ? -t : t;
			s->set_vals += 2;
			if (GenerateSchedule(s)) {
				retval = true;
				break;
			} else {
				s->set_vals -= 2;
				s->round[w]->team[t] = 0;
				s->round[w]->team[abs(choice)] = 0;
			}
		}
	}
	free(order);
	//printf("Returning\n");
	return retval;
}

// Create a new schedule for N teams with random matchups
Schedule *CreateSchedule(int num_teams) {
	Schedule *s = calloc(1, sizeof(*s));
	s->num_teams = num_teams;
	s->num_rounds = (num_teams * 2) - 2;
	s->round = calloc(s->num_rounds, sizeof(*(s->round)));

	// allocate memory
	for (int i = 0; i < s->num_rounds; i++) {
		s->round[i] = calloc(1, sizeof(*(s->round[i])));
		s->round[i]->team = calloc(s->num_teams + 1, sizeof(*(s->round[i]->team)));
	}
	// fill teams
	GenerateSchedule(s);

	return s;
}

// Delete a schedule and free all memory
void DeleteSchedule(Schedule *s) {
	for (int i = 0; i < s->num_rounds; i++) {
		free(s->round[i]->team);
		free(s->round[i]);
	}
	free(s->round);
	free(s);
}

// Print a schedule
void PrintSchedule(Schedule *s) {
	for (int i = 0; i < s->num_rounds; i++) {
		printf("\t|\t%d", i + 1);
	}
	printf("\t|\n");
	for (int i = 1; i <= s->num_teams; i++) {
		printf("%d", i);
		for (int j = 0; j < s->num_rounds; j++) {
			printf("\t|\t%d", s->round[j]->team[i]);
		}
		printf("\t|\n");
		fflush(stdout);
	}
}


// Determine if a schedule meets the hard requirements
// returns 0 if it does, SCHED_INVALID if not
int CheckHardReq(Schedule *s) {
#define OPP_POS 0x01
#define OPP_NEG 0x02
	int retval = 0;
	char *opponents = calloc(s->num_teams + 1, sizeof(*opponents));
	// check that every team is played exactly twice
	for (int i = 1; i <= s->num_teams; i++) {
		memset(opponents, 0, s->num_teams + 1);
		for (int j = 0; j < s->num_rounds; j++) {
			if (s->round[j]->team[i] > 0) {
				int tmp = s->round[j]->team[i];
				if (opponents[tmp] & OPP_POS) {
					retval |= SCHED_INVALID;
					break;
				} else {
					opponents[tmp] |= OPP_POS;
				}
			} else if (s->round[j]->team[i] < 0) {
				int tmp = -s->round[j]->team[i];
				if (opponents[tmp] & OPP_NEG) {
					retval |= SCHED_INVALID;
					break;
				} else {
					opponents[tmp] |= OPP_NEG;
				}
			} else {
				retval |= SCHED_INVALID;
				break;
			}
		}
		if (retval) {
			break;
		}
	}
	free(opponents);
	return retval;
#undef OPP_NEG
#undef OPP_POS
}

// Determine if a schedule meets the soft requirements
// returns 0 if it meets both, SCHED_ATMOST if the atmost contraint fails
// SCHED_REPEAT if the norepeat contraint fails. These values can OR together
int CheckSoftReq(Schedule *s) {
	int retval = 0;
	// keeps track of number of consecutive games
	int *atmost_count = calloc(s->num_teams + 1, sizeof(*atmost_count));
	// keeps track of location of consectutive games
	int *atmost_val = calloc(s->num_teams + 1, sizeof(*atmost_val));
	// keeps track of last game played
	int *repeat_val = calloc(s->num_teams + 1, sizeof(*repeat_val));
	for (int i = 0; i < s->num_rounds; i++) {
		for (int j = 1; j <= s->num_teams; j++) {
			// check atmost
			if (atmost_val[j] > 0 && s->round[i]->team[j] > 0) {
				atmost_count[j]++;
			} else if (atmost_val[j] < 0 && s->round[i]->team[j] < 0) {
				atmost_count[j]++;
			} else {
				atmost_count[j] = 1;
				atmost_val[j] = (s->round[i]->team[j] > 0) ? 1 : -1;
			}
			if (atmost_count[j] > 3) {
				retval |= SCHED_ATMOST;
			}
			// check repeats
			if (repeat_val[j] == abs(s->round[i]->team[j])) {
				retval |= SCHED_REPEAT;
			}
			repeat_val[j] = abs(s->round[i]->team[j]);
		}
	}
	free(repeat_val);
	free(atmost_val);
	free(atmost_count);
	return retval;
}

// Neighborhood functions
// Swaps the home and away games for team i and j
void SwapHomes(Schedule *s, int t_i, int t_j) {
	int j = 0;
	for (int i = 0; i < s->num_rounds; i++) {
		if (abs(s->round[i]->team[t_i]) == t_j) {
			j++;
			s->round[i]->team[t_i] *= -1;
			s->round[i]->team[t_j] *= -1;
		} 
		if (j == 2) {
			break;
		}
	}
}

// Swaps rounds k and l
void SwapRounds(Schedule *s, int r_k, int r_l) {
	Round *tmp = s->round[r_k];
	s->round[r_k] = s->round[r_l];
	s->round[r_l] = tmp;
}

// Swaps teams i and j
void SwapTeams(Schedule *s, int t_i, int t_j) {
	for (int i = 0; i < s->num_rounds; i++) {
		// teams are playing each other, skip
		if (abs(s->round[i]->team[t_i]) == t_j) {
			continue;
		} else {
			// swap the two teams
			int tmp = s->round[i]->team[t_i];
			s->round[i]->team[t_i] = s->round[i]->team[t_j];
			s->round[i]->team[t_j] = tmp;
			// update the other teams
			s->round[i]->team[abs(tmp)] = (tmp > 0) ? -t_j : t_j;
			tmp = s->round[i]->team[t_i];
			s->round[i]->team[abs(tmp)] = (tmp > 0) ? -t_i : t_i;
		}
	}
}

// swaps games for a single team at rounds k and l
void PartialSwapRounds(Schedule *s, int t_i, int r_k, int r_l) {
	// get list of teams to swap
	int *swap = calloc(s->num_teams + 1, sizeof(*swap));
	int tmp;
	swap[t_i] = 1;
	int updated = 1;
	while (updated) {
		updated = 0;
		for (int i = 1; i <= s->num_teams; i++) {
			if (swap[i]) {
				tmp = abs(s->round[r_k]->team[i]);
				if (!swap[tmp]) {
					swap[tmp] = 1;
					updated++;
				} 
				tmp = abs(s->round[r_l]->team[i]);
				if (!swap[tmp]) {
					swap[tmp] = 1;
					updated++;
				} 
			}
		}
	}

	// swap all affect teams
	for (int i = 1; i <= s->num_teams; i++) {
		if (swap[i]) {
			tmp = s->round[r_k]->team[i];
			s->round[r_k]->team[i] = s->round[r_l]->team[i];
			s->round[r_l]->team[i] = tmp;
		}
	}
	free(swap);
}

// swaps the games of teams i and j, then updates the schedule
void PartialSwapTeams(Schedule *s, int t_i, int t_j, int r_l) {

}