#include "example_evaluator.h"
#include "evaluations/shared/markdown_utils.h"

#include <vector>

namespace {
bool begins(const std::string& a, const std::string& b) { return a.compare(0, b.size(), b) == 0; }
bool cue(const std::string& input) {
    const std::string s = markdown::lowercaseAscii(input);
    return s.find("for example") != std::string::npos ||
           s.find("for instance") != std::string::npos ||
           s.find("e.g.") != std::string::npos ||
           s.find("example:") != std::string::npos ||
           s.find("examples:") != std::string::npos ||
           s.find("sample:") != std::string::npos ||
           s.find("sample input") != std::string::npos ||
           s.find("sample output") != std::string::npos ||
           s.find("sample request") != std::string::npos ||
           s.find("sample response") != std::string::npos ||
           s.find("try this") != std::string::npos ||
           s.find("try the following") != std::string::npos;
}
std::string removeHeadingPrefix(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && s[start] == '#') ++start;
    return markdown::trim(s.substr(start));
}
bool startsWord(const std::string& text, const std::string& prefix) {
    if (!begins(text, prefix)) return false;
    return text.size() == prefix.size() || text[prefix.size()] == ' ' ||
           text[prefix.size()] == ':' || text[prefix.size()] == '-' ||
           text[prefix.size()] == '(';
}
bool sectionName(const std::string& s) {
    const std::string t = markdown::lowercaseAscii(removeHeadingPrefix(s));
    if (startsWord(t, "example") || startsWord(t, "examples") ||
        startsWord(t, "worked example") || startsWord(t, "practical example") ||
        startsWord(t, "quick example") || startsWord(t, "usage example") ||
        startsWord(t, "example usage")) return true;
    return t == "sample" || t == "samples" ||
           startsWord(t, "sample input") || startsWord(t, "sample output") ||
           startsWord(t, "sample request") || startsWord(t, "sample response") ||
           startsWord(t, "sample usage") || startsWord(t, "sample commands");
}
bool label(const std::string& s) {
    std::string t = markdown::lowercaseAscii(markdown::trim(s));
    if (begins(t, "- ") || begins(t, "* ") || begins(t, "+ ")) t = t.substr(2);
    if (begins(t, "**")) t = t.substr(2);
    const auto colon = t.find(':');
    if (colon == std::string::npos || colon + 1 >= t.size()) return false;
    std::string before = markdown::trim(t.substr(0, colon));
    if (before.size() >= 2 && before.substr(before.size()-2) == "**")
        before = markdown::trim(before.substr(0, before.size()-2));
    if (before == "example" || before == "examples" || before == "sample" ||
        before == "samples" || before == "sample input" || before == "sample output" ||
        before == "sample request" || before == "sample response" || before == "example usage") return true;
    // Numbered examples, e.g. Example 1: or Example 2.3:
    for (const std::string& prefix : {"example ", "sample "}) {
        if (begins(before, prefix)) {
            const std::string number = before.substr(prefix.size());
            if (!number.empty() && number.find_first_not_of("0123456789.") == std::string::npos)
                return true;
        }
    }
    return false;
}
}  // namespace

ExampleMetrics SkillExampleEvaluator::evaluate(const std::string& rawMarkdown, size_t bodyWordCount) {
    const auto lines = markdown::splitLines(markdown::stripFrontmatter(rawMarkdown));
    ExampleMetrics m;
    bool fenced = false;
    char fenceSymbol = 0;
    size_t fenceSize = 0;
    bool exampleSection = false;
    size_t sectionHeading = lines.size();
    size_t lastExampleLabel = lines.size();
    std::vector<bool> counted(lines.size(), false);

    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string stripped = markdown::trim(lines[i]);
        char foundSymbol = 0;
        size_t foundSize = 0;
        if (fenced) {
            if (markdown::closingFence(lines[i], fenceSymbol, fenceSize)) fenced = false;
            continue;
        }
        if (markdown::fenceDelimiter(lines[i], foundSymbol, foundSize)) {
            fenced = true;
            fenceSymbol = foundSymbol;
            fenceSize = foundSize;
            bool nearby = false;
            size_t checked = 0;
            for (size_t j = i; j > 0 && checked < 2;) {
                --j;
                if (markdown::trim(lines[j]).empty()) continue;
                ++checked;
                if (cue(lines[j]) || label(lines[j])) nearby = true;
            }
            if (exampleSection || nearby) {
                ++m.exampleCodeBlocks;
                ++m.exampleCount;
                if (exampleSection && sectionHeading < lines.size() && counted[sectionHeading]) {
                    counted[sectionHeading] = false;
                    --m.exampleCount;
                }
                if (lastExampleLabel < lines.size() && i - lastExampleLabel <= 3 && counted[lastExampleLabel]) {
                    counted[lastExampleLabel] = false;
                    --m.exampleCount;
                }
            }
            continue;
        }
        if (stripped.empty()) continue;
        if (stripped[0] == '#') {
            exampleSection = sectionName(stripped);
            sectionHeading = exampleSection ? i : lines.size();
            if (exampleSection) {
                ++m.exampleSections;
                if (i + 1 < lines.size()) {
                    ++m.exampleCount;
                    counted[i] = true;
                }
            }
        } else if (label(stripped)) {
            ++m.exampleSections;
            ++m.exampleCount;
            counted[i] = true;
            lastExampleLabel = i;
        }
    }
    m.hasExamples = m.exampleCount > 0;
    if (bodyWordCount > 0)
        m.examplesPer1000Words = 1000.0 * static_cast<double>(m.exampleCount) / static_cast<double>(bodyWordCount);
    return m;
}
