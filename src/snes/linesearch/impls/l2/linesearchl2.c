#include <private/linesearchimpl.h>
#include <petscsnes.h>

#undef __FUNCT__
#define __FUNCT__ "LineSearchApply_L2"

/*@C
   LineSearchL2 - This routine is not a line search at all;
   it simply uses the full step.  Thus, this routine is intended
   to serve as a template and is not recommended for general use.

   Logically Collective on SNES and Vec

   Input Parameters:
+  snes - nonlinear context
.  lsctx - optional context for line search (not used here)
.  x - current iterate
.  f - residual evaluated at x
.  y - search direction
.  fnorm - 2-norm of f
-  xnorm - norm of x if known, otherwise 0

   Output Parameters:
+  g - residual evaluated at new iterate y
.  w - new iterate
.  gnorm - 2-norm of g
.  ynorm - 2-norm of search length
-  flag - PETSC_TRUE on success, PETSC_FALSE on failure

   Options Database Key:
.  -snes_ls basic - Activates SNESLineSearchNo()

   Level: advanced

.keywords: SNES, nonlinear, line search, cubic

.seealso: SNESLineSearchCubic(), SNESLineSearchQuadratic(),
          SNESLineSearchSet(), SNESLineSearchNoNorms()
@*/
PetscErrorCode  LineSearchApply_L2(LineSearch linesearch)
{

  PetscBool      changed_y, changed_w;
  PetscErrorCode ierr;
  Vec             X;
  Vec             F;
  Vec             Y;
  Vec             W;
  SNES            snes;
  PetscReal       gnorm;
  PetscReal       ynorm;
  PetscReal       xnorm;
  PetscReal       steptol, maxstep;

  PetscViewer     monitor;
  PetscBool       domainerror;
  PetscReal       lambda, lambda_old, lambda_mid, lambda_update, delLambda;
  PetscReal       fnrm, fnrm_old, fnrm_mid;
  PetscReal       delFnrm, delFnrm_old, del2Fnrm;
  PetscInt        i, max_its;

  PetscFunctionBegin;

  ierr = LineSearchGetVecs(linesearch, &X, &F, &Y, &W, PETSC_NULL);CHKERRQ(ierr);
  ierr = LineSearchGetNorms(linesearch, &xnorm, &gnorm, &ynorm);CHKERRQ(ierr);
  ierr = LineSearchGetLambda(linesearch, &lambda);CHKERRQ(ierr);
  ierr = LineSearchGetSNES(linesearch, &snes);CHKERRQ(ierr);
  ierr = LineSearchSetSuccess(linesearch, PETSC_TRUE);CHKERRQ(ierr);
  ierr = LineSearchGetMaxIts(linesearch, &max_its);CHKERRQ(ierr);
  ierr = LineSearchGetStepTolerance(linesearch, &steptol);CHKERRQ(ierr);
  ierr = LineSearchGetMaxStep(linesearch, &maxstep);CHKERRQ(ierr);

  ierr = LineSearchGetMonitor(linesearch, &monitor);CHKERRQ(ierr);

  /* precheck */
  ierr = LineSearchPreCheck(linesearch, &changed_y);CHKERRQ(ierr);
  lambda_old = 0.0;
  fnrm_old = gnorm*gnorm;
  lambda_mid = 0.5*(lambda + lambda_old);

  for (i = 0; i < max_its; i++) {

  /* compute the norm at the midpoint */
  ierr = VecCopy(X, W);CHKERRQ(ierr);
  ierr = VecAXPY(W, -lambda_mid, Y);CHKERRQ(ierr);
  ierr = SNESComputeFunction(snes, W, F);CHKERRQ(ierr);
  ierr = VecNorm(F, NORM_2, &fnrm_mid);CHKERRQ(ierr);
  fnrm_mid = fnrm_mid*fnrm_mid;

  /* compute the norm at lambda */
  ierr = VecCopy(X, W);CHKERRQ(ierr);
  ierr = VecAXPY(W, -lambda, Y);CHKERRQ(ierr);
  ierr = SNESComputeFunction(snes, W, F);CHKERRQ(ierr);
  ierr = VecNorm(F, NORM_2, &fnrm);CHKERRQ(ierr);
  fnrm = fnrm*fnrm;

  /* this gives us the derivatives at the endpoints -- compute them from here

   a = x - a0

   p_0(x) = (x / dA - 1)(2x / dA - 1)
   p_1(x) = 4(x / dA)(1 - x / dA)
   p_2(x) = (x / dA)(2x / dA - 1)

   dp_0[0] / dx = 3 / dA
   dp_1[0] / dx = -4 / dA
   dp_2[0] / dx = 1 / dA

   dp_0[dA] / dx = -1 / dA
   dp_1[dA] / dx = 4 / dA
   dp_2[dA] / dx = -3 / dA

   d^2p_0[0] / dx^2 =  4 / dA^2
   d^2p_1[0] / dx^2 = -8 / dA^2
   d^2p_2[0] / dx^2 =  4 / dA^2
     */

    delLambda    = lambda - lambda_old;
    delFnrm      = (3.*fnrm - 4.*fnrm_mid + 1.*fnrm_old) / delLambda;
    delFnrm_old  = (-3.*fnrm_old + 4.*fnrm_mid -1.*fnrm) / delLambda;
    del2Fnrm = (delFnrm - delFnrm_old) / delLambda;

    /* check for positive curvature -- looking for that root wouldn't be a good thing. */
    while ((del2Fnrm < 0.0) && (fabs(delLambda) > steptol)) {
      fnrm_old = fnrm_mid;
      lambda_old = lambda_mid;
      lambda_mid = 0.5*(lambda_old + lambda);
      ierr = VecCopy(X, W);CHKERRQ(ierr);
      ierr = VecAXPY(W, -lambda_mid, Y);CHKERRQ(ierr);
      ierr = SNESComputeFunction(snes, W, F);CHKERRQ(ierr);
      ierr = VecNorm(F, NORM_2, &fnrm_mid);CHKERRQ(ierr);
      fnrm_mid = fnrm_mid*fnrm_mid;
      delLambda    = lambda - lambda_old;
      delFnrm      = (3.*fnrm - 4.*fnrm_mid + 1.*fnrm_old) / delLambda;
      delFnrm_old  = (-3.*fnrm_old + 4.*fnrm_mid -1.*fnrm) / delLambda;
      del2Fnrm = (delFnrm - delFnrm_old) / delLambda;
    }

    if (monitor) {
      ierr = PetscViewerASCIIAddTab(monitor,((PetscObject)linesearch)->tablevel);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(monitor,"    Line search: lambdas = [%g, %g, %g], fnorms = [%g, %g, %g]\n",
                                    lambda, lambda_mid, lambda_old, PetscSqrtReal(fnrm), PetscSqrtReal(fnrm_mid), PetscSqrtReal(fnrm_old));CHKERRQ(ierr);
      ierr = PetscViewerASCIISubtractTab(monitor,((PetscObject)linesearch)->tablevel);CHKERRQ(ierr);
    }

    /* compute the search direction */
    lambda_update = lambda - delFnrm*delLambda / (delFnrm - delFnrm_old);
    if (PetscIsInfOrNanScalar(lambda_update)) break;
    if (lambda_update > maxstep) {
      break;
    }

    /* compute the new state of the line search */
    lambda_old = lambda;
    lambda = lambda_update;
    fnrm_old = fnrm;
    lambda_mid = 0.5*(lambda + lambda_old);
  }
  /* construct the solution */
  ierr = VecCopy(X, W);CHKERRQ(ierr);
  ierr = VecAXPY(W, -lambda, Y);CHKERRQ(ierr);

  /* postcheck */
  ierr = LineSearchPostCheck(linesearch, &changed_y, &changed_w);CHKERRQ(ierr);
  if (changed_y) {
    ierr = VecAXPY(X, -lambda, Y);CHKERRQ(ierr);
  } else {
    ierr = VecCopy(W, X);CHKERRQ(ierr);
  }
  ierr = SNESComputeFunction(snes,X,F);CHKERRQ(ierr);
  ierr = SNESGetFunctionDomainError(snes, &domainerror);CHKERRQ(ierr);
  if (domainerror) {
    ierr = LineSearchSetSuccess(linesearch, PETSC_FALSE);CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }

  ierr = LineSearchSetLambda(linesearch, lambda);CHKERRQ(ierr);
  ierr = LineSearchComputeNorms(linesearch);CHKERRQ(ierr);
  ierr = LineSearchGetNorms(linesearch, &xnorm, &gnorm, &ynorm);CHKERRQ(ierr);

  if (monitor) {
    ierr = PetscViewerASCIIAddTab(monitor,((PetscObject)linesearch)->tablevel);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(monitor,"    Line search terminated: lambda = %g, fnorms = %g\n", lambda, gnorm);CHKERRQ(ierr);
    ierr = PetscViewerASCIISubtractTab(monitor,((PetscObject)linesearch)->tablevel);CHKERRQ(ierr);
  }
  if (lambda <= steptol) {
    ierr = LineSearchSetSuccess(linesearch, PETSC_FALSE);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

EXTERN_C_BEGIN
#undef __FUNCT__
#define __FUNCT__ "LineSearchCreate_L2"
PetscErrorCode LineSearchCreate_L2(LineSearch linesearch)
{
  PetscFunctionBegin;
  linesearch->ops->apply          = LineSearchApply_L2;
  linesearch->ops->destroy        = PETSC_NULL;
  linesearch->ops->setfromoptions = PETSC_NULL;
  linesearch->ops->reset          = PETSC_NULL;
  linesearch->ops->view           = PETSC_NULL;
  linesearch->ops->setup          = PETSC_NULL;
  PetscFunctionReturn(0);
}
EXTERN_C_END
