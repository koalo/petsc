#ifndef lint
static char vcid[] = "$Id: mpiaijpc.c,v 1.26 1997/01/06 20:24:32 balay Exp balay $";
#endif
/*
   Defines a block Jacobi preconditioner for the MPIAIJ format.
   Handles special case of  single block per processor.
   This file knows about storage formats for MPIAIJ matrices.
   The general case is handled in aijpc.c
*/
#include "src/mat/impls/aij/mpi/mpiaij.h"
#include "src/pc/pcimpl.h"
#include "src/vec/vecimpl.h"
#include "src/pc/impls/bjacobi/bjacobi.h"
#include "sles.h"

typedef struct {
  Vec  x, y;
} PC_BJacobi_MPIAIJ;

#undef __FUNC__  
#define __FUNC__ "PCDestroy_BJacobi_MPIAIJ"
int PCDestroy_BJacobi_MPIAIJ(PetscObject obj)
{
  PC                pc = (PC) obj;
  PC_BJacobi        *jac = (PC_BJacobi *) pc->data;
  PC_BJacobi_MPIAIJ *bjac = (PC_BJacobi_MPIAIJ *) jac->data;
  int               ierr;

  ierr = SLESDestroy(jac->sles[0]); CHKERRQ(ierr);
  PetscFree(jac->sles);
  ierr = VecDestroy(bjac->x); CHKERRQ(ierr);
  ierr = VecDestroy(bjac->y); CHKERRQ(ierr);
  if (jac->l_lens) PetscFree(jac->l_lens);
  if (jac->g_lens) PetscFree(jac->g_lens);
  PetscFree(bjac); PetscFree(jac); 
  return 0;
}


#undef __FUNC__  
#define __FUNC__ "PCSetUpOnBlocks_BJacobi_MPIAIJ"
int PCSetUpOnBlocks_BJacobi_MPIAIJ(PC pc)
{
  int               ierr;
  PC_BJacobi        *jac = (PC_BJacobi *) pc->data;
  PC_BJacobi_MPIAIJ *bjac = (PC_BJacobi_MPIAIJ *) jac->data;

  ierr = SLESSetUp(jac->sles[0],bjac->x,bjac->y); CHKERRQ(ierr);
  return 0;
}

#undef __FUNC__  
#define __FUNC__ "PCApply_BJacobi_MPIAIJ"
int PCApply_BJacobi_MPIAIJ(PC pc,Vec x, Vec y)
{
  int               ierr,its;
  PC_BJacobi        *jac = (PC_BJacobi *) pc->data;
  PC_BJacobi_MPIAIJ *bjac = (PC_BJacobi_MPIAIJ *) jac->data;
  Scalar            *x_array,*x_true_array, *y_array,*y_true_array;

  /* 
      The VecPlaceArray() is to avoid having to copy the 
    y vector into the bjac->x vector. The reason for 
    the bjac->x vector is that we need a sequential vector
    for the sequential solve.
  */
  VecGetArray_Fast(x,x_array); 
  VecGetArray_Fast(y,y_array); 
  VecGetArray_Fast(bjac->x,x_true_array); 
  VecGetArray_Fast(bjac->y,y_true_array);  
  VecPlaceArray_Fast(bjac->x,x_array); 
  VecPlaceArray_Fast(bjac->y,y_array); 
  ierr = SLESSolve(jac->sles[0],bjac->x,bjac->y,&its); 
  VecPlaceArray_Fast(bjac->x,x_true_array); 
  VecPlaceArray_Fast(bjac->y,y_true_array); 
  return 0;
}

#undef __FUNC__  
#define __FUNC__ "PCSetUp_BJacobi_MPIAIJ"
int PCSetUp_BJacobi_MPIAIJ(PC pc)
{
  PC_BJacobi        *jac = (PC_BJacobi *) pc->data;
  Mat               mat = pc->mat, pmat = pc->pmat;
  Mat_MPIAIJ        *pmatin = (Mat_MPIAIJ *) pmat->data;
  Mat_MPIAIJ        *matin = 0;
  int               ierr, m;
  SLES              sles;
  Vec               x,y;
  PC_BJacobi_MPIAIJ *bjac;
  KSP               subksp;
  PC                subpc;
  MatType           type;

  if (jac->use_true_local) {
    MatGetType(pc->mat,&type,PETSC_NULL);
    if (type != MATMPIAIJ) SETERRQ(1,0,"Incompatible matrix type.");
    matin = (Mat_MPIAIJ *) mat->data;
  }

  /* set default direct solver with no Krylov method */
  if (!pc->setupcalled) {
    char *prefix;
    ierr = SLESCreate(MPI_COMM_SELF,&sles); CHKERRQ(ierr);
    PLogObjectParent(pc,sles);
    ierr = SLESGetKSP(sles,&subksp); CHKERRQ(ierr);
    ierr = KSPSetType(subksp,KSPPREONLY); CHKERRQ(ierr);
    ierr = SLESGetPC(sles,&subpc); CHKERRQ(ierr);
    ierr = PCSetType(subpc,PCILU); CHKERRQ(ierr);
    ierr = PCGetOptionsPrefix(pc,&prefix);CHKERRQ(ierr);
    ierr = SLESSetOptionsPrefix(sles,prefix); CHKERRQ(ierr);
    ierr = SLESAppendOptionsPrefix(sles,"sub_"); CHKERRQ(ierr);
    ierr = SLESSetFromOptions(sles); CHKERRQ(ierr);
/*
   This is not so good. The only reason we need to generate this vector
  is so KSP may generate seq vectors for the local solves
*/
    ierr = MatGetSize(pmatin->A,&m,&m); CHKERRQ(ierr);
    ierr = VecCreateSeq(MPI_COMM_SELF,m,&x); CHKERRQ(ierr);
    ierr = VecCreateSeq(MPI_COMM_SELF,m,&y); CHKERRQ(ierr);
    PLogObjectParent(pc,x);
    PLogObjectParent(pc,y);

    pc->destroy       = PCDestroy_BJacobi_MPIAIJ;
    pc->apply         = PCApply_BJacobi_MPIAIJ;
    pc->setuponblocks = PCSetUpOnBlocks_BJacobi_MPIAIJ;

    bjac         = (PC_BJacobi_MPIAIJ *) PetscMalloc(sizeof(PC_BJacobi_MPIAIJ));CHKPTRQ(bjac);
    PLogObjectMemory(pc,sizeof(PC_BJacobi_MPIAIJ));
    bjac->x      = x;
    bjac->y      = y;

    jac->sles    = (SLES*) PetscMalloc( sizeof(SLES) ); CHKPTRQ(jac->sles);
    jac->sles[0] = sles;
    jac->data    = (void *) bjac;
  }
  else {
    sles = jac->sles[0];
    bjac = (PC_BJacobi_MPIAIJ *)jac->data;
  }
  /* if (jac->l_true[0] == USE_TRUE_MATRIX) {
    ierr = SLESSetOperators(sles,matin->A,matin->A,pc->flag);
  }
  else */ if (jac->use_true_local)
    ierr = SLESSetOperators(sles,matin->A,pmatin->A,pc->flag);
  else
    ierr = SLESSetOperators(sles,pmatin->A,pmatin->A,pc->flag);
  CHKERRQ(ierr);
  return 0;
}


