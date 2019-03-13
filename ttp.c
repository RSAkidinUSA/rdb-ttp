#include "ttp.h"
#include <string.h>
#include <math.h>
#include <float.h>

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
bool GenerateSchedule(Schedule *s) {
	bool retval = false;
	if (ScheduleEmpty(s)) {
		return true;
	}
	int t = 0, w;
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
	s->cost.updated = calloc(s->num_teams + 1, sizeof(*(s->cost.updated)));
	s->cost.total_cost = 0;

	// allocate memory
	for (int i = 0; i < s->num_rounds; i++) {
		s->round[i] = calloc(1, sizeof(*(s->round[i])));
		s->round[i]->team = calloc(s->num_teams + 1, sizeof(*(s->round[i]->team)));
	}

	return s;
}

// copies a schedule from source to destination
// requires both be initialized
// if copy_costs is true, copy the cost table, else skip
static void CopySchedule(Schedule *dst, Schedule *src, bool copy_costs) {
	for (int i = 0; i < src->num_rounds; i++) {
		for (int j = 1; j <= src->num_teams; j++) {
			dst->round[i]->team[j] = src->round[i]->team[j];
		}
	}
	for (int i = 1; i <= src->num_teams; i++) {
		dst->cost.team_cost[i] = src->cost.team_cost[i];
	}
	dst->cost.total_cost = src->cost.total_cost;
	if (copy_costs) {
		for (int i = 0; i < src->num_teams * src->num_teams; i++) {
			dst->cost.distance[i] = src->cost.distance[i];
		}
	}
}

// update the costs for all teams with their updated flags set to true
static void UpdateCost(Schedule *s) {
	for (int i = 1; i <= s->num_teams; i++) {
		if (s->cost.updated[i]) {
			s->cost.total_cost -= s->cost.team_cost[i];
			s->cost.team_cost[i] = 0;

			int prev_loc, new_loc;
			// actual cost update
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
			// end cost update

			s->cost.total_cost += s->cost.team_cost[i];
			s->cost.updated[i] = false;
		}
	}
}

// Initialize the distances and calculate the current cost
// returns 0 on failure, else a positive value
unsigned long InitCost(Schedule *s, char *filename) {
	FILE *fptr = fopen(filename, "r");
	if (fptr == NULL) {
		return 0;
	}
	// read in costs
	for (int i = 0; i < s->num_teams * s->num_teams; i++) {
		if (!fscanf(fptr, "%d ", &(s->cost.distance[i]))) {
			fclose(fptr);
			return 0;
		}
	}
	fclose(fptr);
	
	for (int i = 1; i <= s->num_teams; i++) {
		s->cost.updated[i] = true;
	}
	UpdateCost(s);

	return s->cost.total_cost;
}

// Delete a schedule and free all memory
void DeleteSchedule(Schedule *s) {
	free(s->cost.updated);
	free(s->cost.team_cost);
	free(s->cost.distance);
	for (int i = 0; i < s->num_rounds; i++) {
		free(s->round[i]->team);
		free(s->round[i]);
	}
	free(s->round);
	free(s);
}

static void inline __PrintTeamCost(Schedule *s, int t) {
	printf("Team %d cost: %lu\n", t, s->cost.team_cost[t]);
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
			if (abs(s->round[j]->team[i]) == i) {
				retval |= SCHED_INVALID;
			}
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
// takes an optional argument nbv, which is incremented each time a constratin is found
int CheckSoftReq(Schedule *s, int *nbv) {
	int retval = 0;
	if (nbv) {
		*nbv = 0;
	}
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
				if (nbv) {
					*nbv += 1;
				}
			}
			// check repeats
			if (repeat_val[j] == abs(s->round[i]->team[j])) {
				retval |= SCHED_REPEAT;
				if (nbv) {
					*nbv += 1;
				}
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
static void SwapHomes(Schedule *s, int t_i, int t_j) {
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
	s->cost.updated[t_i] = true;
	s->cost.updated[t_j] = true;
}

// Swaps rounds k and l
static void SwapRounds(Schedule *s, int r_k, int r_l) {
	Round *tmp = s->round[r_k];
	s->round[r_k] = s->round[r_l];
	s->round[r_l] = tmp;
	for (int i = 1; i <= s->num_teams; i++) {
		s->cost.updated[i] = true;
	}
}

// Swaps teams i and j
static void SwapTeams(Schedule *s, int t_i, int t_j) {
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
	for (int i = 1; i <= s->num_teams; i++) {
		s->cost.updated[i] = true;
	}
}

// swaps games for a single team at rounds k and l
static void PartialSwapRounds(Schedule *s, int t_i, int r_k, int r_l) {
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
			s->cost.updated[i] = true;
		}
	}
	free(swap);
}

// swaps the games of teams i and j, then updates the schedule
static void PartialSwapTeams(Schedule *s, int t_i, int t_j, int r_k) {
	// if trying an invalid swap, just return
	if (t_i == abs(s->round[r_k]->team[t_j]) ||
			t_j == abs(s->round[r_k]->team[t_i])) {
		return;
	}
	// first swap the current round
	int tmp = s->round[r_k]->team[t_i];
	s->round[r_k]->team[t_i] = s->round[r_k]->team[t_j];
	s->round[r_k]->team[t_j] = tmp;
	// now swap the affected teams in the same round
	s->round[r_k]->team[abs(tmp)] = (tmp > 0) ? - t_j : t_j;
	tmp = s->round[r_k]->team[t_i];
	s->round[r_k]->team[abs(tmp)] = (tmp > 0) ? - t_i : t_i;
	// update costs:
	s->cost.updated[t_i] = true;
	s->cost.updated[t_j] = true;
	s->cost.updated[abs(s->round[r_k]->team[t_i])] = true;
	s->cost.updated[abs(s->round[r_k]->team[t_j])] = true;
	// now run recursively on any affected rounds
	for (int i = 0; i < s->num_rounds; i++) {
		if (i == r_k) {
			continue;
		}
		if (s->round[r_k]->team[t_i] == s->round[i]->team[t_i] ||
			s->round[r_k]->team[t_j] == s->round[i]->team[t_j]) {
			PartialSwapTeams(s, t_i, t_j, i);
		}
		int t1 = abs(s->round[r_k]->team[t_i]);
		int t2 = abs(s->round[r_k]->team[t_j]);
		if (s->round[r_k]->team[t1] == s->round[i]->team[t1] ||
			s->round[r_k]->team[t2] == s->round[i]->team[t2]) {
			PartialSwapTeams(s, t1, t2, i);
		}
	}
}

static double __Sublinear(int v) {
	return 1 + (sqrt(v) * log(v) / 2);
}

// Objective function
static double __Objective(Schedule *s, int weight, int nbv) {
	if (nbv) {
		double tmp = weight * __Sublinear(nbv);
		return sqrt((double) (s->cost.total_cost * s->cost.total_cost) + (tmp * tmp));
	} else {
		return (double) s->cost.total_cost;
	}
}

static void __DoRandomChange(Schedule *s, bool undo) {
	// teams
	static int t_i = 1; static int t_j = 1;
	// rounds
	static int r_k = 0;	static int r_l = 0;
	// function
	static int f = 0;

	if (!undo) {
		t_i = (rand() % s->num_teams) + 1;
		do {
			t_j = (rand() % s->num_teams) + 1;
		} while (t_j == t_i);
		r_k = rand() % s->num_rounds;
		do {
			r_l = rand() % s->num_rounds;
		} while (r_l == r_k);
		f = rand() % 5;
	}
	switch(f) {
		case 0:
			SwapHomes(s, t_i, t_j);
			break;
		case 1:
			SwapRounds(s, r_k, r_l);
			break;
		case 2:
			SwapTeams(s, t_i, t_j);
			break;
		case 3:
			PartialSwapRounds(s, t_i, r_k, r_l);
			break;
		case 4:
			PartialSwapTeams(s, t_i, t_j, r_l);
			break;
	}
	UpdateCost(s);
}

#define UL_INF ((unsigned long) ~0)

// Runs the simulated annealing algorithm for a given temperature and beta
// requires initial schedule with initial cost
// Best feasible is stored in s
void Anneal(Schedule *sbi, Settings settings) {
	// best feasible so far
	Schedule *sbf = CreateSchedule(sbi->num_teams);
	if (!CheckSoftReq(sbi, NULL)) {
		CopySchedule(sbf, sbi, false);
	} else {
		sbf->cost.total_cost = UL_INF;
	}
	double best_feasible = DBL_MAX, nbf = DBL_MAX;
	double best_infeasible = DBL_MAX, nbi = DBL_MAX;
	int best_temp = 0;
	int reheat = 0;
	int nbv;
	double total_cycles = (settings.max_reheat + 1) * (settings.max_phase + 1) \
			* (settings.max_counter + 1);
	double num_cycles = 0;
	if (settings.update) {
		printf("Percentage complete:\n%.2f", 0.0);
	}
	while (reheat <= settings.max_reheat) {
		int phase = 0;
		while (phase <= settings.max_phase) {
			int counter = 0;
			while (counter <= settings.max_counter) {
				bool accept;
				CheckSoftReq(sbi, &nbv);
				double old_cost = __Objective(sbi, settings.weight, nbv);
				__DoRandomChange(sbi, false);
				CheckSoftReq(sbi, &nbv);
				double new_cost = __Objective(sbi, settings.weight, nbv);

				if ((new_cost < old_cost) || 
						(nbv == 0 && new_cost < best_feasible) || 
						(nbv > 0 && new_cost < best_infeasible)) {
					accept = true;
				} else if (exp((old_cost - new_cost) / settings.temp)) {
					accept = true;
				} else {
					accept = false;
				}
				if (accept) {
					if (nbv == 0) {
						nbf = (new_cost < best_feasible) ? 
								new_cost : best_feasible;
						if (nbf < best_feasible) {
							sbf->cost.total_cost = (unsigned long) new_cost;
							CopySchedule(sbf, sbi, false);
						}
					} else {
						nbi = (new_cost < best_infeasible) ? 
								new_cost : best_infeasible;
					}
					if (nbf < best_feasible || nbi < best_infeasible) {
						reheat = 0; counter = 0; phase = 0; num_cycles = 0;
						best_temp = settings.temp;
						best_feasible = nbf;
						best_infeasible = nbi;
						if (nbv == 0) {
							settings.weight = settings.weight / settings.theta;
						} else {
							settings.weight = settings.weight * settings.delta;
						}
					} else {
						counter++;
						num_cycles++;
					}
				} else {
					// undo the change
					__DoRandomChange(sbi, true);
				}
			} // counter
			phase++;
			if (settings.update) {
				printf("\r%.2f%%\t\t\t", (num_cycles / total_cycles) * 100.00);
			}
			settings.temp = settings.temp * settings.beta;
		} // phase
		reheat++;
		settings.temp = 2 * best_temp;
	} // reheat
	if (settings.update) {
		printf("\n");
	}
	if (!CheckHardReq(sbf)) {
		CopySchedule(sbi, sbf, true);
	}
	DeleteSchedule(sbf);
}