#ifndef OPTLIST_HPP
#define OPTLIST_HPP

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/***************************************************************************
*                                CONSTANTS
***************************************************************************/
#define    OL_NOINDEX    -1
/***************************************************************************
*                            TYPE DEFINITIONS
***************************************************************************/
typedef struct option_t
{
    char option;             
    char *argument;     
    int argIndex;              
    struct option_t *next;      
} 	option_t;
/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
static option_t *MakeOpt(const char option, char *const argument, const int index);
static size_t MatchOpt(const char argument, char *const options);
option_t *GetOptList(const int argc, char *const argv[], char *const options);
void FreeOptList(option_t *list);
char *FindFileName(const char *const fullPath);

#endif
