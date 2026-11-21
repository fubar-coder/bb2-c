// test_basictest_readme - the examples of the result file in tests/README.md
// ("Blitz test sources and the result file"): every line of a fenced block
// marked basictest-result is read by the codec (shared/basictest) without
// an error, and written back it gives the same line.
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "basictest.h"
}

namespace {

struct example_line {
    uint32_t number;            // the line in README.md
    std::string text;
};

// the lines of the basictest-result blocks
std::vector<example_line> examples(unsigned &blocks)
{
    std::ifstream in(BASICTEST_README, std::ios::binary);
    REQUIRE(in.good());
    std::vector<example_line> lines;
    std::string line;
    uint32_t number = 0;
    bool inside = false;
    blocks = 0;
    while (std::getline(in, line)) {
        number++;
        if (!inside) {
            if (line == "```basictest-result") {
                inside = true;
                blocks++;
            }
        } else if (line == "```") {
            inside = false;
        } else {
            lines.push_back({ number, line });
        }
    }
    REQUIRE(!inside);
    return lines;
}

} // namespace

TEST_CASE("tests/README.md: the result file examples read back", "[basictest]")
{
    unsigned blocks = 0;
    auto lines = examples(blocks);
    REQUIRE(blocks >= 1u);
    REQUIRE(lines.size() >= 10u);
    std::vector<int> kinds;
    for (const auto &l : lines) {
        CAPTURE(l.number, l.text);
        std::string copy = l.text;
        basictest_entry e{};
        basictest_error err{};
        int rc = basictest_read(reinterpret_cast<uint8_t *>(copy.data()), copy.size(), l.number,
                                &e, &err);
        if (rc != BASICTEST_OK) {
            char msg[200];
            basictest_error_text(&err, msg, sizeof msg);
            FAIL_CHECK(msg);
            continue;
        }
        kinds.push_back(e.kind);
        std::vector<uint8_t> buf(1024);
        size_t n = basictest_write(&e, buf.data(), buf.size());
        REQUIRE(n > 0u);
        CHECK(std::string(reinterpret_cast<const char *>(buf.data()), n) == l.text + "\n");
    }
    // the example is a whole file: begin first, end last
    REQUIRE(!kinds.empty());
    CHECK(kinds.front() == BASICTEST_BEGIN);
    CHECK(kinds.back() == BASICTEST_END);
}
