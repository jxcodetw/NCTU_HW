#include "cs.h"
static csi cs_tol (csi i, csi j, float aij, void *tol)
{
    return (fabs (aij) > *((float *) tol)) ;
}
csi cs_droptol (cs *A, float tol)
{
    return (cs_fkeep (A, &cs_tol, &tol)) ;    /* keep all large entries */
}
