#include "main.h"

#include "evaluations/reuse/reuse_evaluator.h"
#include "test/test.h"
#include "evaluations/completeness/completeness_evaluator.h"
#include "evaluations/examples/example_evaluator.h"
#include "evaluations/length/length_evaluator.h"
#include "evaluations/readability/readability_evaluator.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct SkillResult {
    std::string repository;
    std::string path;
    double survival = 0.0;          // 0..1
    double completeness = 0.0;      // 0..1
    double examplesPer1000Words = 0.0;
    double wordCount = 0.0;
    double fleschReadingEase = 0.0;
};

std::string csvEscape(const std::string& value) {
    std::string out = "\"";
    for (char c : value) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += "\"";
    return out;
}

std::string xmlEscape(const std::string& value) {
    std::string out;
    for (char c : value) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default: out += c; break;
        }
    }
    return out;
}

double safeMax(const std::vector<SkillResult>& rows,
               double SkillResult::*member,
               double fallback = 1.0) {
    double result = 0.0;
    for (const auto& row : rows) {
        const double value = row.*member;
        if (std::isfinite(value)) result = std::max(result, value);
    }
    return result > 0.0 ? result : fallback;
}

void writeCsv(const std::string& filename,
              const std::vector<SkillResult>& rows) {
    std::ofstream out(filename);
    if (!out) throw std::runtime_error("Could not create CSV file: " + filename);

    out << "repository,skill_path,survival_rate_percent,completeness_score,"
           "examples_per_1000_words,word_count,flesch_reading_ease\n";
    out << std::fixed << std::setprecision(4);

    for (const auto& row : rows) {
        out << csvEscape(row.repository) << ','
            << csvEscape(row.path) << ','
            << row.survival * 100.0 << ','
            << row.completeness << ','
            << row.examplesPer1000Words << ','
            << row.wordCount << ','
            << row.fleschReadingEase << '\n';
    }
}

struct Plot {
    const char* title;
    const char* xLabel;
    double SkillResult::*value;
    double xMax;
};


struct LinearFit {
    bool valid = false;
    double slope = 0.0;
    double intercept = 0.0;
    double rSquared = 0.0;
    std::size_t n = 0;
};

LinearFit calculateLinearFit(const std::vector<SkillResult>& rows,
                             double SkillResult::*member) {
    LinearFit fit;
    double sumX = 0.0, sumY = 0.0, sumXX = 0.0, sumXY = 0.0;
    double sumYY = 0.0;

    for (const auto& row : rows) {
        const double x = row.*member;
        const double y = row.survival * 100.0;
        if (!std::isfinite(x) || !std::isfinite(y)) continue;
        sumX += x;
        sumY += y;
        sumXX += x * x;
        sumXY += x * y;
        sumYY += y * y;
        ++fit.n;
    }

    if (fit.n < 2) return fit;

    const double n = static_cast<double>(fit.n);
    const double denominator = n * sumXX - sumX * sumX;
    if (std::abs(denominator) < 1e-12) return fit;

    fit.slope = (n * sumXY - sumX * sumY) / denominator;
    fit.intercept = (sumY - fit.slope * sumX) / n;

    const double totalSS = sumYY - (sumY * sumY / n);
    const double residualSS =
        sumYY + fit.slope * fit.slope * sumXX + n * fit.intercept * fit.intercept
        - 2.0 * fit.slope * sumXY
        - 2.0 * fit.intercept * sumY
        + 2.0 * fit.slope * fit.intercept * sumX;

    fit.rSquared = totalSS > 1e-12
        ? std::clamp(1.0 - residualSS / totalSS, 0.0, 1.0)
        : 0.0;
    fit.valid = true;
    return fit;
}

void drawScatterPlot(std::ofstream& out,
                     const std::vector<SkillResult>& rows,
                     const Plot& plot,
                     double left, double top,
                     double width, double height) {
    const double right = left + width;
    const double bottom = top + height;
    const double plotLeft = left + 68.0;
    const double plotRight = right - 20.0;
    const double plotTop = top + 42.0;
    const double plotBottom = bottom - 54.0;
    const double plotWidth = plotRight - plotLeft;
    const double plotHeight = plotBottom - plotTop;

    auto mapX = [&](double value) {
        return plotLeft + (std::clamp(value, 0.0, plot.xMax) / plot.xMax) * plotWidth;
    };
    auto mapY = [&](double survivalPercent) {
        return plotBottom - (std::clamp(survivalPercent, 0.0, 100.0) / 100.0) * plotHeight;
    };

    out << "<g font-family=\"Arial, sans-serif\">\n";
    out << "<text x=\"" << left + 8 << "\" y=\"" << top + 22
        << "\" font-size=\"17\" font-weight=\"bold\">"
        << xmlEscape(plot.title) << "</text>\n";

    for (int tick = 0; tick <= 100; tick += 20) {
        const double y = mapY(tick);
        out << "<line x1=\"" << plotLeft << "\" y1=\"" << y
            << "\" x2=\"" << plotRight << "\" y2=\"" << y
            << "\" stroke=\"#dddddd\" stroke-width=\"1\"/>\n";
        out << "<text x=\"" << plotLeft - 10 << "\" y=\"" << y + 4
            << "\" text-anchor=\"end\" font-size=\"11\" fill=\"#444\">"
            << tick << "%</text>\n";
    }

    for (int tick = 0; tick <= 5; ++tick) {
        const double value = plot.xMax * tick / 5.0;
        const double x = mapX(value);
        out << "<line x1=\"" << x << "\" y1=\"" << plotTop
            << "\" x2=\"" << x << "\" y2=\"" << plotBottom
            << "\" stroke=\"#eeeeee\" stroke-width=\"1\"/>\n";
        out << "<text x=\"" << x << "\" y=\"" << plotBottom + 18
            << "\" text-anchor=\"middle\" font-size=\"11\" fill=\"#444\">"
            << std::setprecision(3) << value << "</text>\n";
    }

    out << "<line x1=\"" << plotLeft << "\" y1=\"" << plotTop
        << "\" x2=\"" << plotLeft << "\" y2=\"" << plotBottom
        << "\" stroke=\"#333\"/>\n";
    out << "<line x1=\"" << plotLeft << "\" y1=\"" << plotBottom
        << "\" x2=\"" << plotRight << "\" y2=\"" << plotBottom
        << "\" stroke=\"#333\"/>\n";

    out << "<text x=\"" << (plotLeft + plotRight) / 2
        << "\" y=\"" << bottom - 10
        << "\" text-anchor=\"middle\" font-size=\"12\">"
        << xmlEscape(plot.xLabel) << "</text>\n";
    out << "<text transform=\"translate(" << left + 16 << ","
        << (plotTop + plotBottom) / 2
        << ") rotate(-90)\" text-anchor=\"middle\" font-size=\"12\">"
        << "Survival rate (%)</text>\n";

    // Draw observations first so the fitted line remains visible above them.
    for (const auto& row : rows) {
        const double xValue = row.*(plot.value);
        if (!std::isfinite(xValue) || !std::isfinite(row.survival)) continue;
        const double x = mapX(xValue);
        const double y = mapY(row.survival * 100.0);
        out << "<circle cx=\"" << x << "\" cy=\"" << y
            << "\" r=\"3.2\" fill=\"#2474b5\" fill-opacity=\"0.52\">"
            << "<title>" << xmlEscape(row.repository + " — " + row.path)
            << " | " << plot.xLabel << ": " << xValue
            << " | Survival: " << row.survival * 100.0 << "%</title>"
            << "</circle>\n";
    }

    const LinearFit fit = calculateLinearFit(rows, plot.value);
    if (fit.valid) {
        const double x1 = 0.0;
        const double x2 = plot.xMax;
        const double y1 = fit.intercept + fit.slope * x1;
        const double y2 = fit.intercept + fit.slope * x2;
        out << "<line x1=\"" << mapX(x1) << "\" y1=\"" << mapY(y1)
            << "\" x2=\"" << mapX(x2) << "\" y2=\"" << mapY(y2)
            << "\" stroke=\"#d1495b\" stroke-width=\"2.5\" "
               "stroke-dasharray=\"7,4\"/>\n";
        out << "<text x=\"" << plotRight - 4 << "\" y=\"" << plotTop + 15
            << "\" text-anchor=\"end\" font-size=\"11\" fill=\"#a52a3a\">"
            << "OLS fit: y=" << std::setprecision(3) << fit.intercept
            << (fit.slope >= 0 ? "+" : "") << fit.slope << "x"
            << " | R²=" << fit.rSquared << " | n=" << fit.n
            << "</text>\n";
    } else {
        out << "<text x=\"" << plotRight - 4 << "\" y=\"" << plotTop + 15
            << "\" text-anchor=\"end\" font-size=\"11\" fill=\"#555\">"
            << "Regression unavailable (need at least 2 distinct x values)"
            << "</text>\n";
    }
    out << "</g>\n";
}

void writeScatterSvg(const std::string& filename,
                     const std::vector<SkillResult>& rows,
                     const std::string& chartTitle) {
    std::ofstream out(filename);
    if (!out) throw std::runtime_error("Could not create SVG file: " + filename);

    const Plot plots[] = {
        {"Completeness vs. survival", "Completeness score (0–1)",
         &SkillResult::completeness, 1.0},
        {"Example density vs. survival", "Examples per 1,000 words",
         &SkillResult::examplesPer1000Words,
         std::max(1.0, safeMax(rows, &SkillResult::examplesPer1000Words) * 1.05)},
        {"Skill length vs. survival", "Word count",
         &SkillResult::wordCount,
         std::max(1.0, safeMax(rows, &SkillResult::wordCount) * 1.05)},
        {"Readability vs. survival", "Flesch Reading Ease",
         &SkillResult::fleschReadingEase, 120.0}
    };

    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" "
           "width=\"1200\" height=\"900\" viewBox=\"0 0 1200 900\">\n";
    out << "<rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n";
    out << "<text x=\"600\" y=\"34\" text-anchor=\"middle\" "
           "font-family=\"Arial\" font-size=\"23\" font-weight=\"bold\">"
        << xmlEscape(chartTitle) << "</text>\n";

    drawScatterPlot(out, rows, plots[0], 25, 55, 565, 390);
    drawScatterPlot(out, rows, plots[1], 610, 55, 565, 390);
    drawScatterPlot(out, rows, plots[2], 25, 465, 565, 390);
    drawScatterPlot(out, rows, plots[3], 610, 465, 565, 390);
    out << "</svg>\n";
}

} // namespace

int main(int argc, char* argv[]) {
    // Toggle this to false when you want to run the survival analysis.
    bool testing = false;
    if (testing) {
        Tester::runTests();
        return 0;
    }

    // Usage:
    //   ./skill_analyzer [database] [observation_timestamp] [output_prefix]
    const std::string dbPath =
        argc > 1 ? argv[1] : "agent_skills_sample.db";
    const std::string cutoff =
        argc > 2 ? argv[2] : "2026-07-31T23:59:59Z";
    const std::string outputPrefix =
        argc > 3 ? argv[3] : "skill_survival";

    try {
        AnalysisOptions options;
        options.minSimilarityThreshold = 0.80;
        options.maxSimilarityThreshold = 0.999;
        options.observationTimestamp = cutoff;

        SkillReuseAnalyzer database(dbPath, options);
        if (!database.isOpen()) {
            std::cerr << "Could not open SQLite database: " << dbPath << '\n';
            return 1;
        }

        std::cout << "Connected to: " << dbPath << '\n';
        std::cout << "Artifacts in database: "
                  << database.getArtifactCount() << '\n';
        std::cout << "Observation cutoff: " << cutoff << "\n\n";

        SkillReadabilityAnalyzer readability;
        std::vector<SkillResult> results;

        // A zero limit means process all repositories returned by the parser.
        const auto repositories = database.getMultiSkillRepositories(0);

        for (const auto& repository : repositories) {
            const auto skills = database.getSkillsByRepository(repository);

            for (const auto& skill : skills) {
                if (skill.content.empty()) continue;

                SkillResult row;
                row.repository = skill.repoFullName;
                row.path = skill.path;

                // Survival is calculated per skill occurrence, rather than
                // only for near-match groups.
                row.survival = SkillReuseAnalyzer::calculatePersistenceScore(
                    skill.firstCommitAt,
                    skill.lastCommitAt,
                    cutoff,
                    skill.commitCount,
                    options.recencyHalfLifeDays
                );

                row.completeness = evaluateSkillCompleteness(skill.content);

                const LengthMetrics length =
                    SkillLengthEvaluator::evaluate(skill.content);
                row.wordCount = static_cast<double>(length.wordCount);

                const ExampleMetrics examples =
                    SkillExampleEvaluator::evaluate(skill.content, length.wordCount);
                row.examplesPer1000Words = examples.examplesPer1000Words;

                row.fleschReadingEase = readability.calculateScore(
                    skill.content, ReadabilityMetric::FleschReadingEase
                );

                results.push_back(std::move(row));
            }
        }

        if (results.empty()) {
            std::cout << "No skill records were available to evaluate.\n";
            return 0;
        }

        // Keep the complete dataset, and create a second view excluding
        // zero-survival records so the nonzero distribution is easier to inspect.
        std::vector<SkillResult> nonzeroResults;
        nonzeroResults.reserve(results.size());
        for (const auto& row : results) {
            if (row.survival > 0.0) nonzeroResults.push_back(row);
        }

        const std::string allCsv = outputPrefix + "_all.csv";
        const std::string allSvg = outputPrefix + "_all_scatter.svg";
        const std::string nonzeroCsv = outputPrefix + "_nonzero.csv";
        const std::string nonzeroSvg = outputPrefix + "_nonzero_scatter.svg";

        writeCsv(allCsv, results);
        writeScatterSvg(allSvg, results, "All Skills (including 0% survival)");
        writeCsv(nonzeroCsv, nonzeroResults);
        if (!nonzeroResults.empty()) {
            writeScatterSvg(nonzeroSvg, nonzeroResults,
                            "Skills with Nonzero Survival");
        }

        double totalSurvival = 0.0;
        for (const auto& row : results) totalSurvival += row.survival;

        std::cout << "Evaluated skills: " << results.size() << '\n'
                  << "Average survival rate: "
                  << std::fixed << std::setprecision(2)
                  << (100.0 * totalSurvival / results.size()) << "%\n\n";

        std::cout << "Zero-survival records: "
                  << (results.size() - nonzeroResults.size()) << '\n'
                  << "Nonzero-survival records: " << nonzeroResults.size() << '\n';
        std::cout << "\nGenerated output files:\n"
                  << "  All skills CSV: " << allCsv << '\n'
                  << "  All skills scatter plots: " << allSvg << '\n'
                  << "  Nonzero-survival CSV: " << nonzeroCsv << '\n';
        if (!nonzeroResults.empty()) {
            std::cout << "  Nonzero-survival scatter plots: " << nonzeroSvg << '\n';
        } else {
            std::cout << "  Nonzero-survival scatter plots: skipped (no nonzero records)\n";
        }

        std::cout << "\nLowest-survival skills:\n";
        std::vector<SkillResult> ranked = results;
        std::sort(ranked.begin(), ranked.end(),
                  [](const SkillResult& a, const SkillResult& b) {
                      return a.survival < b.survival;
                  });

        const std::size_t displayCount = std::min<std::size_t>(10, ranked.size());
        for (std::size_t i = 0; i < displayCount; ++i) {
            std::cout << "  " << std::setprecision(1)
                      << ranked[i].survival * 100.0 << "%  "
                      << ranked[i].repository << " : " << ranked[i].path
                      << '\n';
        }

    } catch (const std::exception& ex) {
        std::cerr << "Analysis failed: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
