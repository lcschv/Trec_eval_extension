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
#include <stdlib.h>

double log2(double x);

static int 
te_calc_cam (const EPI *epi, const REL_INFO *rel_info,
	      const RESULTS *results, const TREC_MEAS *tm, TREC_EVAL *eval);
static PARAMS default_ndcg_gains = { NULL, 0, NULL};

/* See trec_eval.h for definition of TREC_MEAS */
TREC_MEAS te_meas_cam =
    {"cam",
     "    Convex aggregating measure (CAM)\n\
    Compute the CAM measure according to Christina Lioma, Jakob Grue Simonsen, and\n\
    Birger Larsen in 'Evaluation Measures for Relevance and Credibility in Ranked Lists'. \n\
    In Proceedings of the ACM SIGIR International Conference on Theory of Information Retrieval (ICTIR '17)\n\
    This evaluation measure was designed to measure the effectiveness of both relevance and credibility\n\
    in ranked lists of retrieval results simultaneously and without bias in favour of either relevance or credibility\n\
    The values are set to the appropriate relevance and credibility by default.  \n\
    In this metric we compute the CAM of nDCG for each aspect.",
     te_init_meas_s_float_p_pair,
     te_calc_cam,
     te_acc_meas_s,
     te_calc_avg_meas_s,
     te_print_single_meas_s_float,
     te_print_final_meas_s_float_p,
     &default_ndcg_gains, -1};

/* Keep track of valid rel_levels and associated gains */
/* Initialized in setup_gains */
typedef struct {
    long rel_level;
    long num_at_level;
    double gain;
} REL_GAIN;

typedef struct {
    REL_GAIN *rel_gains;
    long num_gains;
    long total_num_at_levels;
} GAINS;

static int setup_gains (const TREC_MEAS *tm, const RES_RELS *res_rels,
			GAINS *gains);
static double get_gain (const long rel_level, const GAINS *gains);
static int comp_rel_gain ();

static int 
te_calc_cam (const EPI *epi, const REL_INFO *rel_info,
	      const RESULTS *results, const TREC_MEAS *tm, TREC_EVAL *eval)
{
    
    
    double results_gain, results_dcg;
    double ideal_gain, ideal_dcg;
    long cur_level, num_at_level;
    long i;
    long pa;
    
    float tmp_ndcg;
    tmp_ndcg = 0.0;
    for (pa = 0; pa <= 1; pa++)
    {
        RES_RELS res_rels = {.num_rel_ret=0, .num_ret = 0, .num_nonpool=0, .num_unjudged_in_pool=0,.num_rel=0,.num_rel_levels=0,.rel_levels =0,.results_rel_list=0};
        REL_GAIN tmp_relgain = {.rel_level = 0, .num_at_level=0, .gain=0};
        GAINS gains = {.rel_gains = &tmp_relgain, .num_gains=0, .total_num_at_levels=0};
            if (UNDEF == te_form_res_rels_two (epi, rel_info, results, &res_rels, &pa))
        return (UNDEF);
        
        if (UNDEF == setup_gains (tm, &res_rels, &gains))
        return (UNDEF);
        results_dcg = 0.0;
        ideal_dcg = 0.0;
        cur_level = gains.num_gains - 1;
        ideal_gain = (cur_level >= 0) ? gains.rel_gains[cur_level].gain : 0.0;
        num_at_level = 0;

        for (i = 0; i < res_rels.num_ret && ideal_gain > 0.0; i++) {
        /* Calculate change in results dcg */
        results_gain = get_gain (res_rels.results_rel_list[i], &gains);
        if (results_gain != 0)
            /* Note: i+2 since doc i has rank i+1 */
            results_dcg += results_gain / log2((double) (i+2));
        /* Calculate change in ideal dcg */
        num_at_level++;
        while (cur_level >= 0 &&
               num_at_level > gains.rel_gains[cur_level].num_at_level) {
            num_at_level = 1;
            cur_level--;
            ideal_gain = (cur_level >= 0) ? gains.rel_gains[cur_level].gain:0.0;
        }
        if (ideal_gain > 0.0)
            ideal_dcg += ideal_gain / log2((double)(i + 2));
        if (epi->debug_level > 0) 
            printf("ndcg: %ld %ld %3.1f %6.4f %3.1f %6.4f\n",
               i, cur_level, results_gain, results_dcg,
               ideal_gain, ideal_dcg);
        }
        while (i < res_rels.num_ret) {
        /* Calculate change in results dcg */
        results_gain = get_gain (res_rels.results_rel_list[i], &gains);
        if (results_gain != 0)
            results_dcg += results_gain / log2((double) (i+2));
        if (epi->debug_level > 0) 
            printf("ndcg: %ld %ld %3.1f %6.4f %3.1f %6.4f\n",
               i, cur_level, results_gain, results_dcg, 0.0, ideal_dcg);
        i++;
        }
        while (ideal_gain > 0.0) {
        /* Calculate change in ideal dcg */
        num_at_level++;
        while (cur_level >= 0 &&
               num_at_level > gains.rel_gains[cur_level].num_at_level) {
            num_at_level = 1;
            cur_level--;
            ideal_gain = (cur_level >= 0) ? gains.rel_gains[cur_level].gain:0.0;
        }
        if (ideal_gain > 0.0)
            ideal_dcg += ideal_gain / log2((double)(i + 2));
        if (epi->debug_level > 0) 
            printf("ndcg: %ld %ld %3.1f %6.4f %3.1f %6.4f\n",
               i, cur_level, 0.0, results_dcg,
               ideal_gain, ideal_dcg);
        i++;
        }
        Free (gains.rel_gains);
        if (ideal_dcg > 0.0) {
            /*Change the value 0.5 if you want to change the amount of each ndcg.
            This now gives equal weights for each aspect. */
            float by_aspect;
            by_aspect = 0;
            by_aspect = results_dcg / ideal_dcg;
            tmp_ndcg = tmp_ndcg + (0.5*by_aspect);
        }
        else{
            printf("Something went wrong. ideal_dcg equals to 0.\n");
            exit(1);
        }
        

    }
    
    /* Compare sum to ideal NDCG */
    if (ideal_dcg > 0.0) {
        eval->values[tm->eval_index].value = tmp_ndcg;
    }

    
    return (1);
}

static int
setup_gains (const TREC_MEAS *tm, const RES_RELS *res_rels, GAINS *gains)
{
    FLOAT_PARAM_PAIR *pairs = NULL;
    long num_pairs = 0;
    long i,j;
    long num_gains;

    if (tm->meas_params) {
	pairs = (FLOAT_PARAM_PAIR *) tm->meas_params->param_values;
	num_pairs = tm->meas_params->num_params;
    }

    if (NULL == (gains->rel_gains = Malloc(res_rels->num_rel_levels + num_pairs,
					   REL_GAIN)))
	return (UNDEF);
    num_gains = 0;
    for (i = 0; i < num_pairs; i++) {
	gains->rel_gains[num_gains].rel_level = atol (pairs[i].name);
	gains->rel_gains[num_gains].gain = (double) pairs[i].value;
	gains->rel_gains[num_gains].num_at_level = 0;
	num_gains++;
    }

    for (i = 0; i < res_rels->num_rel_levels; i++) {
	for (j = 0; j < num_gains && gains->rel_gains[j].rel_level != i; j++)
	    ;
	if (j < num_gains)
	    /* Was included in list of parameters. Update occurrence info */
	    gains->rel_gains[j].num_at_level = res_rels->rel_levels[i];
	else {
	    /* Not included in list of parameters. New gain level */
	    gains->rel_gains[num_gains].rel_level = i;
	    gains->rel_gains[num_gains].gain = (double) i;
	    gains->rel_gains[num_gains].num_at_level = res_rels->rel_levels[i];
	    num_gains++;
	}
    }

    /* Sort gains by increasing gain value */
    qsort ((char *) gains->rel_gains,
           (int) num_gains,
           sizeof (REL_GAIN),
           comp_rel_gain);

    gains->total_num_at_levels = 0;
    for (i = 0; i < num_gains; i++)
	gains->total_num_at_levels += gains->rel_gains[i].num_at_level;	

    gains->num_gains = num_gains;
    return (1);
}

static int comp_rel_gain (REL_GAIN *ptr1, REL_GAIN *ptr2)
{
    return (ptr1->gain - ptr2->gain);
}

static double
get_gain (const long rel_level, const GAINS *gains)
{
    long i;
    for (i = 0; i < gains->num_gains; i++)
	if (rel_level == gains->rel_gains[i].rel_level)
	    return (gains->rel_gains[i].gain);
    return (0.0);   /* Print Error ?? */
}

