// Tankoban 3 - Play Picker source row (M6). See StremioRow.h.

#include "ui/StremioRow.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSet>
#include <QStyle>
#include <QSvgRenderer>
#include <QTimer>
#include <QVBoxLayout>

namespace tankoban {

namespace {

// lucide-style inner SVG paths (Icons.h has no play/copy glyphs and is out of M6
// scope, so M6 renders the few icons it needs locally via QSvgRenderer).
const QString kCopyPath = QStringLiteral(
    "<rect width='14' height='14' x='8' y='8' rx='2' ry='2'/>"
    "<path d='M4 16c-1.1 0-2-.9-2-2V4c0-1.1.9-2 2-2h10c1.1 0 2 .9 2 2'/>");
const QString kCheckPath = QStringLiteral("<path d='M20 6 9 17l-5-5'/>");
const QString kPlayPolygon = QStringLiteral("<polygon points='8 5 19 12 8 19'/>");

QIcon renderIcon(const QString& inner, const QColor& color, int px, bool filled)
{
    const QString svg = filled
        ? QStringLiteral("<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' "
                         "fill='%1' stroke='none'>%2</svg>").arg(color.name(), inner)
        : QStringLiteral("<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' "
                         "fill='none' stroke='%1' stroke-width='2' stroke-linecap='round' "
                         "stroke-linejoin='round'>%2</svg>").arg(color.name(), inner);
    QSvgRenderer renderer(svg.toUtf8());
    const int hp = px * 2;
    QPixmap pm(hp, hp);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    renderer.render(&p);
    p.end();
    pm.setDevicePixelRatio(2.0);
    return QIcon(pm);
}

// --- Badge selection: 1:1 with Harbor format-badge.tsx streamBadges()/qualityConfidence(),
// rendered as text chips (M6 does not import Harbor's image badge pack). ---

enum class Confidence { Labeled, Unverified, Unlabeled };

Confidence qualityConfidence(const ScoredStream& s)
{
    const bool nothingDetected =
        (s.resolution == Resolution::SD || s.resolution == Resolution::None)
        && s.source == Source::Other && s.codec == Codec::Other
        && s.hdrFormat == HdrFormat::None && s.audio.codec == AudioCodec::Other;
    if (nothingDetected)
        return Confidence::Unlabeled;

    for (const ScoreReason& r : s.reasons) {
        const QString& sig = r.signal;
        if (sig.startsWith(QStringLiteral("fresh-fake-"))
            || sig == QStringLiteral("fresh-soft-flag")
            || sig == QStringLiteral("fresh-prerelease-soft")
            || sig == QStringLiteral("fresh-prebluray-suspect")
            || sig == QStringLiteral("size-mismatch")
            || sig == QStringLiteral("title-says-hires-filename-says-cam")) {
            return Confidence::Unverified;
        }
    }

    const bool claimsHighRes =
        s.resolution == Resolution::FourK || s.resolution == Resolution::FullHD;
    const bool directStream = !s.url.isEmpty() && s.infoHash.isEmpty();
    if (claimsHighRes && s.size <= 0 && s.source == Source::Other && !directStream)
        return Confidence::Unverified;
    return Confidence::Labeled;
}

QStringList streamBadges(const ScoredStream& s)
{
    QStringList out;

    // 1) capture source overrides resolution. NOTE: the M6 prompt adds SCR here;
    //    Harbor's sourceBadge() omits SCR (it falls through to resolution). Kept as
    //    an additive, honest chip since the scorer already tiers SCR as ROUGH.
    QString capture;
    if (s.source == Source::CAM)
        capture = QStringLiteral("CAM");
    else if (s.source == Source::TS || s.source == Source::HDTS)
        capture = QStringLiteral("TS");
    else if (s.source == Source::TC)
        capture = QStringLiteral("TC");
    else if (s.source == Source::SCR)
        capture = QStringLiteral("SCR");

    const Confidence conf = qualityConfidence(s);
    if (!capture.isEmpty()) {
        out << capture;
    } else if (conf == Confidence::Unlabeled) {
        out << QStringLiteral("NO LABEL");
    } else if (conf == Confidence::Unverified) {
        out << QStringLiteral("UNKNOWN");
    } else {
        switch (s.resolution) {
        case Resolution::FourK: out << QStringLiteral("4K"); break;
        case Resolution::FullHD: out << QStringLiteral("1080p"); break;
        case Resolution::HD: out << QStringLiteral("720p"); break;
        case Resolution::SD_480:
            out << (s.source == Source::DVDRip ? QStringLiteral("DVD") : QStringLiteral("480p"));
            break;
        case Resolution::SD:
            out << (s.source == Source::DVDRip ? QStringLiteral("DVD") : QStringLiteral("SD"));
            break;
        case Resolution::None: break;
        }
    }

    // 2) release/source badge, only when no capture source.
    if (capture.isEmpty()) {
        if (s.remux)
            out << QStringLiteral("REMUX");
        else if (s.source == Source::BluRay || s.source == Source::BDRip)
            out << QStringLiteral("BluRay");
        else if (s.source == Source::WEB_DL || s.source == Source::WEBRip
                 || s.source == Source::HDRip)
            out << QStringLiteral("WEB-DL");
        else if (s.source == Source::HDTV)
            out << QStringLiteral("HDTV");
    }

    // 3) codec.
    if (s.codec == Codec::HEVC)
        out << QStringLiteral("HEVC");
    else if (s.codec == Codec::AV1)
        out << QStringLiteral("AV1");

    // 4) HDR.
    if (s.hdrFormat == HdrFormat::DV || s.hdrFormat == HdrFormat::DV_HDR10)
        out << QStringLiteral("DV");
    else if (s.hdrFormat == HdrFormat::HDR10Plus)
        out << QStringLiteral("HDR10+");
    else if (s.hdrFormat == HdrFormat::HDR10)
        out << QStringLiteral("HDR10");
    else if (s.hdrFormat == HdrFormat::HLG)
        out << QStringLiteral("HLG");

    // 5) audio.
    switch (s.audio.codec) {
    case AudioCodec::Atmos: out << QStringLiteral("ATMOS"); break;
    case AudioCodec::TrueHD: out << QStringLiteral("TRUEHD"); break;
    case AudioCodec::DTS_HD_MA: out << QStringLiteral("DTS-HD"); break;
    case AudioCodec::DTS: out << QStringLiteral("DTS"); break;
    case AudioCodec::DDPlus: out << QStringLiteral("DD+"); break;
    case AudioCodec::FLAC: out << QStringLiteral("FLAC"); break;
    case AudioCodec::AAC: out << QStringLiteral("AAC"); break;
    default:
        if (s.audio.channels == 1)
            out << QStringLiteral("MONO");
        break;
    }
    return out;
}

const QString kRowQss = QStringLiteral(R"qss(
QWidget#StremioRow { background: rgba(26,29,36,0.40); border: 1px solid rgba(255,255,255,0.05); border-radius: 16px; }
QWidget#StremioRow[failed="true"] { background: rgba(229,72,77,0.07); border: 1px solid rgba(229,72,77,0.40); }
QLabel#RowTile {
    background: #1a1d24; border: 1px solid rgba(255,255,255,0.06); border-radius: 12px;
    color: #aab1bd; font-family:"Inter","Segoe UI",sans-serif; font-size: 22px; font-weight: 700;
}
QLabel#RowHeadline { color: #f3f1ea; font-family:"Inter","Segoe UI",sans-serif; font-size: 16px; font-weight: 600; }
QLabel#RowDesc { color: #aab1bd; font-family:"Inter","Segoe UI",sans-serif; font-size: 14px; }
QLabel#RowStatus { color: #e5484d; font-family:"Inter","Segoe UI",sans-serif; font-size: 13px; font-weight: 600; }
QLabel#RowStatus[muted="true"] { color: #8b909b; }
QLabel#RowBadge {
    color: #aab1bd; background: rgba(18,19,23,0.85); border: 1px solid rgba(255,255,255,0.06);
    border-radius: 7px; font-family:"Inter","Segoe UI",sans-serif; font-size: 11px; font-weight: 600; padding: 2px 7px;
}
QLabel#RowBadge[danger="true"] { color: #e58a8d; border: 1px solid rgba(229,72,77,0.45); }
QPushButton#RowCopy { background: transparent; border: none; border-radius: 8px; }
QPushButton#RowCopy:hover { background: rgba(18,19,23,0.6); }
QPushButton#RowPlay { background: #e8b923; border: none; border-radius: 32px; }
QPushButton#RowPlay:hover { background: rgba(232,185,35,0.90); }
QPushButton#RowPlay[playable="false"] { background: rgba(26,29,36,0.70); border: 1px solid rgba(255,255,255,0.06); }
)qss");

} // namespace

StremioRow::StremioRow(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("StremioRow"));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(kRowQss);
    // A row never compresses below its content (56px tile / 64px play + padding);
    // without this the layout squeezes rows flat when the list overflows a
    // not-yet-scrollable column (scroll lands with real data in M9).
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    setMinimumHeight(96);

    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(20, 20, 20, 20);
    row->setSpacing(20);

    auto* logoCol = new QWidget(this);
    logoCol->setFixedWidth(68);
    auto* logoLay = new QVBoxLayout(logoCol);
    logoLay->setContentsMargins(0, 0, 0, 0);
    logoLay->addStretch();
    m_tile = new QLabel(logoCol);
    m_tile->setObjectName(QStringLiteral("RowTile"));
    m_tile->setFixedSize(56, 56);
    m_tile->setAlignment(Qt::AlignCenter);
    logoLay->addWidget(m_tile, 0, Qt::AlignHCenter);
    logoLay->addStretch();
    row->addWidget(logoCol);

    auto* content = new QWidget(this);
    auto* cl = new QVBoxLayout(content);
    cl->setContentsMargins(0, 0, 0, 0);
    cl->setSpacing(6);
    cl->addStretch();
    m_headline = new QLabel(content);
    m_headline->setObjectName(QStringLiteral("RowHeadline"));
    m_headline->setWordWrap(true);
    cl->addWidget(m_headline);
    m_description = new QLabel(content);
    m_description->setObjectName(QStringLiteral("RowDesc"));
    m_description->setWordWrap(true);
    cl->addWidget(m_description);
    m_badgeRow = new QWidget(content);
    m_badgeLayout = new QHBoxLayout(m_badgeRow);
    m_badgeLayout->setContentsMargins(0, 2, 0, 0);
    m_badgeLayout->setSpacing(6);
    cl->addWidget(m_badgeRow);
    m_status = new QLabel(content);
    m_status->setObjectName(QStringLiteral("RowStatus"));
    cl->addWidget(m_status);
    cl->addStretch();
    row->addWidget(content, 1);

    auto* actions = new QWidget(this);
    auto* al = new QHBoxLayout(actions);
    al->setContentsMargins(0, 0, 0, 0);
    al->setSpacing(8);
    m_copy = new QPushButton(actions);
    m_copy->setObjectName(QStringLiteral("RowCopy"));
    m_copy->setFixedSize(36, 36);
    m_copy->setIconSize(QSize(16, 16));
    m_copy->setCursor(Qt::PointingHandCursor);
    al->addWidget(m_copy, 0, Qt::AlignVCenter);
    m_play = new QPushButton(actions);
    m_play->setObjectName(QStringLiteral("RowPlay"));
    m_play->setFixedSize(64, 64);
    m_play->setIconSize(QSize(26, 26));
    al->addWidget(m_play, 0, Qt::AlignVCenter);
    row->addWidget(actions, 0, Qt::AlignVCenter);

    connect(m_play, &QPushButton::clicked, this, [this]() {
        const bool playable = (!m_stream.url.isEmpty() && m_stream.url != QStringLiteral("#"))
                              || !m_stream.infoHash.isEmpty();   // torrents stream via StreamEngine
        if (playable)
            emit playRequested(m_stream);
    });
    connect(m_copy, &QPushButton::clicked, this, [this]() {
        if (m_copyLink.isEmpty())
            return;
        QGuiApplication::clipboard()->setText(m_copyLink);
        m_copied = true;
        m_copy->setIcon(renderIcon(kCheckPath, QColor(0xe8, 0xb9, 0x23), 16, false));
        QTimer::singleShot(1200, this, [this]() {
            m_copied = false;
            m_copy->setIcon(renderIcon(kCopyPath, QColor(0x8b, 0x90, 0x9b), 16, false));
        });
    });
}

void StremioRow::rebuildBadges(const QStringList& labels)
{
    QLayoutItem* item = nullptr;
    while ((item = m_badgeLayout->takeAt(0)) != nullptr) {
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }
    static const QSet<QString> dangerLabels = {
        QStringLiteral("CAM"), QStringLiteral("TS"), QStringLiteral("TC"),
        QStringLiteral("SCR"), QStringLiteral("NO LABEL"), QStringLiteral("UNKNOWN")};
    for (const QString& label : labels) {
        auto* chip = new QLabel(label, m_badgeRow);
        chip->setObjectName(QStringLiteral("RowBadge"));
        if (dangerLabels.contains(label))
            chip->setProperty("danger", true);
        m_badgeLayout->addWidget(chip);
    }
    m_badgeLayout->addStretch();
    m_badgeRow->setVisible(!labels.isEmpty());
}

void StremioRow::setStream(const ScoredStream& stream, bool failed, const QString& addonLogo)
{
    m_stream = stream;
    Q_UNUSED(addonLogo); // M6: no network addon logos; initial-letter tile only.

    setProperty("failed", failed);
    style()->unpolish(this);
    style()->polish(this);

    const QString addonName = !stream.addonName.isEmpty() ? stream.addonName : stream.addonId;
    const QString trimmedAddon = addonName.trimmed();
    m_tile->setText(trimmedAddon.isEmpty() ? QStringLiteral("?")
                                           : trimmedAddon.left(1).toUpper());

    const QString headline = !stream.name.trimmed().isEmpty() ? stream.name.trimmed() : addonName;
    m_headline->setText(headline);

    const QString desc = !stream.title.trimmed().isEmpty() ? stream.title.trimmed()
                                                           : stream.description.trimmed();
    m_description->setText(desc);
    m_description->setVisible(!desc.isEmpty());

    rebuildBadges(streamBadges(stream));

    const bool hasDirectUrl = !stream.url.isEmpty() && stream.url != QStringLiteral("#");
    const bool isTorrent = !stream.infoHash.isEmpty();
    const bool playable = hasDirectUrl || isTorrent;   // torrents now stream via StreamEngine

    m_copyLink = hasDirectUrl ? stream.url
                              : (!stream.externalUrl.isEmpty() ? stream.externalUrl : QString());
    m_copy->setVisible(!m_copyLink.isEmpty());
    m_copied = false;
    m_copy->setIcon(renderIcon(kCopyPath, QColor(0x8b, 0x90, 0x9b), 16, false));

    m_play->setProperty("playable", playable);
    style()->unpolish(m_play);
    style()->polish(m_play);
    m_play->setEnabled(playable);
    m_play->setCursor(playable ? Qt::PointingHandCursor : Qt::ArrowCursor);
    m_play->setIcon(renderIcon(kPlayPolygon,
                               playable ? QColor(0x12, 0x13, 0x17) : QColor(0x8b, 0x90, 0x9b),
                               26, true));

    QString status;
    bool muted = true;
    if (failed) {
        status = QStringLiteral("Unavailable, try another.");
        muted = false;
    } else if (isTorrent && !hasDirectUrl) {
        status = QStringLiteral("Torrent · P2P stream");
    } else if (!playable) {
        if (stream.url == QStringLiteral("#"))
            status = QStringLiteral("Configure addon");
        else if (!stream.externalUrl.isEmpty())
            status = QStringLiteral("External source");
        else if (!stream.ytId.isEmpty())
            status = QStringLiteral("YouTube source");
        else if (!stream.nzbUrl.isEmpty())
            status = QStringLiteral("NZB source");
    }
    m_status->setText(status);
    m_status->setProperty("muted", muted);
    style()->unpolish(m_status);
    style()->polish(m_status);
    m_status->setVisible(!status.isEmpty());
}

} // namespace tankoban
