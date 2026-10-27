#include "length_evaluator.h"
#include "evaluations/shared/markdown_utils.h"

LengthMetrics SkillLengthEvaluator::evaluate(const std::string& rawMarkdown) {
    const std::string body = markdown::stripFrontmatter(rawMarkdown);
    LengthMetrics m;
    m.totalFileLineCount = markdown::splitLines(rawMarkdown).size();
    m.byteCount = body.size();
    m.characterCount = markdown::utf8Codepoints(body);
    const auto lines = markdown::splitLines(body);
    m.lineCount = lines.size();
    bool inside = false;
    char fenceSymbol = 0;
    size_t fenceSize = 0;
    for (const auto& line : lines) {
        char detectedSymbol = 0;
        size_t detectedSize = 0;
        if (!inside && markdown::fenceDelimiter(line, detectedSymbol, detectedSize)) {
            inside = true;
            fenceSymbol = detectedSymbol;
            fenceSize = detectedSize;
            continue;
        }
        if (inside && markdown::closingFence(line, fenceSymbol, fenceSize)) {
            inside = false;
            continue;
        }
        if (inside) {
            ++m.codeLineCount;
            m.codeWordCount += markdown::wordCount(line);
        } else {
            ++m.proseLineCount;
            m.proseWordCount += markdown::wordCount(line);
        }
    }
    m.wordCount = m.proseWordCount + m.codeWordCount;
    return m;
}
