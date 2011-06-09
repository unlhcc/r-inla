
/* GMRFLib-sparse-interface.h
 * 
 * Copyright (C) 2001-2006 Havard Rue
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * The author's contact information:
 *
 *       H{\aa}vard Rue
 *       Department of Mathematical Sciences
 *       The Norwegian University of Science and Technology
 *       N-7491 Trondheim, Norway
 *       Voice: +47-7359-3533    URL  : http://www.math.ntnu.no/~hrue  
 *       Fax  : +47-7359-3524    Email: havard.rue@math.ntnu.no
 *
 * RCSId: $Id: sparse-interface.h,v 1.29 2010/02/27 08:32:07 hrue Exp $
 *
 */

/*!
  \file sparse-interface.h
  \brief Typedefs and defines for the interface to the sparse-matrix libraries.
*/

#ifndef __GMRFLib_SPARSE_INTERFACE_H__
#define __GMRFLib_SPARSE_INTERFACE_H__

#if !defined(__FreeBSD__)
#include <malloc.h>
#endif
#include <stdlib.h>

#undef __BEGIN_DECLS
#undef __END_DECLS
#ifdef __cplusplus
#define __BEGIN_DECLS extern "C" {
#define __END_DECLS }
#else
#define __BEGIN_DECLS					       /* empty */
#define __END_DECLS					       /* empty */
#endif

__BEGIN_DECLS typedef enum {

	/**
	 * \brief Lapack's band-solver
	 */
	GMRFLib_SMTP_BAND = 1,

	/**
	 * \brief The solver in the TAUCS-library
	 */
	GMRFLib_SMTP_TAUCS = 2,

	/**
	 * \brief An empty template. Not in use.
	 */
	GMRFLib_SMTP_PROFILE = 3
} GMRFLib_smtp_tp;

typedef enum {

	/**
	 * \brief The default reordering choice, which depends on the sparse-solver used.
	 * 
	 * Currently, it minimise the bandwidth for the band-solver, and nested dissection using the TAUCS-solver 
	 */
	GMRFLib_REORDER_DEFAULT = 0,

	/**
	 * \brief Identity (no reordering) 
	 */
	GMRFLib_REORDER_IDENTITY,

	/**
	 * \brief Minmise the bandwidth 
	 */
	GMRFLib_REORDER_BAND,

	/**
	 * \brief The nested dissection reordering in the METIS-library 
	 */
	GMRFLib_REORDER_METIS,

	/**
	 * \brief Multiple minimum degree reordering 
	 */
	GMRFLib_REORDER_GENMMD,

	/**
	 * \brief Approximate minimum degree reordering 
	 */
	GMRFLib_REORDER_AMD,

	/**
	 * \brief True minimum degree reordering 
	 */
	GMRFLib_REORDER_MD,

	/**
	 * \brief Multiple minimum degree reordering 
	 */
	GMRFLib_REORDER_MMD
} GMRFLib_reorder_tp;

/*! 
  \struct GMRFLib_fact_info_tp problem-setup.h
  \brief Description of Qmatrix
 */
typedef struct {

	/**
	 *  \brief Size of Q 
	 */
	int n;

	/**
	 *  \brief Number of non-zero elements 
	 */
	int nnzero;

	/**
	 *  \brief Fillin in L 
	 */
	int nfillin;
} GMRFLib_fact_info_tp;

typedef struct {

	/**
	 * \brief Sparse-matrix type.					       
	 */
	GMRFLib_smtp_tp smtp;

	/**
	 *  \brief Permutation 
	 */
	int *remap;

	/**
	 *  \brief The Cholesky factorisation (smtp == BAND)
	 */
	double *bchol;

	/**
	 *  \brief The bandwidth of the Cholesky factorisation (smtp == BAND)
	 */
	int bandwidth;

	/**
	 *  \brief The Cholesky factorisation (smtp == TAUCS)
	 */
	taucs_ccs_matrix *L;

	/**
	 *  \brief The symbolic factorisation (smtp == TAUCS)
	 */
	supernodal_factor_matrix *symb_fact;

	/**
	 *  \brief The inverse of the diagonal of L (smtp = TAUCS)
	 */
	double *L_inv_diag;

	 /**
	 *  \brief Info about the factorization 
	 */
	GMRFLib_fact_info_tp finfo;



} GMRFLib_sm_fact_tp;

/* 
   
 */

int GMRFLib_compute_reordering(GMRFLib_sm_fact_tp * sm_fact, GMRFLib_graph_tp * graph);
int GMRFLib_free_reordering(GMRFLib_sm_fact_tp * sm_fact);
int GMRFLib_build_sparse_matrix(GMRFLib_sm_fact_tp * sm_fact, GMRFLib_Qfunc_tp * Qfunc, void *Qfunc_arg, GMRFLib_graph_tp * graph);
int GMRFLib_factorise_sparse_matrix(GMRFLib_sm_fact_tp * sm_fact, GMRFLib_graph_tp * graph);
int GMRFLib_free_fact_sparse_matrix(GMRFLib_sm_fact_tp * sm_fact);
int GMRFLib_solve_lt_sparse_matrix(double *rhs, GMRFLib_sm_fact_tp * fact_tp, GMRFLib_graph_tp * graph);
int GMRFLib_solve_llt_sparse_matrix(double *rhs, GMRFLib_sm_fact_tp * fact_tp, GMRFLib_graph_tp * graph);
int GMRFLib_solve_llt_sparse_matrix_special(double *rhs, GMRFLib_sm_fact_tp * fact_tp, GMRFLib_graph_tp * graph, int idx);
int GMRFLib_solve_lt_sparse_matrix_special(double *rhs, GMRFLib_sm_fact_tp * sm_fact, GMRFLib_graph_tp * graph, int findx, int toindx, int remapped);
int GMRFLib_solve_l_sparse_matrix(double *rhs, GMRFLib_sm_fact_tp * sm_fact, GMRFLib_graph_tp * graph);
int GMRFLib_solve_l_sparse_matrix_special(double *rhs, GMRFLib_sm_fact_tp * sm_fact, GMRFLib_graph_tp * graph, int findx, int toindx, int remapped);
int GMRFLib_comp_cond_meansd(double *cmean, double *csd, int indx, double *x, int remapped, GMRFLib_sm_fact_tp * sm_fact, GMRFLib_graph_tp * graph);
int GMRFLib_log_determinant(double *logdet, GMRFLib_sm_fact_tp * sm_fact, GMRFLib_graph_tp * graph);
int GMRFLib_compute_Qinv(void *problem, int storage);
int GMRFLib_valid_smtp(int smtp);
int GMRFLib_bitmap_factorisation(const char *filename_body, GMRFLib_sm_fact_tp * sm_fact, GMRFLib_graph_tp * graph);
const char *GMRFLib_reorder_name(GMRFLib_reorder_tp r);
int GMRFLib_reorder_id(const char *name);

__END_DECLS
#endif
