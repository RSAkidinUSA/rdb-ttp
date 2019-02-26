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
	int numTeams;
	int numRounds;
	Round **round;
} Schedule;

Schedule *create_schedule(int numTeams);
void delete_schedule(Schedule *s);
void print_schedule(Schedule *s);
#define SCHED_ATMOST	0x01
#define SCHED_REPEAT	0x02
int valid_schedule(Schedule *s);

// Neighborhood functions
void swap_homes(Schedule *s, int t_i, int t_j);
void swap_round(Schedule *s, int r_i, int r_j);

#endif /* TTP_H */