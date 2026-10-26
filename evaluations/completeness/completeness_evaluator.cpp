#include "completeness_evaluator.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <stack>
#include <unordered_set>
#include <vector>

namespace {

// Strips YAML frontmatter and checks for key fields ('name:', 'description:')
std::string extractBody(const std::string& text, bool& outHasFrontmatter, bool& outHasName, bool& outHasDesc) {
    outHasFrontmatter = false;
    outHasName = false;
    outHasDesc = false;

    size_t startPos = text.find("---");
    if (startPos == 0 || (startPos != std::string::npos && startPos < 10 && text.substr(0, startPos) == std::string(startPos, ' '))) {
        size_t endPos = text.find("---", startPos + 3);
        if (endPos != std::string::npos) {
            outHasFrontmatter = true;
            std::string frontmatterContent = text.substr(startPos + 3, endPos - (startPos + 3));

            if (frontmatterContent.find("name:") != std::string::npos) outHasName = true;
            if (frontmatterContent.find("description:") != std::string::npos) outHasDesc = true;

            return text.substr(endPos + 3);
        }
    }
    return text;
}

// Counts unclosed or mismatched brackets (), [], {}
size_t countUnclosedBrackets(const std::string& text) {
    std::stack<char> st;
    size_t errors = 0;

    for (char c : text) {
        if (c == '(' || c == '[' || c == '{') {
            st.push(c);
        } else if (c == ')' || c == ']' || c == '}') {
            if (st.empty()) {
                errors++;
            } else {
                char top = st.top();
                if ((c == ')' && top == '(') || (c == ']' && top == '[') || (c == '}' && top == '{')) {
                    st.pop();
                } else {
                    errors++;
                }
            }
        }
    }
    return errors + st.size();
}

// Counts TODO, FIXME, [INSERT], etc.
size_t countPlaceholders(const std::string& text) {
    std::vector<std::string> patterns = {
        "TODO", "FIXME", "XXX", "TBD", "LOREM IPSUM",
        "[INSERT", "<INSERT", "[YOUR_", "<YOUR_", "INSERT_HERE"
    };

    std::string upperText = text;
    std::transform(upperText.begin(), upperText.end(), upperText.begin(), ::toupper);

    size_t count = 0;
    for (const auto& pat : patterns) {
        size_t pos = 0;
        while ((pos = upperText.find(pat, pos)) != std::string::npos) {
            count++;
            pos += pat.length();
        }
    }
    return count;
}

// Extracts prose sentences outside code blocks and headers
std::vector<std::string> extractSentences(const std::string& text) {
    std::vector<std::string> sentences;
    std::string current;
    bool inCodeBlock = false;
    std::istringstream stream(text);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.find("```") != std::string::npos) {
            inCodeBlock = !inCodeBlock;
            continue;
        }
        if (inCodeBlock || (!line.empty() && line[0] == '#')) continue;

        for (size_t i = 0; i < line.length(); ++i) {
            char c = line[i];
            current += c;

            if (c == '.' || c == '!' || c == '?') {
                if (i + 1 == line.length() || std::isspace(static_cast<unsigned char>(line[i + 1]))) {
                    size_t first = current.find_first_not_of(" \t\r\n- *1234567890.");
                    if (first != std::string::npos) {
                        sentences.push_back(current.substr(first));
                    }
                    current.clear();
                }
            }
        }
        if (!current.empty()) current += " ";
    }

    if (!current.empty()) {
        size_t first = current.find_first_not_of(" \t\r\n- *1234567890.");
        if (first != std::string::npos) {
            sentences.push_back(current.substr(first));
        }
    }

    return sentences;
}

} // anonymous namespace

double evaluateSkillCompleteness(const std::string& skill) {
    if (skill.empty()) return 0.0;

    // 1. Frontmatter & Header Structure (Weight: 0.25)
    bool hasFrontmatter = false, hasName = false, hasDesc = false;
    std::string bodyText = extractBody(skill, hasFrontmatter, hasName, hasDesc);

    double frontmatterScore = 0.0;
    if (hasFrontmatter) frontmatterScore += 0.40;
    if (hasName) frontmatterScore += 0.30;
    if (hasDesc) frontmatterScore += 0.30;

    size_t headerCount = 0;
    std::istringstream bodyStream(bodyText);
    std::string line;
    while (std::getline(bodyStream, line)) {
        if (!line.empty() && line[0] == '#') headerCount++;
    }
    if (headerCount > 0) {
        frontmatterScore = std::min(1.0, frontmatterScore + 0.10);
    }

    // 2. Syntax & Bracket Balance (Weight: 0.25)
    size_t codeFenceCount = 0, pos = 0;
    while ((pos = skill.find("```", pos)) != std::string::npos) {
        codeFenceCount++;
        pos += 3;
    }
    size_t unclosedCodeBlocks = codeFenceCount % 2;
    size_t unclosedBrackets = countUnclosedBrackets(bodyText);

    // 3. Punctuation & Sentence Health (Weight: 0.25)
    std::vector<std::string> sentences = extractSentences(bodyText);
    size_t validPunctuation = 0, truncatedCount = 0, validCapitals = 0;
    std::unordered_set<std::string> danglingEndings = { "and", "or", "with", "the", "to", "if", "when", "for", "because", "a", "an", "is" };

    for (const auto& sent : sentences) {
        if (sent.empty()) continue;
        if (std::isupper(static_cast<unsigned char>(sent[0]))) validCapitals++;

        if (sent.back() == '.' || sent.back() == '!' || sent.back() == '?' || sent.back() == ':') {
            validPunctuation++;
        }

        if (sent.length() >= 3 && sent.substr(sent.length() - 3) == "...") {
            truncatedCount++;
        } else {
            std::string lastWord = sent.substr(sent.find_last_of(" \t") == std::string::npos ? 0 : sent.find_last_of(" \t") + 1);
            std::transform(lastWord.begin(), lastWord.end(), lastWord.begin(), ::tolower);
            if (danglingEndings.count(lastWord)) truncatedCount++;
        }
    }

    double punctScore = sentences.empty() ? 0.30 : (0.60 * (static_cast<double>(validPunctuation) / sentences.size())) +
                                                   (0.40 * (static_cast<double>(validCapitals) / sentences.size()));

    if (unclosedBrackets > 0) punctScore -= std::min(0.30, unclosedBrackets * 0.10);
    if (unclosedCodeBlocks > 0) punctScore -= 0.25;
    punctScore = std::max(0.0, punctScore);

    // 4. Grammar & Clause Completion (Weight: 0.25)
    double grammarScore = sentences.empty() ? 1.0 : 1.0 - (static_cast<double>(truncatedCount) / sentences.size());
    grammarScore = std::max(0.0, grammarScore);

    // 5. Content Cleanliness & Sufficiency (Weight: 0.25)
    std::istringstream wordStream(bodyText);
    std::string w;
    size_t totalWords = 0;
    while (wordStream >> w) totalWords++;

    size_t placeholders = countPlaceholders(skill);

    double cleanlinessScore = 1.0;
    if (totalWords < 30) {
        cleanlinessScore *= (static_cast<double>(totalWords) / 30.0);
    }
    if (placeholders > 0) {
        cleanlinessScore -= std::min(0.50, placeholders * 0.15);
    }
    cleanlinessScore = std::max(0.0, cleanlinessScore);

    // Weighted Total
    double total = (0.25 * frontmatterScore) +
                   (0.25 * punctScore) +
                   (0.25 * grammarScore) +
                   (0.25 * cleanlinessScore);

    return std::max(0.0, std::min(1.0, std::round(total * 1000.0) / 1000.0));
}