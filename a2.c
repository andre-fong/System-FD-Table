/*
	CSCB09: Assignment 2
	Andre Fong
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<dirent.h>
#include<unistd.h>

// For storing data in FD table
typedef struct fdNode {
	pid_t pid;
	int fd;
	char filename[1024];
	ino_t inode;
	struct fdNode *next;
} FDNode;

// For writing and reading data from compositeTable.bin
typedef struct fdData {
	pid_t pid;
	int fd;
	char filename[1024];
	ino_t inode;
} FDData;

// For storing data for processes with over X FDs (threshold)
typedef struct processNode {
	pid_t pid;
	int fdCount;
	struct processNode *next;
} ProcessNode;

FDNode *newFDNode(pid_t pid, int fd, char *filename, ino_t inode) {
	FDNode *new = malloc(sizeof(FDNode));
	if (new == NULL) {
		fprintf(stderr, "Error allocating memory for FDNode\n");
		exit(1);
	}
	new->pid = pid;
	new->fd = fd;
	strncpy(new->filename, filename, 1024);
	new->inode = inode;
	new->next = NULL;
	return new;
}
ProcessNode *newPNode(pid_t pid, int fdCount) {
	ProcessNode *new = malloc(sizeof(ProcessNode));
	if (new == NULL) {
		fprintf(stderr, "Error allocating memory for ProcessNode\n");
		exit(1);
	}
	new->pid = pid;
	new->fdCount = fdCount;
	new->next = NULL;
	return new;
}

// Precond: head exists
void insertFDAtTail(FDNode *head, FDNode *new) {
	FDNode *tr = head;
	while (tr->next != NULL) tr = tr->next;
	tr->next = new;
}
// Precond: head exists
void insertPAtTail(ProcessNode *head, ProcessNode *new) {
	ProcessNode *tr = head;
	while (tr->next != NULL) tr = tr->next;
	tr->next = new;
}

void deleteFDList(FDNode *head) {
	FDNode *pre = head;
	FDNode *tr = NULL;
	while (pre != NULL) {
		tr = pre->next;
		free(pre);
		pre = tr;
	}
}
void deletePList(ProcessNode *head) {
	ProcessNode *pre = head;
	ProcessNode *tr = NULL;
	while (pre != NULL) {
		tr = pre->next;
		free(pre);
		pre = tr;
	}
}

void printFDList(FDNode *head, bool perProcess, bool systemWide, bool Vnodes, bool composite, bool sawPID, pid_t targetPID, FILE *out) {
	if (sawPID) printf("Target PID: %d\n\n", targetPID);
	
	if (perProcess) {
		printf("\t PID\tFD\n");
		printf("\t==============================================\n");
	
		FDNode *tr = head;
		while (tr != NULL) {
			if ((sawPID && tr->pid == targetPID) || !sawPID) printf("\t %d\t%d\n", tr->pid, tr->fd);
			tr = tr->next;
		}
		printf("\t==============================================\n\n");
	}
	if (systemWide) {
		printf("\t PID\tFD\tFilename\n");
		printf("\t==============================================\n");
	
		FDNode *tr = head;
		while (tr != NULL) {
			if ((sawPID && tr->pid == targetPID) || !sawPID) printf("\t %d\t%d\t%s\n", tr->pid, tr->fd, tr->filename);
			tr = tr->next;
		}
		printf("\t==============================================\n\n");
	}
	if (Vnodes) {
		printf("\t FD\tInode\n");
		printf("\t==============================================\n");
	
		FDNode *tr = head;
		while (tr != NULL) {
			if ((sawPID && tr->pid == targetPID) || !sawPID) printf("\t %d\t%ld\n", tr->fd, tr->inode);
			tr = tr->next;
		}
		printf("\t==============================================\n\n");
	}
	if (composite) {
		fprintf(out, "\t PID\tFD\tFilename\t\tInode\n");
		fprintf(out, "\t==============================================\n");
	
		FDNode *tr = head;
		while (tr != NULL) {
			if ((sawPID && tr->pid == targetPID) || !sawPID) fprintf(out, "\t %d\t%d\t%s\t\t%ld\n", tr->pid, tr->fd, tr->filename, tr->inode);
			tr = tr->next;
		}
		fprintf(out, "\t==============================================\n\n");
	}
	
}

// Precond: threshold exists in main program
void printPList(ProcessNode *head, int threshold) {
	printf("## Offending processes —— #FD threshold=%d\n", threshold);
	if (head == NULL) printf(" (none)");
	
	ProcessNode *tr = head;
	while (tr != NULL) {
		printf(" %d (%d),", tr->pid, tr->fdCount);
		tr = tr->next;
	}
	printf("\n");
}

/*
 * Function: extractFlagValue
 * ----------------------------
 * Extracts a positive numerical value from a string 
 * with the form: --<flagName>=<positiveNumericalValue>
 * 
 * flag: string with above flag format
 * flagName: name of flag to validate
 *
 * returns: the positive numerical value from the flag if flag is valid
 *          <0 on error (if flag is wrong format)
 */
int extractFlagValue(char *flag, char *flagName) {
	if (flag == NULL || flagName == NULL) return -1;
	
	// Ensure first two chars of flag is "--"
	char *tr = flag;
	for (int i = 0; i < 2; i++) {
		if (flag[i] != '-' || flag[i] == '\0') return -2;
		tr++;
	}
	
	// Find first occurrence of flagName and make sure it's right after the flag delimiter
	char *firstOccur;
	firstOccur = strstr(tr, flagName);
	
	// No occurrence of flagName
	if (firstOccur == NULL) return -3;
	
	// flagName not located immediately after "--"
	if (firstOccur != tr) return -4;
	
	// flagName is correct, ensure '=' located right after flagName
	firstOccur += strlen(flagName);
	if (*firstOccur != '=') return -5;
	firstOccur++;
	
	// Convert value after '=' to long int, error if anything after '=' cannot be converted
	char *leftover;
	long value = strtol(firstOccur, &leftover, 10);
	
	if (*leftover != '\0') return -6;
	
	return value;
}

int main(int argc, char **argv) {
	// CLA values
	bool perProcess = false, systemWide = false, Vnodes = false, composite = false, outputTXT = false, outputBinary = false;
	int threshold = 0, targetPID = 0;
	
	bool sawPID = false, thresholdExists = false;

	// Parse through CLAs
	for (int i = 1; i < argc; i++) {
		if (strncmp(argv[i], "--per-process", 13) == 0) perProcess = true;
		else if (strncmp(argv[i], "--systemWide", 12) == 0) systemWide = true;
		else if (strncmp(argv[i], "--Vnodes", 8) == 0) Vnodes = true;
		else if (strncmp(argv[i], "--composite", 11) == 0) composite = true;
		else if (strncmp(argv[i], "--output_TXT", 12) == 0) outputTXT = true;
		else if (strncmp(argv[i], "--output_binary", 15) == 0) outputBinary = true;
		else {
			char *leftover;
			long numArg = strtol(argv[i], &leftover, 10);
			
			// If arg is positional arg (integer)
			if (leftover[0] == '\0') {
				if (!sawPID && numArg >= 0) {
					targetPID = numArg;
					sawPID = true;
				}
				else if (sawPID) {
					fprintf(stderr, "args: PID positional argument should only be provided once\n");
					exit(1);
				}
				else {
					fprintf(stderr, "args: provided PID %ld must be >= 0\n", numArg);
					exit(1);
				}
			}
			
			// Test if arg is --threshold=X
			else {
				int thresholdRes = extractFlagValue(argv[i], "threshold");
				
				// No remaining possible arguments that match argv[i]
				if (thresholdRes < 0) {
					fprintf(stderr, "args: unsupported argument: \"%s\"\n", argv[i]);
					exit(1);
				}
				
				thresholdExists = true;
				threshold = thresholdRes;
			}
		}
	}
	
	// Set default behavior if no CLAs
	if (!perProcess && !systemWide && !Vnodes && !composite) {
		composite = true;
	}
	
	// Declare FD list and process list
	FDNode *fdHead = NULL;
	ProcessNode *pHead = NULL;
	
	// Search /proc for process IDs (if targetPID provided, only search for targetPID)
	DIR *proc = opendir("/proc");
	if (proc == NULL) {
		fprintf(stderr, "err: directory /proc not accessible\n");
		exit(1);
	}
	
	struct dirent *procEntry;
	
	while ((procEntry = readdir(proc)) != NULL) {
		char *leftover;
		long processID = strtol(procEntry->d_name, &leftover, 10);
		
		// Didn't find pid
		if (leftover[0] != '\0') continue;

		// Found pid (directory entry is an int)
		char procDirName[255];
		sprintf(procDirName, "/proc/%ld/fd", processID);
		
		DIR *procFDs = opendir(procDirName);
		if (procFDs == NULL) {
			// Expected behavior, certain processes will have restricted access
			continue;
		}
		
		int fdCount = 0;
		struct dirent *procFDEntry;
		while ((procFDEntry = readdir(procFDs)) != NULL) {
			// Fixes . and .. being listed as entries to /proc/[pid]/fd
			if (strncmp(procFDEntry->d_name, ".", 1) == 0 || strncmp(procFDEntry->d_name, "..", 2) == 0) continue;
			
			// Get filename
			char fdLink[1024];
			sprintf(fdLink, "/proc/%ld/fd/%s", processID, procFDEntry->d_name);
			
			char filename[1024];
			ssize_t len;
			
			if ((len = readlink(fdLink, filename, sizeof(filename) - 1)) != -1) filename[len] = '\0';
			else {
				printf("fd entry = %s\n", procFDEntry->d_name);
				fprintf(stderr, "err: Could not read link: %s\n", fdLink);
				exit(1);
			}
			
			// Get inode
			struct stat buf;
			ino_t inode;
			if (stat(fdLink, &buf) != -1) inode = buf.st_ino;
			else {
				fprintf(stderr, "err: Could not get stat of link: %s\n", fdLink);
				exit(1);
			}
			
			// Add to FD data structure
			char *leftover2;
			long fd = strtol(procFDEntry->d_name, &leftover2, 10);
			
			if (fdHead == NULL) fdHead = newFDNode(processID, fd, filename, inode);
			else insertFDAtTail(fdHead, newFDNode(processID, fd, filename, inode));
			
			fdCount++;
		}
		
		// Add to process data structure if threshold exists
		if (thresholdExists && fdCount > threshold) {
			if (pHead == NULL) pHead = newPNode(processID, fdCount);
			else insertPAtTail(pHead, newPNode(processID, fdCount));
		}
			
		int err;
		if ((err = closedir(procFDs)) != 0) {
			fprintf(stderr, "err: directory %s was not closed\n", procDirName);
			exit(1);
		}
		
	}
	
	int err;
	if ((err = closedir(proc)) != 0) {
		fprintf(stderr, "err: directory /proc was not closed\n");
		exit(1);
	}
	
	// Save composite table to txt and/or binary
	if (outputTXT) {
		FILE *txtOut = fopen("compositeTable.txt", "w");
		if (txtOut == NULL) {
			fprintf(stderr, "err: compositeTable.txt could not be created/opened\n");
			exit(1);
		}
		
		printFDList(fdHead, false, false, false, true, false, 0, txtOut);
		
		if (fclose(txtOut) != 0) {
			fprintf(stderr, "err: compositeTable.txt could not be closed\n");
			exit(1);
		}
	}
	
	if (outputBinary) {
		FILE *binOut = fopen("compositeTable.bin", "wb");
		if (binOut == NULL) {
			fprintf(stderr, "err: compositeTable.bin could not be created/opened\n");
			exit(1);
		}
		
		FDNode *tr = fdHead;
		while (tr != NULL) {
			FDData new;
			new.pid = tr->pid;
			new.fd = tr->fd;
			strncpy(new.filename, tr->filename, 1024);
			new.inode = tr->inode;
			
			fwrite(&new, sizeof(FDData), 1, binOut);
			
			tr = tr->next;
		}
		
		if (fclose(binOut) != 0) {
			fprintf(stderr, "err: compositeTable.bin could not be closed\n");
			exit(1);
		}
	}
	
	// Print tables
	printFDList(fdHead, perProcess, systemWide, Vnodes, composite, sawPID, targetPID, stdout);
	if (thresholdExists) printPList(pHead, threshold);
	
	// Free allocated memory
	deleteFDList(fdHead);
	deletePList(pHead);
	
	return 0;
}

