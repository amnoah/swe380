#include "completeness_test.h"

#include <iostream>
#include <cassert>
#include <cmath>
#include <string>

#include "evaluations/completeness/completeness_evaluator.h"

using namespace std;

// Test content with valid frontmatter and sufficient body text.
const string completeSkill =
    "---\n"
    "name: Text Analysis Skill\n"
    "description: Instructions for analyzing and processing text.\n"
    "---\n"
    "# Overview\n"
    "This skill provides instructions for analyzing text and "
    "processing information efficiently. It explains how to "
    "identify important information and organize the results. "
    "Users can follow these instructions to complete their tasks. "
    "The process includes reviewing documents and producing "
    "accurate and consistent results. Every step is described "
    "clearly to ensure that users understand the expected outcome. "
    "The final result should be reviewed before completion.";


// Test 1: Empty skill
void CompletenessTest::testEmptySkill()
{
    cout << "\nTest 1: Empty Skill" << endl;

    double score = evaluateSkillCompleteness("");

    assert(score == 0.0);

    cout << "Completeness score: " << score << endl;
    cout << "PASS" << endl;
}


// Test 2: Complete skill
void CompletenessTest::testCompleteSkill()
{
    cout << "\nTest 2: Complete Skill" << endl;

    double score = evaluateSkillCompleteness(completeSkill);

    assert(score > 0.80);
    assert(score <= 1.0);

    cout << "Completeness score: " << score << endl;
    cout << "PASS" << endl;
}


// Test 3: Score range
void CompletenessTest::testScoreRange()
{
    cout << "\nTest 3: Score Range" << endl;

    string skill =
        "# Introduction\n"
        "This skill explains how to process documents and "
        "organize information for users.";

    double score = evaluateSkillCompleteness(skill);

    assert(score >= 0.0);
    assert(score <= 1.0);

    cout << "Completeness score: " << score << endl;
    cout << "PASS" << endl;
}


// Test 4: Frontmatter detection
void CompletenessTest::testFrontmatterDetection()
{
    cout << "\nTest 4: Frontmatter Detection" << endl;

    string skill =
        "---\n"
        "name: Example Skill\n"
        "description: An example skill for testing.\n"
        "---\n"
        "# Introduction\n"
        "This skill provides instructions for analyzing text "
        "and processing information efficiently.";

    double score = evaluateSkillCompleteness(skill);

    assert(score > 0.0);
    assert(score <= 1.0);

    cout << "Completeness score: " << score << endl;
    cout << "PASS" << endl;
}


// Test 5: Missing frontmatter
void CompletenessTest::testMissingFrontmatter()
{
    cout << "\nTest 5: Missing Frontmatter" << endl;

    string skillWithFrontmatter = completeSkill;

    string skillWithoutFrontmatter =
        "# Overview\n"
        "This skill provides instructions for analyzing text and "
        "processing information efficiently. It explains how to "
        "identify important information and organize the results. "
        "Users can follow these instructions to complete their tasks. "
        "The process includes reviewing documents and producing "
        "accurate and consistent results. Every step is described "
        "clearly to ensure that users understand the expected outcome. "
        "The final result should be reviewed before completion.";

    double withFrontmatter =
        evaluateSkillCompleteness(skillWithFrontmatter);

    double withoutFrontmatter =
        evaluateSkillCompleteness(skillWithoutFrontmatter);

    assert(withFrontmatter > withoutFrontmatter);

    cout << "Score with frontmatter: "
         << withFrontmatter << endl;

    cout << "Score without frontmatter: "
         << withoutFrontmatter << endl;

    cout << "PASS" << endl;
}


// Test 6: Missing name and description
void CompletenessTest::testMissingNameAndDescription()
{
    cout << "\nTest 6: Missing Name and Description" << endl;

    string skill =
        "---\n"
        "author: Test Author\n"
        "version: 1.0\n"
        "---\n"
        "# Introduction\n"
        "This skill provides instructions for analyzing text "
        "and processing information efficiently.";

    double score = evaluateSkillCompleteness(skill);

    assert(score >= 0.0);
    assert(score <= 1.0);

    double completeScore =
        evaluateSkillCompleteness(completeSkill);

    assert(completeScore > score);

    cout << "Score with missing metadata: "
         << score << endl;

    cout << "Score with complete metadata: "
         << completeScore << endl;

    cout << "PASS" << endl;
}


// Test 7: Bracket balance
void CompletenessTest::testBracketBalance()
{
    cout << "\nTest 7: Bracket Balance" << endl;

    string validSkill = completeSkill;

    string invalidSkill = completeSkill;
    invalidSkill += "\nAdditional content with an unclosed bracket (";

    double validScore =
        evaluateSkillCompleteness(validSkill);

    double invalidScore =
        evaluateSkillCompleteness(invalidSkill);

    assert(validScore > invalidScore);

    cout << "Valid bracket score: "
         << validScore << endl;

    cout << "Invalid bracket score: "
         << invalidScore << endl;

    cout << "PASS" << endl;
}


// Test 8: Unclosed code block
void CompletenessTest::testUnclosedCodeBlock()
{
    cout << "\nTest 8: Unclosed Code Block" << endl;

    string validSkill = completeSkill;

    string invalidSkill = completeSkill;
    invalidSkill += "\n```cpp\n";
    invalidSkill += "int main() {\n";
    invalidSkill += "    return 0;\n";

    double validScore =
        evaluateSkillCompleteness(validSkill);

    double invalidScore =
        evaluateSkillCompleteness(invalidSkill);

    assert(validScore > invalidScore);

    cout << "Valid code block score: "
         << validScore << endl;

    cout << "Unclosed code block score: "
         << invalidScore << endl;

    cout << "PASS" << endl;
}


// Test 9: Punctuation health
void CompletenessTest::testPunctuationHealth()
{
    cout << "\nTest 9: Punctuation Health" << endl;

    string validSkill = completeSkill;

    string invalidSkill =
        "---\n"
        "name: Example Skill\n"
        "description: Example description.\n"
        "---\n"
        "# Overview\n"
        "this skill provides instructions for analyzing text "
        "and processing information efficiently "
        "it explains how to identify important information "
        "and organize the results";

    double validScore =
        evaluateSkillCompleteness(validSkill);

    double invalidScore =
        evaluateSkillCompleteness(invalidSkill);

    assert(validScore > invalidScore);

    cout << "Valid punctuation score: "
         << validScore << endl;

    cout << "Invalid punctuation score: "
         << invalidScore << endl;

    cout << "PASS" << endl;
}


// Test 10: Incomplete sentences
void CompletenessTest::testIncompleteSentences()
{
    cout << "\nTest 10: Incomplete Sentences" << endl;

    string validSkill =
        "---\n"
        "name: Example Skill\n"
        "description: Example description.\n"
        "---\n"
        "# Overview\n"
        "This skill provides instructions for analyzing text. "
        "It explains how to process information efficiently. "
        "Users can follow these instructions to complete tasks. "
        "The final result should be reviewed before completion.";

    string incompleteSkill =
        "---\n"
        "name: Example Skill\n"
        "description: Example description.\n"
        "---\n"
        "# Overview\n"
        "This skill provides instructions for analyzing text. "
        "It explains how to process information efficiently. "
        "Users can follow these instructions to complete tasks and";

    double validScore =
        evaluateSkillCompleteness(validSkill);

    double incompleteScore =
        evaluateSkillCompleteness(incompleteSkill);

    assert(validScore > incompleteScore);

    cout << "Complete sentence score: "
         << validScore << endl;

    cout << "Incomplete sentence score: "
         << incompleteScore << endl;

    cout << "PASS" << endl;
}


// Test 11: Placeholder detection
void CompletenessTest::testPlaceholderDetection()
{
    cout << "\nTest 11: Placeholder Detection" << endl;

    string validSkill = completeSkill;

    string placeholderSkill = completeSkill;
    placeholderSkill += "\nTODO: Complete this section.";

    double validScore =
        evaluateSkillCompleteness(validSkill);

    double placeholderScore =
        evaluateSkillCompleteness(placeholderSkill);

    assert(validScore > placeholderScore);

    cout << "Valid content score: "
         << validScore << endl;

    cout << "Placeholder content score: "
         << placeholderScore << endl;

    cout << "PASS" << endl;
}


// Test 12: Short content
void CompletenessTest::testShortContent()
{
    cout << "\nTest 12: Short Content" << endl;

    string shortSkill =
        "---\n"
        "name: Short Skill\n"
        "description: Short description.\n"
        "---\n"
        "# Overview\n"
        "This is a short skill.";

    double shortScore =
        evaluateSkillCompleteness(shortSkill);

    double completeScore =
        evaluateSkillCompleteness(completeSkill);

    assert(shortScore >= 0.0);
    assert(shortScore < completeScore);

    cout << "Short content score: "
         << shortScore << endl;

    cout << "Complete content score: "
         << completeScore << endl;

    cout << "PASS" << endl;
}


// Test 13: Headers and code exclusion
void CompletenessTest::testHeaderAndCodeExclusion()
{
    cout << "\nTest 13: Header and Code Exclusion" << endl;

    string skill =
        "---\n"
        "name: Example Skill\n"
        "description: Example description.\n"
        "---\n"
        "# Overview\n"
        "This skill provides instructions for analyzing text. "
        "It explains how to process information efficiently. "
        "Users can follow these instructions to complete tasks.\n"
        "```cpp\n"
        "this is incomplete text without punctuation\n"
        "TODO: This is an example placeholder in code.\n"
        "```\n"
        "# Additional Information\n"
        "The final result should be reviewed before completion.";

    double score = evaluateSkillCompleteness(skill);

    assert(score >= 0.0);
    assert(score <= 1.0);

    cout << "Completeness score: "
         << score << endl;

    cout << "PASS" << endl;
}


// Run all tests
void CompletenessTest::runAllTests()
{
    cout << "=================================" << endl;
    cout << " SKILL COMPLETENESS ANALYZER TESTS" << endl;
    cout << "=================================" << endl;

    testEmptySkill();
    testCompleteSkill();
    testScoreRange();
    testFrontmatterDetection();
    testMissingFrontmatter();
    testMissingNameAndDescription();
    testBracketBalance();
    testUnclosedCodeBlock();
    testPunctuationHealth();
    testIncompleteSentences();
    testPlaceholderDetection();
    testShortContent();
    testHeaderAndCodeExclusion();

    cout << "\n=================================" << endl;
    cout << " ALL TESTS COMPLETED SUCCESSFULLY" << endl;
    cout << "=================================" << endl;
}
