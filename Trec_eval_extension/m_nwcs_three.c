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
#include <ctype.h>

double log2(double x);

static int 
te_calc_nwcs_three (const EPI *epi, const REL_INFO *rel_info,
		const RESULTS *results, const TREC_MEAS *tm, TREC_EVAL *eval);
static PARAMS default_nlre = { NULL, 0, NULL}; // useless



/* See trec_eval.h for definition of TREC_MEAS */
TREC_MEAS te_meas_nwcs_three =
    {"nwcs_three",
     "    Normalised Weighted Cumulative Score (NWCS)\n\
    Compute the nLRE measure according to Christina Lioma, Jakob Grue Simonsen, and\n\
    Birger Larsen in 'Evaluation Measures for Relevance and Credibility in Ranked Lists'. \n\
    In Proceedings of the ACM SIGIR International Conference on Theory of Information Retrieval (ICTIR '17)\n\
    This evaluation measure was designed to measure the effectiveness of both relevance and credibility\n\
    in ranked lists of retrieval results simultaneously and without bias in favour of either relevance or credibility\n\
    The values are set to the appropriate relevance and credibility by default.\n\
    This is a variation to handle three aspects.\n",
     te_init_meas_s_float_p_pair,
     te_calc_nwcs_three,
     te_acc_meas_s,
     te_calc_avg_meas_s,
     te_print_single_meas_s_float,
     te_print_final_meas_s_float_p,
     &default_nlre, -1};


typedef struct {
    char *docno;
    float sim;
    long pos_rank_rel;
    long pos_rank_cred;
    long rel;
    long cred;
    long third;
    long ideal_sum;
} DOCNO_IDEAL_INFO;



static long max_docno_info_ideal = 0;
static float calc_nwcs_three();

static int 
te_calc_nwcs_three (const EPI *epi, const REL_INFO *rel_info,
	       const RESULTS *results, const TREC_MEAS *tm, TREC_EVAL *eval)
{
    RES_RELS res_rels;
    long num_results;
    // DOCNO_IDEAL_INFO *docno_info_ideal;

    float nWCS;
    TEXT_RESULTS_INFO *text_results_info;
    TEXT_QRELS_CRED_INFO *trec_qrels;

    
    text_results_info = (TEXT_RESULTS_INFO *) results->q_results;
    trec_qrels = (TEXT_QRELS_CRED_INFO *) rel_info->q_rel_info;


    num_results = text_results_info->num_text_results;

    DOCNO_IDEAL_INFO *docno_info_ideal;


    if (NULL == (docno_info_ideal = te_chk_and_malloc (docno_info_ideal, &max_docno_info_ideal,
				    trec_qrels->num_text_qrels, sizeof (DOCNO_IDEAL_INFO))))
	return (UNDEF);
	

    if (UNDEF == te_form_res_rels_threeaspects (epi, rel_info, results, &res_rels, &docno_info_ideal))
	return (UNDEF);
    int i;
    
    max_docno_info_ideal = 0;
    nWCS = calc_nwcs_three(num_results, docno_info_ideal);
    eval->values[tm->eval_index].value = nWCS;
    

    return (1);
}

static float
calc_nwcs_three(int num_results, DOCNO_IDEAL_INFO *ranking_with_assessments){

	int i;
	double wcs, ideal_wcs;
	double nWCS;
	wcs = 0;
	ideal_wcs = 0;
	nWCS = 0;
	double logval;

    /*Compute the WCS and IdealWCS of a ranking. Attention that the ideal_sum here is the total sum over all assessments.
	  If you want to change the weight of each aspect
	  you have to change in the form_res_rels_twoaspects.c where we compute the ideal_sum value.*/

	for (i = 0; i < num_results; i++) {
        wcs += ((0.3333333*ranking_with_assessments[i].rel)+(0.33333*ranking_with_assessments[i].cred)+(0.333333*ranking_with_assessments[i].third))/log2(i+2);
        ideal_wcs += (0.33333333*ranking_with_assessments[i].ideal_sum)/log2(i+2);
        logval = log2(i+2);
     //    printf("DocNO:%s,Rel:%ld,Cred:%ld,Third:%ld,IdealSum:%ld\n",ranking_with_assessments[i].docno,ranking_with_assessments[i].rel, ranking_with_assessments[i].cred,ranking_with_assessments[i].third, ranking_with_assessments[i].ideal_sum);
    	// printf("WCS[%ld]: %lf,ideal_WCS[%ld]: %lf -- logval:%lf\n",i, wcs,i,ideal_wcs, logval);
    }
    if (ideal_wcs > 0){
    	nWCS = wcs/ideal_wcs;
    }
	return (nWCS);
}
