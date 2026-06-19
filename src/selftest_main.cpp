// Pure-logic self-test (CTest-registered). No external deps; asserts via exit code.
// Grows as pure-logic helpers (VolumeCurve, langScore) land in later plans.
#include "util/TimeFormat.h"
#include "util/VolumeCurve.h"
#include "ui/StreamRowPaint.h"
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

static void checkNear(double got, double want, const char* label)
{
    if (std::fabs(got - want) > 1e-6) {
        std::printf("FAIL: %s got %.6f want %.6f\n", label, got, want);
        ++failures;
    }
}

static void checkBool(bool got, bool want, const char* label)
{
    if (got != want) {
        std::printf("FAIL: %s got %d want %d\n", label, got, want);
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

    // Harbor volume curve (default shell slider).
    checkNear(volume_curve::fractionFromValue(1.0), 0.6, "fractionFromValue(1)");
    checkNear(volume_curve::valueFromFraction(0.6), 1.0, "valueFromFraction(0.6)");
    checkNear(volume_curve::fractionFromValue(6.0), 1.0, "fractionFromValue(6)");
    checkNear(volume_curve::valueFromFraction(1.0), 6.0, "valueFromFraction(1)");

    // ---- StreamRowPaint (sources-list rebuild) ----
    {
        using namespace tankoban;
        ScoredStream a;
        a.addonId = "torrentio";
        a.infoHash = "abc";
        a.fileIdx = 0;
        check(streamRowKey(a), "torrentio|h:abc:0");

        ScoredStream u;
        u.addonId = "comet";
        u.url = "http://x/y.mkv";
        check(streamRowKey(u), "comet|u:http://x/y.mkv");

        ScoredStream q;
        q.resolution = Resolution::FullHD;
        check(qualityGroupOf(q), "1080p");
        q.resolution = Resolution::None;
        check(qualityGroupOf(q), "SD");

        // Unlabeled -> NO LABEL. Must set the Other sentinels: defaults are Unknown,
        // which qualityConfidence does NOT treat as "nothing detected".
        ScoredStream bare;
        bare.source = Source::Other;
        bare.codec = Codec::Other;
        bare.audio.codec = AudioCodec::Other;
        check(streamBadges(bare).join("|"), "NO LABEL");

        // 1080p HEVC ATMOS -> ordered chips (size>0 keeps confidence Labeled).
        ScoredStream rich;
        rich.resolution = Resolution::FullHD;
        rich.codec = Codec::HEVC;
        rich.audio.codec = AudioCodec::Atmos;
        rich.size = 5;
        check(streamBadges(rich).join("|"), "1080p|HEVC|ATMOS");

        ScoredStream tor;
        tor.infoHash = "deadbeef";
        checkBool(isPlayable(tor), true, "torrent playable");
        ScoredStream dead;
        dead.url = "#";
        checkBool(isPlayable(dead), false, "placeholder not playable");
    }

    std::printf(failures ? "SELFTEST FAILED (%d)\n" : "SELFTEST OK\n", failures);
    return failures ? 1 : 0;
}
