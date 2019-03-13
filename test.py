#!/usr/bin/python3
import sys
import subprocess
import time
import threading

DO_TIMING = False
LOCK = threading.Lock()
PERCENT_COMPLETE = 0
NUM_THREADS = 4
NUM_REHEATS = 2
NUM_PHASES 	= 3
NUM_COUNTER = 3
MARGIN = (1 / (NUM_THREADS * NUM_REHEATS * NUM_PHASES * NUM_COUNTER))

def worker(results, num_teams, temp):
	global PERCENT_COMPLETE
	temp = 300 + (temp * 50)
	beta = 0.9999
	weight = 4000
	weight *= 2
	delta = 1.04
	for reheat in range(1, 1 + NUM_REHEATS):
		reheat *= 5
		for phase in range(0, 0 + NUM_PHASES):
			phase = 3100 + (phase * 2000)
			for counter in range(0, 0 + NUM_COUNTER):
				counter = 3000 + (counter * 1000)
				successful = 0
				min_time = sys.maxsize; max_time = 0; avg_time = 0
				min_cost = sys.maxsize; max_cost = 0; avg_cost = 0
				min_seed = 0; max_seed = 0
				for seed in range(0,4):
					args = ["./rdb-ttp", str(num_teams), \
							"-s", str(seed), "-b", str(beta), "-d", str(delta), \
							"-w", str(weight), "-r", str(reheat), "-p", str(phase), \
							"-c", str(counter), "-t", str(temp)]
					try:
						comp_time = time.thread_time_ns()
						out = subprocess.check_output(args)
						comp_time = time.thread_time_ns() - comp_time
						out = out.decode('utf-8')
						if out.find("Valid Schedule!") != -1:
							cost = int(out.splitlines()[1].split(' ')[1])
							successful += 1
							avg_time += comp_time
							avg_cost += cost
							if (cost < min_cost):
								min_cost = cost
								min_seed = seed
							elif (cost > max_cost):
								max_cost = cost
								max_seed = seed
							if (comp_time < min_time):
								min_time = comp_time
							elif (comp_time > max_time):
								max_time = comp_time
					except Exception as e:
						print("Error: %s" % (e,))
				LOCK.acquire()
				results.write("%f,%f,%f,%d,%d,%d,%d,%d,%d,%d,%d,%d," % \
						(temp, beta, weight, delta, reheat, phase, counter, \
						successful, min_seed, max_seed, min_cost, max_cost))
				if (successful > 0):
					results.write("%d" % (avg_cost / successful,))
				else:
					results.write("%d" % (0,))
				if DO_TIMING:
					results.write(",%d,%d,%d\n" % (min_time, max_time, avg_time / successful))
				else:
					results.write("\n")
				results.flush()
				PERCENT_COMPLETE += MARGIN
				print("\rPercentage complete: %f" % (PERCENT_COMPLETE * 100,), end='')
				LOCK.release()

def main():
	global PERCENT_COMPLETE
	assert (sys.version_info >= (3, 7))
	if len(sys.argv) < 2:
		print("Usage: %s # teams" % (sys.argv[0],))
		return 1
	num_teams = int(sys.argv[1])
	if num_teams % 2 or num_teams < 3:
		print("Number of teams must be even and greater than 3")
		return 2
	results = open("results.csv", "w")
	
	results.write("Temp,Beta,Weight,Delta,Reheat,Phase,Counter,Successful,"\
			"Min-Seed,Max-Seed,Min-Cost,Max-Cost,Avg-Cost")
	if DO_TIMING:
		results.write(",Min-Time,Max-Time,Avg-Time\n")
	else:
		results.write("\n")

	results.flush()
	threads = []
	PERCENT_COMPLETE = 0
	for temp in range(0, NUM_THREADS):
		t = threading.Thread(target=worker, args=(results, num_teams, temp))
		threads.append(t)
		t.start()

	for t in threads:
		t.join()
	print("\nDone.")
	results.close()
	return 0

if __name__ == "__main__":
	main()