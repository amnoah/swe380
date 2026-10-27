# SWE380 RQ2 — Revised skill quality analysis

**Data:** supplied July 2026 GitSkills sample; 29,786 file occurrences, **13,000 unique content hashes**. Results are observational, not causal.

## Measurement revisions

- **Separate prose and code tokens:** `prose_word_count` measures tokens outside fenced code; `code_word_count` measures tokens inside fenced code. Markdown code fences are not counted as words. `word_count` is their sum. Inline code and indented/unfenced snippets remain in the prose category (limitation).
- **500-line criterion:** `over_500_lines` tests **raw SKILL.md lines including front matter**, not body word count. **802/13,000 = 6.17%** exceed 500 raw lines. The threshold is a descriptive guideline, not a quality rule. Word-count tertiles at 367 and 909 are a **chosen sample-based grouping**, not intrinsically objective cutoffs.
- **Quality proxies:** two-or-more repositories with the same SHA; robustness test: two-or-more **distinct repository owners** using the owner part of `owner/repo`. This is an owner-name heuristic; forks/templates can still bias reuse. Maintenance = any **additional recorded commit after the first**, for the selected representative file when history is available (**3,010/13,000** only), NOT a direct edit count.
- **Extractor vs dataset `body_chars`:** exact UTF-8 codepoint body-length agreement **1,112/13,000 (8.55%)**; difference within ±2 characters **12,931/13,000 (99.47%)**. Difference is extractor minus dataset field. Most frequent differences: {-1: 11602, 0: 1112, -2: 217, -245: 2, -169: 2}. A likely systematic front-matter-boundary convention yields −1 for many rows: adding one codepoint for records marked valid frontmatter produces **12,174/13,000 (93.65%)** matches (diagnostic sensitivity check, not a correction). The exceptions need investigation. Do not call these direct full agreement.

## Baseline comparisons

**Examples vs no examples** (C++ detector, unvalidated):

| Group | Skills | Reused 2+ repos | Reused 2+ owners | History available | Any additional recorded commits (history subset) |
|---|---:|---:|---:|---:|---:|
| Detected examples | 2,288 | 28.06% | 26.31% | 445 | 34.16% |
| No detected examples | 10,712 | 20.65% | 16.70% | 2,565 | 32.36% |

**Word-count tertiles:**

| Group | Skills | Reused 2+ repos | Reused 2+ owners | History available | Any additional recorded commits (history subset) |
|---|---:|---:|---:|---:|---:|
| Long | 4,326 | 23.00% | 21.31% | 903 | 41.42% |
| Medium | 4,337 | 22.90% | 16.51% | 1,018 | 32.02% |
| Short | 4,337 | 19.97% | 17.36% | 1,089 | 25.90% |

**500-line threshold:**

| Group | Skills | Reused 2+ repos | Reused 2+ owners | History available | Any additional recorded commits (history subset) |
|---|---:|---:|---:|---:|---:|
| 500 lines or fewer | 12,198 | 21.35% | 17.63% | 2,857 | 32.38% |
| More than 500 lines | 802 | 31.17% | 29.93% | 153 | 37.25% |

## Compare examples within each length group

| Length group | N with examples | N without | Reused with examples | Reused without | Difference (points) | Unadjusted odds ratio (95% CI) |
|---|---:|---:|---:|---:|---:|---:|
| Long | 1,249 | 3,077 | 26.82% | 21.45% | +5.37 | 1.34 (1.15–1.56) |
| Medium | 637 | 3,700 | 27.79% | 22.05% | +5.73 | 1.36 (1.13–1.65) |
| Short | 402 | 3,935 | 32.34% | 18.70% | +13.63 | 2.08 (1.66–2.60) |

## Continuous and adjusted associations

- Spearman prose words vs repository count: **-0.027**.
- Spearman fenced-code words vs repository count: **+0.085**.
- **Adjusted prose length / reuse:** OR 0.910 (95% CI 0.876–0.947), p=2.51e-06, n=12,989.
- **Adjusted code length / reuse:** OR 1.072 (95% CI 1.053–1.092), p=1.33e-14, n=12,989.
- **Adjusted examples / reuse:** OR 1.469 (95% CI 1.319–1.636), p=2.74e-12, n=12,989.
- **Robustness, 2+ distinct owners (prose):** OR 0.862 (95% CI 0.827–0.900), p=6.75e-12, n=12,989.
- **Robustness, 2+ distinct owners (code):** OR 1.149 (95% CI 1.126–1.172), p=4.58e-43, n=12,989.
- **Robustness, 2+ distinct owners (examples):** OR 1.636 (95% CI 1.463–1.830), p=6.17e-18, n=12,989.
- **Maintenance, history subset (prose):** OR 1.394 (95% CI 1.287–1.509), p=2.34e-16, n=3,010.
- **Maintenance, history subset (code):** OR 0.993 (95% CI 0.959–1.029), p=0.709, n=3,010.
- **Maintenance, history subset (examples):** OR 0.998 (95% CI 0.788–1.264), p=0.985, n=3,010.

Regressions control for snapshot representative-repository stars and age, scripts, references, plus copy count in the maintenance model. For `log1p` variables, ORs describe a one-unit change in the natural logarithm of (1 + words); it is not the OR for one additional word. The regression CSV also supplies the approximate OR for doubling (1 + words).

## Detector disagreement and manual validation

- C++ detector A compared against **exploratory_broad_baseline_NOT_historical_detector**.
- Agreement: **51.84%**, Cohen kappa **0.185**; A-only **114**, B-only **6,147**. **Do not confuse these values with the externally reported 86%, κ=.57, 1,144/626 disagreements** unless the actual second-detector CSV was provided.
- `validation_review.csv` contains **120** entries: up to 40 disagreements of each direction, plus 40 additional uniformly random skill hashes. Two independent annotators should label usable worked examples (not generic code or merely mentioning examples), then adjudicate.
- The disagreement-enriched review set is not prevalence-representative; report sensitivity by disagreement category and results from the random sample separately. No gold labels exist yet, so **accuracy, precision, recall and F1 cannot be claimed**.

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
