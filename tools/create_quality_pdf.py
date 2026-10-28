#!/usr/bin/env python3
"""Rebuild revised RQ2 PDF from current CSV analysis results."""
import json
from pathlib import Path
import pandas as pd
from reportlab.lib import colors
from reportlab.lib.styles import getSampleStyleSheet,ParagraphStyle
from reportlab.platypus import SimpleDocTemplate,Paragraph,Spacer,Table,TableStyle,PageBreak,Image,KeepTogether,HRFlowable
from reportlab.lib.enums import TA_LEFT
R=Path(__file__).resolve().parents[1]/'results'
s=json.loads((R/'summary.json').read_text())
g=pd.read_csv(R/'group_comparisons.csv');w=pd.read_csv(R/'examples_within_length_groups.csv');m=pd.read_csv(R/'regression_results.csv')
BLUE=colors.HexColor('#18314F'); TEAL=colors.HexColor('#146B74'); MUTED=colors.HexColor('#506079'); LIGHT=colors.HexColor('#E9F1F5')
styles=getSampleStyleSheet()
styles.add(ParagraphStyle(name='HeroCustom',fontName='Helvetica-Bold',fontSize=20.5,leading=25,textColor=BLUE,spaceAfter=13))
styles.add(ParagraphStyle(name='HeadCustom',fontName='Helvetica-Bold',fontSize=11.5,leading=15,textColor=TEAL,spaceBefore=11,spaceAfter=7))
styles.add(ParagraphStyle(name='BodyCustom',fontName='Helvetica',fontSize=8.9,leading=13,textColor=BLUE,spaceAfter=8))
styles.add(ParagraphStyle(name='SmCustom',fontName='Helvetica',fontSize=7.5,leading=10.5,textColor=MUTED,spaceAfter=6))
styles.add(ParagraphStyle(name='TabCustom',fontName='Helvetica',fontSize=8,leading=11.4,textColor=BLUE))
P=lambda t,kind='BodyCustom':Paragraph(t,styles[kind])
def T(rows,widths):
    t=Table([[P(str(v),'TabCustom') for v in r] for r in rows],colWidths=widths,repeatRows=1,hAlign='LEFT')
    t.setStyle(TableStyle([('BACKGROUND',(0,0),(-1,0),LIGHT),('ROWBACKGROUNDS',(0,1),(-1,-1),[colors.white,colors.HexColor('#F8FAFC')]),('VALIGN',(0,0),(-1,-1),'TOP'),('BOTTOMPADDING',(0,0),(-1,-1),7),('TOPPADDING',(0,0),(-1,-1),7),('LEFTPADDING',(0,0),(-1,-1),7),('RIGHTPADDING',(0,0),(-1,-1),7),('LINEBELOW',(0,0),(-1,0),1,TEAL)]))
    return t

def get(or_outcome,term):
    z=m[(m.outcome==or_outcome)&(m.term==term)].iloc[0]
    return f'{z.odds_ratio:.2f} ({z.or_95ci_low:.2f}–{z.or_95ci_high:.2f})'

story=[]
def add(x):story.append(x)
add(P('Skill quality indicators<br/>Methodology audit &amp; revised results','HeroCustom'))
add(P('SWE380 • Research Question 2 • GitSkills sample (July 2026 snapshot) • October 2026','SmCustom'))
add(HRFlowable(width='100%',thickness=1,color=TEAL,spaceAfter=14))
add(P('<b>Question.</b> Which measurable properties of agent skills, especially the presence of examples and skill length, are associated with cross-repository reuse or subsequent maintenance?'))
add(T([['13,000 distinct contents','29,786 occurrences','3,010 histories available'],['One row per content hash','Observed files and paths','Representative file histories only']],[176,176,176]))
add(P('Corrections to the previous report','HeadCustom'))
for a,b in [
('Code and prose separated','Measure `prose_word_count` and `code_word_count` independently (fenced code only). Total `word_count` is the sum; inline code still counts as prose.'),
('500-line threshold retained','Measure full raw SKILL.md lines, including front matter: '+str(s['n_over_500_lines'])+' / 13,000 ('+f"{s['share_over_500_lines_percent']:.2f}"+'%) exceed 500 lines (body-only: '+str(s['n_body_over_500_lines'])+'). Tertiles remain an explicitly chosen descriptive grouping.'),
('Two reuse outcomes','Primary: same SHA in two or more repositories. Robustness: same SHA in repositories from at least two different owner-name strings.'),
('Maintenance label corrected','“Any additional recorded commits (history-available representatives only)” replaces vague “Later commits.” The first commit is not treated as maintenance.'),
('Dataset body_chars checked',f"Exact matches {s['exact_body_chars_matches']:,}/13,000 ({100*s['exact_body_chars_matches']/13000:.1f}%); differences within ±2 codepoints {s['body_chars_within_two']:,}/13,000. Most differ by −1; {s['body_chars_boundary_adjusted_matches']:,} match after a front-matter-boundary adjustment (sensitivity test only). Exceptions remain."),
]:
    add(P(f'<b>{a}.</b> {b}'))
add(P('A length in words and a length in lines answer different questions. Neither is an established quality measure; both are evaluated as candidate indicators.','SmCustom'))
add(PageBreak())
add(P('Sample comparisons','HeroCustom'))
add(P('An exact copy in multiple repositories is a reuse proxy, not proof that an AI agent actually invoked the skill.'))
ex=g[g.group_type=='example_presence']
add(P('Examples and independent-owner robustness','HeadCustom'))
add(T([['Detected examples','Distinct skills','≥2 repositories','≥2 owners']]+[
 [z.group,f'{z.n_unique_skills:,}',f'{z.reused_2plus_repos_pct:.1f}%',f'{z.reused_2plus_owners_pct:.1f}%']
 for z in ex.itertuples()], [190,97,121,120]))
add(P('Length tertiles (combined body tokens; descriptive cutoffs 367 and 909)','HeadCustom'))
lg=g[g.group_type=='length_tertile'].set_index('group')
add(T([['Length category','Distinct skills','≥2 repositories','≥2 owners']]+[
 [q,f'{int(lg.loc[q,"n_unique_skills"]):,}',f'{lg.loc[q,"reused_2plus_repos_pct"]:.1f}%',f'{lg.loc[q,"reused_2plus_owners_pct"]:.1f}%'] for q in ['Short','Medium','Long']],[190,97,121,120]))
add(P('Examples within length groups','HeadCustom'))
w=w.set_index('length_tertile')
add(T([['Length','With examples','Without examples','Rate difference','Unadjusted OR (95% CI)']]+[
 [q,f'{w.loc[q,"reused_with_examples_pct"]:.1f}%',f'{w.loc[q,"reused_without_examples_pct"]:.1f}%',f'{w.loc[q,"risk_difference_points"]:+.1f} pp',f'{w.loc[q,"unadjusted_or"]:.2f} ({w.loc[q,"or_95_low"]:.2f}–{w.loc[q,"or_95_high"]:.2f})']
 for q in ['Short','Medium','Long']],[72,104,109,89,154]))
add(Spacer(1,8))
add(Image(str(R/'reuse_comparisons_revised.png'),width=502,height=184))
add(P('These comparisons are unadjusted associations from the sample, not causal effects.','SmCustom'))
add(PageBreak())
add(P('Adjusted models and checks','HeroCustom'))
add(P('Logistic regression includes log(1+prose words), log(1+fenced-code words), detected example presence, repository stars/age, scripts, and references. The maintenance model also adjusts for copy count.'))
add(P('Odds ratios (95% confidence intervals)','HeadCustom'))
add(T([['Predictor','2+ repositories','2+ different owners','Any later commit*']]+[
 ['Prose length (log1p)',get('reused','log1p_prose_word_count'),get('reused_owner','log1p_prose_word_count'),get('edited','log1p_prose_word_count')],
 ['Code length (log1p)',get('reused','log1p_code_word_count'),get('reused_owner','log1p_code_word_count'),get('edited','log1p_code_word_count')],
 ['Has examples',get('reused','has_examples'),get('reused_owner','has_examples'),get('edited','has_examples')]
],[150,128,128,122]))
add(P('* “Any later commit” means one or more additional recorded commits on the representative file in the <b>3,010 skills with available commit histories only</b>, not 13,000 skills. These commits are not necessarily edits to documentation.','SmCustom'))
add(P('<b>Interpretation:</b> After adjustment, prose and fenced-code length have different associations. Report the effect sizes rather than treating total words as a single predictor. For a log1p term, an odds ratio describes a one-unit increase in ln(1+tokens), <i>not</i> a single additional word.'))
add(P('Detector disagreement and validity','HeadCustom'))
c=s['detector_comparison']
add(P(f'The provided project includes one existing C++ example detector (A). The comparison used a newly defined broad-rule baseline (B), <b>not</b> the externally reported second detector. In this sample A-only={c["detector_a_only"]:,}, B-only={c["detector_b_only"]:,}; agreement={c["agreement_percent"]:.1f}%, κ={c["cohens_kappa"]:.2f}. These statistics must not be substituted for the separately reported 86%, κ=0.57 and 1,144/626 disagreements.'))
add(P('The 120-row manual-review sheet prioritizes up to 40 disagreements in each direction, then selects 40 additional random cases. It includes independent reviewer fields and an adjudicated gold-label field; <b>all gold labels are blank</b>. No accuracy/precision/recall claim is justified yet.'))
add(P('Other limitations','HeadCustom'))
add(P('Different repository owners are an imperfect independence proxy; skill templates and forks may inflate reuse. Fenced-code parsing ignores inline and indented code. Dataset body length convention differs systematically by one codepoint for many files; nontrivial mismatches require inspection. History availability is nonrandom, and current documentation cannot establish whether examples existed before copying.'))
add(P('Reproduce and cite','HeadCustom'))
add(P('Build quality_export with CMake, run against agent_skills_sample.db, then run tools/analyze_quality.py. Supply --comparison-csv with file_sha,has_examples to reproduce a real second-detector comparison. Annotate validation_review.csv before evaluating performance.','SmCustom'))
add(P('Dataset: https://huggingface.co/datasets/mvaccargiu/gitskills ; paper: https://arxiv.org/pdf/2608.10906','SmCustom'))

def footer(c,doc):
    c.saveState();c.setFont('Helvetica',7.5);c.setFillColor(MUTED);c.drawString(40,25,'SWE380 • RQ2 • Revised methods • GitSkills sample');c.drawRightString(570,25,str(doc.page));c.restoreState()
pdf=SimpleDocTemplate(str(R/'Skill_Quality_Research_Report.pdf'),pagesize=(612,792),leftMargin=40,rightMargin=40,topMargin=44,bottomMargin=43,title='Skill Quality Indicators — Revised Methodology & Results')
pdf.build(story,onFirstPage=footer,onLaterPages=footer)
print(R/'Skill_Quality_Research_Report.pdf')
