// Pure-logic self-test (CTest-registered). No external deps; asserts via exit code.
// Grows as pure-logic helpers (VolumeCurve, langScore) land in later plans.
#include "util/TimeFormat.h"
#include <cstdio>
#include <cmath>

static int failures = 0;
static void check(const QString& got, const char* want)
{
    if (got != QString::fromUtf8(want)) {
        std::printf("FAIL: got '%s' want '%s'\n", got.toUtf8().constData(), want);
        ++failures;
    }
}

int main()
{
    // fmtTime (spec §7.1)
    check(fmtTime(0),               "0:00");
    check(fmtTime(65),              "1:05");
    check(fmtTime(425),             "7:05");
    check(fmtTime(3729),            "1:02:09");
    check(fmtTime(-5),              "0:00");
    check(fmtTime(std::nan("")),    "0:00");
    check(fmtTime(3600),            "1:00:00");
    check(fmtTime(59.9),            "0:59");

    std::printf(failures ? "SELFTEST FAILED (%d)\n" : "SELFTEST OK\n", failures);
    return failures ? 1 : 0;
}
