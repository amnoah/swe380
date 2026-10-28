# SWE380

Welcome to our class project for SWE 380! This read me will be updated soon with more information.

# Setup Note

To get this repository working, you will need to add the "agent_skills_sample.db" file from the link below into the root
folder of the repository:

https://github.com/giuseppedestefanis/gitskills-sample/blob/main/agent_skills_sample.zip


## Research Question 2: Skill Quality Indicators

This research examines whether skill length and
the presence of examples are associated with
skill reuse and subsequent maintenance.

### Skill Length Evaluation
- Separate prose and code word counts
- Character and line counts
- Analysis of the 500-line guideline

### Examples Evaluation
- Detects examples in SKILL.md files
- Measures example presence, count, and density
- Compares two rule-based detection approaches

### Dataset
Analyzed 13,000 unique skills from the GitSkills
sample database.

### Build and Test
Run in Visual Studio Developer PowerShell:

    cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug
    cmake --build build
    ctest --test-dir build --output-on-failure

### Run Feature Extraction

    .\build\quality_export.exe .\agent_skills_sample.db .\results\skill_quality_features.csv

### Statistical Analysis

    python -m pip install -r requirements-quality.txt
    python tools/analyze_quality.py results/skill_quality_features.csv --db agent_skills_sample.db --outdir results/rq2

### Current Status
- Feature extraction completed
- 13,000 skills analyzed
- Automated C++ tests passed locally
- Statistical analysis completed
- Manual example validation remains pending

Results are observational and do not establish
causation. Detector accuracy requires human validation.
