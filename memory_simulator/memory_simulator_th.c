/*  Lab 7: Memory Simulator
 *  
 *  File:       memory_simulator_th.c
 *  Version:    1.00
 *  Author:     Trey Harrison (CWID: 11368768)
 *  Email:      ntharrison@crimson.ua.edu
 *  Created:    16 April, 2016
 *  Modified:   19 April, 2016
 *  
 *  IMPORTANT:  The program must be compiled using ANSI C specification C99 
 *  or later in order for the program to build.
 *
 *  This program collects specifications of a two-tier memory system along
 *  with the references to be simulated via user input.  It then processes 
 *  that data to indicate for the given scenario what the hit/miss rate is, 
 *  what the best possible hit/miss rate is, a table showing status of each 
 *  memory reference, and a table showing the final state of the cache memory 
 *  tier.
 *  
 *  Copyright (C) 2016, Trey Harrison
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *  
 *  Source: <https://github.com/Attrit1on/embedded_systems>
 *  
 */

#include <avr/wdt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define FIFO 0
#define LRU 1
#define READ 0
#define WRITE 1
#define HIT 1
#define MISS 0

// cache colums
#define DIRTY 0
#define VALID 1
//#define MM_BLK 2
#define TAG 3
#define USED 4

// mem_references columns
#define ACCESS_TYPE 0
#define ADDRESS 1
#define MM_BLK	2
#define CM_SET  3
#define CM_BLK	4
#define HITMISS	5

int mm_size, cache_size, blk_size, set_size, num_cache_blocks, r_policy, num_references;
int address_size, offset_size, index_size, tag_size, total_cache_size;
int** mem_references;
int** cache;

void readUserInteger(int *val, int lower_bound, int upper_bound, int set_associative) {
	while (1) {
		// Get input from user and convert from string to integer
		char input[BUFSIZ];
		if (fgets(input, sizeof(input), stdin) != NULL) {
			*val = atoi(input);
		}

		// For set-associative setting only, check to see if n-way associative
		if (set_associative) {
			if (input[0] == 'N' || input[0] == 'n') {
				*val = num_cache_blocks;
				break;
			}
		}

		// Check if input is a power of 2
		if (!((*val & (~*val + 1)) == *val)) {
			printf("Error: Input must be a power of 2! Try again: ");
		}
		// Check if input satisfies boundary conditions, input is valid if so
		else if (*val >= lower_bound && *val <= upper_bound) {
			break;
		}
		else {
			printf("Error: Invalid input! Try again: ");
		}
	}
}

void getUserInput() {	
	// Get the main memory size
	printf("\n\nEnter the size of the main memory in bytes(min: 4, max: 32768): ");
	readUserInteger(&mm_size, 4, 32768, 0);

	// Get the cache size
	printf("\nEnter the size of the cache in bytes(min: 2, max: 32768): ");
	readUserInteger(&cache_size, 2, 32768, 0);
	while (cache_size > mm_size) {
		printf("Error: the cache cannot be larger than the main memory\n");
		readUserInteger(&cache_size, 2, 32768, 0);
	}

	// Get the block size
	printf("\nEnter the block/line size in bytes(min: 2, max: 32768): ");
	readUserInteger(&blk_size, 2, 32768, 0);
	while(blk_size > cache_size) {
		printf("Error: Block size cannot be larger than cache size! Try again: ");
		readUserInteger(&blk_size, 2, 32768, 0);
	}

	// calculate the number for cache blocks
	num_cache_blocks = cache_size/blk_size;

	// Get the set-associativity
	printf("\nEnter the set-associativity (input n for a fully-associative mapping): ");
	readUserInteger(&set_size, 1, num_cache_blocks, 1);
	while(set_size > num_cache_blocks) {
		printf("Error: The set-associativity cannot be greater than %d! Try again: ", num_cache_blocks);
		readUserInteger(&set_size, 1, 32768, 1);
	}
	
	// Get the replacement policy
	printf("\nEnter the replacement policy(L=LRU, F=FIFO): ");
	while(1) {
		char input[BUFSIZ];
		fgets(input, sizeof(input), stdin);
		// Check for line feed in second character
		if (input[1] == 10) {
			if (input[0] == 'L' || input[0] == 'l') {
				r_policy = LRU;
				break;
			}
			else if (input[0] == 'F' || input[0] == 'f') {
				r_policy = FIFO;
				break;
			}
		}
		printf("Error: Invalid input! Try again: ");
	}
}

void readInputFile() {
	// Get the file name and open the file
	FILE *input_file;
	printf("\nPlease enter the name of the file containing the memory references: ");
	while(1) {
		char input[BUFSIZ] = {0};
		fgets(input, sizeof(input), stdin);
		for (int i = 0; i < sizeof(input); i++) {
			if (input[i] == 10) {
				input[i] = 0;
				break;
			}
		}
		input_file = fopen(input, "r");
		if (input_file) {
			break;
		}
		else {
			printf("Error: File not found! Try again: ");
		}
	}

	// Read the memory references from the file and store in the mem_references array
	char input[16] = {0};
	char read_write;
	int reference;
	for (int i = 0; fgets(input, sizeof(input), input_file) != NULL; i++) {
		if (i == 0) {
			sscanf(input, "%d", &num_references);
			// Initialize the mem_references array
			mem_references = malloc(num_references*sizeof(int*));
			for (int i = 0; i < num_references; i++) {
				mem_references[i] = malloc(6*sizeof(int*));
			}
		}
		else if (i > 1) {
			sscanf(input, "%c %d", &read_write, &reference);
			if (read_write == 'R' || read_write == 'r') {
				mem_references[i-2][ACCESS_TYPE] = READ;

			}
			else if (read_write == 'W' || read_write == 'w') {
				mem_references[i-2][ACCESS_TYPE] = WRITE;
			}
			else {
				printf("Error: Invalid input file syntax! Program terminating!");
				exit(0);
			}
			if (reference > mm_size-1 || reference < 0) {
				printf("Error: The reference to address %d is to a non-existent location! Program terminating!", reference);
				exit(0);
			}
			mem_references[i-2][ADDRESS] = reference;
			mem_references[i-2][MM_BLK] = reference/blk_size;
			mem_references[i-2][CM_SET] = (reference/blk_size)%(num_cache_blocks/set_size);	
		}
	}
}

void calculateSizes() {
	address_size = log2(mm_size);
	offset_size = log2(blk_size);
	index_size = log2(num_cache_blocks/set_size);
	tag_size = address_size - offset_size - index_size;
	total_cache_size = cache_size + (num_cache_blocks*(tag_size+2))/8;
}

void computeIndexAndTag(int address, int* index, int* tag) {
	int index_mask = 0, tag_mask = 0;
	for (int i = 0; i < index_size; i++) {
		index_mask |= (1<<(i + offset_size));
	}
	for (int i = 0; i < tag_size; i++) {
		tag_mask |= (1<<(i + offset_size + index_size));
	}
	*index = (address&index_mask)>>offset_size;
	*tag = (address&tag_mask)>>(offset_size+index_size);
}

void fifoSimulator() {
	int *fifo_positions;
	fifo_positions = malloc((num_cache_blocks/set_size)*sizeof(int*));
	memset(fifo_positions, 0x00, (num_cache_blocks/set_size)*sizeof(int*));
	// Process each memory reference one at a time
	for (int i = 0; i < num_references; i++) {
			// Get the current address
			int address = mem_references[i][ADDRESS];
			// Get the index and tag for the current address
			int index, tag;
			computeIndexAndTag(mem_references[i][ADDRESS], &index, &tag);
			// Check for a hit
			int hit = 0;
			for (int j = set_size*index; j < (set_size*index)+set_size; j++) {
				if (cache[j][VALID] == 1) {
					if (cache[j][TAG] == tag) {
						hit = 1;
						mem_references[i][CM_BLK] = j;
						mem_references[i][HITMISS] = HIT;
						cache[j][VALID] = 1;
						if (mem_references[i][ACCESS_TYPE] == WRITE) {
							cache[j][DIRTY] = 1;
						}
					}
				}
			}
			// Handle a reference for which there is not a hit
			if (hit == 0) {
				mem_references[i][HITMISS] = MISS;
				int cache_block = index*set_size+fifo_positions[index];
				//printf("\ncache_block: %d", cache_block);
				fifo_positions[index]++;
				mem_references[i][CM_BLK] = cache_block;


				if (mem_references[i][ACCESS_TYPE] == WRITE) {
					cache[cache_block][DIRTY] = 1;
				}
				else {
					cache[cache_block][DIRTY] = 0;
				}
				cache[cache_block][VALID] = 1;
				cache[cache_block][TAG] = tag;
				cache[cache_block][MM_BLK] = address/blk_size;
			}
			// If this sets fifo index is maxed, reset to 0
			if (fifo_positions[index] == set_size) {
				fifo_positions[index] = 0;
			}
	}
	// Free memory allocated to fifo_positions
	free(fifo_positions);
}

void lruSimulator() {
	// Process each memory reference one at a time
	for (int i = 0; i < num_references; i++) {
		// Get the current address
		int address = mem_references[i][ADDRESS];
		// Get the index and tag for the current address
		int index, tag;
		computeIndexAndTag(mem_references[i][ADDRESS], &index, &tag);
		int hit = 0;
		// Check for a hit
		for (int j = set_size*index; j < (set_size*index)+set_size; j++) {
			if (cache[j][VALID] == 1) {
				if (cache[j][TAG] == tag) {
					hit = 1;
					mem_references[i][CM_BLK] = j;
					mem_references[i][HITMISS] = HIT;
					if (mem_references[i][ACCESS_TYPE] == WRITE) {
						cache[j][DIRTY] = 1;
					}

					// Handle LRU field
					for (int k = index*set_size; k < (index*set_size)+set_size; k++) {
						if (cache[k][USED] != -1) {
							cache[k][USED]++;
						}
					}
					cache[j][USED] = 0;
				}
			}
		}
		// Handle a reference for which there is not a hit
		if (hit == 0) {
			mem_references[i][HITMISS] = MISS;
			int cache_block = index*set_size;
			for (int j = index*set_size; j < (index*set_size)+set_size; j++) {
				if (cache[j][USED] != -1) {
					cache[j][USED]++;
				}
				if (cache[j][USED] > cache[cache_block][USED]) {
					cache_block = j;
					cache[j][USED] = 0;
				}
				else if (cache[j][USED] == -1) {
					cache_block = j;
					cache[j][USED] = 0;
					for (int k = 0; k < (index*set_size)+set_size; k++) {
						if (cache[j][USED] != -1) {
							cache[j][USED]++;
						}
					}
					break;
				}

			}
			mem_references[i][CM_BLK] = cache_block;


			if (mem_references[i][ACCESS_TYPE] == WRITE) {
				cache[cache_block][DIRTY] = 1;
			}
			else {
				cache[cache_block][DIRTY] = 0;
			}
			cache[cache_block][VALID] = 1;
			cache[cache_block][TAG] = tag;
			cache[cache_block][MM_BLK] = address/blk_size;
		}
	}
}

void calculateHitRates(float* highest_hit_rate, float* actual_hit_rate) {
	// Calculate the best possible hit rate
	int possible_hit_count = 0;
	for (int i = 0; i < num_references; i++) {
		for (int j = 0; j < i; j++) {
			if (mem_references[j][MM_BLK] == mem_references[i][MM_BLK]) {
				possible_hit_count++;
				break;
			}
		}
	}
	*highest_hit_rate = (float)100.0*possible_hit_count/num_references;
	
	// Calculate the actual hit rate
	int actual_hit_count = 0;
	for (int i = 0; i < num_references; i++) {
		if (mem_references[i][HITMISS] == HIT) {
			actual_hit_count++;
		}
	}
	*actual_hit_rate = (float)100.0*actual_hit_count/num_references;
}

void printBinary(int val, int numBits) {
	// Print val in binary with numBits bits or print X if unknown val(i.e. -1)
	for (int i = 1; i <= numBits; i++) {
		if (val == -1) {
			printf("X");
		}
		else if (val & (1<<(numBits-i))) {
			printf("1");
		}
		else {
			printf("0");
		}
	}
}

void printOutput() {
	printf("\n\nSimulator Output:");
	printf("\nTotal address lines required = %d", address_size);
	printf("\nNumber of bits for offset = %d", offset_size);
	printf("\nNumber of bits for index = %d", index_size);
	printf("\nNumber of bits for tag = %d", tag_size);
	printf("\nTotal cache size required = %d bytes", total_cache_size);

	printf("\n\n---Memory Reference Table---");
	printf("\nmm address\tmm blk #\tcm set #\tcm blk #\thit/miss\n");
	for (int i = 0; i < num_references; i++) {
		for (int j = 1; j < 6; j++) {
			if (j<5) {
				printf("%d\t\t", mem_references[i][j]);
			}
			else {
				if (mem_references[i][j] == HIT) {
					printf("hit");
				}
				else {
					printf("miss");
				}
			}
		}
		printf("\n");
	}

	float highest_hit_rate, actual_hit_rate;
	calculateHitRates(&highest_hit_rate, &actual_hit_rate);
	printf("\nHighest possible hit rate = %.2f%%", highest_hit_rate);
	printf("\nActual hit rate = %.2f%%", actual_hit_rate);

	printf("\n\n---Final Cache---");
	printf("\nCache Blk #\tDirty Bit\tValid Bit\tTag\t\tMM Blk #\n");
	for (int i = 0; i < num_cache_blocks; i++) {
		for (int j = -1; j < 4; j++) {
			if (j==-1) {
				printf("%d\t\t", i);
			}
			else if (j < 2) {
				printf("%d\t\t", cache[i][j]);
			}
			else if (j == 2) {
				printBinary(cache[i][TAG], tag_size);
				printf("\t");
				if (tag_size < 8) {
					printf("\t");
				}
			}
			else {
				if (cache[i][MM_BLK] == -1) {
					printf("XXX");
				}
				else {
					printf("%d", cache[i][MM_BLK]);
				}
			}
		}
		printf("\n");
	}
}

int main() {
	// Execute main program loop
	while(1) {
		printf("\n***Press 'ctrl+c' to exit the program at any point***");
		getUserInput();
		readInputFile();
		calculateSizes();

		// Allocate the memory to the cache array based on user input
		cache = malloc(num_cache_blocks*sizeof(int*));
		for (int i = 0; i < num_cache_blocks; i++) {
			cache[i] = malloc(5*sizeof(int*));
			memset(cache[i], 0x00, 5*sizeof(int*));
		}
		// Set relevant fields of the cache to -1
		for (int i = 0; i < num_cache_blocks; i++) {
			for (int j = 2; j < 5; j++) {
				cache[i][j] = -1;
			}
		}

		// Pick simulation type based on user selected replacement policy
		if (r_policy == FIFO) {
			fifoSimulator();
		}
		else {
			lruSimulator();
		}
		printOutput();

		// Free up user allocation(malloc) of global arrays
		free(cache);
		free(mem_references);

		// Check if user wishes to continue
		printf("\nContinue?(y/n): ");
		while(1) {
			char input[BUFSIZ];
			fgets(input, sizeof(input), stdin);
			// Check for line feed in second character
			if (input[1] == 10) {
				if (input[0] == 'Y' || input[0] == 'y') {
					break;
				}
				else if (input[0] == 'N' || input[0] == 'n') {
					printf("\n***Goodbye!\n");
					exit(0);
				}
			}
			printf("Error: You must enter y(yes) or n(no)! Try again: ");
		}
		// Main program loop end
	} 
	return 0;
}