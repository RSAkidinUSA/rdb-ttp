#ifndef TTP_H
#define TTP_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// Team names
static const char *const TEAM_NAMES[] = {
		"ATL","NYM","PHI","MON",\
		"FLA","PIT","CIN","CHI",\
		"STL","MIL","HOU","COL",\
		"SF", "SD", "LA", "ARI", 0
};

// Array of who each team is playing for a given week
// Teams start at 1, team 0 is unused
typedef struct {
	int *team;
} Round;

typedef struct {
	unsigned long *team_cost;
	int *distance;
	unsigned long total_cost;
	bool *updated;
} Cost;

// Array of pointers to weeks. Allows for swapping weeks quickly
// weeks start at 0
typedef struct {
	int num_teams;
	int num_rounds;
	int set_vals;
	Cost cost;
	Round **round;
} Schedule;

// settings for simulated annealing
typedef struct {
	double temp;
	double beta; 
	double weight;
	unsigned theta; 
	unsigned delta;
	unsigned max_reheat;
	unsigned max_phase;
	unsigned max_counter;
} Settings;

Schedule *CreateSchedule(int num_teams);
bool GenerateSchedule(Schedule *s);
unsigned long InitCost(Schedule *s, char *filename);
void DeleteSchedule(Schedule *s);
void PrintTeamCost(Schedule *s, int t);
void PrintSchedule(Schedule *s, const char * const*team_names);
#define SCHED_INVALID	0x01
int CheckHardReq(Schedule *s);
#define SCHED_ATMOST	0x02
#define SCHED_REPEAT	0x04
int CheckSoftReq(Schedule *s, int *nbv);

// Neighborhood functions
void SwapHomes(Schedule *s, int t_i, int t_j);
void SwapRounds(Schedule *s, int r_k, int r_l);
void SwapTeams(Schedule *s, int t_i, int t_j);
void PartialSwapRounds(Schedule *s, int t_i, int r_k, int r_l);
void PartialSwapTeams(Schedule *s, int t_i, int t_j, int r_k);

// Annealing algorithm
void Anneal(Schedule *s, Settings settings);

#endif /* TTP_H */