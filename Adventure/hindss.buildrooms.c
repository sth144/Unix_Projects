/***********************************************************************************************
 *	Title: buildrooms program. 	
 *	Author: Sean Hinds
 *	Date: 02/14/18
 *	Description: This C program builds a directory with process ID in the name
 *		which contains room data files for an adventure game. These rooms are connected
 *		in a graph data structure which the user navigates in the adventure game
 * ********************************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* define the Room struct which will be used to store data to be written to files */

struct Room {
	
	char* name;
	int num;
	char* type;
	FILE* fileptr;
	int numConnections;
	int connections[6];

};

/* structs to be utilized in the stack data structure used in the depth first search */

struct IntStackNode {

	int value;
	struct IntStackNode* next;

};

struct IntStack {

	int size;
	struct IntStackNode* top;

};


/* function signatures */

void generateRoomNames(struct Room*, int);
void openFileStreams(struct Room*, int);
void connectRoomGraph(struct Room*, int);
void connect(struct Room*, int, int);
int areConnected(struct Room*, int, int);
int minConnEach(struct Room*, int, int);
int dfsReachesAll(struct Room*, int, int);
void stackPush(struct IntStack*, int);
int stackPop(struct IntStack*);
void assignRoomStatuses(struct Room*, int);
void deallocateRooms(struct Room*, int);


int main() {

	/* seed random number generator */

	srand(time(NULL));

	/* local variables */
	
	int i;
	int numRooms = 7;

	/* 7 Room array */
	
	struct Room rooms[numRooms];

	/* generate and set room names from a set of 10 names. Also sets room num */

	generateRoomNames(rooms, numRooms);

	/* open 7 room file streams with name chosen from array */

	openFileStreams(rooms, numRooms);
	
	/* connect room graph */
	
	connectRoomGraph(rooms, numRooms);	

	/* assign room statuses */

	assignRoomStatuses(rooms, numRooms);

	/* free memory */

	deallocateRooms(rooms, numRooms);	

	/* exit status 0 */	

	return 0;

}


/* this function generates 7 random room names from 10 available names and stores them as attributes of Room structs */

void generateRoomNames(struct Room* roomArr, int numPicks) {

	// printf("** generateRoomNames() **\n");

	/* initialize room names and local variables */

	int i;
	for (i = 0; i < numPicks; i++) {
		roomArr[i].name = (char* ) malloc(15);
		memset(roomArr[i].name, 0, 15);
	}

	const int librarySize = 10;

	char* nameLibrary[librarySize];

	nameLibrary[0] = "Bridge";
	nameLibrary[1] = "Galley";
	nameLibrary[2] = "Barracks";
	nameLibrary[3] = "Armory";
	nameLibrary[4] = "Library";
	nameLibrary[5] = "Workshop";
	nameLibrary[6] = "Tower";
	nameLibrary[7] = "Lookout";
	nameLibrary[8] = "Outpost";
	nameLibrary[9] = "Forest";

	int pick;
	int picks[numPicks];

	int needNewPick;		// Boolean

	/* generate unique random integers as indices of room names in nameLibrary */

	int j;
	for (i = 0; i < numPicks; i++) {	

		do {
	
			needNewPick = 0;
	
			pick = rand() % (librarySize);
			
			/* ensure random number is unique */

			for (j = 0; j < i; j++) {
				if (picks[j] == pick) {
					
					needNewPick = 1;

				}
			}
		
		} while (needNewPick);

		picks[i] = pick;

	}

	/* store chosen room names */

	for (i = 0; i < numPicks; i++) {
	
		strcpy(roomArr[i].name, nameLibrary[picks[i]]);
		roomArr[i].num = i;
	
	}

}


/* this function opens filestream data member of each Room struct in roomArr array */

void openFileStreams(struct Room* roomArr, int numStreams) {

	// printf("** openFileStreams() **\n");

	/* define local variables */

	int strLen = 30;
	char* filestring = malloc(sizeof(char* ) * strLen);
	char* filepath = malloc(sizeof(char* ) * strLen);
	memset(filepath, 0, strLen);
	char* pid = malloc(sizeof(char* ) * strLen);
	memset(pid, 0, strLen);
	sprintf(pid, "%d", getpid());

	/* construct file path string amd make directory */

	sprintf(filepath, "%s", "hindss.rooms.");
	strcat(filepath, pid);
	mkdir(filepath, 0777);
	strcat(filepath, "/");

	/* iterate through and open filestreams */

	int i;
	for (i = 0; i < numStreams; i++) {
		
		memset(filestring, 0, strLen);
		sprintf(filestring, "%s", filepath);
		strcat(filestring, roomArr[i].name);
				
		roomArr[i].fileptr = fopen(filestring, "w");
		
	}

	/* free locally allocated variables */

	free(filestring);
	free(filepath);
	free(pid);

	/* for each room of 7, write "ROOM NAME: $name" to file */	 	

	for (i = 0; i < numStreams; i++) {

		fprintf(roomArr[i].fileptr, "ROOM NAME: %s\n", roomArr[i].name);		

	}

}


/* Connect rooms until they form a complete graph of two-way connections with no self or duplicate connections */

void connectRoomGraph(struct Room* roomArr, int numRooms) {
	
	// printf("** connectFileGraph() **\n");

	int k;
	int o;	
	for (k = 0; k < numRooms; k++) {
	
		/* set every connection to -1 */

		for (o = 0; o < (numRooms - 1); o++) {
			roomArr[k].connections[o] = -1;
		}

		/* set numConnections to 0 for each room struct */

		roomArr[k].numConnections = 0;

	}	
	

	/* while there are rooms not in the graph */
	
	int graphComplete = 0;

	int i;
	int j;
	
	while (!graphComplete) {

		/* choose random endpoints */

		i = rand() % (numRooms);
		j = i;
			
		/* ensure uniqueness of endpoints selected */
	
		while (j == i) {
			j = rand() % (numRooms);
		}			
		
		/* if these rooms are not already connected */

		if (!areConnected(roomArr, i, j)) {

			/* add two way connection */

			connect(roomArr, i, j);
	
			/* dfs and check that each room has at least 3 connections to check if graph complete. 
 				THIS DEPTH FIRST SEARCH IS UNNECESSARY, as a graph of 7 rooms with a minimum of 3 outgoing connections 
  				each will always be complete. I left it in because it would allow a programmer to increase the number of rooms
  				in the graph beyond 7.  */

			if (dfsReachesAll(roomArr, numRooms, i) && minConnEach(roomArr, numRooms, 3)) {

				graphComplete++;		// set graph complete

			}

		}
	
	}

}


/* Connect rooms a and b */

void connect(struct Room* roomArr, int a, int b) {
	
	// printf("** connect %d %d **\n", a, b);

	int o = 0;		// index
	int connectionSet = 0;	// Boolean

	/* add b to a's connections and increment a's numConnections */

	while (!connectionSet) {
		if (roomArr[a].connections[o] == -1) {
			roomArr[a].connections[o] = b;
			roomArr[a].numConnections++;
			connectionSet++;
		}
		else {
			/* increment connections index to search for open slot in connections array (sentinel = -1) */
			o++;
		}
	}	

	/* add a to b's connections and increment b's numConnections */

	int p = 0;
	connectionSet = 0;
	while (!connectionSet) {
		if (roomArr[b].connections[p] == -1) {
			roomArr[b].connections[p] = a;
			roomArr[b].numConnections++;
			connectionSet++;
		}
		else {
			/* increment connections index to search for open slot in connections array (sentinel = -1 ) */
			p++;
		}
	}

	/* write connections to room filestreams for rooms a and b */

	fprintf(roomArr[a].fileptr, "CONNECTION %d: %s\n", o + 1, roomArr[b].name);
	fprintf(roomArr[b].fileptr, "CONNECTION %d: %s\n", p + 1, roomArr[a].name);

}


/* Boolean function to determine whether rooms a and b of roomArr are connected */

int areConnected(struct Room* roomArr, int a, int b) {

	// printf("** areConnected() **\n");

	int i;
	for (i = 0; i < 6; i++) {
		if (roomArr[a].connections[i] == b) {
			//printf("yes connected room %d connection %d to %d\n", a, i, b);
			
			return 1;
		} 
	}
	//printf("not connected\n");
	return 0;

}


/* determine that each room has at least a certain number of connections */

int minConnEach(struct Room* roomArr, int numRooms, int min) {

	int i;
	for (i = 0; i < numRooms; i++) {
		if (roomArr[i].numConnections < min) {
			return 0;
		}
	}

	return 1;

}


/* Boolean function which performs a depth first search on the room graph to determine if it is complete */

int dfsReachesAll(struct Room* roomGraph, int numRooms, int begin) {

	// printf("** dfsReachesAll() **\n");

	int i;
	int j;

	/* initialize array to store ID numbers of rooms visited */
	
	int* roomsVisited = (int*) malloc(numRooms * sizeof(int));
	
	for (i = 0; i < numRooms; i++) {
		roomsVisited[i] = -1;
	}

	/* initialize a dynArr stack of ints */

	struct IntStack* stack = (struct IntStack*) malloc(numRooms * sizeof(struct IntStack));
	
	stack->size = 0;
	stack->top = NULL;
	
	int examine;
	int roomVisited;
	int marked;

	/* push begin room index to the stack */

	stackPush(stack, begin);

	/* while stack not empty */

	while (stack->size > 0) {
		
		/* pop an int from the stack to examine */

		examine = stackPop(stack);
		
		/* if int not in roomsVisited */
		
		roomVisited = 0;
		for (i = 0; i < numRooms; i++) {
			if (examine == roomsVisited[i]) {
				roomVisited++;
			} 
		}
		
		if (!roomVisited) {
		
			/* add ID number to roomsVisited */
	
			marked = 0;
			for (i = 0; i < numRooms; i++) {
				if (roomsVisited[i] == -1 && marked == 0) {
					roomsVisited[i] = examine;
					marked++;

					if (roomsVisited[numRooms - 1] != -1) {
						return 1;
					}

				}
			}			
	
			/* push neighbors to the stack */
			
			for (i = 0; i < (numRooms - 1); i++) {
				roomVisited = 0;
				if (roomGraph[examine].connections[i] != -1) {
					
					for (j = 0; j < numRooms; j++) {
						
						if (roomsVisited[j] == roomGraph[examine].connections[i]) {
							
							roomVisited++;
							
						}
					}
			
					if (!roomVisited) {
						stackPush(stack, roomGraph[examine].connections[i]);
					}	

				}
			}
		
		}

	}
	
	int ret = 1;
	for (i = 0; i < numRooms; i++) {
		if (roomsVisited[i] == -1) {
			
			ret = 0;
		}
	} 
	
	free(roomsVisited);
	free(stack);

	return ret;
}


/* push a node to the stack within the depth first search */

void stackPush(struct IntStack* stack, int val) {

	// printf("** stackPush() **\n");

	/* allocate an intstacknode */
	struct IntStackNode* newNode = malloc(sizeof(struct IntStackNode* ));	

	/* set member data */
	newNode->value = val;
	newNode->next = stack->top;
	
	/* set new node as top */
	stack->top = newNode;

	/* increment stack size */
	stack->size++;

}


/* pop a node from the stack within depth first search. Returns the ID number of that node */

int stackPop(struct IntStack* stack) {

	// printf("** stackPop() **\n");

	/* grab top node val */
	int result = stack->top->value;	
	
	/* point garbage node toward top */
	struct IntStackNode* garbage;
	garbage = stack->top;	
	
	/* set top to top->next; */	
	stack->top = stack->top->next;
	
	/* deallocate garbage top node */
	free(garbage);

	/* decrement stack size */
	stack->size--;	
	
	/* return val from above */
	return result;

}


/* assign START (1), MID (5), and END (1) room statuses to rooms in roomArr */ 

void assignRoomStatuses(struct Room* roomArr, int numRooms) {

	// printf("** assignRoomStatuses() **\n");

	/* local variables */	

	int i;
	int startIndex = rand() % (numRooms - 1);
	int endIndex = startIndex;
	int strLen = 12;

	char* start = "START_ROOM";
	char* end = "END_ROOM";
	char* mid = "MID_ROOM";	

	/* ensure uniqueness of end and start indices */

	while (endIndex == startIndex) {
		
		endIndex = rand() % (numRooms - 1);

	}

	/* mark start and end room files, as well as mid rooms */ 

	for (i = 0; i < numRooms; i++) {
		
		roomArr[i].type = (char* ) malloc(15); 
		memset(roomArr[i].type, 0, 15);

		if (i == startIndex) {
			
			strcpy(roomArr[i].type, start);

		}
		else if (i == endIndex) {

			strcpy(roomArr[i].type, end);

		}
		else {
			
			strcpy(roomArr[i].type, mid);

		}
	
		/* print type to filestream for each room struct. Will write to end of file */

		fprintf(roomArr[i].fileptr, "ROOM TYPE: %s\n", roomArr[i].type);
	
	}

}


/* free memory allocated for Room structs */

void deallocateRooms(struct Room* roomArr, int roomCount) {

	// printf("** freeFileStreams() **\n");

	int i;

	for (i = 0; i < roomCount; i++) {
		
		/* close all filestreams */

		fclose(roomArr[i].fileptr);

	}
	
	for (i = 0; i < roomCount; i++) {
	
		/* deallocate room name strings */

		free(roomArr[i].name);

	}

	for (i = 0; i < roomCount; i++) {

		/* free type strings */

		free(roomArr[i].type);

	}

}
