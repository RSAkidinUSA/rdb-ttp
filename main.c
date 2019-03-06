#include "ttp.h"
#include <argp.h>

enum {ERR_USAGE = 1, ERR_NTEAM, ERR_FILENAME, ERR_GENSCHED, ERR_REQS};

const char *argp_program_version = 
	"rdb-ttp v1.0";
const char *argp_program_bug_address = 
	"<rsardb11@vt.edu>";
/* Program documentation */
static char doc[] = 
	"rdb-ttp -- a c implementation of the travelling team problem";

static char args_doc[] = "NUMTEAMS";

static struct argp_option options[] = {
	{ "seed", 's', "seed", 0, "Starting seed" },
	{ "temperature", 't', "temp", 0, "Starting temperature for annealing" },
	{ "beta", 'b', "beta", 0, "Beta value for annealing" },
	{ "weight", 'w', "weight", 0, "Weight value for annealing" }, 
	{ "delta", 'd', "delta", 0, "Theta/Delta value for annealing" }, 
	{ "max-reheat", 'r', "reheat", 0, "Maximum reheat value" },
	{ "max-phase", 'p', "phase", 0, "Maximum phase value" },
	{ "max-counter", 'c', "counter", 0, "Maxmimum counter value" },
	{ "Print", 'P', 0, 0, "Print the final schedule" },
	{ "verbose", 'v', 0, 0, "Print the settings used to anneal" },
	{ 0 }
};

struct arguments {
	unsigned num_teams;
	unsigned seed;
	bool print, verbose;
	Settings *settings;
};

static bool PRINT_SCHEDULE;

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	struct arguments *args = state->input;
	char *ptr;
	switch(key) {
		case 's':
			args->seed = strtoul(arg, &ptr, 10);
			if (ptr == arg) {
				printf("Error: Seed must be an integer greater than or equal to 0\n");
				return ERR_USAGE;
			}
			break;
		case 't':
			args->settings->temp = strtof(arg, &ptr);
			if (ptr == arg || args->settings->temp <= 0) {
				printf("Error: Temperature must be a positive float\n");
				return ERR_USAGE;
			}
			break;
		case 'b':
			args->settings->beta = strtof(arg, &ptr);
			if (ptr == arg || args->settings->beta <= 0 || args->settings->beta >= 1) {
				printf("Error: Beta must be between 0 and 1\n");
				return ERR_USAGE;
			}
			break;
		case 'w':
			args->settings->weight = strtof(arg, &ptr);
			if (ptr == arg || args->settings->weight <= 0) {
				printf("Error: Weight must be must be a positive float\n");
				return ERR_USAGE;
			}
			break;
		case 'd':
			args->settings->delta = args->settings->theta = strtoul(arg, &ptr, 10);
			if (ptr == arg || args->settings->delta <= 1) {
				printf("Error: Delta must be a positive integer greater than 1\n");
				return ERR_USAGE;
			}
			break;
		case 'r':
			args->settings->max_reheat = strtoul(arg, &ptr, 10);
			if (ptr == arg || args->settings->max_reheat == 0) {
				printf("Error: Max reheat must be a positive integer\n");
				return ERR_USAGE;
			}
			break;
		case 'p':
			args->settings->max_phase = strtoul(arg, &ptr, 10);
			if (ptr == arg || args->settings->max_phase == 0) {
				printf("Error: Max phase must be a positive integer\n");
				return ERR_USAGE;
			}
			break;
		case 'c':
			args->settings->max_counter = strtoul(arg, &ptr, 10);
			if (ptr == arg || args->settings->max_counter == 0) {
				printf("Error: Max counter must be a positive integer\n");
				return ERR_USAGE;
			}
			break;
		case 'P':
			args->print = true;
			break;
		case 'v':
			args->verbose = true;
			break;
		case ARGP_KEY_ARG:
			if (state->arg_num >= 1) {
				argp_usage(state);
			}
			args->num_teams = strtoul(arg, &ptr, 10);
			if (ptr == arg || args->num_teams < 3 || args->num_teams % 2) {
				printf("Error: Number of teams must be even and greater than 3\n");
				return ERR_NTEAM;
			}
			break;
		case ARGP_KEY_END:
			if (state->arg_num < 1) {
				argp_usage(state);
			}
			break;

		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };


unsigned GetArgs(int argc, char **argv, unsigned *num_teams, Settings *settings) {
	struct arguments arguments;
	int retval;
	// defaults
	arguments.seed = 0;
	arguments.num_teams = 0;
	arguments.print = false;
	arguments.verbose = false;
	arguments.settings = settings;
	settings->temp = 100;
	settings->beta = 0.9; 
	settings->weight = 1;
	settings->theta = settings->delta = 3; 
	settings->max_reheat = 5;
	settings->max_phase = 100;
	settings->max_counter = 100;

	if ((retval = argp_parse(&argp, argc, argv, 0, 0, &arguments))) {
		return retval;
	}

	PRINT_SCHEDULE = arguments.print;

	srand(arguments.seed);

	*num_teams = arguments.num_teams;
	if (arguments.verbose) {
		printf("Building schedule for %d teams with seed %d\n", \
				arguments.num_teams, arguments.seed);
		printf("Settings:\n");
		printf("Starting temp: %f\nBeta: %f\nWeight: %f\nTheta/Delta: %d\n"\
				"Max Reheat: %d\nMax Phase: %d\nMax Counter: %d\n", \
				settings->temp,	settings->beta, settings->weight, \
				settings->theta, settings->max_reheat, settings->max_phase, \
				settings->max_counter);
	}
	return 0;
}

int main(int argc, char **argv) {
	int invalid, retval;
	unsigned num_teams;
	char *filename;
	Schedule *s;
	Settings settings;
	if ((retval = GetArgs(argc, argv, &num_teams, &settings))) {
		return retval;
	}

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
	invalid = CheckHardReq(s);
	invalid |= CheckSoftReq(s, NULL);
	if (invalid) {
		if (invalid & SCHED_INVALID) {
			printf("Schedule is invalid\n");
		}
		if (invalid & SCHED_ATMOST) {
			printf("Schedule violates atmost contraint.\n");
		}
		if (invalid & SCHED_REPEAT) {
			printf("Schedule violates repeat contraint.\n");
		}
		retval = ERR_REQS;
	} else {
		printf("Valid Schedule!\n");
		printf("Cost: %lu\n", s->cost.total_cost);
		if (PRINT_SCHEDULE) {
			PrintSchedule(s, TEAM_NAMES);
		}
		retval = 0;
	}

	DeleteSchedule(s);

	return retval;
}