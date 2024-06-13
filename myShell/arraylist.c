#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include "arraylist.h"

#ifndef DEBUG
#define DEBUG 0
#endif

void al_init(arraylist_t *L, unsigned size)
{
    L->data = malloc(size * sizeof(char*));
    L->length = 0;
    L->capacity = size;
}

void al_destroy(arraylist_t *L)
{
    for (int i = 0; i<L->length; i++){
        if(L->data[i] != NULL){
            free(L->data[i]);
        }
    }
    free(L->data);
}

unsigned al_length(arraylist_t *L)
{
    return L->length;
}

void al_push(arraylist_t *L, char *item)
{
    //printf("pushing item into arraylist: %s\n", item);
    if (L->length == L->capacity) {
	L->capacity *= 2;
	char **temp = realloc(L->data, L->capacity * sizeof(char*));
	if (!temp) { 
	    // for our own purposes, we can decide how to handle this error
	    // for more general code, it would be better to indicate failure to our caller
	    fprintf(stderr, "Out of memory!\n");
	    exit(EXIT_FAILURE);
	}
    
	L->data = temp;
	if (DEBUG) printf("Resized array to %u\n", L->capacity);
    }
    
    L->data[L->length] = item;
        
    // 
    // else{
    //     L->data[L->length] = malloc((strlen(item) + 1) * sizeof(char));
    //     strcpy(L->data[L->length], item);
    //     L->data[L->length][strlen(item)] = '\0';
    // }
   
    L->length++;
}

// returns 1 on success and writes popped item to dest
// returns 0 on failure (list empty)

char al_pop(arraylist_t *L, char **dest)
{
    if (L->length == 0) return 0;

    L->length--;
    *dest = L->data[L->length];


    return 1;
}

