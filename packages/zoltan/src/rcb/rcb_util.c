/*====================================================================
 * ------------------------
 * | CVS File Information |
 * ------------------------
 *
 * $RCSfile$
 *
 * $Author$
 *
 * $Date$
 *
 * $Revision$
 *
 *====================================================================*/
#ifndef lint
static char *cvs_rcbutilc_id = "$Id$";
#endif


#include "lbi_const.h"
#include "lb_const.h"
#include "rcb_const.h"
#include "all_allo_const.h"

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/* PROTOTYPES */

static void initialize_dot(LB *, struct rcb_dot *, LB_GID, LB_LID, int, float);

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

int LB_RCB_Build_Structure(LB *lb, int *num_obj, int *max_obj, 
                                 int wgtflag)
{
/*
 *  Function to build the geometry-based data structures for 
 *  Steve Plimpton's RCB implementation.
 */
char *yo = "LB_RCB_Build_~tructure";
RCB_STRUCT *rcb;                      /* Data structure for RCB.             */
LB_GID *objs_global;                  /* Array of global IDs returned by the
                                         application.                        */
LB_LID *objs_local;                   /* Array of local IDs returned by the
                                         application.                        */
float  *objs_wgt;                     /* Array of object weights returned by 
                                         the application.                    */
LB_GID obj_global_id;                 /* Global ID returned by application.  */
LB_LID obj_local_id;                  /* Local ID returned by application.   */
int num_geom;                         /* # values per object used to describe
                                         the geometry.                       */
float wgt = 1.;
int found;
int i, ierr = 0;

  /*
   * Allocate an RCB data structure for this load balancing object.
   * If the previous data structure is still there, free the Dots first;
   * the other fields can be reused.
   */

  if (lb->Data_Structure == NULL) {
    rcb = (RCB_STRUCT *) LB_MALLOC(sizeof(RCB_STRUCT));
    if (rcb == NULL) {
      fprintf(stderr, "[%d] Error from %s: Insufficient memory\n",
              lb->Proc, yo);
      return(LB_MEMERR);
    }
    lb->Data_Structure = (void *) rcb;
    rcb->Tree_Ptr = (struct rcb_tree *) LB_Malloc(
	lb->Num_Proc* sizeof(struct rcb_tree), __FILE__, __LINE__);
    rcb->Box = (struct rcb_box *) LB_MALLOC(sizeof(struct rcb_box));
    if (rcb->Tree_Ptr == NULL || rcb->Box == NULL) {
      fprintf(stderr, "[%d] Error from %s: Insufficient memory\n",
              lb->Proc, yo);
      LB_Free((void **) &(rcb->Tree_Ptr));
      LB_Free((void **) &(rcb->Box));
      LB_Free((void **) &rcb);
      return(LB_MEMERR);
    }
  }
  else {
    rcb = (RCB_STRUCT *) lb->Data_Structure;
    LB_Free((void **) &(rcb->Dots));
  }

  /*
   * Allocate space for objects in RCB data structure.  Allow extra space
   * for objects that are imported to the processor.
   */

  *num_obj = lb->Get_Num_Obj(lb->Get_Num_Obj_Data, &ierr);
  if (ierr) {
    fprintf(stderr, "[%d] Error in %s:  Error returned from user function"
                    "Get_Num_Obj.\n", lb->Proc, yo);
    LB_Free((void **) &(rcb->Tree_Ptr));
    LB_Free((void **) &(rcb->Box));
    LB_Free((void **) &rcb);
    return(LB_FATAL);
  }
  *max_obj = 1.5 * *num_obj + 1;
  rcb->Dots = (struct rcb_dot *) LB_Malloc((*max_obj)*sizeof(struct rcb_dot),
      __FILE__, __LINE__);
  if (rcb->Dots == NULL) {
    fprintf(stderr, "[%d] Error from %s: Insufficient memory\n",
            lb->Proc, yo);
    LB_Free((void **) &(rcb->Tree_Ptr));
    LB_Free((void **) &(rcb->Box));
    LB_Free((void **) &rcb);
    return(LB_MEMERR);
  }

  /*
   * Compute the number of geometry fields per object.  For RCB, this
   * value should be one, two or three, describing the x-, y-, and z-coords.
   */

  num_geom = lb->Get_Num_Geom(lb->Get_Num_Geom_Data, &ierr);
  if (num_geom > 3) {
    fprintf(stderr, "[%d] Error in %s:  Number of geometry fields %d is "
                    "too great for RCB; valid range is 1-3\n",
                    lb->Proc, yo, num_geom);
    LB_Free((void **) &(rcb->Tree_Ptr));
    LB_Free((void **) &(rcb->Box));
    LB_Free((void **) &(rcb->Dots));
    LB_Free((void **) &rcb);
    exit(LB_FATAL);
  }
  if (ierr) {
    fprintf(stderr, "[%d] Error in %s:  Error returned from user function"
                    "Get_Num_Geom.\n", lb->Proc, yo);
    LB_Free((void **) &(rcb->Tree_Ptr));
    LB_Free((void **) &(rcb->Box));
    LB_Free((void **) &(rcb->Dots));
    LB_Free((void **) &rcb);
    exit(LB_FATAL);
  }

  /*
   *  Access objects based on the method provided by the application.
   */

  if (lb->Get_Obj_List != NULL) {

    /*
     *  Call the application for the IDs of all objects and initialize the
     *  dot for each object.
     */

    objs_global = (LB_GID *) LB_Malloc((*num_obj)*sizeof(LB_GID),
        __FILE__, __LINE__);
    objs_local  = (LB_LID *) LB_Malloc((*num_obj)*sizeof(LB_LID),
        __FILE__, __LINE__);
    objs_wgt    = (float  *) LB_Malloc((*num_obj)*sizeof(float),
        __FILE__, __LINE__);
    if (objs_global == NULL || objs_local == NULL || objs_wgt == NULL) {
      fprintf(stderr, "[%d] Error from %s: Insufficient memory\n",
              lb->Proc, yo);
      LB_Free((void **) &objs_global);
      LB_Free((void **) &objs_local);
      LB_Free((void **) &objs_wgt);
      LB_Free((void **) &(rcb->Tree_Ptr));
      LB_Free((void **) &(rcb->Box));
      LB_Free((void **) &(rcb->Dots));
      LB_Free((void **) &rcb);
      return(LB_MEMERR);
    }

    lb->Get_Obj_List(lb->Get_Obj_List_Data, objs_global, objs_local, 
                     wgtflag, objs_wgt, &ierr);
    if (ierr) {
      fprintf(stderr, "[%d] Error in %s:  Error returned from user function"
                      "Get_Obj_List.\n", lb->Proc, yo);
      LB_Free((void **) &objs_global);
      LB_Free((void **) &objs_local);
      LB_Free((void **) &objs_wgt);
      LB_Free((void **) &(rcb->Tree_Ptr));
      LB_Free((void **) &(rcb->Box));
      LB_Free((void **) &(rcb->Dots));
      LB_Free((void **) &rcb);
      return(LB_FATAL);
    }

    for (i = 0; i < *num_obj; i++) {
      initialize_dot(lb, &(rcb->Dots[i]), objs_global[i], objs_local[i],
                     wgtflag, objs_wgt[i]);
    }
    LB_Free((void **) &objs_global);
    LB_Free((void **) &objs_local);
    LB_Free((void **) &objs_wgt);
  }
  else if (lb->Get_First_Obj != NULL && lb->Get_Next_Obj != NULL) {

    /*
     *  Call the application for each object and initialize the dot for 
     *  that object.
     */

    i = 0;
    found = lb->Get_First_Obj(lb->Get_First_Obj_Data, &obj_global_id,
                              &obj_local_id, wgtflag, &wgt, &ierr);
    if (ierr) {
      fprintf(stderr, "[%d] Error in %s:  Error returned from user function"
                      "Get_First_Obj.\n", lb->Proc, yo);
      LB_Free((void **) &(rcb->Tree_Ptr));
      LB_Free((void **) &(rcb->Box));
      LB_Free((void **) &(rcb->Dots));
      LB_Free((void **) &rcb);
      return(LB_FATAL);
    }

    while (found) {
      initialize_dot(lb, &(rcb->Dots[i]), obj_global_id, obj_local_id,
                     wgtflag, wgt);
      i++;
      found = lb->Get_Next_Obj(lb->Get_Next_Obj_Data, obj_global_id,
                               obj_local_id, &obj_global_id, &obj_local_id,
                               wgtflag, &wgt, &ierr);
      if (ierr) {
        fprintf(stderr, "[%d] Error in %s:  Error returned from user function"
                        "Get_Next_Obj.\n", lb->Proc, yo);
        LB_Free((void **) &(rcb->Tree_Ptr));
        LB_Free((void **) &(rcb->Box));
        LB_Free((void **) &(rcb->Dots));
        LB_Free((void **) &rcb);
        return(LB_FATAL);
      }
    }
    if (i != *num_obj) {
      fprintf(stderr, "Error in %s:  Number of objects returned %d != "
                      "Number of objects declared %d\n", yo, i, *num_obj);
      fprintf(stderr, "Check implementation of LB_FIRST_OBJ_FN and "
                      "LB_NEXT_OBJ_FN \n");
      LB_Free((void **) &(rcb->Tree_Ptr));
      LB_Free((void **) &(rcb->Box));
      LB_Free((void **) &(rcb->Dots));
      LB_Free((void **) &rcb);
      return(LB_FATAL);
    }
  }
  else {
    fprintf(stderr, "Error in %s:  Must define and register either "
                    "LB_OBJ_LIST_FN or LB_FIRST_OBJ_FN/LB_NEXT_OBJ_FN pair\n",
                     yo);
    fprintf(stderr, "Cannot perform RCB without one of these functions.\n");
    LB_Free((void **) &(rcb->Tree_Ptr));
    LB_Free((void **) &(rcb->Box));
    LB_Free((void **) &(rcb->Dots));
    LB_Free((void **) &rcb);
    return(LB_FATAL);
  }
  return(LB_OK);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

void LB_RCB_Free_Structure(LB *lb)
{
/*
 * Deallocate the persistent RCB data structures in lb->Structure.
 */
int ierr = 0;
char *yo = "LB_RCB_Free_Structure";

RCB_STRUCT *rcb;                      /* Data structure for RCB.             */

  rcb = (RCB_STRUCT *) lb->Data_Structure;

  if (rcb != NULL) {
    LB_Free((void **) &(rcb->Tree_Ptr));
    LB_Free((void **) &(rcb->Box));
    LB_Free((void **) &(rcb->Dots));
    LB_Free((void **) &rcb);
  }
}



/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

static void initialize_dot(LB *lb, struct rcb_dot *dot, LB_GID global_id, 
                           LB_LID local_id, int wgtflag, float wgt)
{
/*
 *  Function that initializes the dot data structure for RCB.  It uses the 
 *  global ID, coordinates and weight provided by the application.  
 */
int ierr = 0;
char *yo = "initialize_dot";

  LB_SET_GID(dot->Tag.Global_ID, global_id);
  LB_SET_LID(dot->Tag.Local_ID, local_id);
  dot->Tag.Proc = lb->Proc;
  dot->X[0] = dot->X[1] = dot->X[2] = 0.0;
  lb->Get_Geom(lb->Get_Geom_Data, global_id, local_id, dot->X, &ierr);
  if (ierr) {
    fprintf(stderr, "[%d] %s: Error: Error returned from user defined "
                    "Get_Geom function.\n", lb->Proc, yo);
    exit (-1);
  }
  if (wgtflag)
     dot->Weight = wgt;
}
