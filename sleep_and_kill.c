#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <errno.h>

/* wait for one second */
#define WAITING_TIME 1

#define USAGE "./a.out <path> [args]\n"

static int sig_count = 0;
static pid_t child_pid;

static void sigchld_handler(int sig, siginfo_t *siginfo, void *context) {
	if (sig == SIGCHLD &&
		siginfo->si_pid == child_pid)
	{
		sig_count++;
	}
}

static int register_sigchld_handler() {
	struct sigaction new_action;

	memset(&new_action, '\0', sizeof(new_action));
	new_action.sa_sigaction = &sigchld_handler;

	/*
	 * using SA_NOCLDSTOP, SIGCHLD is received
	 * only when the child terminates its work
	 */
	new_action.sa_flags = SA_SIGINFO | SA_NOCLDSTOP;

	if (sigaction(SIGCHLD, &new_action, NULL) < 0) {
		perror("ERROR: sigaction\n");
		return -1;
	}
	return 0;
}

static void kill_child() {
	if (sig_count)
		/* the child has terminated its work */
		return;

	if (kill(child_pid, SIGKILL) < 0)
		perror("ERROR: can't send SIGKILL signal \n");
}

static int sleep_for_waiting_time() {
	struct timespec req, rem;
	
	memset(&req, '\0', sizeof(req));
	memset(&rem, '\0', sizeof(rem));
	req.tv_sec = WAITING_TIME;

	/* 
	 * nanosleep can be interrupted by the signal handler 
	 * so use this cycle
	 */
	while (nanosleep(&req, &rem) < 0) {
		if (errno != EINTR) {
			perror("ERROR: nanosleep error\n");
			return -1;
		}
		
		req.tv_sec = rem.tv_sec;
		req.tv_nsec = rem.tv_nsec;
	}
	return 0;
}

int main(int argc, char *argv[]) {
	if (argc == 1) {
		printf(USAGE);
		return -1;
	}

	if (register_sigchld_handler() < 0) {
		return -1;
	}

	child_pid = fork();
	if (child_pid == -1) {
		perror("ERROR: fork\n");
		return -1;
	}

	if (child_pid == 0) {
		/* child routine */
		
		/* argv[1] -- child process */
		if (execvp(argv[1], argv + 1) == -1) {
			perror("ERROR: execvp \n");
			return -1;
		}
	}
	else if (child_pid > 0) {
		if (sleep_for_waiting_time() < 0) {
			perror("ERROR: sleep_for_second()\n");
			return -1;
		}
		if (sig_count > 0) {
			printf("Child has already terminated its work\n");
		} else {
			printf("kill child process with pid=%d\n", child_pid);
			kill_child();
		}
	}
	return 0;
}
