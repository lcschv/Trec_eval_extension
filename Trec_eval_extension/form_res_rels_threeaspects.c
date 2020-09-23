/* 
   Copyright (c) 2008 - Chris Buckley. 

   Permission is granted for use and modification of this file for
   research, non-commercial purposes. 
*/
#include "common.h"
#include "sysfunc.h"
#include "trec_eval.h"
#include "trec_format.h"
#include "functions.h"
/* Takes the top docs and judged docs for a query, and returns a
   rel_rank object giving the ordered relevance values for retrieved
   docs, plus relevance occurrence statatistics.
   Relevance value is
       value in text_qrels if docno is in text_qrels and was judged
           (assumed to be a small non-negative integer)
       RELVALUE_NONPOOL (-1) if docno is not in text_qrels
       RELVALUE_UNJUDGED (-2) if docno is in text_qrels and was not judged.

   This procedure may be called repeatedly for a given topic - returned
   values are cached until the query changes.

   results and rel_info formats must be "trec_results" and "qrels"
   respectively.  

   UNDEF returned if error, 0 if used cache values, 1 if new values.
*/

static int comp_sim_docno (), comp_docno (), comp_rel_docno(), comp_cred_docno(), comp_sum_docno();
// static float calc_lre(), calc_clre();

/* Definitions used for temporary and cached values */
typedef struct {
    char *docno;
    float sim;
    long rank;
    long rel;
    long cred;
} DOCNO_INFO;


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


/* Current cached query */
static char *current_query = "no query";
static long max_current_query = 0;

/* Space reserved for cached returned values */
static long *rel_levels;
static long max_rel_levels = 0;
static RES_RELS saved_res_rels;
static long *ranked_rel_list;
static long max_ranked_rel_list = 0;

/* Space reserved for intermediate values */
static DOCNO_INFO *docno_info;
static DOCNO_IDEAL_INFO *docno_info_ideal;
static DOCNO_IDEAL_INFO *docno_info_run;
static long max_docno_info = 0;
static long max_docno_info_ideal = 0;

int max(int num1, int num2);
double log2(double x);
double floor(double x);

int
te_form_res_rels_threeaspects (const EPI *epi, const REL_CRED_INFO *rel_info,
		  const RESULTS *results, RES_RELS *res_rels, DOCNO_IDEAL_INFO **ideal)
{
    long i;
    long num_results;
    long max_rel;
    long max_cred;
    long max_third;


    TEXT_RESULTS_INFO *text_results_info;

    TEXT_QRELS_THREE_INFO *trec_qrels;

    TEXT_QRELS_THREE *qrels_ptr, *end_qrels;


    
    /* Check that format type of result info and rel info are correct */
    if (strcmp ("qrels", rel_info->rel_format) ||
	strcmp ("trec_results", results->ret_format)) {
	fprintf (stderr, "trec_eval.form_res_qrels: rel_info format not qrels or results format not trec_results\n");
	return (UNDEF);
    }

    /* Make sure enough space for query and save copy */
    i = strlen(results->qid)+1;
    if (NULL == (current_query =
		 te_chk_and_malloc (current_query, &max_current_query,
				    i, sizeof (char))))
	return (UNDEF);
    (void) strncpy (current_query, results->qid, i);

    text_results_info = (TEXT_RESULTS_INFO *) results->q_results;
    trec_qrels = (TEXT_QRELS_THREE_INFO *) rel_info->q_rel_info;
    

    num_results = text_results_info->num_text_results;

    /* Check and reserve space for output structure */
    /* Reserve space for temp structure copying results */
    if (NULL == (ranked_rel_list =
		 te_chk_and_malloc (ranked_rel_list, &max_ranked_rel_list,
				    num_results, sizeof (long))) ||
        NULL == (docno_info =
		 te_chk_and_malloc (docno_info, &max_docno_info,
				    num_results, sizeof (DOCNO_INFO))))
	return (UNDEF);

    for (i = 0; i < num_results; i++) {
	docno_info[i].docno = text_results_info->text_results[i].docno;
	docno_info[i].sim = text_results_info->text_results[i].sim;
    }

    
    /* Sort results by sim, breaking ties lexicographically using docno */
    qsort ((char *) docno_info,
	   (int) num_results,
	   sizeof (DOCNO_INFO),
	   comp_sim_docno);

    

    /* Only look at epi->max_num_docs_per_topic (not normally an issue) */
    if (num_results > epi->max_num_docs_per_topic)
	num_results = epi->max_num_docs_per_topic;

    /* Add ranks to docno_info (starting at 1) */
    for (i = 0; i < num_results; i++) {
        docno_info[i].rank = i+1;
    }

    /* Sort trec_top lexicographically */
    qsort ((char *) docno_info,
           (int) num_results,
           sizeof (DOCNO_INFO),
           comp_docno);

    /* Error checking for duplicates */
    for (i = 1; i < num_results; i++) {
	if (0 == strcmp (docno_info[i].docno,
			 docno_info[i-1].docno)) {
	    fprintf (stderr, "trec_eval.form_res_qrels: duplicate docs %s",
		     docno_info[i].docno);
	    return (UNDEF);
	}
    }

 //    /* Find max_rel among qid, reserve and zero space for rel_levels */
 //    /* Check for duplicate docnos. */
    // qrels_ptr = trec_qrels->text_qrels;
    qrels_ptr = trec_qrels->text_qrels_three;
    end_qrels = &trec_qrels->text_qrels_three [trec_qrels->num_text_qrels];
   
    max_rel = qrels_ptr->rel;
    max_cred = qrels_ptr->cred;
    qrels_ptr++;
    int num_assessed_documents;
    num_assessed_documents = 1;
   while (qrels_ptr < end_qrels) {
        
        num_assessed_documents++;

	if (max_rel < qrels_ptr->rel)
	    max_rel = qrels_ptr->rel;
	if (max_cred < qrels_ptr->cred)
	    max_cred = qrels_ptr->cred;
    if (max_third < qrels_ptr->third)
        max_third = qrels_ptr->third;
	
	if (0 == strcmp ((qrels_ptr-1)->docno, qrels_ptr->docno)) {
	    fprintf (stderr, "trec_eval.form_res_rels: duplicate docs %s\n",
		     qrels_ptr->docno);
	    return (UNDEF);
	}
	qrels_ptr++;
    }



    if (NULL == (rel_levels =
		 te_chk_and_malloc (rel_levels, &max_rel_levels,
				    (max_rel+1),
				    sizeof (long))))
	return (UNDEF);
    (void) memset (rel_levels, 0, (max_rel+1) * sizeof (long));
        
    
    /* Check and reserve space for output structure */
    /* Reserve space for temp structure copying results */
    if (NULL == (docno_info_run = te_chk_and_malloc (docno_info_run, &max_docno_info_ideal,
				    num_results, sizeof (DOCNO_IDEAL_INFO))))
	return (UNDEF);
	
    max_docno_info_ideal = 0;
    if (NULL == (docno_info_ideal = te_chk_and_malloc (docno_info_ideal, &max_docno_info_ideal,
                    num_assessed_documents, sizeof (DOCNO_IDEAL_INFO))))
    return (UNDEF);
    
    
   

    /*Initializing the doc_info_ideal with the assessements values and sum.
    Attention to the sum. If you with to change the weights of each aspect, please change the line with the 
    docno_info_ideal[i].ideal_sum = qrels_ptr->cred + qrels_ptr->rel; using a parameter.
    */
    qrels_ptr = trec_qrels->text_qrels_three;
    end_qrels = &trec_qrels->text_qrels_three [trec_qrels->num_text_qrels];
    i=0;
    
    while (qrels_ptr < end_qrels) {
        docno_info_ideal[i].docno = qrels_ptr->docno;
        // printf("qrels_ptr->docno:%s, qrels_ptr->rel:%ld, qrels_ptr->cred:%ld\n", docno_info_ideal[i].docno,qrels_ptr->rel, qrels_ptr->cred);
        docno_info_ideal[i].rel = qrels_ptr->rel;
        docno_info_ideal[i].cred = qrels_ptr->cred;
        docno_info_ideal[i].third = qrels_ptr->third;
        docno_info_ideal[i].ideal_sum = qrels_ptr->cred + qrels_ptr->rel + qrels_ptr->third;
        qrels_ptr++;
        i++;
    }

	/* Point back to the beggining of the array */
	qrels_ptr = trec_qrels->text_qrels_three;
    end_qrels = &trec_qrels->text_qrels_three [trec_qrels->num_text_qrels];

	for (i = 0; i < num_results; i++) {
		docno_info_run[i].docno = text_results_info->text_results[i].docno;
		docno_info_run[i].sim = text_results_info->text_results[i].sim;
        // printf("docno:%s, sim: %lf\n", text_results_info->text_results[i].docno, text_results_info->text_results[i].sim);
		while (qrels_ptr < end_qrels &&
		       strcmp (qrels_ptr->docno, docno_info_run[i].docno) < 0) {
		    if (qrels_ptr->rel >= 0)
		    qrels_ptr++;
		}
		if (qrels_ptr >= end_qrels ||
		    strcmp (qrels_ptr->docno, docno_info_run[i].docno) > 0) {
		    /* Doc is non-judged */
		    docno_info_run[i].rel = 0;
			docno_info_run[i].cred = 0;
            docno_info_run[i].third = 0;
		}
		else {
		    /* Doc is in pool, assign relevance */
		    if (qrels_ptr->rel < 0){
			/* In pool, but unjudged (eg, infAP uses a sample of pool)*/
				docno_info_run[i].rel = 0;
		    } else {
				docno_info_run[i].rel = qrels_ptr->rel;
		    }
		    if (qrels_ptr->cred < 0){
			/* In pool, but unjudged (eg, infAP uses a sample of pool)*/
				docno_info_run[i].cred = 0;
		    }
		     else {
				docno_info_run[i].cred = qrels_ptr->cred;
		    }
            if (qrels_ptr->third < 0){
            /* In pool, but unjudged (eg, infAP uses a sample of pool)*/
                docno_info_run[i].third = 0;
            }
             else {
                docno_info_run[i].third = qrels_ptr->third;
            }
		    if (qrels_ptr->cred >= 0)
		    qrels_ptr++;
		}
    }
    
    
    /*Sorting the ideal ranking by the sum of assessments to get the Ideal ranking*/
    qsort ((char *) docno_info_ideal,
       (int) num_assessed_documents,
       sizeof (DOCNO_IDEAL_INFO),
       comp_sum_docno);


    /* Sort results by sim, breaking ties lexicographically using docno */
    qsort ((char *) docno_info_run,
	   (int) num_results,
	   sizeof (DOCNO_IDEAL_INFO),
	   comp_sim_docno);

    for (i = 0; i < num_results; i++) {
        if (i < num_assessed_documents){
            docno_info_run[i].ideal_sum = docno_info_ideal[i].ideal_sum;
        }
        else{
            docno_info_run[i].ideal_sum = 0;   
        }
        
    }

    *ideal = docno_info_run;


    return (1);
}


// int max(int num1, int num2)
// {
//     return (num1 > num2 ) ? num1 : num2;
// }

static int 
comp_sim_docno (ptr1, ptr2)
DOCNO_INFO *ptr1;
DOCNO_INFO *ptr2;
{
    if (ptr1->sim > ptr2->sim)
        return (-1);
    if (ptr1->sim < ptr2->sim)
        return (1);
    return (strcmp (ptr2->docno, ptr1->docno));
}

static int 
comp_docno (ptr1, ptr2)
DOCNO_INFO *ptr1;
DOCNO_INFO *ptr2;
{
    return (strcmp (ptr1->docno, ptr2->docno));
}


static int 
comp_rel_docno (ptr1, ptr2)
DOCNO_IDEAL_INFO *ptr1;
DOCNO_IDEAL_INFO *ptr2;
{
    if (ptr1->rel > ptr2->rel)
        return (-1);
    if (ptr1->rel < ptr2->rel)
        return (1);
    // return (strcmp (ptr2->docno, ptr1->docno));
    return (comp_sim_docno (ptr1, ptr2));
}

static int 
comp_cred_docno (ptr1, ptr2)
DOCNO_IDEAL_INFO *ptr1;
DOCNO_IDEAL_INFO *ptr2;
{
    if (ptr1->cred > ptr2->cred)
        return (-1);
    if (ptr1->cred < ptr2->cred)
        return (1);
    return (comp_sim_docno (ptr1, ptr2));
}


static int 
comp_sum_docno (ptr1, ptr2)
DOCNO_IDEAL_INFO *ptr1;
DOCNO_IDEAL_INFO *ptr2;
{
    if (ptr1->ideal_sum > ptr2->ideal_sum)
        return (-1);
    if (ptr1->ideal_sum < ptr2->ideal_sum)
        return (1);
    return (comp_docno (ptr1, ptr2));
}

int 
te_form_res_rels_cleanup_threeaspects ()
{
    if (max_current_query > 0) {
	Free (current_query);
	max_current_query = 0;
	current_query = "no_query";
    }
    if (max_rel_levels > 0) {
	Free (rel_levels);
	max_rel_levels = 0;
    }
    if (max_ranked_rel_list > 0) {
	Free (ranked_rel_list);
	max_ranked_rel_list = 0;
    }
    if (max_docno_info > 0) {
	Free (docno_info);
	max_docno_info = 0;
    }

    if (max_docno_info_ideal > 0) {
    Free (docno_info_ideal);
    max_docno_info_ideal = 0;
    }

    return (1);
}
