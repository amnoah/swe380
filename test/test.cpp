#include "test.h"

#include <iostream>
#include "impl/readability/readability_test.h"
#include "impl/reuse/reuse_test.h"

using namespace std;

void Tester::runTests() {
    ReadabilityTest::runAllTests();
    ReuseTest::runAllTests();
}