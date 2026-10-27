#ifndef SWE380_MARKDOWN_UTILS_H
#define SWE380_MARKDOWN_UTILS_H

#include <string>
#include <vector>

namespace markdown {

// Strip only a leading, closed YAML front matter block; leave malformed front matter intact.
std::string stripFrontmatter(const std::string& markdown);
std::vector<std::string> splitLines(const std::string& input);
std::string trim(const std::string& input);
std::string lowercaseAscii(std::string input);
bool fenceDelimiter(const std::string& line, char& symbol, size_t& count);
bool closingFence(const std::string& line, char symbol, size_t minCount);
// UTF-8 codepoints, not bytes (assumes valid UTF-8 from SQLite records).
size_t utf8Codepoints(const std::string& value);
// Whitespace- and punctuation-separated word-like tokens. Includes code text.
size_t wordCount(const std::string& text);

}  // namespace markdown
#endif
