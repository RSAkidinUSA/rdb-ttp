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
	s->cost.distance = calloc(s->num_teams * s->num_teams, sizeof(*(s->cost.distance)));
	s->cost.team_cost = calloc(s->num_teams + 1, sizeof(*(s->cost.team_cost)));

	// allocate memory
	for (int i = 0; i < s->num_rounds; i++) {
		s->round[i] = calloc(1, sizeof(*(s->round[i])));
		s->round[i]->team = calloc(s->num_teams + 1, sizeof(*(s->round[i]->team)));
	}
	// fill teams
	GenerateSchedule(s);

	return s;
}

// Initialize the distances and calculate the current cost
// returns 0 on failure, else a positive value
int InitCost(Schedule *s, char *filename) {
	FILE *fptr = fopen(filename, "r");
	if (fptr == NULL) {
		return 0;
	}
	// read in costs
	for (int i = 0; i < s->num_teams * s->num_teams; i++) {
		fscanf(fptr, " ");
		if (!fscanf(fptr, "%d", &(s->cost.distance[i]))) {
			fclose(fptr);
			return 0;
		}
	}
	fclose(fptr);

	int prev_loc, new_loc;
	for (int i = 1; i <= s->num_teams; i++) {
		prev_loc = i;
		for (int j = 0; j < s->num_rounds; j++) {
			new_loc = (s->round[j]->team[i] > 0) ? i : abs(s->round[j]->team[i]);;
			int dist = ((prev_loc - 1) * s->num_teams) + (new_loc - 1);
			prev_loc = new_loc;
			s->cost.team_cost[i] += s->cost.distance[dist];
		}
		// add the trip home
		new_loc = i;
		int dist = ((prev_loc - 1) * s->num_teams) + (new_loc - 1);
		s->cost.team_cost[i] += s->cost.distance[dist];
	}

	int total_cost = 0;
	for (int i = 1; i <= s->num_teams; i++) {
		total_cost += s->cost.team_cost[i];
	}

	return total_cost;
}

// Delete a schedule and free all memory
void DeleteSchedule(Schedule *s) {
	free(s->cost.distance);
	free(s->cost.team_cost);
	for (int i = 0; i < s->num_rounds; i++) {
		free(s->round[i]->team);
		free(s->round[i]);
	}
	free(s->round);
	free(s);
}

static void inline __PrintTeamCost(Schedule *s, int t) {
	printf("Team %d cost: %d\n", t, s->cost.team_cost[t]);
}

// Print each teams cost
// if t is 0 print all teams, else just print t
void PrintTeamCost(Schedule *s, int t) {
	if (t) {
		__PrintTeamCost(s, t);
	} else {
		for (int i = 1; i <= s->num_teams; i++) {
			__PrintTeamCost(s, i);
		}
	}
}

// Print a schedule
// Takes a schedule to print and a list of team names (must be at least as long as the number of teams)
// If team names is NULL, just print values
void PrintSchedule(Schedule *s, const char * const *team_names) {
	printf("Slot");
	int num_team_names = 0;
	while (team_names[num_team_names]) {
		num_team_names++;
	}
	for (int i = 1; i <= s->num_teams; i++) {
		if (team_names && i <= num_team_names) {
			printf("\t%s", team_names[i - 1]);
		} else {
			printf("\t%d", i);
		}
	}
	printf("\n\n");
	for (int j = 0; j < s->num_rounds; j++) {
		printf("%d", j);
		for (int i = 1; i <= s->num_teams; i++) {
			int tmp = s->round[j]->team[i];
			if (team_names && abs(tmp) <= num_team_names) {				
				printf("\t%s%s", (tmp < 0) ? "@" : "", team_names[abs(tmp) - 1]);
			} else {
				printf("\t%d", s->round[j]->team[i]);
			}
		}
		printf("\n");
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
	// first swap the current round
	int tmp = s->round[r_l]->team[t_i];
	s->round[r_l]->team[t_i] = s->round[r_l]->team[t_j];
	s->round[r_l]->team[t_j] = tmp;
	// now swap the affected teams in the same round
	s->round[r_l]->team[abs(tmp)] = (tmp > 0) ? - t_j : t_j;
	tmp = s->round[r_l]->team[t_i];
	s->round[r_l]->team[abs(tmp)] = (tmp > 0) ? - t_i : t_i;
	// now run recursively on any affected rounds
	for (int i = 0; i < s->num_rounds; i++) {
		if (i == r_l) {
			continue;
		}
		if (s->round[r_l]->team[t_i] == s->round[i]->team[t_i] ||
			s->round[r_l]->team[t_j] == s->round[i]->team[t_j]) {
			PartialSwapTeams(s, t_i, t_j, i);
		}
		int t1 = abs(s->round[r_l]->team[t_i]);
		int t2 = abs(s->round[r_l]->team[t_j]);
		if (s->round[r_l]->team[t1] == s->round[i]->team[t1] ||
			s->round[r_l]->team[t2] == s->round[i]->team[t2]) {
			PartialSwapTeams(s, t1, t2, i);
		}
	}
}