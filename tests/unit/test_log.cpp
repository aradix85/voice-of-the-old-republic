// log: what reaches the session file for Write, Trace, Once, Edge and
// BlockLog. The log keeps process-wide state, so this is one sequence.

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "doctest.h"
#include "log.h"

namespace {

namespace fs = std::filesystem;

// The newest session file: Init puts it in the "logs" folder beside the
// folder of the module, which here is the test executable.
fs::path NewestLog() {
    const fs::path dir = fs::path(acclog::PatchDir()).parent_path() / "logs";
    fs::path newest;
    fs::file_time_type newestTime{};
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (newest.empty() || entry.last_write_time() > newestTime) {
            newest     = entry.path();
            newestTime = entry.last_write_time();
        }
    }
    return newest;
}

// "Tag: text" of every line whose tag starts with "T.", in file order.
std::vector<std::string> TestLines(const fs::path& file) {
    std::vector<std::string> out;
    std::ifstream in(file, std::ios::binary);
    const std::string marker = "[accessibility] ";
    for (std::string line; std::getline(in, line);) {
        const size_t at = line.find(marker);
        if (at == std::string::npos) continue;
        std::string rest = line.substr(at + marker.size());
        if (rest.rfind("T.", 0) == 0) out.push_back(rest);
    }
    return out;
}

bool StartsWith(const std::string& s, const char* prefix) {
    return s.rfind(prefix, 0) == 0;
}

}  // namespace

TEST_CASE("The log writes, folds repeats and reports what it folded") {
    acclog::Init(nullptr);
    REQUIRE(acclog::PatchDir()[0] != '\0');

    acclog::Write("T.write", "plain %d", 1);

    // Trace: the same content again is counted, not written.
    acclog::Trace("T.trace", "same");
    acclog::Trace("T.trace", "same");
    acclog::Trace("T.trace", "same");
    acclog::Trace("T.trace", "other");

    // Once: only the first line for a tag.
    acclog::Once("T.once", "first");
    acclog::Once("T.once", "second");

    // Edge: a line per change of state, with how long the last one held.
    acclog::Edge("T.edge", 1, "on");
    acclog::Edge("T.edge", 1, "on again");
    acclog::Edge("T.edge", 1, "on again");
    acclog::Edge("T.edge", 0, "off");

    // BlockLog: an identical block folds, a different one prints in full.
    for (int i = 0; i < 2; ++i) {
        acclog::BlockLog b("T.block");
        b.Line("head");
        b.Line("  row %d", 7);
    }
    {
        acclog::BlockLog b("T.block");
        b.Line("head");
        b.Line("  row %d", 8);
    }

    // With Key(), identity is the key stream: differing display text folds.
    for (int i = 0; i < 2; ++i) {
        acclog::BlockLog b("T.keyed");
        b.Line("panel at %d", i);
        b.Key("panel");
    }

    // A pending count surfaces by itself once its tag has been quiet for a
    // second, on the next call of any kind.
    acclog::Trace("T.stale", "tick");
    acclog::Trace("T.stale", "tick");
    Sleep(1100);
    acclog::Write("T.after", "later");

    // Shutdown reports what is still pending.
    acclog::Trace("T.tail", "end");
    acclog::Trace("T.tail", "end");
    acclog::Shutdown();

    const std::vector<std::string> lines = TestLines(NewestLog());
    const std::vector<std::string> expectedStart = {
        "T.write: plain 1",
        "T.trace: same",
        "T.trace: (repeated 2x more)",
        "T.trace: other",
        "T.once: first",
        "T.edge: on",
        "T.edge: (prev state held 2x)",
        "T.edge: off",
        "T.block: head",
        "T.block:   row 7",
    };
    REQUIRE(lines.size() == 20);
    CHECK(std::equal(expectedStart.begin(), expectedStart.end(), lines.begin()));
    CHECK(StartsWith(lines[10], "T.block: (repeated 1x"));
    CHECK(lines[10].find("unchanged: head)") != std::string::npos);
    CHECK(lines[11] == "T.block: head");
    CHECK(lines[12] == "T.block:   row 8");
    CHECK(lines[13] == "T.keyed: panel at 0");
    CHECK(lines[14] == "T.stale: tick");
    // The stale sweep runs before the line that triggered it; the keyed
    // block's fold is as old and surfaces in the same sweep.
    CHECK(lines[15] == "T.stale: (repeated 1x more)");
    CHECK(StartsWith(lines[16], "T.keyed: (repeated 1x"));
    CHECK(lines[17] == "T.after: later");
    CHECK(lines[18] == "T.tail: end");
    CHECK(lines[19] == "T.tail: (repeated 1x more)");
}
