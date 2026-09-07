/*************************************************
 * C program to count no of lines, words and 	 *
 * characters in a file.		                 *
 *************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_PROC 100
#define MAX_FORK 1000
typedef long long i64;
i64 chmin(i64 *x, i64 y) { *x = *x < y ? *x : y; return *x; }

typedef struct count_t {
	int linecount;
	int wordcount;
	int charcount;
} count_t;

typedef struct plist_t {
	int pid;
	i64 offset;
	int pipefd[2];
    int is_complete;
} plist_t;

int CRASH = 0;

count_t word_count(FILE* fp, long offset, long size)
{
	char ch;
	long rbytes = 0;

	count_t count;
	// Initialize counter variables
	count.linecount = 0;
	count.wordcount = 0;
	count.charcount = 0;

	// printf("[pid %d] reading %ld bytes from offset %ld\n", getpid(), size, offset);

	if(fseek(fp, offset, SEEK_SET) < 0) {
		printf("[pid %d] fseek error!\n", getpid());
	}

	while ((ch=getc(fp)) != EOF && rbytes < size) {
		// Increment character count if NOT new line or space
		if (ch != ' ' && ch != '\n') { ++count.charcount; }

		// Increment word count if new line or space character
		if (ch == ' ' || ch == '\n') { ++count.wordcount; }

		// Increment line count if new line character
		if (ch == '\n') { ++count.linecount; }
		rbytes++;
	}

	srand(getpid());
	if(CRASH > 0 && (rand()%100 < CRASH))
	{
		printf("[pid %d] crashed.\n", getpid());
		abort();
	}

	return count;
}

int main(int argc, char **argv)
{
	long fsize;
	FILE *fp;
	int numJobs;
	plist_t plist[MAX_PROC];
	count_t total, count;
	int i, pid;
	int nFork = 0;

	if(argc < 3) {
		printf("usage: wc_mul <# of processes> <filname>\n");
		return 0;
	}

	if(argc > 3) {
		CRASH = atoi(argv[3]);
		if(CRASH < 0) CRASH = 0;
		if(CRASH > 50) CRASH = 50;
	}
	printf("CRASH RATE: %d\n", CRASH);


	numJobs = atoi(argv[1]);
	if(numJobs > MAX_PROC) numJobs = MAX_PROC;
    numJobs = numJobs > 1 ? numJobs : 1;

	total.linecount = 0;
	total.wordcount = 0;
	total.charcount = 0;

	// Open file in read-only mode
	fp = fopen(argv[2], "r");

	if(fp == NULL) {
		printf("File open error: %s\n", argv[2]);
		printf("usage: wc <# of processes> <filname>\n");
		return 0;
	}

	fseek(fp, 0L, SEEK_END);
	fsize = ftell(fp);

	fclose(fp);
	// calculate file offset and size to read for each child
    i64 child_fsize = (fsize + numJobs - 1) / numJobs;
    for (int i = 0; i < numJobs; ++i) {
        plist[i].offset = i * child_fsize;
        plist[i].is_complete = 0;
    }

    int complte_jobs = 0;
    while(complte_jobs < numJobs) {
        for (i = 0; i < numJobs; i++) {
            if (plist[i].is_complete) continue;
            // set pipe;
            if (pipe(plist[i].pipefd) == -1) {
                printf("pipe error");
                exit(1);
            }
            if (nFork++ > MAX_FORK) return 0;
            pid = fork();
            if (pid < 0) {
                printf("Fork failed.\n");
            } else if (pid == 0) {
            // Child
                close(plist[i].pipefd[0]);
                fp = fopen(argv[2], "r");
                count = word_count(fp, plist[i].offset, child_fsize);
                // send the result to the parent through pipe
                if (write(plist[i].pipefd[1], &count, sizeof(count)) !=
                    sizeof(count)) {
                    printf("write error");
                }
                close(plist[i].pipefd[1]);
                fclose(fp);
                return 0;
            } else {
                plist[i].pid = pid;
                close(plist[i].pipefd[1]);
            }
        }
        // check if child process sucessfully done
        if (pid <= 0) continue;
        for (int i = 0; i < numJobs; ++i) {
            if (plist[i].is_complete) continue;
            count_t cnt;
            i64 x = read(plist[i].pipefd[0], &cnt, sizeof(count_t));
            close(plist[i].pipefd[0]);
            int status;
            if (waitpid(plist[i].pid, &status, 0) == -1) {
                // printf("child process crash #%d", plist[i].pid);
                continue;
            }
            if (x == sizeof(count_t) && WIFEXITED(status) && WEXITSTATUS(status)==EXIT_SUCCESS) {
                total.linecount += cnt.linecount;
                total.wordcount += cnt.wordcount;
                total.charcount += cnt.charcount;
                plist[i].is_complete = 1;
                ++complte_jobs;
            }
        }
    }
	// Parent
	// wait for all children
	// check their exit status
	// read the result of normalliy terminated child
	// re-crete new child if there is one or more failed child
	// close pipe
    // for (int i = 0; i < numJobs; ++i) {
    //     count_t cnt;
    //     size_t x = read(plist[i].pipefd[0], &cnt, sizeof(count_t));
    //     if ( x != sizeof(count_t) ) {
    //         printf("read error");
    //         // rebuild
    //     } else {
    //         close(plist[i].pipefd[0]);
    //         waitpid(plist[i].pid, NULL, 0);
    //         total.linecount += cnt.linecount;
    //         total.wordcount += cnt.wordcount;
    //         total.charcount += cnt.charcount;
    //     }
    // }

	printf("\n========== Final Results ================\n");
	printf("Total Lines : %d \n", total.linecount);
        printf("Total Words : %d \n", total.wordcount);
        printf("Total Characters : %d \n", total.charcount);
        printf("=========================================\n");

	return(0);
}

