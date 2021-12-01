/*
 * GraphApp - Cross-Platform Graphics Programming Library.
 *
 * File: array.h -- memory allocation functions.
 * Platform: Neutral  Version: 3.00  Date: 2002/08/02
 *
 * Version: 3.00  Changes: New version by L. Patrick.
 */

/*  Copyright (C) 1993-2001 L. Patrick
*/

extern long app_mem_length(void *v);

/*
 *  C macro defintions.
 */

#define create(type)  ( app_zero_alloc(sizeof(type)) )
#define array(n,type) ( app_zero_alloc(n*sizeof(type)) )
#define len(a)        ( app_mem_length((a))/sizeof((a)[0]) )
#define append(a,e)   ( (a)=app_realloc((a),app_mem_length(a)+ \
			sizeof((a)[0])), (a)[len(a)-1]=(e) )
#define discard(a)    ( app_free((a)), (a)=0 )
