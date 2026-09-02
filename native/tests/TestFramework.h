#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <stdexcept>

enum class TestStatus { PASS, FAIL, SKIPPED, NOT_IMPLEMENTED };

struct TestResult {
    std::string name;
    TestStatus status;
    std::string error_msg;
    double duration_ms;
};

class TestRunner {
public:
    static TestRunner& Instance() { static TestRunner instance; return instance; }

    void RegisterTest(const std::string& name, std::function<void()> func) {
        m_tests.push_back({name, func});
    }

    void SkipTest(const std::string& name, const std::string& reason) {
        m_results.push_back({name, TestStatus::SKIPPED, reason, 0.0});
    }

    void NotImplemented(const std::string& name) {
        m_results.push_back({name, TestStatus::NOT_IMPLEMENTED, "Not implemented yet", 0.0});
    }

    void FailCurrentTest(const std::string& error) {
        throw std::runtime_error(error);
    }

    int RunAll() {
        std::cout << "======================================\n";
        std::cout << " PDF Elite Native Regression Suite\n";
        std::cout << "======================================\n\n";

        int failed = 0;
        int passed = 0;
        int skipped = 0;
        int not_impl = 0;

        for (const auto& test : m_tests) {
            std::cout << "Running: " << test.name << "... "; std::cout.flush();
            auto start = std::chrono::high_resolution_clock::now();
            try {
                test.func(); std::cout << "Func returned!\n"; std::cout.flush();
                auto end = std::chrono::high_resolution_clock::now();
                double ms = std::chrono::duration<double, std::milli>(end - start).count();
                m_results.push_back({test.name, TestStatus::PASS, "", ms});
                std::cout << "PASS (" << ms << " ms)\n";
                passed++;
            } catch (const std::exception& e) {
                auto end = std::chrono::high_resolution_clock::now();
                double ms = std::chrono::duration<double, std::milli>(end - start).count();
                m_results.push_back({test.name, TestStatus::FAIL, e.what(), ms});
                std::cout << "FAIL\n  Error: " << e.what() << "\n";
                failed++;
            }
        }

        for(const auto& res : m_results) {
            if (res.status == TestStatus::SKIPPED) {
                std::cout << "Skipped: " << res.name << " (" << res.error_msg << ")\n";
                skipped++;
            } else if (res.status == TestStatus::NOT_IMPLEMENTED) {
                std::cout << "Not Impl: " << res.name << "\n";
                not_impl++;
            }
        }

        std::cout << "\n======================================\n";
        std::cout << "Results: " << passed << " passed, " << failed << " failed, " 
                  << skipped << " skipped, " << not_impl << " not implemented.\n";
        
        return failed > 0 ? 1 : 0;
    }

private:
    struct TestCase { std::string name; std::function<void()> func; };
    std::vector<TestCase> m_tests;
    std::vector<TestResult> m_results;
};

#define TEST(name) \
    void TestFunc_##name(); \
    struct TestRegister_##name { \
        TestRegister_##name() { TestRunner::Instance().RegisterTest(#name, TestFunc_##name); } \
    } test_register_##name; \
    void TestFunc_##name()

#define EXPECT_TRUE(cond) if (!(cond)) TestRunner::Instance().FailCurrentTest("EXPECT_TRUE failed: " #cond)
#define EXPECT_FALSE(cond) if (cond) TestRunner::Instance().FailCurrentTest("EXPECT_FALSE failed: " #cond)
#define EXPECT_EQ(val1, val2) if ((val1) != (val2)) TestRunner::Instance().FailCurrentTest("EXPECT_EQ failed: " #val1 " != " #val2)
#define EXPECT_NE(val1, val2) if ((val1) == (val2)) TestRunner::Instance().FailCurrentTest("EXPECT_NE failed: " #val1 " == " #val2)
#define EXPECT_GT(val1, val2) if (!((val1) > (val2))) TestRunner::Instance().FailCurrentTest("EXPECT_GT failed: " #val1 " > " #val2)
#define EXPECT_LT(val1, val2) if (!((val1) < (val2))) TestRunner::Instance().FailCurrentTest("EXPECT_LT failed: " #val1 " < " #val2)
#define EXPECT_GE(val1, val2) if (!((val1) >= (val2))) TestRunner::Instance().FailCurrentTest("EXPECT_GE failed: " #val1 " >= " #val2)
#define EXPECT_LE(val1, val2) if (!((val1) <= (val2))) TestRunner::Instance().FailCurrentTest("EXPECT_LE failed: " #val1 " <= " #val2)
#define EXPECT_NEAR(val1, val2, eps) if (std::abs((val1) - (val2)) > (eps)) TestRunner::Instance().FailCurrentTest("EXPECT_NEAR failed: " #val1 " ~= " #val2)
#define EXPECT_FLOAT_EQ(val1, val2) if (std::abs((val1) - (val2)) > 1e-4f) TestRunner::Instance().FailCurrentTest("EXPECT_FLOAT_EQ failed: " #val1 " == " #val2)
