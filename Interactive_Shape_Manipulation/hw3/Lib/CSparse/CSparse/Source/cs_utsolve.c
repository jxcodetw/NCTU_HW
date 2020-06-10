#include "cs.h"
/* solve U'x=b where x and b are dense.  x=b on input, solution on output. */
csi cs_utsolve (const cs *U, float *x)
{
    csi p, j, n, *Up, *Ui ;
    float *Ux ;
    if (!CS_CSC (U) || !x) return (0) ;                     /* check inputs */
    n = U->n ; Up = U->p ; Ui = U->i ; Ux = U->x ;
    for (j = 0 ; j < n ; j++)
    {
        for (p = Up [j] ; p < Up [j+1]-1 ; p++)
        {
            x [j] -= Ux [p] * x [Ui [p]] ;
        }
        x [j] /= Ux [Up [j+1]-1] ;
    }
    return (1) ;
}