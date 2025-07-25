/*
------------------------------------------------------------------------------
isaac64.c: My random number generator for 64-bit machines.
By Bob Jenkins, 1996.  Public Domain.

(https://burtleburtle.net/bob/rand/isaacafa.html)

Slighly modified by Tripp R.
------------------------------------------------------------------------------
*/
#ifndef STANDARD
#include "standard.h"
#endif
#ifndef ISAAC64
#include "isaac64.h"
#endif

extern    ub8 randrsl[RANDSIZ], randCount;
static    ub8 mm[RANDSIZ];
static    ub8 aa = 0, bb = 0, cc = 0;

#define ind(mm,x)  (*(ub8 *)((ub1 *)(mm) + ((x) & ((RANDSIZ-1)<<3))))
#define rngstep(mix,a,b,mm,m,m2,r,x) \
{ \
  x = *m;  \
  a = (mix) + *(m2++); \
  *(m++) = y = ind(mm,x) + a + b; \
  *(r++) = b = ind(mm,y>>RANDSIZL) + x; \
}

void isaac64(void)
{
    register ub8 a, b, x, y, * m, * m2, * r, * mend;
    m = mm; r = randrsl;
    a = aa; b = bb + (++cc);
    for (m = mm, mend = m2 = m + (RANDSIZ / 2); m < mend; )
    {
        rngstep(~(a ^ (a << 21)), a, b, mm, m, m2, r, x);
        rngstep(a ^ (a >> 5), a, b, mm, m, m2, r, x);
        rngstep(a ^ (a << 12), a, b, mm, m, m2, r, x);
        rngstep(a ^ (a >> 33), a, b, mm, m, m2, r, x);
    }
    for (m2 = mm; m2 < mend; )
    {
        rngstep(~(a ^ (a << 21)), a, b, mm, m, m2, r, x);
        rngstep(a ^ (a >> 5), a, b, mm, m, m2, r, x);
        rngstep(a ^ (a << 12), a, b, mm, m, m2, r, x);
        rngstep(a ^ (a >> 33), a, b, mm, m, m2, r, x);
    }
    bb = b; aa = a;
}

#define mix(a,b,c,d,e,f,g,h) \
{ \
   a-=e; f^=h>>9;  h+=a; \
   b-=f; g^=a<<9;  a+=b; \
   c-=g; h^=b>>23; b+=c; \
   d-=h; a^=c<<15; c+=d; \
   e-=a; b^=d>>14; d+=e; \
   f-=b; c^=e<<20; e+=f; \
   g-=c; d^=f>>17; f+=g; \
   h-=d; e^=g<<14; g+=h; \
}

void isc_randinit(word flag)
{
    word i;
    ub8 a, b, c, d, e, f, g, h;
    aa = bb = cc = (ub8)0;
    a = b = c = d = e = f = g = h = 0x9e3779b97f4a7c13ULL;  /* the golden ratio */

    for (i = 0; i < 4; ++i)                    /* scramble it */
    {
        mix(a, b, c, d, e, f, g, h);
    }

    for (i = 0; i < RANDSIZ; i += 8)   /* fill in mm[] with messy stuff */
    {
        if (flag)                  /* use all the information in the seed */
        {
            a += randrsl[i]; b += randrsl[i + 1]; c += randrsl[i + 2]; d += randrsl[i + 3];
            e += randrsl[i + 4]; f += randrsl[i + 5]; g += randrsl[i + 6]; h += randrsl[i + 7];
        }
        mix(a, b, c, d, e, f, g, h);
        mm[i] = a; mm[i + 1] = b; mm[i + 2] = c; mm[i + 3] = d;
        mm[i + 4] = e; mm[i + 5] = f; mm[i + 6] = g; mm[i + 7] = h;
    }

    if (flag)
    {        /* do a second pass to make all of the seed affect all of mm */
        for (i = 0; i < RANDSIZ; i += 8)
        {
            a += mm[i]; b += mm[i + 1]; c += mm[i + 2]; d += mm[i + 3];
            e += mm[i + 4]; f += mm[i + 5]; g += mm[i + 6]; h += mm[i + 7];
            mix(a, b, c, d, e, f, g, h);
            mm[i] = a; mm[i + 1] = b; mm[i + 2] = c; mm[i + 3] = d;
            mm[i + 4] = e; mm[i + 5] = f; mm[i + 6] = g; mm[i + 7] = h;
        }
    }

    isaac64();          /* fill in the first set of results */
    randCount = RANDSIZ;    /* prepare to use the first set of results */
}

/*
int main()
{
    ub8 i, j;
    aa = bb = cc = (ub8)0;
    for (i = 0; i < RANDSIZ; ++i)
        mm[i] = (ub8)0;

    randinit(TRUE);
    for (i = 0; i < 2; ++i)
    {
        isaac64();
        for (j = 0; j < RANDSIZ; ++j)
        {
            printf("%.8lx%.8lx", (ub4)(randrsl[j] >> 32), (ub4)randrsl[j]);
            if ((j & 3) == 3)
                printf("\n");
        }
    }
}

*/