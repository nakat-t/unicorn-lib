#include "unicorn/normal.hpp"
#include "unicorn/character.hpp"
#include "unicorn/format.hpp"
#include "unicorn/string.hpp"
#include "unicorn/ucd-tables.hpp"
#include "unicorn/unit-test.hpp"
#include <algorithm>
#include <future>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

using namespace RS;
using namespace RS::Unicorn;
using namespace RS::Unicorn::Literals;
using namespace RS::Literals;

namespace {

    using UnicornDetail::normalization_identity_table;
    using UnicornDetail::normalization_test_table;

    void norm_test(const Strings& u8data, size_t line, NormalizationForm form, size_t i, size_t j) {
        auto& orig(u8data[i]);
        auto& expect(u8data[j]);
        Ustring norm;
        TRY(norm = normalize(orig, form));
        TEST_EQUAL(norm, expect);
        if (norm != expect)
            FAIL("Failed normalization test $1: $2 $3q => $4q"_fmt(line, form, orig, expect));
    }

    void do_main_tests(size_t b, size_t e) {
        e = std::min(e, range_count(normalization_test_table));
        b = std::min(b, e);
        Strings hexcodes;
        std::vector<std::u32string> u32data;
        auto row_iter = std::begin(normalization_test_table) + b;
        for (size_t line = b; line < e; ++line) {
            auto& row = *row_iter++;
            u32data.clear();
            for (auto&& field: row) {
                u32data.push_back(std::u32string());
                str_split(Ustring(field), overwrite(hexcodes));
                for (auto&& hc: hexcodes)
                    u32data.back() += char32_t(strtoul(hc.data(), nullptr, 16));
            }
            TEST_EQUAL(u32data.size(), 5);
            if (u32data.size() == 5) {
                Strings u8data;
                for (auto& u: u32data)
                    u8data.push_back(to_utf8(u));
                norm_test(u8data, line, NFC, 0, 1);
                norm_test(u8data, line, NFC, 1, 1);
                norm_test(u8data, line, NFC, 2, 1);
                norm_test(u8data, line, NFC, 3, 3);
                norm_test(u8data, line, NFC, 4, 3);
                norm_test(u8data, line, NFD, 0, 2);
                norm_test(u8data, line, NFD, 1, 2);
                norm_test(u8data, line, NFD, 2, 2);
                norm_test(u8data, line, NFD, 3, 4);
                norm_test(u8data, line, NFD, 4, 4);
                norm_test(u8data, line, NFKC, 0, 3);
                norm_test(u8data, line, NFKC, 1, 3);
                norm_test(u8data, line, NFKC, 2, 3);
                norm_test(u8data, line, NFKC, 3, 3);
                norm_test(u8data, line, NFKC, 4, 3);
                norm_test(u8data, line, NFKD, 0, 4);
                norm_test(u8data, line, NFKD, 1, 4);
                norm_test(u8data, line, NFKD, 2, 4);
                norm_test(u8data, line, NFKD, 3, 4);
                norm_test(u8data, line, NFKD, 4, 4);
            }
        }
    }

    void do_identity_tests(const std::vector<char32_t>& uchars, size_t b, size_t e) {
        e = std::min(e, uchars.size());
        b = std::min(b, e);
        Ustring s;
        std::u32string us;
        for (size_t i = b; i < e; ++i) {
            us = {uchars[i]};
            s = to_utf8(us);
            TEST_EQUAL(normalize(s, NFC), s);
            TEST_EQUAL(normalize(s, NFD), s);
            TEST_EQUAL(normalize(s, NFKC), s);
            TEST_EQUAL(normalize(s, NFKD), s);
        }
    }

}

void test_unicorn_normal_normalization() {

    auto norm_tests = range_count(normalization_test_table);
    std::vector<char32_t> identity_chars;
    for (auto&& row: normalization_identity_table)
        for (char32_t u = row.key; u <= row.value; ++u)
            identity_chars.push_back(u);

    #ifdef __CYGWIN__

        // Multithreaded version seems to have problems on Cygwin
        do_main_tests(0, norm_tests);
        do_identity_tests(identity_chars, 0, 10000);

    #else

        auto thread_count = std::thread::hardware_concurrency();
        auto per_thread = norm_tests / thread_count + 1;
        std::vector<std::future<void>> threads;
        for (size_t base = 0; base < norm_tests; base += per_thread) {
            auto call = [=] { do_main_tests(base, base + per_thread); };
            threads.push_back(std::async(call));
        }
        for (auto& t: threads)
            t.wait();
        threads.clear();
        per_thread = identity_chars.size() / thread_count + 1;
        for (size_t base = 0; base < identity_chars.size(); base += per_thread) {
            auto call = [=] { do_identity_tests(identity_chars, base, base + per_thread); };
            threads.push_back(std::async(call));
        }
        for (auto& t: threads)
            t.wait();

    #endif

}
