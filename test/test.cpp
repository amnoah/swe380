#include "test.h"

#include <iostream>

#include "impl/completeness/completeness_test.h"
#include "impl/examples/examples_test.h"
#include "impl/length/length_test.h"
#include "impl/readability/readability_test.h"
#include "impl/reuse/reuse_test.h"

using namespace std;

void Tester::runTests() {
    CompletenessTest::runAllTests();
    ExamplesTest::runAllTests();
    LengthTest::runAllTests();
    ReadabilityTest::runAllTests();
    ReuseTest::runAllTests();
}