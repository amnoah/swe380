#ifndef SWE380_LENGTH_EVALUATOR_H
#define SWE380_LENGTH_EVALUATOR_H
#include <cstddef>
#include <string>

struct LengthMetrics {
    size_t wordCount = 0;       // Prose + fenced code tokens (backward-compatible)
    size_t proseWordCount = 0;  // Tokens outside fenced code blocks
    size_t codeWordCount = 0;   // Tokens inside fenced code blocks
    size_t proseLineCount = 0;  // Non-fence lines outside code blocks
    size_t totalFileLineCount = 0; // Raw SKILL.md lines including front matter
    size_t characterCount = 0;  // UTF-8 codepoints in body, including formatting
    size_t byteCount = 0;       // Bytes in body
    size_t lineCount = 0;       // Lines in body, excluding trailing empty line after final newline
    size_t codeLineCount = 0;   // Non-fence lines inside fenced blocks
};

class SkillLengthEvaluator {
public:
    static LengthMetrics evaluate(const std::string& rawMarkdown);
};
#endif
