/*
------------------------------------------------------------------------------
isaac64.h: definitions for a random number generator
Bob Jenkins, 1996, Public Domain

(https://burtleburtle.net/bob/rand/isaacafa.html)

Slighly modified by Tripp R.
------------------------------------------------------------------------------
*/

#include <stdbool.h>
#include "standard.h"


#ifndef ISAAC64
#define ISAAC64

#define RANDSIZL   (8)
#define RANDSIZ    (1<<RANDSIZL)

ub8 randrsl[RANDSIZ], randCount;

/*
------------------------------------------------------------------------------
 If (flag==TRUE), then use the contents of randrsl[0..255] as the seed.
------------------------------------------------------------------------------
*/
void isc_randinit(word flag);

void isaac64(void);


/*
------------------------------------------------------------------------------
 Call isc_rand() to retrieve a single 64-bit random value
------------------------------------------------------------------------------
*/
#define isc_rand() \
   (!randCount-- ? (isaac64(), randCount=RANDSIZ-1, randrsl[randCount]) : \
                 randrsl[randCount])

#endif  /* RAND */

