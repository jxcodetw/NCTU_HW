#include "cs.h"
/* print a sparse matrix; use %g for integers to avoid differences with csi */
csi cs_print (const cs *A, csi brief)
{
    csi p, j, m, n, nzmax, nz, *Ap, *Ai ;
    float *Ax ;
    if (!A) { printf ("(null)\n") ; return (0) ; }
    m = A->m ; n = A->n ; Ap = A->p ; Ai = A->i ; Ax = A->x ;
    nzmax = A->nzmax ; nz = A->nz ;
    printf ("CSparse Version %d.%d.%d, %s.  %s\n", CS_VER, CS_SUBVER,
        CS_SUBSUB, CS_DATE, CS_COPYRIGHT) ;
    if (nz < 0)
    {
        printf ("%g-by-%g, nzmax: %g nnz: %g, 1-norm: %g\n", (float) m,
            (float) n, (float) nzmax, (float) (Ap [n]), cs_norm (A)) ;
        for (j = 0 ; j < n ; j++)
        {
            printf ("    col %g : locations %g to %g\n", (float) j, 
                (float) (Ap [j]), (float) (Ap [j+1]-1)) ;
            for (p = Ap [j] ; p < Ap [j+1] ; p++)
            {
                printf ("      %g : %g\n", (float) (Ai [p]), Ax ? Ax [p] : 1) ;
                if (brief && p > 20) { printf ("  ...\n") ; return (1) ; }
            }
        }
    }
    else
    {
        printf ("triplet: %g-by-%g, nzmax: %g nnz: %g\n", (float) m,
            (float) n, (float) nzmax, (float) nz) ;
        for (p = 0 ; p < nz ; p++)
        {
            printf ("    %g %g : %g\n", (float) (Ai [p]), (float) (Ap [p]),
                Ax ? Ax [p] : 1) ;
            if (brief && p > 20) { printf ("  ...\n") ; return (1) ; }
        }
    }
    return (1) ;
}
