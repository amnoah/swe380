#include "markdown_utils.h"

#include <algorithm>
#include <cctype>

namespace markdown {
std::string trim(const std::string& input) {
    const auto start = input.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    return input.substr(start, input.find_last_not_of(" \t\r\n") - start + 1);
}

std::string lowercaseAscii(std::string input) {
    std::transform(input.begin(), input.end(), input.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return input;
}

std::vector<std::string> splitLines(const std::string& input) {
    std::vector<std::string> lines;
    if (input.empty()) return lines;
    size_t start = 0;
    while (start < input.size()) {
        const auto next = input.find('\n', start);
        if (next == std::string::npos) {
            std::string line = input.substr(start);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            lines.push_back(std::move(line));
            break;
        }
        std::string line = input.substr(start, next - start);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(std::move(line));
        start = next + 1;
    }
    return lines;
}

std::string stripFrontmatter(const std::string& input) {
    const size_t offset = input.compare(0, 3, "\xEF\xBB\xBF") == 0 ? 3 : 0;
    const size_t firstEnd = input.find('\n', offset);
    if (trim(input.substr(offset, firstEnd == std::string::npos ? std::string::npos : firstEnd - offset)) != "---")
        return input.substr(offset);
    if (firstEnd == std::string::npos) return input.substr(offset);
    size_t p = firstEnd + 1;
    while (p < input.size()) {
        const size_t end = input.find('\n', p);
        const std::string line = trim(input.substr(p, end == std::string::npos ? std::string::npos : end - p));
        if (line == "---" || line == "...") {
            return end == std::string::npos ? "" : input.substr(end + 1);
        }
        if (end == std::string::npos) break;
        p = end + 1;
    }
    // An unclosed block may be ordinary Markdown; do not discard the entire file.
    return input.substr(offset);
}

bool fenceDelimiter(const std::string& line, char& symbol, size_t& count) {
    // CommonMark fences: at most 3 leading spaces, >=3 repeated ` or ~.
    const auto first = line.find_first_not_of(' ');
    if (first == std::string::npos || first > 3) return false;
    symbol = line[first];
    if (symbol != '`' && symbol != '~') return false;
    count = 0;
    while (first + count < line.size() && line[first + count] == symbol) ++count;
    return count >= 3;
}

bool closingFence(const std::string& line, char symbol, size_t minCount) {
    char found = 0;
    size_t count = 0;
    if (!fenceDelimiter(line, found, count) || found != symbol || count < minCount) return false;
    const size_t offset = line.find_first_not_of(' ');
    return trim(line.substr(offset + count)).empty();
}

size_t utf8Codepoints(const std::string& value) {
    size_t count = 0;
    for (unsigned char c : value) if ((c & 0xC0) != 0x80) ++count;
    return count;
}

size_t wordCount(const std::string& value) {
    size_t words = 0;
    bool inWord = false;
    for (unsigned char c : value) {
        // All non-ASCII UTF-8 bytes count as word constituents (continuations don't split tokens).
        const bool wordChar = std::isalnum(c) || c >= 128 || c == '_';
        if (wordChar && !inWord) ++words;
        inWord = wordChar;
    }
    return words;
}
}  // namespace markdown
