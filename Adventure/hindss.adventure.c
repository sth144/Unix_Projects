/***********************************************************************************************************
 *	Title: Adventure
 *	Author: Sean Hinds
 *	Date: 02/14/18
 *	Description: This C program works in conjunction with the buildrooms program, which must be invoked 
 *			first in order to construct the room graph through which the player will move.
 *			This program provides the gameplay engine for the adventure game.
 * ********************************************************************************************************/

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>


/* define a room struct to store data parsed from room files in room subdirectory */

struct Room {

	char name[128];
	int num;
	char type[16];
	int numNeighbors;	
	char neighbors[6][128];

};

/* QueueNode and Queue structs used to track user's path */

struct QueueNode {

	int value;
	struct QueueNode* next;

};

struct Queue {

	int size;
	struct QueueNode* first;
	struct QueueNode* last;

};


/* function signatures */

void* wait(void*);
void threadTime();
void engine(struct Room*, struct Queue*, char*, DIR*);
char* getMostRecentSubDirName();
void targetSubDir(char*, char*);
void readRooms(struct Room*, DIR*, char*);
int runRound(struct Room* roomArr, struct Room** current, char* bfr, size_t bfrLen, struct Queue* path);
void display(struct Room*);
void pushQueue(struct Queue*, int);
int popQueue(struct Queue*);
void printPath(struct Room*, struct Queue*);
void win();
void cleanup(struct Room*, struct Queue*);
void strTruncLast(char*);

/* declare and initialize a global mutex from the pthreads library */

pthread_mutex_t mutex_a = PTHREAD_MUTEX_INITIALIZER;

/* declare a second thread for timekeeping */

pthread_t timeThread;

int stopTimeThreadWaiting;


int main() {

	/* lock mutex in the main thread */

	pthread_mutex_lock(&mutex_a);

	/* start second thread */

	stopTimeThreadWaiting = 0;

	pthread_create(&timeThread, NULL, wait, &stopTimeThreadWaiting);

	/* declare variables */

	char roomDirName[128];

	char prefix[128];

	struct Room roomArr[7];

	/* initialize queue to track user path */

	struct Queue* path = malloc(sizeof(struct Queue));

	path->size = 0;
	path->first = NULL;
	path->last = NULL;

	/* set roomDirName and file prefix */
  	
	targetSubDir(roomDirName, prefix);

 	/* open subdirectory with most recent timestamp */

	DIR* roomDir = opendir(roomDirName);

	/* check room directory */

	if (roomDir) {

		/* read rooms into room graph */

		readRooms(roomArr, roomDir, prefix);

		/* run engine with parameters */

		engine(roomArr, path, prefix, roomDir);

	}

	/* cleanup */	

	if (roomDir) {

		closedir(roomDir);
	
	}

	cleanup(roomArr, path);

	/* this will ensure that the timing thread does not print time when mutex is unlocked */

	stopTimeThreadWaiting++;	

	/* unlock the mutex and join timing thread to ensure that it completes execution */
	
	pthread_mutex_unlock(&mutex_a);

	pthread_join(timeThread, NULL);

	/* exit with status 0 */

	return 0;

}


/* Returns a string containing the filename of the subdirectory most recently modified */

char* getMostRecentSubDirName() {

	// printf("** get most recent subdir **\n");

	/* perform stat() call on rooms directories in . to find most recent st_mtime from stat struct 
	   referenced the "Manipulating Directories" page at https://oregonstate.instructure.com/courses/1662153/pages/2-dot-4-manipulating=directories */	
	
	struct dirent* newestDir;
	DIR* workingDir = opendir(".");

	int newestDirTime = -1;

	/* prefix for searching */

	char prefix[32] = "hindss.rooms.";
	char newestDirName[256];
	memset(newestDirName, 0, sizeof(newestDirName));

	struct dirent* entity;
	struct stat attributes;

	if (workingDir > 0) {

		/* iterate through each entity in . */

		while ((entity = readdir(workingDir)) != NULL) {

			/* if d_name of the entity has prefix as a substring */

			if (strstr(entity->d_name, prefix) != NULL) {
				
				/* store stat struct returned from call to stat on entity in attributes */

				stat(entity->d_name, &attributes);

				/* if entity being examined is the most recent examined so far */

				if ((int) attributes.st_mtime > newestDirTime) {
			
					/* update newestDirTime, newestDirName, newestDir */

					newestDirTime = (int) attributes.st_mtime;
					memset(newestDirName, 0, sizeof(newestDirName));

					strcpy(newestDirName, entity->d_name);
					newestDir = entity;
			
				}

			}	


		}

	}

	/* close . and return name of newest subdirectory */

	closedir(workingDir);

	return newestDir->d_name;

}


/* Store name of most recently modified subdirectory in roomDirName */

void targetSubDir(char* roomDirName, char* prefix) {
	
	// printf("** target most recent subdir **\n");

	memset(roomDirName, 0, sizeof(roomDirName));
	memset(prefix, 0, sizeof(prefix));

	strcpy(prefix, getMostRecentSubDirName());
	strcpy(roomDirName, prefix);

	strcat(prefix, "/");

}


/* Game engine */

void engine(struct Room* roomArr, struct Queue* path, char* prefix, DIR* roomDir) {

	// printf("** engine **\n");

	/* declare room pointer and local variables */

	struct Room* current;
	
	size_t bfrLen = 60;
	char* bfr = (char* )malloc(bfrLen * sizeof(char));

	int gameStatus = 0;
	int i;
	int neighborWasSelected;

	/* locate start room */

	for (i = 0; i < 7; i++) {

		if (strstr(roomArr[i].type, "START_ROOM")) {

			current = &roomArr[i];

		}

	}

	/* while game has not been won */

	while (!gameStatus) {

		/* run a round and store status at end of round (> 0) = win */

		gameStatus = runRound(roomArr, &current, bfr, bfrLen, path);	

	}

	/* win when while loop exits */

	win();
	printPath(roomArr, path);

	free(bfr);	

}


/* Read room files from room subdirectory and store data in Room structs */

void readRooms(struct Room* dest, DIR* src, char* prefix) {

	// printf("** readRooms **");

	struct dirent* roomEntity;
	FILE* roomFile;
	char filename[128];
	char str[128];
	char substr[128] = "ROOM TYPE: ";
	int arrIndex = 0;
	int offset = 11;
	int c;

	/* initialize rooms in room array */

	int i;
	int j;

	for (i = 0; i < 7; i++) {


		memset(dest[i].name, 0, sizeof(char) * 128);	
		dest[i].num = i;
		memset(dest[i].name, 0, sizeof(char) * 16);
		dest[i].numNeighbors = 0;
		
		for (j = 0; j < 6; j++) {

			memset(dest[i].neighbors[j], 0, 128);

		} 

	}

	/* read each file */

	while ((roomEntity = readdir(src)) != NULL) {

		/* do not examine . or .. */

		if (strcmp(roomEntity->d_name, ".") != 0 && strcmp(roomEntity->d_name, "..") != 0) {			

			dest[arrIndex].num = arrIndex;		// set num field of Room struct

			memset(filename, 0, sizeof(filename));
			strcpy(filename, prefix);		// copy prefix to filename
		
			strcat(filename, roomEntity->d_name);	// concatenate roomname to filename
		
			roomFile = fopen(filename, "r");	// open the file

			c = 0;

			/* while a line can be read from file */

			while (fgets(str, 60, roomFile)) {	

				/* if line describes name */

				if (strstr(str, "ROOM NAME")) {

					/* parse name and store in dest[arrIndex].name */
				
					strcpy(str, str + offset);
	
					strTruncLast(str);
	
					strcpy(dest[arrIndex].name, str);
	
				}

				/* if line describes connection */

				else if (strstr(str, "CONNECTION")) {			

					/* parse and store in dest[arrIndex].connections */

					strcpy(str, str + offset + 3);
		
					strTruncLast(str);

					strcpy(dest[arrIndex].neighbors[c], str);
				
					/* increment numNeighbors */

					dest[arrIndex].numNeighbors++;
		
					/* increment connections index */
					
					c++; 

				}

				/* if line describes room type */

				else if (strstr(str, "ROOM TYPE")) {

					/* parse and store in dest[arrIndex].type */
	
					strcpy(str, str + offset);

					strTruncLast(str);

					strcpy(dest[arrIndex].type, str);

				}

			}

			fclose(roomFile);

			arrIndex++;

		}

	}
	
}


/* Prompt user with their current location and possible options, accept user input */

int runRound(struct Room* roomArr, struct Room** current, char* bfr, size_t bfrLen, struct Queue* path) {

	int i;

	int neighborWasSelected;

	/* display() */
			
	display(*current);

	/* get input and validate */
	
	memset(bfr, 0, bfrLen * sizeof(char));

	getline(&bfr, &bfrLen, stdin);
	
	printf("\n");

	/* trim newlines */

	strTruncLast(bfr);

	/* if user entered 'time' */

	if (strcmp(bfr, "time") == 0) {
			
		/* pass control to timing thread */

		threadTime();
	
	}
		
	else {

		neighborWasSelected = 0;				// Boolean to indicate whether a valid room selected
	
		for (i = 0; i < (*current)->numNeighbors; i++) {

			/* if bfr matches a neighbor */

			if (strcmp(bfr, (*current)->neighbors[i]) == 0) {
				
				neighborWasSelected++;				
	
			}		

		}

		/* if user entered a valid room */

		if (neighborWasSelected) {
			
			/* push the room to the queue */

			pushQueue(path, (*current)->num);
			
			for (i = 0; i < 7; i++) {

				if (strcmp(bfr, roomArr[i].name) == 0 && neighborWasSelected) {

					/* update current pointer to room whose name matches bfr */

					*current = &roomArr[i];

				}

			}

		}

		else {
	
		/* if input not valid */
			
			printf("HUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
			 
		}
			
		/* if END_ROOM */
		
		if (strcmp((*current)->type, "END_ROOM") == 0) {		

			/* increment finished (Boolean) */
			 
			return 1;

		}
		
	}

	return 0;

}


/* Print current room and connecting room to console. Prompt user for input */

void display(struct Room* cur) {

	// printf("** display **\n");

	/* list where the player is */
		
	printf("CURRENT LOCATION: %s\n", cur->name);

	/* list possible connections that can be followed */
	
	printf("POSSIBLE CONNECTIONS: ");

	int i;
	for (i = 0; i < cur->numNeighbors; i++) {

		printf("%s", cur->neighbors[i]);
	
		if (i < cur->numNeighbors - 1)	{
			printf(", ");	
		}
		else {
			printf(".\n");
		}

	}
		
	printf("WHERE TO? >");

}


/* Push a node to the back of the queue */

void pushQueue(struct Queue* queue, int num) {

	// printf("** pushQueue **\n"); 

	/* allocate a new node with value = number */
	
	struct QueueNode* newNode = malloc(sizeof(struct QueueNode));

	/* new->next = NULL */

	newNode->next = NULL;

	newNode->value = num;

	/* queue->last->next = new */

	if (queue->last != NULL) {
		queue->last->next = newNode;
	}
	else {
		queue->first = newNode;
	}

	/* queue->last = new */

	queue->last = newNode;

	queue->size++;

}


/* Pop a node from the front of the queue and return the node value */

int popQueue(struct Queue* queue) {
	
	// printf("** popQueue **\n");

	/* store value of first node */

	int result = queue->first->value;

	/* declare garbage pointer and set */

	struct QueueNode* garbage;

	garbage = queue->first;

	/* move queue first pointer */

	queue->first = queue->first->next;

	/* deallocate garbage */

	free(garbage);

	/* return stored value from first node */	

	queue->size--;

	return result;

}


/* Print the contents of the pqth queue */

void printPath(struct Room* roomArr, struct Queue* path) {

	// printf("** print path **\n");

	/* declare a node pointer */

	struct QueueNode* node;

	int num;

	/* set pointer to first node in path */

	node = path->first; 

	/* while pointer not null */

	printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", path->size);

	while (node != NULL) {
	
		/* print room */	

		num = node->value;

		printf("%s\n", roomArr[num].name);

		node = node->next;

	}

}


/* Print the contents of the queue */

void win() {

	// printf("** win **\n");

	printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");

}


/* deallocate the queue */

void cleanup(struct Room* roomArr, struct Queue* queue) {

	// printf("** cleanup **\n");

	struct QueueNode* garbage;

	garbage = queue->first;

	while (garbage != NULL) {

		queue->first = queue->first->next;

		free(garbage);

		garbage = queue->first;

	}

	free(queue);

}


/* Truncate the last character of a string */

void strTruncLast(char* input) {

	// printf("** str Trunc **\n");

	int i = 0;

	while (input[i] != 0) {

		i++;

	}

	input[i - 1] = 0;

}


/* Called within the timing thread. If stopWaiting is not set, prints current time to console and dumps time to currentTime.txt */

void* wait(void* stopWaiting) {

	// printf("** wait **\n");

	/* local variables */

	time_t curTime;
	struct tm* timeptr;
	char filename[128];
	strcpy(filename, "currentTime.txt");
	FILE* outFile;
	char output[128];

	/* as long as stopWaiting is not set */

	if (!(*(int* ) stopWaiting)) {

		pthread_mutex_lock(&mutex_a);
		
		/* mutex locked within timing thread */

		time(&curTime);
		timeptr = localtime(&curTime);

		/* format time using strftime function */

		strftime(output, sizeof(output), "%I:%M%p, %A, %B %d, %Y", timeptr);		
			
		outFile = fopen(filename, "w");
		fprintf(outFile, "%s\n", output);
		fclose(outFile);			

		/* unlocking mutex in time thread */

		pthread_mutex_unlock(&mutex_a);
	
	}

}

/* Called within the main thread to free mutex so timing thread can execute code within the wait() function */

void threadTime() {

	// printf("** threadTime **\n");

	char filename[128];
	strcpy(filename, "currentTime.txt");
	FILE* inFile;
	char timeStr[128];

	pthread_mutex_unlock(&mutex_a);

	// printf("unlocked mutex in main thread\n");

	/* join timing thread, which should be waiting to execute within wait() function */

	pthread_join(timeThread, NULL);

	// printf("locking mutex in main thread\n");
 
	pthread_mutex_lock(&mutex_a);

	/* relaunch timing thread */	

	pthread_create(&timeThread, NULL, wait, &stopTimeThreadWaiting);

	inFile = fopen(filename, "r");

	fgets(timeStr, 128, inFile);

	printf(" %s\n", timeStr);

}
