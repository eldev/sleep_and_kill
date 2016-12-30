#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define DEBUG

/* wait for one second */
#define WAITING_TIME 1

#define USAGE "USAGE: ./a.out <path> [args]\n"

inline static void kill_child(pid_t child_pid) {
	if (kill(child_pid, SIGKILL) < 0)
		perror("ERROR: can't send SIGKILL signal \n");
}

#define PATH_LENGTH 64
#define LINE_LENGTH 128
#define STATE_STR "State:"

/*
 * if child is alive, will return 1
 * else return 0
 */
static int is_child_alive(pid_t child_pid) {
	char path[PATH_LENGTH];
	char line[LINE_LENGTH];
	char *p;
	FILE *status_file;

	snprintf(path, PATH_LENGTH, "/proc/%ld/status", (long)child_pid);
	status_file = fopen(path, "r");
	if (!status_file) {
		/* process has terminated already ? */
		return 0;
	}
	while (fgets(line, LINE_LENGTH,	status_file)) {
		if (strncmp(line, STATE_STR, strlen(STATE_STR)) != 0)
			continue;
		p = line + strlen(STATE_STR);
		while ((*p) == '\t')
			p++;
		break;
	}
	if (fclose(status_file) != 0)
		perror("ERROR: fclose\n");

	/* Now @p points to state letter */
#ifdef DEBUG
	printf("%s", p);
#endif
	return (*p) != 'Z' ? 1 : 0;
}

int main(int argc, char *argv[]) {
	pid_t child_pid;

	if (argc == 1) {
		printf(USAGE);
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
		if (sleep(WAITING_TIME))
			printf("Receive a signal which is not ignored\n");

		if (is_child_alive(child_pid)) {
			printf("Kill child process with pid=%d\n", child_pid);
			kill_child(child_pid);
		} else {
			printf("Child has already terminated its work\n");
		}
	}
	return 0;
}