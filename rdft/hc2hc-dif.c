/*
 * Copyright (c) 2002 Matteo Frigo
 * Copyright (c) 2002 Steven G. Johnson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* $Id: hc2hc-dif.c,v 1.1 2002-07-25 04:25:06 stevenj Exp $ */

/* decimation in frequency Cooley-Tukey */
#include "rdft.h"
#include "hc2hc.h"

static void apply(plan *ego_, R *I, R *O)
{
     plan_hc2hc *ego = (plan_hc2hc *) ego_;
     R *I0 = I;

     {
          plan_rdft *cld0 = (plan_rdft *) ego->cld0;
          plan_rdft *cldm = (plan_rdft *) ego->cldm;
          uint i, r = ego->r, m = ego->m, vl = ego->vl;
          int is = ego->is, ivs = ego->ivs;
	  
          for (i = 0; i < vl; ++i, I += ivs) {
	       cld0->apply((plan *) cld0, I, I);
               ego->k(I + is, I + (r * m - 1) * is, ego->W, ego->ios, m, is);
	       cldm->apply((plan *) cldm, I + is*(m/2), I + is*(m/2));
	  }
     }

     /* two-dimensional r x vl sub-transform: */
     {
	  plan_rdft *cld = (plan_rdft *) ego->cld;
	  cld->apply((plan *) cld, I0, O);
     }
}

static int applicable(const solver_hc2hc *ego, const problem *p_)
{
     if (X(rdft_hc2hc_applicable)(ego, p_)) {
	  int ivs, ovs;
	  uint vl;
          const hc2hc_desc *e = ego->desc;
          const problem_rdft *p = (const problem_rdft *) p_;
          iodim *d = p->sz.dims;
	  uint m = d[0].n / e->radix;
	  X(rdft_hc2hc_vecstrides)(p, &vl, &ivs, &ovs);
          return (1
		  && (e->genus->okp(e, p->I + d[0].is,
				    p->I + (e->radix * m - 1) * d[0].is, 
				    (int)m * d[0].is, 0, m, d[0].is))
		  && (e->genus->okp(e, p->I + ivs + d[0].is,
				    p->I + ivs + (e->radix * m - 1) * d[0].is, 
				    (int)m * d[0].is, 0, m, d[0].is))
	       );
     }
     return 0;
}

static void finish(plan_hc2hc *ego)
{
     const hc2hc_desc *d = ego->slv->desc;
     ego->ios = X(mkstride)(ego->r, ego->m * ego->is);
     ego->super.super.ops =
          X(ops_add)(X(ops_add)(ego->cld->ops,
				X(ops_mul)(ego->vl,
					   X(ops_add)(ego->cld0->ops,
						      ego->cldm->ops))),
		     X(ops_mul)(ego->vl * ((ego->m - 1)/2) / d->genus->vl,
				d->ops));
}

static int score(const solver *ego_, const problem *p_, int flags)
{
     const solver_hc2hc *ego = (const solver_hc2hc *) ego_;
     const problem_rdft *p;
     uint n;

     if (!applicable(ego, p_))
          return BAD;

     p = (const problem_rdft *) p_;

     /* emulate fftw2 behavior */
     if ((p->vecsz.rnk > 0) && NO_VRECURSE(flags))
	  return BAD;

     n = p->sz.dims[0].n;
     if (0
	 || n <= 16 
	 || n / ego->desc->radix <= 4
	  )
          return UGLY;

     return GOOD;
}


static plan *mkplan(const solver *ego, const problem *p, planner *plnr)
{
     static const hc2hcadt adt = {
	  X(rdft_mkcldrn_dif), finish, applicable, apply
     };
     return X(mkplan_rdft_hc2hc)((const solver_hc2hc *) ego, p, plnr, &adt);
}


solver *X(mksolver_rdft_hc2hc_dif)(khc2hc codelet, const hc2hc_desc *desc)
{
     static const solver_adt sadt = { mkplan, score };
     static const char name[] = "rdft-dif";

     return X(mksolver_rdft_hc2hc)(codelet, desc, name, &sadt);
}
