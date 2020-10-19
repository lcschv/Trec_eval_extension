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
te_calc_nlre_three (const EPI *epi, const REL_INFO *rel_info,
        const RESULTS *results, const TREC_MEAS *tm, TREC_EVAL *eval);
static PARAMS default_nlre_three = { NULL, 0, NULL};


/* See trec_eval.h for definition of TREC_MEAS */
TREC_MEAS te_meas_nlre_three =
    {"nlre_three",
     "    Normalised Local Rank Error (NLRE)\n\
    Compute the nLRE measure according to Christina Lioma, Jakob Grue Simonsen, and\n\
    Birger Larsen in 'Evaluation Measures for Relevance and Credibility in Ranked Lists'. \n\
    In Proceedings of the ACM SIGIR International Conference on Theory of Information Retrieval (ICTIR '17)\n\
    The default values of u, v and x are 1/3 (uniform) controling how much we wish to penalize each aspect\n\
    so far it can only be overridden on the code.\n",
     te_init_meas_s_float_p_pair,
     te_calc_nlre_three,
     te_acc_meas_s,
     te_calc_avg_meas_s,
     te_print_single_meas_s_float,
     te_print_final_meas_s_float_p,
     &default_nlre_three, -1};


typedef struct {
    char *docno;
    float sim;
    long pos_rank_rel;
    long pos_rank_cred;
    long pos_rank_third;
    long rel;
    long cred;
    long third;
} DOCNO_IDEAL_INFO;


static DOCNO_IDEAL_INFO *docno_info_ideal;
static long max_docno_info_ideal = 0;
static float calc_lre_three(), calc_clre_three();

static int 
te_calc_nlre_three (const EPI *epi, const REL_INFO *rel_info,
           const RESULTS *results, const TREC_MEAS *tm, TREC_EVAL *eval)
{
    RES_RELS res_rels;
    long num_results;
    long max_rel;
    long max_cred;
    long max_third;
    float error;
    float c_lre;
    float nlre;
    TEXT_RESULTS_INFO *text_results_info;
    TEXT_QRELS_THREE_INFO *trec_qrels;

    TEXT_QRELS_THREE *qrels_ptr;

    text_results_info = (TEXT_RESULTS_INFO *) results->q_results;
    trec_qrels = (TEXT_QRELS_THREE_INFO *) rel_info->q_rel_info;

    qrels_ptr = trec_qrels->text_qrels_three;
    max_rel = qrels_ptr->rel;
    max_cred = qrels_ptr->cred;
    max_third = qrels_ptr->third;

    num_results = text_results_info->num_text_results;

    DOCNO_IDEAL_INFO *docno_info_ideal;


    if (NULL == (docno_info_ideal = te_chk_and_malloc (docno_info_ideal, &max_docno_info_ideal,
                    num_results, sizeof (DOCNO_IDEAL_INFO))))
    return (UNDEF);
    

    if (UNDEF == te_form_res_three (epi, rel_info, results, &res_rels, &docno_info_ideal))
    return (UNDEF);
    
    max_docno_info_ideal = 0;
    error = calc_lre_three(num_results, docno_info_ideal);
    c_lre = calc_clre_three(num_results);
    // printf("%lf --- %lf\n", error,c_lre);
    nlre = 1 - (error/c_lre);
    eval->values[tm->eval_index].value = nlre;
    

    return (1);
}

static float
calc_lre_three(int num_results, DOCNO_IDEAL_INFO *idealrankings){
    float LRE = 0;
    float u = 0.333333;
    float v = 0.333333;
    float x = 0.333333;
    int i, j;
    float e_r, e_c, e_x;

    /*The LRE is equal to 1 if n = 1*/
    if (num_results < 2){
        return (0);
    }

    /*Compute the Local Rank Error a ranking (LRE) */
    for (i = 0; i < num_results; i++) {
        j = i+1;
        if (j < num_results){
            // printf("i:%ld\n",i );
            e_r = MAX(0, idealrankings[i].pos_rank_rel - idealrankings[j].pos_rank_rel);
            e_c = MAX(0, idealrankings[i].pos_rank_cred - idealrankings[j].pos_rank_cred);
            e_x = MAX(0, idealrankings[i].pos_rank_third - idealrankings[j].pos_rank_third);
            // printf("Errors:%lf -- %lf --- %lf \n",e_r, e_c, e_x);
            LRE += ((((u+e_r) * (v+e_c) * (x+e_x)) - (u*v*x))/log2(i+2)); /*The +2 is because the for starts from 0.*/ 
        }
    }
    return (LRE);
}

static float calc_clre_three(int num_results){
    int j;
    double u= 0.33333;
    double v=0.333333;
    double x= 0.33333;
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
        clre += ((pow((num_results - (2*j) -1),3)) + ((u+v+x)*(num_results-(2*j)-1)))/(1+(log2(1+j)));
        // printf("%lf\n",clre );
    }
    return (clre);
}
