#ifndef lint
static char vcid[] = "$Id: zplog.c,v 1.1 1996/03/23 19:22:22 curfman Exp curfman $";
#endif

#include "zpetsc.h"
#include "sys.h"
#include "pinclude/petscfix.h"

#ifdef HAVE_FORTRAN_CAPS
#define plogeventbegin_       PLOGEVENTBEGIN
#define plogeventend_         PLOGEVENTEND
#define plogflops_            PLOGFLOPS
#define plogallbegin_         PLOGALLBEGIN
#define plogdestroy_          PLOGDESTROY
#define plogbegin_            PLOGBEGIN
#define plogdump_             PLOGDUMP
#define plogeventregister_    PLOGEVENTREGISTER
#define plogstagepop_         PLOGSTAGEPOP
#define plogstageregister_    PLOGSTAGEREGISTER
#define plogstagepush_        PLOGSTAGEPUSH
#elif !defined(HAVE_FORTRAN_UNDERSCORE)
#define plogeventbegin_       plogeventbegin
#define plogeventend_         plogeventend
#define plogflops_            plogflops
#define plogallbegin_         plogallbegin
#define plogdestroy_          plogdestroy
#define plogbegin_            plogbegin
#define plogeventregister_    plogeventregister
#define plogdump_             plogdump
#define plogstagepop_         plogstagepop  
#define plogstageregister_    plogstageregister
#define plogstagepush_        plogstagepush
#endif

#if defined(__cplusplus)
extern "C" {
#endif

void plogdump_(CHAR name, int *__ierr,int len ){
#if defined(PETSC_LOG)
  char *t1;
  FIXCHAR(name,len,t1);
  *__ierr = PLogDump(t1);
  FREECHAR(name,t1);
#endif
}
void plogeventregister_(int *e,CHAR string,CHAR color,int *__ierr,int len1,
                        int len2){
#if defined(PETSC_LOG)
  char *t1,*t2;
  FIXCHAR(string,len1,t1);
  FIXCHAR(color,len2,t2);
  *__ierr = PLogEventRegister(e,t1,t2);
#endif
}

void plogallbegin_(int *__ierr){
#if defined(PETSC_LOG)
  *__ierr = PLogAllBegin();
#endif
}

void plogdestroy_(int *__ierr){
#if defined(PETSC_LOG)
  *__ierr = PLogDestroy();
#endif
}

void plogbegin_(int *__ierr){
#if defined(PETSC_LOG)
  *__ierr = PLogBegin();
#endif
}

void plogeventbegin_(int e,int o1,int o2,int o3,int o4){
#if defined(PETSC_LOG)
  PetscObject t1,t2,t3,t4;
  if (o1) t1 = (PetscObject) MPIR_ToPointer(*(int*)(o1)); else t1 = 0;
  if (o2) t2 = (PetscObject) MPIR_ToPointer(*(int*)(o2)); else t2 = 0;
  if (o3) t3 = (PetscObject) MPIR_ToPointer(*(int*)(o3)); else t3 = 0;
  if (o4) t4 = (PetscObject) MPIR_ToPointer(*(int*)(o4)); else t4 = 0;

  if (_PLB) (*_PLB)(e,1,t1,t2,t3,t4);
#if defined(HAVE_MPE)
  if (UseMPE && MPEFlags[e]) MPE_Log_event(MPEBEGIN+2*e,0,"");
#endif
#endif
}

void plogeventend_(int e,int o1,int o2,int o3,int o4){
#if defined(PETSC_LOG)
  PetscObject t1,t2,t3,t4;
  if (o1) t1 = (PetscObject) MPIR_ToPointer(*(int*)(o1)); else t1 = 0;
  if (o2) t2 = (PetscObject) MPIR_ToPointer(*(int*)(o2)); else t2 = 0;
  if (o3) t3 = (PetscObject) MPIR_ToPointer(*(int*)(o3)); else t3 = 0;
  if (o4) t4 = (PetscObject) MPIR_ToPointer(*(int*)(o4)); else t4 = 0;
  if (_PLE) (*_PLE)(e,1,t1,t2,t3,t4);
#if defined(HAVE_MPE)
  if (UseMPE && MPEFlags[e]) MPE_Log_event(MPEBEGIN+2*e+1,0,"");
#endif
#endif
}

void plogflops_(int *f) {
  PLogFlops(*f);
}

void plogstagepop_(int *__ierr )
{
#if defined(PETSC_LOG)
  *__ierr = PLogStagePop();
#endif
}

void plogstageregister_(int *stage,char *sname, int *__ierr ){
#if defined(PETSC_LOG)
  *__ierr = PLogStageRegister(*stage,sname);
#endif
}

void plogstagepush_(int *stage, int *__ierr ){
#if defined(PETSC_LOG)
  *__ierr = PLogStagePush(*stage);
#endif
}

#if defined(__cplusplus)
}
#endif
