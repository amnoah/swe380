#!/usr/bin/env python3
"""Run AFTER two reviewers have labeled validation_review.csv (0/1).
Rows are disagreement-enriched; metrics for all rows are NOT population estimates.
Usage: python tools/validate_examples.py results/validation_review.csv
"""
from __future__ import annotations
import sys
import numpy as np
import pandas as pd
def cohen_kappa_score(a,b):
    import numpy as np
    a=np.asarray(a,dtype=int);b=np.asarray(b,dtype=int)
    agree=np.mean(a==b); p1=np.mean(a);p2=np.mean(b)
    chance=p1*p2+(1-p1)*(1-p2)
    return (agree-chance)/(1-chance) if chance!=1 else float('nan')
if len(sys.argv) != 2:
    raise SystemExit(__doc__)
df=pd.read_csv(sys.argv[1],dtype={'file_sha':str})
for col in ('reviewer_1_gold','reviewer_2_gold','adjudicated_gold'):
    df[col]=pd.to_numeric(df[col],errors='coerce')
for col in ('reviewer_1_gold','reviewer_2_gold','adjudicated_gold'):
    invalid=df[col].notna() & ~df[col].isin([0,1])
    if invalid.any():
        raise SystemExit(f'Invalid gold values in {col}; require 0 or 1')
paired=df[df.reviewer_1_gold.notna() & df.reviewer_2_gold.notna()]
print(f'Complete paired annotations: {len(paired)}/{len(df)}')
if len(paired):
    print(f'Human agreement: {(paired.reviewer_1_gold==paired.reviewer_2_gold).mean():.3f}')
    print(f'Human Cohen kappa: {cohen_kappa_score(paired.reviewer_1_gold,paired.reviewer_2_gold):.3f}')
    print(f'Human disagreements to adjudicate: {(paired.reviewer_1_gold != paired.reviewer_2_gold).sum()}')
valid=df[df.adjudicated_gold.notna()]
if valid.empty:
    raise SystemExit('No adjudicated labels yet. Have two reviewers label rows, then fill adjudicated_gold.')
print(f'Adjudicated cases: {len(valid)}/{len(df)}')
for stratum,sub in [('all_selected',valid),('random_remaining_population',valid[valid.selection_stratum.eq('random_remaining_population')]),('disagreement_enriched',valid[valid.selection_stratum.eq('disagreement_enriched')])]:
    if sub.empty: continue
    print('\n',stratum,'n=',len(sub),'(selection-biased: NOT population performance)')
    for name in ('detector_a','detector_b'):
        gold=sub.adjudicated_gold.astype(int).to_numpy(); pred=sub[name].astype(int).to_numpy()
        tp=int(np.sum((gold==1)&(pred==1)));fp=int(np.sum((gold==0)&(pred==1)))
        fn=int(np.sum((gold==1)&(pred==0)));tn=int(np.sum((gold==0)&(pred==0)))
        precision=tp/(tp+fp) if (tp+fp) else 0
        recall=tp/(tp+fn) if (tp+fn) else 0
        f1=2*precision*recall/(precision+recall) if (precision+recall) else 0
        print(f'{name}: TP {tp}, FP {fp}, FN {fn}, TN {tn}; precision {precision:.3f}, recall {recall:.3f}, F1 {f1:.3f}')
print('\nFor unbiased population-level accuracy, randomly sample from the full dataset and annotate it without enrichment.')
