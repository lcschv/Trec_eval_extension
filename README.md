# Trec_eval.9.0_extension

To list the measures available for multi-aspect evaluation:
```
./trec_eval -h -m twoaspects
./trec_eval -h -m threeaspects
```
which will show that the added measures are:
1. nlre : NLRE using NDCG on two aspects
1. cam : CAM using NDCG on two aspects
1. cam_map : CAM using MAP on two aspects
1. nlre_three : NLRE using NDCG on three aspects
1. cam_three_ndcg : CAM using NDCG on three aspects
1. cam_map_three : CAM using MAP on three aspects
1. nwcs : NWCS for two aspects
1. nwcs_three : NWCS for three aspects

## Example usage

Be sure to specify `-c` to compute averages over all topics and not only the topics in the results file.

Be sure to specify `-M 1000` to limit results to 1000 documents per topic.

For two aspects:
```
./trec_eval -c -M 1000 -m nlre -R qrels_twoaspects samples/sanitytests/qrels_sample_twoaspects.txt samples/sanitytests/run_random.txt
 ```
For three aspects:
```
./trec_eval -c -M 1000 -m nlre_three -R qrels_threeaspects samples/sanitytests/qrels_sample.txt samples/sanitytests/run_perfect.txt
```