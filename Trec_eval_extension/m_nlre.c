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
te_calc_nlre (const EPI *epi, const REL_INFO *rel_info,
		const RESULTS *results, const TREC_MEAS *tm, TREC_EVAL *eval);
static PARAMS default_nlre = { NULL, 0, NULL}; // useless


/* See trec_eval.h for definition of TREC_MEAS */
TREC_MEAS te_meas_nlre =
    {"nlre",
     "    Normalised Local Rank Error (NLRE)\n\
    Compute the nLRE measure according to Christina Lioma, Jakob Grue Simonsen, and\n\
    Birger Larsen in 'Evaluation Measures for Relevance and Credibility in Ranked Lists'. \n\
    In Proceedings of the ACM SIGIR International Conference on Theory of Information Retrieval (ICTIR '17)\n\
    This evaluation measure was designed to measure the effectiveness of both relevance and credibility\n\
    in ranked lists of retrieval results simultaneously and without bias in favour of either relevance or credibility\n\
    The values are set to the appropriate relevance and credibility by default.  \n\
    The default values of u and v are 0.5 (default) so far it can only be overridden on the code.\n",
     te_init_meas_s_float_p_pair,
     te_calc_nlre,
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
} DOCNO_IDEAL_INFO;


static DOCNO_IDEAL_INFO *docno_info_ideal;
static long max_docno_info_ideal = 0;
static float calc_lre(), calc_clre();

static int 
te_calc_nlre (const EPI *epi, const REL_INFO *rel_info,
	       const RESULTS *results, const TREC_MEAS *tm, TREC_EVAL *eval)
{
    RES_RELS res_rels;
    long num_results;
    long max_rel;
    long max_cred;
    float error;
    float c_lre;
    float nlre;

    TEXT_RESULTS_INFO *text_results_info;
    TEXT_QRELS_CRED_INFO *trec_qrels;

    TEXT_QRELS_CRED *qrels_ptr;

    text_results_info = (TEXT_RESULTS_INFO *) results->q_results;
    trec_qrels = (TEXT_QRELS_CRED_INFO *) rel_info->q_rel_info;

    qrels_ptr = trec_qrels->text_qrels_cred;
    max_rel = qrels_ptr->rel;
    max_cred = qrels_ptr->cred;

    num_results = text_results_info->num_text_results;

    DOCNO_IDEAL_INFO *docno_info_ideal;


    if (NULL == (docno_info_ideal = te_chk_and_malloc (docno_info_ideal, &max_docno_info_ideal,
				    num_results, sizeof (DOCNO_IDEAL_INFO))))
	return (UNDEF);
	


    if (UNDEF == te_form_res_rels_cred (epi, rel_info, results, &res_rels, &docno_info_ideal))
	return (UNDEF);
    
    max_docno_info_ideal = 0;
    error = calc_lre(num_results, docno_info_ideal);
    c_lre = calc_clre(num_results);
    nlre = 1 - (error/c_lre);
    eval->values[tm->eval_index].value = nlre;
    

    return (1);
}

static float
calc_lre(int num_results, DOCNO_IDEAL_INFO *idealrankings){
	float LRE = 0;
    float u =0.5;
    float v = 0.5;
	int i, j, e_r, e_c;

    /*The LRE is equal to 1 if n = 1*/
    if (num_results < 2){
        return (0);
    }

    /*Compute the Local Rank Error a ranking (LRE) */
	for (i = 0; i < num_results; i++) {
		j = i+1;
		if (j < num_results){
			e_r = max(0, idealrankings[i].pos_rank_rel - idealrankings[j].pos_rank_rel);
			e_c = max(0, idealrankings[i].pos_rank_cred - idealrankings[j].pos_rank_cred);
			LRE += ((u+e_r) * (v+e_c) - (u*v))/log2(i+2); /*The +2 is because the for starts from 0.*/ 
		}
    }
	return (LRE);
}

static float calc_clre(int num_results){
    int j;
    double u= 0.5;
    double v=0.5;
    float clre = 0;

    // FIX FOR SHORT RANKINGS //
    if (num_results <= 2){
        return (1);
    }
    if (num_results == 3){
        return (2);
    }

    /*This loop will compute the value of the normalisation constant.*/
    for (j = 0; j <= floor((num_results/2)-1); ++j)
    {
        clre += ((pow((num_results - (2*j) -1),2)) + ((u+v)*(num_results-(2*j)-1)))/(1+(log2(1+j)));
    }
    return (clre);
}
