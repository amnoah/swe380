#include "evaluations/examples/example_evaluator.h"
#include "evaluations/length/length_evaluator.h"
#include "evaluations/shared/markdown_utils.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    const std::string sample = "---\nname: testing\ndescription: ignored\n---\n# Testing\nRun pytest.\n\n## Examples\n```bash\npytest tests/\n```\n";
    auto a = SkillLengthEvaluator::evaluate(sample);
    assert(a.lineCount == 7);
    assert(a.totalFileLineCount == 11);
    assert(a.wordCount == a.proseWordCount + a.codeWordCount);
    assert(a.codeWordCount == 2);
    assert(a.proseWordCount > 0);
    assert(a.codeLineCount == 1);
    assert(a.wordCount > 0);
    assert(a.characterCount >= a.wordCount);
    auto examples = SkillExampleEvaluator::evaluate(sample, a.wordCount);
    assert(examples.hasExamples);
    assert(examples.exampleCount == 1);
    assert(examples.exampleCodeBlocks == 1);
    assert(examples.exampleSections == 1);
    assert(examples.examplesPer1000Words > 0);

    auto plain = SkillExampleEvaluator::evaluate("# Simple\nRun pytest to test code.\n```bash\npytest tests/\n```\n", 10);
    assert(!plain.hasExamples); // A code block alone is not necessarily an example.
    assert(plain.exampleCodeBlocks == 0);
    auto adjacent = SkillExampleEvaluator::evaluate("Use the following, for example:\n~~~sh\nls\n~~~\n", 10);
    assert(adjacent.exampleCount == 1);
    auto inlineSample = SkillExampleEvaluator::evaluate("Example: `pytest tests/`\n", 3);
    assert(inlineSample.exampleCount == 1);
    auto noFront = SkillLengthEvaluator::evaluate("---\nfoo\nbar\n");
    assert(noFront.lineCount == 3); // Missing closing YAML delimiter is NOT stripped.
    auto utf8 = SkillLengthEvaluator::evaluate("café नमस्ते\n");
    assert(utf8.characterCount < utf8.byteCount);
    assert(utf8.wordCount == 2);
    auto negativeHeader = SkillExampleEvaluator::evaluate("# Sample Size Estimation\nThere are no demonstrations.\n", 7);
    assert(negativeHeader.exampleCount == 0);
    auto negativeCode = SkillExampleEvaluator::evaluate("sample_rate: 500\n", 2);
    assert(negativeCode.exampleCount == 0);
    auto bold = SkillExampleEvaluator::evaluate("**Example:** `ls`\n", 2);
    assert(bold.exampleCount == 1);
    auto empty = SkillLengthEvaluator::evaluate("");
    assert(empty.wordCount == 0 && empty.lineCount == 0);
    std::cout << "All quality feature tests passed.\n";
    auto mix = SkillLengthEvaluator::evaluate("---\nname: x\n---\nDescribe this task.\n```py\nprint(123)\n```\nMore prose.\n");
    assert(mix.proseWordCount == 5);
    assert(mix.codeWordCount == 2);
    assert(mix.wordCount == 7);
    assert(mix.totalFileLineCount == 8);
    return 0;
}
