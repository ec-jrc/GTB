
/***********************************************************************
Author(s): Pierre Soille and Peter Vogt
Copyright (C) 2008-2022 European Union (Joint Research Centre)

This file is an amended version of mspa.c to work with GTB/GWB.
This file requires miallib.

miallib is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

miallib is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with miallib.  If not, see <https://www.gnu.org/licenses/>.
***********************************************************************/

/** @file
 *  Morphological Segmentation of Binary Patterns \cite soille-vogt2009
 *  https://doi.org/10.1016/j.patrec.2008.10.015
 *  @author Pierre Soille and Peter Vogt
 */



#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "miallib.h"
#include "op.h"
#include <time.h>

// Process has done i out of n rounds,
// and we want a bar of width w and resolution r.
static inline void loadBar(int x, int n, int r, int w)
{
    // Only update r times.
    if ( x % (n/r) != 0 ) return;

    // Calculate the ratio of complete-to-incomplete.
    float ratio = x/(float)n;
    int   c     = ratio * w;

    // Show the percentage complete.
    printf("%4d%% [", (int)(ratio*100) );

    // Show the load bar.
    for (x=0; x<c; x++)
       printf("=");
    for (x=c; x<w; x++)
       printf(" ");

    // ANSI Control codes to go back to the
    // previous line and clear it.
    //  printf("]\n\033[F\033[J");
    printf("]\r"); // Move to the first column
    fflush(stdout);
}

extern ERROR_TYPE f_threshstrict(IMAGE *, float, float, float, float);
extern IMAGE *sqedt();
extern IMAGE *ced();
extern IMAGE *read_image();


IMAGE *getexternalboundary(IMAGE *im, float size, float edu)
{
  IMAGE *nim, *imsqedt, *edt;
  char aname[256];

  nim=(IMAGE *)copy_image(im);
  negation(nim);

  imsqedt=(IMAGE *)sqedt(nim);
  uint32_to_float(imsqedt);
  edt=imsqedt;
  imsqrt(edt);

  f_thresh(edt, 0.0,  size*edu, 0.0, 1.0);
  to_uchar(edt);
  bitwise_op(edt, nim, AND_op);

  free_image(nim);
  
  return(edt);
}

IMAGE *fm_preproc(IMAGE *fm, float size, float edu)
{
  IMAGE *da, *fa, *dda, *dfa;

  da=(IMAGE *)copy_image(fm);
  generic_thresh(da, 1, 255, 0, 1); /* 0 missing 1 for background 2 for foreground */
  dda=getexternalboundary(da, size, edu);

#ifdef DEBUG
  writeTiffOneStripPerLine(dda, "dda.tif" , NULL);
#endif

  free_image(da);
  fa=(IMAGE *)copy_image(fm);
  generic_thresh(fa, 2, 255, 0, 1);
  dfa=getexternalboundary(fa, size, edu);

#ifdef DEBUG
  writeTiffOneStripPerLine(dfa, "dfa.tif" , NULL);
  printf("hello 1 in fm_preproc\n");
#endif

  generic_arith(dfa, fa, OR_op);
  generic_arith(dfa, dda, INF_op);
  generic_arith(dfa, fa, SUP_op);

  free_image(fa);
  free_image(dda);

  return (dfa);
}

IMAGE *fm_preproc2(IMAGE *im, int size)
{
  IMAGE *lb, *rb, *tb, *bb, *bbim, *lbim, *rbim, *tbim;
  int box[6];

/* 	 (lb (*imcut fm 0 0 0 0 (- (*getny fm) 1) 0)) */
  lb=imcut(im, 0, 0, 0, 0, GetImNy(im)-1, 0);
  
/* 	 (lbim (*imcreate t_UCHAR size (*getny fm) 1)) */
  lbim=create_image(t_UCHAR, size, GetImNy(im), 1);
  
/* 	 (rb (*imcut fm (- (*getnx fm) 1) */
/* 		     0 */
/* 		     0 */
/* 		     (- (*getnx fm) 1) */
/* 		     (- (*getny fm) 1) */
/* 		     0) */
/* 	     ) */
  rb=imcut(im, GetImNx(im)-1, 0, 0, GetImNx(im)-1, GetImNy(im)-1, 0);
  
/* 	 (rbim (*imcreate t_UCHAR size (*getny fm) 1)) */
  rbim=create_image(t_UCHAR, size, GetImNy(im), 1);
  
/* 	 (tb (*imcut fm */
/* 		     0 */
/* 		     0 */
/* 		     0 */
/* 		     (- (*getnx fm) 1) */
/* 		     0 */
/* 		     0) */
/* 	     ) */
  tb=imcut(im, 0, 0, 0, GetImNx(im)-1, 0, 0);
/* 	 (tbim (*imcreate t_UCHAR (*getnx fm) size 1)) */
  tbim=create_image(t_UCHAR, GetImNx(im), size, 1);
/* 	 (bb (*imcut fm */
/* 		     0 */
/* 		     (- (*getny fm) 1) */
/* 		     0 */
/* 		     (- (*getnx fm) 1) */
/* 		     (- (*getny fm) 1) */
/* 		     0) */
/* 	     ) */
  bb=imcut(im, 0, GetImNy(im)-1, 0, GetImNx(im)-1, GetImNy(im)-1, 0);
/* 	 (bbim (*imcreate t_UCHAR (*getnx fm) size 1)) */
  bbim=create_image(t_UCHAR, GetImNx(im), size, 1);
  
/* 	 (out (*addframebox fm size size size size 0 0 0)) */
  box[0]=box[1]=box[2]=box[3]=size;
  box[4]=box[5]=0;
  generic_addframebox(im, box, 0);

/*     (@imputop out (@dirmax (@imputintop lbim lb */
/* 					(- (*getnx lbim) 1) */
/* 					0 */
/* 					0 */
/* 					OR_op) */
/* 			   3) */
/* 	      0 size 0 */
/* 	      OR_op) */
  imputop(lbim, lb, GetImNx(lbim)-1, 0, 0, OR_op);
  uc_dirmax(lbim, 3);
  imputop(im, lbim, 0, size, 0, 11);

/*     (@imputop out (@dirmax (@imputintop rbim rb */
/* 					0 */
/* 					0 */
/* 					0 */
/* 					OR_op) */
/* 			   1) */
/* 	      (- (*getnx out) size 1) */
/* 	      size */
/* 	      0 */
/* 	      OR_op) */
  imputop(rbim, rb, 0, 0, 0, 11);
  uc_dirmax(rbim, 1);
  imputop(im, rbim, GetImNx(im)-size-1, size, 0, OR_op);

/*     (@imputop out (@dirmax (@imputintop tbim tb */
/* 					0 */
/* 					(- (*getny tbim) 1) */
/* 					0 */
/* 					OR_op) */
/* 			   0) */
/* 	      size */
/* 	      0 */
/* 	      0 */
/* 	      OR_op) */
  imputop(tbim, tb, 0, GetImNy(tbim)-1, 0, OR_op);
  uc_dirmax(tbim, 0);
  imputop(im, tbim, size, 0, 0, 11);

/*     (@imputop out (@dirmax (@imputintop bbim bb */
/* 					0 */
/* 					0 */
/* 					0 */
/* 					OR_op) */
/* 			   2) */
/* 	      size */
/* 	      (- (*getny out) size 1) */

/* 	      0 */
/* 	      OR_op) */
  imputop(bbim, bb, 0, 0, 0, 11);
  uc_dirmax(bbim, 2);
  imputop(im, bbim, size, GetImNy(im)-size -1 , 0, OR_op);
  
#ifdef DEBUG
  writeTiffOneStripPerLine(im, "im.tif", NULL);
#endif

  free_image(rb); free_image(lb); free_image(bb); free_image(tb);
  free_image(rbim); free_image(lbim); free_image(tbim); free_image(bbim);
  
  return im;
}

IMAGE *getcore(IMAGE *im, float size, float edu)
{
  IMAGE *imsqedt, *edt;

  imsqedt=(IMAGE *)sqedt(im);
  uint32_to_float(imsqedt);
  edt=imsqedt;
  imsqrt(edt);
  f_threshstrict(edt, (size*edu), 65535.0, 0.0, 1.0);
  to_uchar(edt);

  return(edt);
}

IMAGE *getpatch(IMAGE *im, float size, int graphfg, float edu)
{
  IMAGE *core;

/*   (*sub im (@rdil (*getcore im size) */
/* 		  im */
/* 		  graphfg) */
/* 	) */
/*   ) */
  core=getcore(im, size, edu);
  uc_rdil(core, im, graphfg, 1);
  generic_arith(core, im, SUBSWAP_op);

#ifdef DEBUG
  printf("hello 1 in getpatch\n");
  writeTiffOneStripPerLine(core, "rdil.tif", NULL);
#endif

  return(core);
}

IMAGE *uc_fillhole(IMAGE *im, int graph)
{
  IMAGE *marker;
  int box[BOXELEM];
  BOX_2D;

  marker=create_image(t_UCHAR, GetImNx(im), GetImNy(im), 1);
  generic_blank(marker,255);
  generic_framebox(marker,box,0);
  generic_arith(marker, im, SUP_op);
  uc_rero(marker, im, graph, 1);

  return marker;
}

IMAGE **setedges(IMAGE *im, float size, int graphfg, int graphbg, float edu, int disk)
{
  IMAGE **iml;
  IMAGE *core, *outer, *corefill, *loecher, *edges, *core1;
  IMAGE *crt_edges;
  IMAGE *i0;

  int counter;
  char aname[256];

  core=getcore(im, size, edu);

  if (disk==1)
    writeTiffOneStripPerLine(core, "disk_core1.tif", NULL);
  else
    core1=copy_image(core);

  corefill=uc_fillhole(core, graphbg);
  loecher=copy_image(corefill);
  generic_arith(loecher, core, SUB_op);
  
#ifdef DEBUG
  printf("hello 1 in setedges\n");
  writeTiffOneStripPerLine(loecher, "loecher.tif", NULL);
#endif
  
  edges=create_image(t_UCHAR, GetImNx(im), GetImNy(im), 1);

  iml=calloc(2,sizeof(IMAGE *));

  int idx=22;
  loadBar(idx, 100, 100, 50);

  for(counter=0; ; counter++){
#ifdef DEBUG;
    printf("counter = %d\n", counter);
    sprintf(aname, "corefill_counter%d.tif", counter);
    writeTiffOneStripPerLine(corefill, aname, NULL);
#endif
    
    volume(corefill);
    if (GetImVol(corefill)==0.0)
      break;
    crt_edges=getexternalboundary(corefill, size, edu);

#ifdef DEBUG;
    sprintf(aname, "edges_before_bitwise_counter%d.tif", counter);
    writeTiffOneStripPerLine(edges, aname, NULL);
    sprintf(aname, "crt_edges_before_bitwise_counter%d.tif", counter);
    writeTiffOneStripPerLine(crt_edges, aname, NULL);
#endif
    
    bitwise_op(edges, crt_edges, OR_op);
    
#ifdef DEBUG;
    sprintf(aname, "edges_after_bitwise_counter%d.tif", counter);
    writeTiffOneStripPerLine(edges, aname, NULL);
    sprintf(aname, "crt_edges_after_bitwise_counter%d.tif", counter);
    writeTiffOneStripPerLine(crt_edges, aname, NULL);
#endif
    
    free_image(crt_edges);
    i0=uc_fillhole(loecher, graphfg);
    
#ifdef DEBUG;
    sprintf(aname, "i0_counter%d.tif", counter);
    writeTiffOneStripPerLine(i0, aname, NULL);
    printf("hello 2 in setedges\n");
#endif
    
    free_image(loecher);
    generic_arith(corefill, i0, SUB_op);
    
    free_image(i0);
    generic_arith(core, corefill, SUB_op);
    
#ifdef DEBUG
    printf("hello 3 in setedges\n");
#endif
    
    free_image(corefill);
    corefill=uc_fillhole(core, graphbg);
    loecher=copy_image(corefill);
    generic_arith(loecher, core, SUB_op);
    
#ifdef DEBUG
    printf("hello 4 in setedges\n");
#endif
  }
  iml[0]=edges;

  if(disk==1)
    core1=(IMAGE *)read_image("disk_core1.tif");
  outer=getexternalboundary(core1, size, edu);

  negation(core1);
  bitwise_op(edges, core1, AND_op);
  free_image(core1);

  
#ifdef DEBUG
  printf("hello 5 in setedges\n");
  writeTiffOneStripPerLine(outer, "test_outer.tif", NULL);
  writeTiffOneStripPerLine(edges, "test_edges.tif", NULL);
#endif
  
  generic_arith(outer, edges, SUB_op);
  iml[1]=outer;

  free_image(core);
  free_image(corefill);
  free_image(loecher);

  return iml;
}

IMAGE *getexternalboundarygeodesic(IMAGE *im, IMAGE *mask, float size, float edu)
{
  //(defun *getexternalboundarygeodesic (im mask size)
  //  (@touchar
  //   (@thresh
  //    (*ced im mask)
  //    0.0001
  //    (+ (* size edu))
  //    0.0 1.0)
  //   )
  //  )
  IMAGE *imdst;

  imdst=(IMAGE *)ced(im, mask);
  f_thresh(imdst, 0.0001, size*edu, 0.0, 1.0);
  to_uchar(imdst);
  
  return imdst;
}


IMAGE *getconnector2core(IMAGE *core, IMAGE *opening, IMAGE *residues, \
			 float size, int oitype, int graphfg, float edu, int disk)
{
  //    (let* (
  // 	 (sk (@sub (@binanchorskeloi (*or opening residues) core oitype)
  // 		   core)
  // 	     )
  //         ; (connector (*getexternalboundary sk (- size 1))) ; should be geodesic
  // 	 (connector (*getexternalboundarygeodesic sk
  // 						  (@or (*sub opening core) residues)
  // 						  (- size 1))) ; should be geodesic
  // 	 )
  //     (@and (@or connector sk) (@or (*sub opening core) residues) )
  // 					; we need to intersect with reconstruction
  //    (@and connector (*rdil sk (@or (*sub opening core) residues) graphfg))
  //
  //    )
  IMAGE *sk, *connector, *i0;

  if (disk==1)
    sk=(IMAGE *)read_image("disk_opening.tif");
  else
    sk=copy_image(opening);

  if (disk==1)
    residues=(IMAGE *)read_image("disk_residues.tif");
  bitwise_op(sk, residues, OR_op);

  loadBar(48, 100, 100, 50);

  if (disk==1)
    core=(IMAGE *)read_image("disk_core.tif");
  binOIthin_FIFO(sk, oitype, 1, core);

  loadBar(80, 100, 100, 50);

#ifdef DEBUG
  writeTiffOneStripPerLine(sk, "sk.tif", NULL);
  printf("hello 1 in getconnector2core\n");
#endif

  generic_arith(sk, core, SUB_op);

  if (disk==1)
    i0=(IMAGE *)read_image("disk_opening.tif");
  else
    i0=copy_image(opening);

  generic_arith(i0, core, SUB_op);

#ifdef DEBUG
  printf("hello 2 in getconnector2core\n");
#endif

  bitwise_op(i0, residues, OR_op);
  if (disk==1){
    free_image(residues);
    free_image(core);
  }

  loadBar(84, 100, 100, 50);\
  
  connector=getexternalboundarygeodesic(sk, i0, size-1, edu);
  
  loadBar(88, 100, 100, 50);
  
  bitwise_op(connector, sk, OR_op);
  bitwise_op(connector, i0, AND_op);
  uc_rdil(sk, i0, graphfg, 1);
  bitwise_op(connector, sk, AND_op);

  free_image(sk);
  free_image(i0);
  
  return connector;
}

/* (defun *getcorridor (connector core opening size oitype) */
/*   (let* ( */
/*          (cor (@setregions (@labelgraph (*tolong connector) graphfg)  */
/* 			   (*wsfah  */
/* 			    (*labelgraph (*tolong core) graphfg)  */
/* 			    (@setlevel (*or opening connector) 0 0 255)  */
/* 			    graphfg 254)  */
/* 			   20)); 20 for range */
/* 	) */
/*     (@setlevel cor 2147483647 2147483647 0) */
/*     (@thresh cor 1 2147483647 0 1) */
/*     (@touchar cor) */
/*     ) */
/*   ) */
IMAGE *getcorridor(IMAGE *connector, IMAGE *core, IMAGE *opening, \
		   float size, int oitype, int graph)
{
  IMAGE *lbl, *imref, *cor;
  IMAGE *se;
  unsigned char *pse;

  se=create_image(t_UCHAR, 3, 3, 1);
  pse=(unsigned char *)GetImPtr(se);
  pse[1]=1; pse[3]=1; pse[5]=1; pse[7]=1;
  if (graph==8){
    pse[0]=1; pse[2]=1; pse[6]=1; pse[8]=1;
  }
  lbl=to_int32(core);
  lbl->DataType = t_UINT32;
  label(lbl, se, 1, 1, 0);

  imref=copy_image(opening);
  bitwise_op(imref, connector, OR_op);
  generic_setlevel(imref, 0, 0, 255);

  lbl->DataType=t_UINT32;
  wsfah(lbl, imref, graph, 254);
  free_image(imref);

  cor=to_int32(connector);
  cor->DataType = t_UINT32;
  label(cor, se, 1, 1, 0);
  cor->DataType = t_INT32;

  lbl->DataType=t_INT32;
  set_regions(cor, lbl, 20); // 20 for range

  free_image(lbl);
  
  i32_setlevel(cor,  2147483647, 2147483647, 0);
  i32_thresh(cor, 1, 2147483647, 0, 1);
  to_uchar(cor);
  
  free_image(se);
  
  return cor;
}

void fsp(char fnin[], char fnout[], float size, int graphfg, float edu, int disk, int transition, int internal)
{
  IMAGE *im, *i0, *core, *patch;
  IMAGE *allHoles, *coreHoles;
  IMAGE *opening, *edges, *perforation, *residues;
  IMAGE *connector, *shortcut, *corridor;
  IMAGE **iml;
  IMAGE *tmp, *tmp2;
  IMAGE *out;
  int index;
  int box[6];
  int graphbg, oitype;
  char desc[512];
  int bufsize;

  sprintf(desc, "\nBased on Morphological Segmentation of Binary Patterns\n"
	  "by Pierre Soille and Peter Vogt\n"
	  "URL https://doi.org/10.1016/j.patrec.2008.10.015\n"
	  "File generated by mspa v2.3\n"
	  "Parameters: eew=%f, graphfg=%d, internal=%d, disk=%d", size, graphfg, internal, disk);

  size=(size+0.98)/sqrt(2);

  if (graphfg==8){
    graphbg=4;
    oitype=0;
  }
  else{
    graphbg=8;
    oitype=1;
  }
  if ( (size==1) && (transition==2) )
    transition=0;

  im=(IMAGE *)read_image(fnin);
  loadBar(1, 100, 100, 50);
  // (setq bufsize (truncate (+ 0.5 (* size 1.5))))
  // (@fm_frame_in im bufsize)

  bufsize=(int)((size*1.5)+0.5);
  box[0]=box[1]=box[2]=box[3]=(int)(bufsize+1.5);
  box[4]=box[5]=0;
  generic_addframebox(im, box, 0);

  // get all holes
  if (internal==1){
    i0=fm_preproc(im, 1, edu); 
    allHoles=uc_fillhole(i0, graphbg);
    generic_arith(allHoles, i0, SUB_op);
    writeTiffOneStripPerLine(allHoles, "allHoles.tif", NULL);
    free_image(allHoles);
  }
    
  // (setq i0 (*fm_preproc im size))
  i0=fm_preproc(im, size, edu);
  free_image(im);
  loadBar(6, 100, 100, 50);

#ifdef DEBUG
  printf("hello 1 in fsp\n");
  writeTiffOneStripPerLine(i0, "i0-after-preproc.tif" , NULL);
#endif

  // (@fm_frame_out i0 bufsize)
  generic_subframebox(i0, box);

  //  (setq i0 (*fm_preproc2 i0 (+ 1 bufsize)))

#ifdef DEBUG
  printf("hello 2 in fsp\n");
  writeTiffOneStripPerLine(i0, "i0.tif", NULL);
#endif

  fm_preproc2(i0, bufsize+1);
  out=copy_image(i0);
  loadBar(8, 100, 100, 50);

#ifdef DEBUG
  writeTiffOneStripPerLine(i0, "i0-preproc2.tif", NULL);
#endif

  box[0]=box[1]=box[2]=box[3]=1;
  box[4]=box[5]=0;
  box[0]=box[1]=box[2]=box[3]=(int)(bufsize+1.5);
  box[4]=box[5]=0;

  // (setq core  (*getcore i0 size))
  core=getcore(i0, size, edu);

#ifdef DEBUG
  printf("hello 3 in fsp\n");
  writeTiffOneStripPerLine(core, "core.tif", NULL);
#endif

  //(setq patch (*getpatch i0 size graphfg))
  patch=getpatch(i0, size, graphfg, edu);

#ifdef DEBUG
  printf("hello 4 in fsp\n");
  writeTiffOneStripPerLine(patch, "patch.tif", NULL);
#endif

  loadBar(16, 100, 100, 50);
  // (setq outer (*getexternalboundary core size))
  // (setq opening (*add outer core))
  opening=getexternalboundary(core, size, edu);
  generic_arith(opening, core, ADD_op);


#ifdef DEBUG
  printf("hello 5 in fsp\n");
  writeTiffOneStripPerLine(opening, "opening.tif", NULL);
#endif

  // (setq iml (*setedges i0 size))
  // (setq edges (car iml))
  // (setq perforation (cadr iml))

  if (disk==1){
    writeTiffOneStripPerLine(out, "disk_out.tif", NULL);
    free_image(out);
    writeTiffOneStripPerLine(patch, "disk_patch.tif", NULL);
    free_image(patch);
    writeTiffOneStripPerLine(opening, "disk_opening.tif", NULL);
    free_image(opening);
    writeTiffOneStripPerLine(core, "disk_core.tif", NULL);
    free_image(core);
  }

  loadBar(20, 100, 100, 50);
  iml=setedges(i0, size, graphfg, graphbg, edu, disk);
  loadBar(42, 100, 100, 50);

  edges=iml[0];
  perforation=iml[1];

#ifdef DEBUG
  printf("hello 6 in fsp\n");
  writeTiffOneStripPerLine(iml[0], "edges.tif", NULL);
  writeTiffOneStripPerLine(iml[1], "perforation.tif", NULL);
#endif

  //  (setq residues (*sub i0 core patch perforation edges))
  residues=i0;

  if (disk==1){
    out=(IMAGE *)read_image("disk_out.tif");
    patch=(IMAGE *)read_image("disk_patch.tif");
    opening=(IMAGE *)read_image("disk_opening.tif");
    core=(IMAGE *)read_image("disk_core.tif");
    
    remove("disk_out.tif");
    remove("disk_patch.tif");
    remove("disk_core1.tif");
  }

#ifdef DEBUG
  printf("hello 7 in fsp\n");
  writeTiffOneStripPerLine(residues, "test_residues.tif", NULL);
 #endif
  
  generic_arith(residues, core, SUB_op);

  loadBar(44, 100, 100, 50);
  
  generic_arith(residues, patch, SUB_op);
  generic_shift(patch, -3);
  bitwise_op(out, patch, OR_op);
  free_image(patch);

#ifdef DEBUG
  printf("hello 8 in fsp\n");
#endif

  generic_arith(residues, perforation, SUB_op);
  generic_shift(perforation, -2);
  bitwise_op(out, perforation, OR_op);
  free_image(perforation);

#ifdef DEBUG
  printf("hello 9 in fsp\n");
#endif

  generic_arith(residues, edges, SUB_op);
  generic_shift(edges, -1);
  bitwise_op(out, edges, OR_op);
  free_image(edges);

#ifdef DEBUG
  printf("hello 10 in fsp\n");
  writeTiffOneStripPerLine(residues, "residues.tif", NULL);
#endif

  loadBar(46, 100, 100, 50);

  //  (if (= transition 2)
  //	(progn
  //	  (setq connector (*getconnector2opening opening residues size oitype))
  //	  (setq corridor (*getcorridor2 connector opening size oitype))
  // 	  )
  //       (progn
  //  	(setq connector (*getconnector2core core opening residues size oitype))
  //	(setq corridor (*getcorridor connector core opening size oitype))
  //	)
  //  )

  if (disk==1){
    writeTiffOneStripPerLine(out, "disk_out.tif", NULL);
    free_image(out);
    // writeTiffOneStripPerLine(core, "disk_core.tif", NULL);
    free_image(core);
    // writeTiffOneStripPerLine(opening, "disk_opening.tif", NULL);
    free_image(opening);
    writeTiffOneStripPerLine(residues, "disk_residues.tif", NULL);
    free_image(residues);
  }

  connector=getconnector2core(core, opening, residues, size, oitype, graphfg, edu, disk);
  loadBar(90, 100, 100, 50);

  if( (size!=1) && (disk!=1) )
    free_image(residues);

  if (disk==1){
    out=(IMAGE *)read_image("disk_out.tif");
    core=(IMAGE *)read_image("disk_core.tif");
    opening=(IMAGE *)read_image("disk_opening.tif");
    if (size==1)
      residues=(IMAGE *)read_image("disk_residues.tif");

    remove("disk_out.tif");
    remove("disk_core.tif");
    remove("disk_opening.tif");
    remove("disk_residues.tif");
  }

#ifdef DEBUG
  printf("hello 11 in fsp\n");
 writeTiffOneStripPerLine(connector, "connector.tif", NULL);
#endif
 
  corridor=getcorridor(connector, core, opening, size, oitype, graphfg);
  free_image(opening);
  generic_shift(core, -4);
  bitwise_op(out, core, OR_op);
  if(internal!=1)
    free_image(core);
  
#ifdef DEBUG
  printf("hello 12 in fsp\n");
  writeTiffOneStripPerLine(corridor, "corridor1.tif", NULL);
#endif

  //  (setq shortcut (*sub connector corridor))
  generic_arith(connector, corridor, SUB_op);
  shortcut=connector;
#ifdef DEBUG
  printf("hello 13 in fsp\n");
#endif

  if (size == 1){
    tmp=copy_image(corridor);
    uc_dilate4(tmp, 1, 1);
    generic_arith(tmp, residues, INF_op);
    bitwise_op(corridor, tmp, OR_op);
    free_image(tmp);

  //  (@and corridor (@setlevel (@prune (@setlevel (*or opening corridor)  1 1 2) 2 4)
  //  			      2 2 1)
  //  	  )

    tmp=copy_image(shortcut);
    uc_dilate4(tmp, 1, 1);
    generic_arith(tmp, residues, INF_op);
    free_image(residues);
    bitwise_op(shortcut, tmp, OR_op);
    
#ifdef DEBUG
    printf("hello 14 in fsp\n");
    writeTiffOneStripPerLine(shortcut, "shortcut1.tif", NULL);
#endif

    free_image(tmp);
  }
  loadBar(93, 100, 100, 50);

  //  (@and shortcut (@setlevel (@prune (@setlevel (*or opening shortcut)  1 1 2) 2 4)
  //			      2 2 1)
  //	  )
  //  ;; end of patch
  
  // generate output

  generic_shift(corridor, -5);
  generic_shift(shortcut, -6);

  bitwise_op(out, corridor, OR_op);
  free_image(corridor);
  bitwise_op(out, shortcut, OR_op);
  free_image(shortcut);

  if (internal==1){
    generic_shift(core, 4);
    tmp=uc_fillhole(core, graphbg);

    //if (disk==1){
      allHoles=(IMAGE *)read_image("allHoles.tif");
      remove("allHoles.tif");
    //}
    coreHoles=copy_image(tmp);
    generic_arith(coreHoles, core, SUB_op);
    negation(coreHoles);
    bitwise_op(allHoles, coreHoles, AND_op);  /* A \ B=A \int (\complement B) */
    free_image(coreHoles);
    generic_setlevel(allHoles, 1, 1, 220);
    generic_arith(tmp, core, SUB_op);
    free_image(core);

 #ifdef DEBUG
    printf("hello 15 in fsp\n");
 #endif

    tmp2=uc_fillhole(tmp, graphbg);
    generic_setlevel(tmp2, 1, 1, 100);
    generic_arith(out, tmp2, ADD_op);
    generic_arith(out, allHoles, SUP_op);
    
    free_image(allHoles);
    free_image(tmp); free_image(tmp2);
  }

  loadBar(97, 100, 100, 50);

  create_lut(out);
  index=0;
  out->lut[index]=56540;
  out->lut[index+256]=56540; /* assuming 256 entries in ColorMap! */
  out->lut[index+512]=56540; /* assuming 256 entries in ColorMap! */

  index=100;
  out->lut[index]=35000;
  out->lut[index+256]=35000; /* assuming 256 entries in ColorMap! */
  out->lut[index+512]=35000; /* assuming 256 entries in ColorMap! */

  index=1;
  out->lut[index]=65535;
  out->lut[index+256]=35980; /* assuming 256 entries in ColorMap! */
  out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

  index=101;
  out->lut[index]=65535;
  out->lut[index+256]=35980; /* assuming 256 entries in ColorMap! */
  out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

  index=3;
  out->lut[index]=0;
  out->lut[index+256]=0; /* assuming 256 entries in ColorMap! */
  out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

  index=103;
  out->lut[index]=0;
  out->lut[index+256]=0; /* assuming 256 entries in ColorMap! */
  out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

  index=5;
  out->lut[index]=0;
  out->lut[index+256]=0; /* assuming 256 entries in ColorMap! */
  out->lut[index+512]=65535; /* assuming 256 entries in ColorMap! */

  index=105;
  out->lut[index]=0;
  out->lut[index+256]=0; /* assuming 256 entries in ColorMap! */
  out->lut[index+512]=65535; /* assuming 256 entries in ColorMap! */

  index=9;
  out->lut[index]=41120;
  out->lut[index+256]=15420; /* assuming 256 entries in ColorMap! */
  out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

  index=109;
  out->lut[index]=41120;
  out->lut[index+256]=15420; /* assuming 256 entries in ColorMap! */
  out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

  index=17;
  out->lut[index]=0;
  out->lut[index+256]=51400; /* assuming 256 entries in ColorMap! */
  out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

  index=117;
  out->lut[index]=0;
  out->lut[index+256]=51400; /* assuming 256 entries in ColorMap! */
  out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

  index=33;
  out->lut[index]=65535;
  out->lut[index+256]=0; /* assuming 256 entries in ColorMap! */
  out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

  index=133;
  out->lut[index]=65535;
  out->lut[index+256]=0; /* assuming 256 entries in ColorMap! */
  out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

  index=65;
  out->lut[index]=65535;
  out->lut[index+256]=65535; /* assuming 256 entries in ColorMap! */
  out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

  index=165;
  out->lut[index]=65535;
  out->lut[index+256]=65535; /* assuming 256 entries in ColorMap! */
  out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

  index=129;
  out->lut[index]=65535;
  out->lut[index+256]=65535; /* assuming 256 entries in ColorMap! */
  out->lut[index+512]=65535; /* assuming 256 entries in ColorMap! */

  index=220;
  out->lut[index]=50000;
  out->lut[index+256]=50000; /* assuming 256 entries in ColorMap! */
  out->lut[index+512]=50000; /* assuming 256 entries in ColorMap! */

  if (transition==1){
    index=35;
    out->lut[index]=65535;
    out->lut[index+256]=0; /* assuming 256 entries in ColorMap! */
    out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

    index=135;
    out->lut[index]=65535;
    out->lut[index+256]=0; /* assuming 256 entries in ColorMap! */
    out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

    index=67;
    out->lut[index]=65535;
    out->lut[index+256]=65535; /* assuming 256 entries in ColorMap! */
    out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

    index=167;
    out->lut[index]=65535;
    out->lut[index+256]=65535; /* assuming 256 entries in ColorMap! */
    out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

    index=37;
    out->lut[index]=65535;
    out->lut[index+256]=0; /* assuming 256 entries in ColorMap! */
    out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

    index=137;
    out->lut[index]=65535;
    out->lut[index+256]=0; /* assuming 256 entries in ColorMap! */
    out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

    index=69;
    out->lut[index]=65535;
    out->lut[index+256]=65535; /* assuming 256 entries in ColorMap! */
    out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

    index=169;
    out->lut[index]=65535;
    out->lut[index+256]=65535; /* assuming 256 entries in ColorMap! */
    out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */
  }
  else{
    index=35;
    out->lut[index]=0;
    out->lut[index+256]=0; /* assuming 256 entries in ColorMap! */
    out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

    index=135;
    out->lut[index]=0;
    out->lut[index+256]=0; /* assuming 256 entries in ColorMap! */
    out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

    index=67;
    out->lut[index]=0;
    out->lut[index+256]=0; /* assuming 256 entries in ColorMap! */
    out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

    index=167;
    out->lut[index]=0;
    out->lut[index+256]=0; /* assuming 256 entries in ColorMap! */
    out->lut[index+512]=0; /* assuming 256 entries in ColorMap! */

    index=37;
    out->lut[index]=0;
    out->lut[index+256]=0; /* assuming 256 entries in ColorMap! */
    out->lut[index+512]=65535; /* assuming 256 entries in ColorMap! */

    index=137;
    out->lut[index]=0;
    out->lut[index+256]=0; /* assuming 256 entries in ColorMap! */
    out->lut[index+512]=65535; /* assuming 256 entries in ColorMap! */

    index=69;
    out->lut[index]=0;
    out->lut[index+256]=0; /* assuming 256 entries in ColorMap! */
    out->lut[index+512]=65535; /* assuming 256 entries in ColorMap! */

    index=169;
    out->lut[index]=0;
    out->lut[index+256]=0; /* assuming 256 entries in ColorMap! */
    out->lut[index+512]=65535; /* assuming 256 entries in ColorMap! */
  }
  loadBar(98, 100, 100, 50);

  //  (@fm_postproc control im)
  //  (@fm_frame_out control bufsize)
  box[0]=box[1]=box[2]=box[3]=(int)(bufsize+1);
  box[4]=box[5]=0;
  generic_subframebox(out, box);

  im=(IMAGE *)read_image(fnin);
  generic_thresh(im, 1, 255, 129, 0);
  generic_arith(out, im, MASK_op);
  free_image(im);

  writeTiffOneStripPerLine(out, fnout, desc);
  loadBar(100, 100, 100, 50);

  printf(" \n");

  exit(0);
}

