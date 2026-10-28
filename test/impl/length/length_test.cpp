#include "length_test.h"
#include "evaluations/length/length_evaluator.h"
#include "evaluations/shared/markdown_utils.h"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace std;

void LengthTest::runAllTests() {

    cout << "=================================" << endl;
    cout << " LENGTH ANALYZER TESTS" << endl;
    cout << "=================================" << endl;

    cout << "\nTest 1: Basic processing" << endl;

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

    cout << "PASS" << endl;
    cout << "\nTest 2: Yaml Stripping" << endl;

    auto noFront = SkillLengthEvaluator::evaluate("---\nfoo\nbar\n");
    assert(noFront.lineCount == 3); // Missing closing YAML delimiter is NOT stripped.

    cout << "PASS" << endl;
    cout << "\nTest 3: UTF Handling" << endl;

    auto utf8 = SkillLengthEvaluator::evaluate("café नमस्ते\n");
    assert(utf8.characterCount < utf8.byteCount);
    assert(utf8.wordCount == 2);

    cout << "PASS" << endl;
    cout << "\nTest 4: Empty Skills" << endl;

    auto empty = SkillLengthEvaluator::evaluate("");
    assert(empty.wordCount == 0 && empty.lineCount == 0);

    cout << "PASS" << endl;
    cout << "\nTest 5: Addl Metadata" << endl;

    auto mix = SkillLengthEvaluator::evaluate("---\nname: x\n---\nDescribe this task.\n```py\nprint(123)\n```\nMore prose.\n");
    assert(mix.proseWordCount == 5);
    assert(mix.codeWordCount == 2);
    assert(mix.wordCount == 7);
    assert(mix.totalFileLineCount == 8);

    cout << "PASS" << endl;

    cout << "\n=================================" << endl;
    cout << " ALL TESTS COMPLETED SUCCESSFULLY" << endl;
    cout << "=================================" << endl;
}
