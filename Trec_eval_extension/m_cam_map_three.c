/* 
   Copyright (c) 2008 - Chris Buckley. 

   Permission is granted for use and modification of this file for
   research, non-commercial purposes. 
*/

#include "common.h"
#include "sysfunc.h"
#include "trec_eval.h"
#include "functions.h"
#include "trec_format.h"

static int
te_calc_cam_map_three (const EPI *epi, const REL_INFO *rel_info,
	     const RESULTS *results, const TREC_MEAS *tm, TREC_EVAL *eval);
/* See trec_eval.h for definition of TREC_MEAS */
TREC_MEAS te_meas_cam_map_three =  {
   "cam_map_three",
   "    Convex aggregating measure (CAM)\n\
    Compute the CAM measure according to Christina Lioma, Jakob Grue Simonsen, and\n\
    Birger Larsen in 'Evaluation Measures for Relevance and Credibility in Ranked Lists'. \n\
    In Proceedings of the ACM SIGIR International Conference on Theory of Information Retrieval (ICTIR '17)\n\
    This evaluation measure was designed to measure the effectiveness of both relevance and credibility\n\
    in ranked lists of retrieval results simultaneously and without bias in favour of either relevance or credibility\n\
    The values are set to the appropriate relevance and credibility by default.  \n\
    In this metric we compute the CAM of MAP for each aspect.\n\
    This metric was extended to handle three aspects.\n",
     te_init_meas_s_float,
     te_calc_cam_map_three,
     te_acc_meas_s,
     te_calc_avg_meas_s,
     te_print_single_meas_s_float,
     te_print_final_meas_s_float,
     NULL, -1};

static int 
te_calc_cam_map_three (const EPI *epi, const REL_INFO *rel_info, const RESULTS *results,
	     const TREC_MEAS *tm, TREC_EVAL *eval)
{
    // RES_RELS res_rels;
    double sum;
    long rel_so_far;
    long i;
    long pa;
    double tmp_map;
    tmp_map = 0.0;
    for (pa = 0; pa <= 2; pa++)
    {
        RES_RELS res_rels = {.num_rel_ret=0, .num_ret = 0, .num_nonpool=0, .num_unjudged_in_pool=0,.num_rel=0,.num_rel_levels=0,.rel_levels =0,.results_rel_list=0};
        if (UNDEF == te_form_res_rels_three (epi, rel_info, results, &res_rels, &pa))
    	return (UNDEF);
        rel_so_far = 0;
        sum = 0.0;
        for (i = 0; i < res_rels.num_ret; i++) {
        	if (res_rels.results_rel_list[i] >= epi->relevance_level) {
        	    rel_so_far++;
        	    sum += (double) rel_so_far / (double) (i + 1);
        	}
        }
        if (rel_so_far) {
            double by_aspect;
            by_aspect = 0;
            by_aspect = sum / (double) res_rels.num_rel;
            tmp_map = tmp_map + (0.3333*by_aspect);
        }    
    }
    /* Average over the rel docs */
    if (rel_so_far) {
	eval->values[tm->eval_index].value = 
	    tmp_map;
    }
    return (1);
}
