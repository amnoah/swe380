#!/usr/bin/env python3
"""Reproducible GitSkills RQ2 analysis, revised after methodological audit.

python tools/analyze_quality.py results/skill_quality_features.csv --db agent_skills_sample.db --outdir results
Optional actual second detector: --comparison-csv path/to/other_detector.csv (file_sha,has_examples)
Default second detector is a separately specified exploratory broad-rule baseline, NOT a historical reviewer detector.
"""
from __future__ import annotations
import argparse, json, math, re, sqlite3
from pathlib import Path
import numpy as np, pandas as pd
from scipy import stats
import statsmodels.api as sm
from statsmodels.stats.proportion import proportion_confint
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SNAPSHOT = pd.Timestamp('2026-07-31T23:59:59Z')

def broad_example_baseline(content: str) -> int:
    """Intentionally distinct heuristic: any Examples/Usage heading, an inline demonstration cue,
    or a fenced programming/CLI code block. Does NOT equate to another team's rule set.
    """
    text = content.replace('\r\n','\n')
    return int(bool(
        re.search(r'(?im)^\s{0,3}#{1,6}\s*(?:.*\bexamples?\b|usage|quick\s*start)\s*(?:\([^)]*\))?\s*$', text)
        or re.search(r'(?im)\b(?:for example|for instance|e\.g\.|example\s*[:：]|examples\s*[:：]|sample\s+(?:input|output))', text)
        or re.search(r'(?im)^\s{0,3}(?:`{3,}|~{3,})\s*(?:bash|sh|shell|python|py|javascript|js|typescript|ts|json|sql|powershell|yaml|yml)\b', text)
    ))

def analyze_detector_comparison(df, db, out, comparison_csv=None):
    d = df[['file_sha','has_examples','representative_repo','representative_path']].copy()
    contexts = {}
    if comparison_csv:
        other = pd.read_csv(comparison_csv, dtype={'file_sha':str})
        if not {'file_sha','has_examples'}.issubset(other.columns):
            raise ValueError('Comparison CSV needs file_sha,has_examples')
        if other.file_sha.duplicated().any():
            raise ValueError('Duplicate file_sha in comparison detector file')
        other = other.rename(columns={'has_examples':'detector_b'})[['file_sha','detector_b']]
        d = d.merge(other,on='file_sha',validate='one_to_one',how='left')
        if d.detector_b.isna().any():
            raise ValueError('Second detector missing skill hashes; require all 13000')
        name = 'external_detector_from_comparison_csv'
    elif db:
        uri = f'file:{db.resolve()}?mode=ro'
        with sqlite3.connect(uri,uri=True) as con:
            matches = con.execute('SELECT file_sha,content FROM artifacts WHERE dedup_primary=1 AND content IS NOT NULL').fetchall()
        flags = {h:broad_example_baseline(s) for h,s in matches}
        d['detector_b'] = d.file_sha.map(flags)
        name = 'exploratory_broad_baseline_NOT_historical_detector'
    else:
        return {}, 0
    d['detector_a'] = d.has_examples.astype(int)
    d['detector_b'] = d.detector_b.astype(int)
    d['comparison_group'] = np.select([
        d.detector_a.eq(1)&d.detector_b.eq(0),
        d.detector_a.eq(0)&d.detector_b.eq(1),
        d.detector_a.eq(1)&d.detector_b.eq(1)],
        ['A_only','B_only','both_positive'],default='both_negative')
    save = d[['file_sha','detector_a','detector_b','comparison_group','representative_repo','representative_path']]
    save.to_csv(out/'detector_comparison.csv',index=False)
    agree = (d.detector_a == d.detector_b).mean()
    p1 = d.detector_a.mean(); p2 = d.detector_b.mean()
    expected = p1*p2+(1-p1)*(1-p2)
    kappa = (agree-expected)/(1-expected) if expected != 1 else np.nan
    # Enrich disputes, plus honest random sample of remaining population.
    selected=[]
    for group, cap, seed in [('A_only',40,380),('B_only',40,381)]:
        g=d[d.comparison_group.eq(group)]
        selected.extend(g.sample(n=min(cap,len(g)),random_state=seed).file_sha.tolist())
    remaining=d[~d.file_sha.isin(selected)]
    selected.extend(remaining.sample(n=min(40,len(remaining)),random_state=382).file_sha.tolist())
    review=d[d.file_sha.isin(selected)].copy()
    review['selection_stratum'] = np.where(review.comparison_group.isin(['A_only','B_only']),
                                            'disagreement_enriched','random_remaining_population')
    # Determine actual selection group by hash to avoid incorrectly classifying random disagreements.
    disagreement_selected=set(selected[:-min(40,len(remaining))])
    review['selection_stratum'] = review.file_sha.map(lambda h: 'disagreement_enriched' if h in disagreement_selected else 'random_remaining_population')
    if db:
        with sqlite3.connect(f'file:{db.resolve()}?mode=ro',uri=True) as con:
            for h in review.file_sha:
                record=con.execute('SELECT content FROM artifacts WHERE file_sha=? AND dedup_primary=1 LIMIT 1',(h,)).fetchone()
                txt=record[0] if record else ''
                cue=re.search(r'(?im)example|sample|usage|quick start|^\s*```|^\s*~~~',txt)
                start=max(0,cue.start()-110) if cue else 0
                contexts[h]=' '.join(txt[start:start+470].split())[:440]
    review['context_preview'] = review.file_sha.map(contexts)
    for col in ['reviewer_1_gold','reviewer_2_gold','adjudicated_gold','error_notes']:
        review[col]=''
    review[['file_sha','representative_repo','representative_path','comparison_group',
            'selection_stratum','detector_a','detector_b','context_preview','reviewer_1_gold',
            'reviewer_2_gold','adjudicated_gold','error_notes']].sort_values(['selection_stratum','comparison_group','file_sha']).to_csv(out/'validation_review.csv',index=False)
    return {'comparison_type':name,'agreement_percent':float(100*agree),'cohens_kappa':float(kappa),
            'detector_a_only':int((d.comparison_group=='A_only').sum()),
            'detector_b_only':int((d.comparison_group=='B_only').sum()),
            'both_positive':int((d.comparison_group=='both_positive').sum()),
            'both_negative':int((d.comparison_group=='both_negative').sum()),
            'review_n':len(review)}, len(review)

def make_groups(df):
    rows=[]
    for typ, col in [('example_presence','example_presence'),('length_tertile','length_tertile'),
                     ('over_500_lines','line_guideline'),('within_length','length_by_example')]:
        for g,sub in df.groupby(col,observed=True):
            h=sub[sub['edited'].notna()]
            n=len(sub); k=int(sub.reused.sum()); k2=int(sub.reused_owner.sum())
            ci=proportion_confint(k,n,method='wilson')
            rows.append(dict(group_type=typ,group=str(g),n_unique_skills=n,
                reused_2plus_repos_n=k,reused_2plus_repos_pct=100*k/n,
                reused_2plus_owners_n=k2,reused_2plus_owners_pct=100*k2/n,
                reused_wilson_low_pct=100*ci[0],reused_wilson_high_pct=100*ci[1],
                history_available_n=len(h),history_with_additional_commits_n=int(h.edited.sum()),
                history_with_additional_commits_pct=float(100*h.edited.mean()) if len(h) else np.nan,
                median_prose_words=float(sub.prose_word_count.median()),
                median_code_words=float(sub.code_word_count.median())))
    return pd.DataFrame(rows)

def within_length(df):
    rows=[]
    for group,sub in df.groupby('length_tertile',observed=True):
        yes=sub[sub.has_examples.eq(1)]; no=sub[sub.has_examples.eq(0)]
        a=int(yes.reused.sum()); b=len(yes)-a; c=int(no.reused.sum());d=len(no)-c
        lor=math.log((a+.5)*(d+.5)/((b+.5)*(c+.5)))
        se=math.sqrt(sum(1/(v+.5) for v in (a,b,c,d)))
        rows.append(dict(length_tertile=group,example_skills=len(yes),no_example_skills=len(no),
                         reused_with_examples_n=a,reused_without_examples_n=c,
                         reused_with_examples_pct=100*a/len(yes) if len(yes) else np.nan,
                         reused_without_examples_pct=100*c/len(no) if len(no) else np.nan,
                         risk_difference_points=100*(a/len(yes)-c/len(no)) if len(yes)*len(no)>0 else np.nan,
                         unadjusted_or=math.exp(lor),or_95_low=math.exp(lor-1.96*se),
                         or_95_high=math.exp(lor+1.96*se)))
    return pd.DataFrame(rows)

def correlations(df):
    rows=[]
    for y,sub in [('repository_count',df),('distinct_owner_count',df),('subsequent_commits',df[df.edited.notna()])]:
        for x in ['word_count','prose_word_count','code_word_count','code_line_count','line_count','example_count']:
            s=sub[[x,y]].dropna()
            z=stats.spearmanr(s[x],s[y])
            rows.append(dict(feature=x,outcome=y,n=len(s),spearman_rho=z.statistic,p_value=z.pvalue))
    return pd.DataFrame(rows)

def regressions(df):
    d=df.copy()
    for x in ['prose_word_count','code_word_count','repo_stars','repository_count','word_count']:
        d['log1p_'+x]=np.log1p(d[x].clip(lower=0))
    created=pd.to_datetime(d.repo_created_at,errors='coerce',utc=True)
    d['repo_age_years']=((SNAPSHOT-created).dt.total_seconds()/(86400*365.25)).clip(lower=0)
    base=['log1p_prose_word_count','log1p_code_word_count','has_examples',
          'log1p_repo_stars','repo_age_years','has_scripts','has_references']
    entries=[]
    for outcome,sub,cols in [('reused',d,base),('reused_owner',d,base),
                             ('edited',d[d.edited.notna()],base+['log1p_repository_count'])]:
        tmp=sub[[outcome]+cols].dropna()
        X=sm.add_constant(tmp[cols].astype(float),has_constant='add')
        Y=tmp[outcome].astype(float)
        model=sm.Logit(Y,X).fit(disp=False,maxiter=250)
        ci=model.conf_int()
        for x,beta in model.params.items():
            entries.append(dict(outcome=outcome,term=x,n=len(tmp),odds_ratio=math.exp(beta),
                                or_95ci_low=math.exp(ci.loc[x,0]),or_95ci_high=math.exp(ci.loc[x,1]),
                                p_value=model.pvalues[x],or_per_doubling=math.exp(beta*math.log(2)) if x.startswith('log1p_') else np.nan))
    return pd.DataFrame(entries)

def chart(df, out):
    fig,axes=plt.subplots(1,2,figsize=(12,4.4))
    sub=df.groupby('has_examples',observed=True).reused.mean().mul(100)
    axes[0].bar(['No examples','Examples'],[sub.get(0,0),sub.get(1,0)])
    axes[0].set_title('Reuse by C++ example detector')
    axes[0].set_ylabel('Content in 2+ repositories (%)')
    groups=df.groupby('length_tertile',observed=True).reused.mean().mul(100)
    axes[1].bar(['Short','Medium','Long'],[groups.get(k,0) for k in ['Short','Medium','Long']])
    axes[1].set_title('Reuse by body word-count tertiles')
    axes[1].set_ylabel('Content in 2+ repositories (%)')
    for ax in axes:
        ax.set_ylim(0,35)
    fig.tight_layout()
    fig.savefig(out/'reuse_comparisons_revised.png',dpi=160)
    plt.close(fig)

def build_report(df,gr,within,cor,reg,comp,out):
    n=len(df); hist=df[df.edited.notna()]
    ci_good=df.body_chars_match.eq(1).sum()
    small=df.body_chars_difference.abs().le(2).sum()
    boundary_adjusted_match=(df.body_chars_difference + df.frontmatter_valid.fillna(0)).eq(0).sum()
    pairs=df.body_chars_difference.value_counts().head(5).to_dict()
    or_line=lambda outcome,term: render_or(reg,outcome,term)
    length_cutoff=df.word_count.quantile([1/3,2/3]).tolist()
    with_e=df[df.has_examples.eq(1)]; no_e=df[df.has_examples.eq(0)]
    def group_tbl(t):
        s=['| Group | Skills | Reused 2+ repos | Reused 2+ owners | History available | Any additional recorded commits (history subset) |',
           '|---|---:|---:|---:|---:|---:|']
        for r in t.itertuples():
            s.append(f'| {r.group} | {r.n_unique_skills:,} | {r.reused_2plus_repos_pct:.2f}% | {r.reused_2plus_owners_pct:.2f}% | {r.history_available_n:,} | {r.history_with_additional_commits_pct:.2f}% |')
        return '\n'.join(s)
    cc=comp.get('comparison_type','not available')
    text=f'''# SWE380 RQ2 — Revised skill quality analysis

**Data:** supplied July 2026 GitSkills sample; 29,786 file occurrences, **{n:,} unique content hashes**. Results are observational, not causal.

## Measurement revisions

- **Separate prose and code tokens:** `prose_word_count` measures tokens outside fenced code; `code_word_count` measures tokens inside fenced code. Markdown code fences are not counted as words. `word_count` is their sum. Inline code and indented/unfenced snippets remain in the prose category (limitation).
- **500-line criterion:** `over_500_lines` tests **raw SKILL.md lines including front matter**, not body word count. **{int(df.over_500_lines.sum()):,}/{n:,} = {100*df.over_500_lines.mean():.2f}%** exceed 500 raw lines. The threshold is a descriptive guideline, not a quality rule. Word-count tertiles at {length_cutoff[0]:.0f} and {length_cutoff[1]:.0f} are a **chosen sample-based grouping**, not intrinsically objective cutoffs.
- **Quality proxies:** two-or-more repositories with the same SHA; robustness test: two-or-more **distinct repository owners** using the owner part of `owner/repo`. This is an owner-name heuristic; forks/templates can still bias reuse. Maintenance = any **additional recorded commit after the first**, for the selected representative file when history is available (**{len(hist):,}/{n:,}** only), NOT a direct edit count.
- **Extractor vs dataset `body_chars`:** exact UTF-8 codepoint body-length agreement **{ci_good:,}/{n:,} ({100*ci_good/n:.2f}%)**; difference within ±2 characters **{small:,}/{n:,} ({100*small/n:.2f}%)**. Difference is extractor minus dataset field. Most frequent differences: {pairs}. A likely systematic front-matter-boundary convention yields −1 for many rows: adding one codepoint for records marked valid frontmatter produces **{boundary_adjusted_match:,}/{n:,} ({100*boundary_adjusted_match/n:.2f}%)** matches (diagnostic sensitivity check, not a correction). The exceptions need investigation. Do not call these direct full agreement.

## Baseline comparisons

**Examples vs no examples** (C++ detector, unvalidated):

{group_tbl(gr[gr.group_type.eq('example_presence')])}

**Word-count tertiles:**

{group_tbl(gr[gr.group_type.eq('length_tertile')])}

**500-line threshold:**

{group_tbl(gr[gr.group_type.eq('over_500_lines')])}

## Compare examples within each length group

| Length group | N with examples | N without | Reused with examples | Reused without | Difference (points) | Unadjusted odds ratio (95% CI) |
|---|---:|---:|---:|---:|---:|---:|
'''
    for r in within.itertuples():
        text+=f'| {r.length_tertile} | {r.example_skills:,} | {r.no_example_skills:,} | {r.reused_with_examples_pct:.2f}% | {r.reused_without_examples_pct:.2f}% | {r.risk_difference_points:+.2f} | {r.unadjusted_or:.2f} ({r.or_95_low:.2f}–{r.or_95_high:.2f}) |\n'
    text+=f'''
## Continuous and adjusted associations

- Spearman prose words vs repository count: **{cor.loc[(cor.feature=='prose_word_count')&(cor.outcome=='repository_count'),'spearman_rho'].iloc[0]:+.3f}**.
- Spearman fenced-code words vs repository count: **{cor.loc[(cor.feature=='code_word_count')&(cor.outcome=='repository_count'),'spearman_rho'].iloc[0]:+.3f}**.
- **Adjusted prose length / reuse:** {or_line('reused','log1p_prose_word_count')}.
- **Adjusted code length / reuse:** {or_line('reused','log1p_code_word_count')}.
- **Adjusted examples / reuse:** {or_line('reused','has_examples')}.
- **Robustness, 2+ distinct owners (prose):** {or_line('reused_owner','log1p_prose_word_count')}.
- **Robustness, 2+ distinct owners (code):** {or_line('reused_owner','log1p_code_word_count')}.
- **Robustness, 2+ distinct owners (examples):** {or_line('reused_owner','has_examples')}.
- **Maintenance, history subset (prose):** {or_line('edited','log1p_prose_word_count')}.
- **Maintenance, history subset (code):** {or_line('edited','log1p_code_word_count')}.
- **Maintenance, history subset (examples):** {or_line('edited','has_examples')}.

Regressions control for snapshot representative-repository stars and age, scripts, references, plus copy count in the maintenance model. For `log1p` variables, ORs describe a one-unit change in the natural logarithm of (1 + words); it is not the OR for one additional word. The regression CSV also supplies the approximate OR for doubling (1 + words).

## Detector disagreement and manual validation

- C++ detector A compared against **{cc}**.
'''
    if comp:
        text+=f'- Agreement: **{comp["agreement_percent"]:.2f}%**, Cohen kappa **{comp["cohens_kappa"]:.3f}**; A-only **{comp["detector_a_only"]:,}**, B-only **{comp["detector_b_only"]:,}**. **Do not confuse these values with the externally reported 86%, κ=.57, 1,144/626 disagreements** unless the actual second-detector CSV was provided.\n'
        text+=f'- `validation_review.csv` contains **{comp["review_n"]}** entries: up to 40 disagreements of each direction, plus 40 additional uniformly random skill hashes. Two independent annotators should label usable worked examples (not generic code or merely mentioning examples), then adjudicate.\n'
    else:
        text+='- No second detector or database supplied; disagreement review is not generated.\n'
    text+='''- The disagreement-enriched review set is not prevalence-representative; report sensitivity by disagreement category and results from the random sample separately. No gold labels exist yet, so **accuracy, precision, recall and F1 cannot be claimed**.

## Validity considerations

- Some example sections are headings only; unlabeled scenarios may be missed. The independent broad heuristic is intentionally higher-recall but can classify ordinary setup code as an example.
- Tokenization measures word-like tokens, not linguistic words; the fenced-code split omits inline and indented code. Body characters are measured as Unicode code points, while some dataset metadata follows another trimming convention.
- File-hash reuse measures byte-identical copies, not agent use. Owner-name distinction is approximate; organizations may control multiple owners or change names. The assignment's 'reuse' outcome is a proxy, not validation of skill quality.
- Commits are not guaranteed text edits/improvements; only primary representatives' histories are joined, and history is available for a minority. Current content features and snapshot metadata do not establish the direction of influence.
- Sample-only results cannot be generalized without testing the full dataset. Odds ratios are associations with potentially unmeasured confounding.

## Reproducibility and sources

Run `quality_export` against the supplied SQLite DB, then `python tools/analyze_quality.py results/skill_quality_features.csv --db agent_skills_sample.db --outdir results`. Supply `--comparison-csv` containing `file_sha,has_examples` for the team's actual second detector. Validate using `python tools/validate_examples.py results/validation_review.csv`. Code and results are included in the ZIP.

GitSkills: https://huggingface.co/datasets/mvaccargiu/gitskills  
Paper: https://arxiv.org/pdf/2608.10906
'''
    (out/'research_report.md').write_text(text,encoding='utf-8')
    return {'n_skills':n,'n_reused_repos':int(df.reused.sum()),'n_reused_owners':int(df.reused_owner.sum()),
            'n_over_500_lines':int(df.over_500_lines.sum()),'share_over_500_lines_percent':float(100*df.over_500_lines.mean()),
            'exact_body_chars_matches':int(ci_good),'body_chars_within_two':int(small),
            'body_chars_boundary_adjusted_matches':int(boundary_adjusted_match),
            'n_body_over_500_lines':int((df.line_count>500).sum()),
            'history_n':len(hist),'examples_n':int(df.has_examples.sum()),
            'examples_reused_pct':float(100*with_e.reused.mean()),
            'nonexamples_reused_pct':float(100*no_e.reused.mean()),'detector_comparison':comp}

def render_or(reg,outcome,term):
    d=reg[(reg.outcome==outcome)&(reg.term==term)]
    if d.empty:return 'not estimable'
    r=d.iloc[0]
    return f'OR {r.odds_ratio:.3f} (95% CI {r.or_95ci_low:.3f}–{r.or_95ci_high:.3f}), p={r.p_value:.3g}, n={int(r.n):,}'

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('csv',type=Path)
    p.add_argument('--outdir',type=Path,default=Path('results'))
    p.add_argument('--db',type=Path)
    p.add_argument('--comparison-csv',type=Path,help='actual other detector flags: file_sha,has_examples')
    a=p.parse_args()
    a.outdir.mkdir(parents=True,exist_ok=True)
    df=pd.read_csv(a.csv,dtype={'file_sha':str})
    assert len(df.file_sha.unique())==len(df),'Expected one record per content hash'
    assert (df.word_count==df.prose_word_count+df.code_word_count).all(), 'length split inconsistent'
    df['reused']=df.repository_count.ge(2).astype(int)
    df['reused_owner']=df.distinct_owner_count.ge(2).astype(int)
    df['edited']=np.where(df.history_fetched.eq(1) & df.commit_count.notna(),
                          df.subsequent_commits.gt(0).astype(float),np.nan)
    t1,t2=df.word_count.quantile([1/3,2/3]).to_list()
    df['length_tertile']=pd.cut(df.word_count,[-np.inf,t1,t2,np.inf],labels=['Short','Medium','Long']).astype(str)
    df['example_presence']=df.has_examples.map({0:'No detected examples',1:'Detected examples'})
    df['line_guideline']=df.over_500_lines.map({0:'500 lines or fewer',1:'More than 500 lines'})
    df['length_by_example']=df.length_tertile+' / '+df.example_presence
    gr=make_groups(df); gr.to_csv(a.outdir/'group_comparisons.csv',index=False)
    within=within_length(df); within.to_csv(a.outdir/'examples_within_length_groups.csv',index=False)
    corr=correlations(df);corr.to_csv(a.outdir/'correlations.csv',index=False)
    reg=regressions(df);reg.to_csv(a.outdir/'regression_results.csv',index=False)
    compare,nreview=analyze_detector_comparison(df,a.db,a.outdir,a.comparison_csv)
    chart(df,a.outdir)
    summary=build_report(df,gr,within,corr,reg,compare,a.outdir)
    (a.outdir/'summary.json').write_text(json.dumps(summary,indent=2))
    print(json.dumps(summary,indent=2))

if __name__=='__main__':main()
