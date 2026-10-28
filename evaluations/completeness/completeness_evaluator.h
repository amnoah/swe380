#ifndef SWE380_COMPLETENESS_EVALUATOR_H
#define SWE380_COMPLETENESS_EVALUATOR_H

#include <string>

/**
 * @brief Evaluates structural completeness of a skill specification.
 * @param skill Content string of the skill file (e.g. SKILL.md).
 * @return Double precision score between 0.00 (incomplete) and 1.00 (fully complete).
 */
double evaluateSkillCompleteness(const std::string& skill);

#endif //SWE380_COMPLETENESS_EVALUATOR_H