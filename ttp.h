#ifndef TTP_H
#define TTP_H

#include <stdlib.h>
#include <stdio.h>

// Array of who each team is playing for a given week
// Teams start at 1, team 0 is unused
typedef struct {
	int *team;
} Round;

// Array of pointers to weeks. Allows for swapping weeks quickly
// weeks start at 0
typedef struct {
	int num_teams;
	int num_rounds;
	int set_vals;
	Round **round;
} Schedule;

Schedule *CreateSchedule(int num_teams);
void DeleteSchedule(Schedule *s);
void PrintSchedule(Schedule *s);
#define SCHED_INVALID	0x01
int CheckHardReq(Schedule *s);
#define SCHED_ATMOST	0x02
#define SCHED_REPEAT	0x04
int CheckSoftReq(Schedule *s);

// Neighborhood functions
void SwapHomes(Schedule *s, int t_i, int t_j);
void SwapRounds(Schedule *s, int r_k, int r_l);
void SwapTeams(Schedule *s, int t_i, int t_j);
void PartialSwapRounds(Schedule *s, int t_i, int r_k, int r_l);
void PartialSwapTeams(Schedule *s, int t_i, int t_j, int r_l);

#endif /* TTP_H */