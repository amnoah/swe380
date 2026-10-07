#include "readability_evaluator.h"

#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>

SkillReadabilityAnalyzer::SkillReadabilityAnalyzer()
    : options_(Options()) {}

SkillReadabilityAnalyzer::SkillReadabilityAnalyzer(Options options)
    : options_(options) {}

double SkillReadabilityAnalyzer::evaluate(const std::string& skillContent, ReadabilityMetric metric) {
    SkillReadabilityAnalyzer analyzer;
    return analyzer.calculateScore(skillContent, metric);
}

double SkillReadabilityAnalyzer::evaluateFile(const std::string& filePath, ReadabilityMetric metric) {
    SkillReadabilityAnalyzer analyzer;
    ReadabilityMetrics m = analyzer.analyzeFile(filePath);
    switch (metric) {
        case ReadabilityMetric::FleschReadingEase: return m.fleschReadingEase;
        case ReadabilityMetric::FleschKincaidGrade: return m.fleschKincaidGrade;
        case ReadabilityMetric::ColemanLiauIndex: return m.colemanLiauIndex;
        case ReadabilityMetric::AutomatedReadability: return m.automatedReadabilityIndex;
        case ReadabilityMetric::GunningFogIndex: return m.gunningFogIndex;
        default: return m.fleschReadingEase;
    }
}

double SkillReadabilityAnalyzer::calculateScore(const std::string& skillContent) const {
    return calculateScore(skillContent, options_.defaultMetric);
}

double SkillReadabilityAnalyzer::calculateScore(const std::string& skillContent, ReadabilityMetric metric) const {
    ReadabilityMetrics m = analyze(skillContent);
    switch (metric) {
        case ReadabilityMetric::FleschReadingEase:
            return m.fleschReadingEase;
        case ReadabilityMetric::FleschKincaidGrade:
            return m.fleschKincaidGrade;
        case ReadabilityMetric::ColemanLiauIndex:
            return m.colemanLiauIndex;
        case ReadabilityMetric::AutomatedReadability:
            return m.automatedReadabilityIndex;
        case ReadabilityMetric::GunningFogIndex:
            return m.gunningFogIndex;
        default:
            return m.fleschReadingEase;
    }
}

ReadabilityMetrics SkillReadabilityAnalyzer::analyzeFile(const std::string& filePath) const {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return ReadabilityMetrics();
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return analyze(buffer.str());
}

ReadabilityMetrics SkillReadabilityAnalyzer::analyze(const std::string& skillContent) const {
    ReadabilityMetrics metrics;

    std::string cleanText = preprocessText(skillContent);
    if (cleanText.empty()) {
        return metrics;
    }

    metrics.characterCount = cleanText.size();

    bool inWord = false;
    std::string currentWord = "";
    std::vector<std::string> words;

    for (size_t i = 0; i < cleanText.size(); ++i) {
        char c = cleanText[i];

        if (std::isalpha(static_cast<unsigned char>(c))) {
            metrics.letterCount++;
            currentWord += std::tolower(static_cast<unsigned char>(c));
            inWord = true;
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            currentWord += c;
            inWord = true;
        } else if (c == '\'' || c == '-') {
            if (inWord && i + 1 < cleanText.size() && std::isalpha(static_cast<unsigned char>(cleanText[i + 1]))) {
                currentWord += c;
            } else if (inWord) {
                words.push_back(currentWord);
                currentWord.clear();
                inWord = false;
            }
        } else {
            if (inWord) {
                words.push_back(currentWord);
                currentWord.clear();
                inWord = false;
            }

            if (c == '.' || c == '!' || c == '?') {
                metrics.sentenceCount++;
            }
        }
    }
    if (inWord && !currentWord.empty()) {
        words.push_back(currentWord);
    }

    metrics.wordCount = words.size();

    if (metrics.wordCount == 0) {
        return metrics;
    }
    if (metrics.sentenceCount == 0) {
        metrics.sentenceCount = 1;
    }

    for (const auto& w : words) {
        size_t syl = countSyllables(w);
        metrics.syllableCount += syl;
        if (syl >= 3) {
            metrics.complexWordCount++;
        }
    }

    double wordsPerSentence = static_cast<double>(metrics.wordCount) / metrics.sentenceCount;
    double syllablesPerWord = static_cast<double>(metrics.syllableCount) / metrics.wordCount;
    double lettersPer100Words = (static_cast<double>(metrics.letterCount) / metrics.wordCount) * 100.0;
    double sentencesPer100Words = (static_cast<double>(metrics.sentenceCount) / metrics.wordCount) * 100.0;
    double charsPerWord = static_cast<double>(metrics.characterCount) / metrics.wordCount;
    double complexWordRatio = static_cast<double>(metrics.complexWordCount) / metrics.wordCount;

    metrics.fleschReadingEase = 206.835 - (1.015 * wordsPerSentence) - (84.6 * syllablesPerWord);
    metrics.fleschKincaidGrade = (0.39 * wordsPerSentence) + (11.8 * syllablesPerWord) - 15.59;
    metrics.colemanLiauIndex = (0.0588 * lettersPer100Words) - (0.296 * sentencesPer100Words) - 15.8;
    metrics.automatedReadabilityIndex = (4.71 * charsPerWord) + (0.5 * wordsPerSentence) - 21.43;
    metrics.gunningFogIndex = 0.4 * (wordsPerSentence + (100.0 * complexWordRatio));

    return metrics;
}

bool SkillReadabilityAnalyzer::isVowel(char c) {
    c = std::tolower(static_cast<unsigned char>(c));
    return (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y');
}

size_t SkillReadabilityAnalyzer::countSyllables(const std::string& word) {
    if (word.empty()) return 0;

    std::string cleanWord;
    for (char c : word) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            cleanWord += std::tolower(static_cast<unsigned char>(c));
        }
    }

    if (cleanWord.empty()) return 0;
    if (cleanWord.length() <= 3) return 1;

    size_t count = 0;
    bool prevIsVowel = false;

    for (size_t i = 0; i < cleanWord.length(); ++i) {
        bool vowel = isVowel(cleanWord[i]);
        if (vowel && !prevIsVowel) {
            count++;
        }
        prevIsVowel = vowel;
    }

    if (cleanWord.back() == 'e' && !cleanWord.empty() && count > 1) {
        if (cleanWord.length() > 2 && !isVowel(cleanWord[cleanWord.length() - 2])) {
            count--;
        }
    }

    if (cleanWord.length() > 2 && (cleanWord.substr(cleanWord.length() - 2) == "ed" || cleanWord.substr(cleanWord.length() - 2) == "es")) {
        if (count > 1) {
            count--;
        }
    }

    return std::max<size_t>(1, count);
}

std::string SkillReadabilityAnalyzer::preprocessText(const std::string& rawText) const {
    std::string text = rawText;

    if (options_.ignoreFrontmatter) {
        text = stripFrontmatter(text);
    }
    if (options_.ignoreCodeBlocks) {
        text = stripCodeBlocks(text);
    }
    if (options_.ignoreMarkdownSyntax) {
        text = stripMarkdownFormatting(text);
    }

    return text;
}

std::string SkillReadabilityAnalyzer::stripFrontmatter(const std::string& text) {
    size_t startPos = text.find("---");
    if (startPos == 0 || (startPos != std::string::npos && startPos < 10 && text.substr(0, startPos) == std::string(startPos, ' '))) {
        size_t endPos = text.find("---", startPos + 3);
        if (endPos != std::string::npos) {
            return text.substr(endPos + 3);
        }
    }
    return text;
}

std::string SkillReadabilityAnalyzer::stripCodeBlocks(const std::string& text) {
    std::string result;
    size_t pos = 0;
    while (pos < text.length()) {
        size_t codeStart = text.find("```", pos);
        if (codeStart == std::string::npos) {
            result += text.substr(pos);
            break;
        }
        result += text.substr(pos, codeStart - pos);
        size_t codeEnd = text.find("```", codeStart + 3);
        if (codeEnd == std::string::npos) {
            break;
        }
        pos = codeEnd + 3;
    }
    return result;
}

std::string SkillReadabilityAnalyzer::stripMarkdownFormatting(const std::string& text) {
    std::string result;
    std::istringstream stream(text);
    std::string line;

    while (std::getline(stream, line)) {
        size_t firstChar = line.find_first_not_of(" \t");
        if (firstChar != std::string::npos) {
            if (line[firstChar] == '#') {
                size_t contentStart = line.find_first_not_of("# \t", firstChar);
                if (contentStart != std::string::npos) {
                    line = line.substr(contentStart);
                } else {
                    line.clear();
                }
            } else if (line[firstChar] == '*' || line[firstChar] == '-' || line[firstChar] == '+') {
                if (firstChar + 1 < line.size() && (line[firstChar + 1] == ' ' || line[firstChar + 1] == '\t')) {
                    line = line.substr(firstChar + 2);
                }
            }
        }
        result += line + "\n";
    }

    return result;
}
