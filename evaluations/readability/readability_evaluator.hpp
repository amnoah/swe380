/**
 * @file SkillReadabilityAnalyzer-v5.hpp
 * @brief C++ Header for the SkillReadabilityAnalyzer object.
 */

#ifndef SKILL_READABILITY_ANALYZER_HPP
#define SKILL_READABILITY_ANALYZER_HPP

#include <string>
#include <vector>

/**
 * @enum ReadabilityMetric
 * @brief Formula types available for readability calculation.
 */
enum class ReadabilityMetric {
    FleschReadingEase,     ///< Flesch Reading Ease (0 to 100+, higher = easier)
    FleschKincaidGrade,    ///< Flesch-Kincaid Grade Level
    ColemanLiauIndex,      ///< Coleman-Liau Index
    AutomatedReadability,  ///< Automated Readability Index (ARI)
    GunningFogIndex        ///< Gunning Fog Index
};

/**
 * @struct ReadabilityMetrics
 * @brief Detailed breakdown of text statistics and calculated readability metrics.
 */
struct ReadabilityMetrics {
    size_t characterCount = 0;
    size_t letterCount = 0;
    size_t wordCount = 0;
    size_t sentenceCount = 0;
    size_t syllableCount = 0;
    size_t complexWordCount = 0;

    double fleschReadingEase = 0.0;
    double fleschKincaidGrade = 0.0;
    double colemanLiauIndex = 0.0;
    double automatedReadabilityIndex = 0.0;
    double gunningFogIndex = 0.0;
};

/**
 * @class SkillReadabilityAnalyzer
 * @brief Object class that evaluates the readability of an inputted agent skill (SKILL.md).
 */
class SkillReadabilityAnalyzer {
public:
    /**
     * @struct Options
     * @brief Configuration settings for skill preprocessing.
     */
    struct Options {
        bool ignoreFrontmatter;
        bool ignoreCodeBlocks;
        bool ignoreMarkdownSyntax;
        ReadabilityMetric defaultMetric;

        /// User-provided default constructor avoids C++ default member initializer rules in nested classes
        Options()
            : ignoreFrontmatter(true),
              ignoreCodeBlocks(true),
              ignoreMarkdownSyntax(true),
              defaultMetric(ReadabilityMetric::FleschReadingEase) {}
    };

    /**
     * @brief Default constructor.
     */
    SkillReadabilityAnalyzer();

    /**
     * @brief Constructor with customized options.
     */
    explicit SkillReadabilityAnalyzer(Options options);

    /**
     * @brief Static single-line evaluation from raw Markdown string.
     */
    static double evaluate(const std::string& skillContent, ReadabilityMetric metric = ReadabilityMetric::FleschReadingEase);

    /**
     * @brief Static single-line evaluation directly from a file path.
     */
    static double evaluateFile(const std::string& filePath, ReadabilityMetric metric = ReadabilityMetric::FleschReadingEase);

    /**
     * @brief Calculates a single numerical readability score using configured metric.
     */
    double calculateScore(const std::string& skillContent) const;

    /**
     * @brief Calculates a single numerical readability score using a specific metric.
     */
    double calculateScore(const std::string& skillContent, ReadabilityMetric metric) const;

    /**
     * @brief Analyzes skill text and returns complete statistics and metric scores.
     */
    ReadabilityMetrics analyze(const std::string& skillContent) const;

    /**
     * @brief Analyzes a skill file directly from disk.
     */
    ReadabilityMetrics analyzeFile(const std::string& filePath) const;

    void setOptions(const Options& options) { options_ = options; }
    const Options& getOptions() const { return options_; }

    static size_t countSyllables(const std::string& word);
    static bool isVowel(char c);

private:
    Options options_;

    std::string preprocessText(const std::string& rawText) const;
    static std::string stripFrontmatter(const std::string& text);
    static std::string stripCodeBlocks(const std::string& text);
    static std::string stripMarkdownFormatting(const std::string& text);
};

#endif // SKILL_READABILITY_ANALYZER_HPP
