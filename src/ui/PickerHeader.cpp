// Tankoban 3 - Play Picker header (M5). See PickerHeader.h.

#include "ui/PickerHeader.h"

#include <QFont>
#include <QVBoxLayout>

namespace tankoban {

namespace {

// Harbor uses CSS text-transform/letter-spacing/line-height. Qt Style Sheets
// support NONE of those on QLabel, so the picker typography is driven by QFont
// (size/weight/family/letter-spacing) and only colors come from the stylesheet.

QFont kickerFont()
{
    QFont f;
    f.setFamilies({QStringLiteral("Inter"), QStringLiteral("Segoe UI")});
    f.setStyleHint(QFont::SansSerif);
    f.setPixelSize(11);
    f.setWeight(QFont::DemiBold);                       // Harbor font-semibold (600)
    f.setLetterSpacing(QFont::AbsoluteSpacing, 3.5);    // Harbor tracking-[0.32em] @ 11px
    return f;
}

QFont titleFont(int px)
{
    QFont f;
    f.setFamilies({QStringLiteral("Fraunces"), QStringLiteral("Iowan Old Style"),
                   QStringLiteral("Georgia")});
    f.setStyleHint(QFont::Serif);
    f.setPixelSize(px);
    f.setWeight(QFont::Medium);                         // Harbor font-medium (500)
    return f;
}

QFont overviewFont()
{
    QFont f;
    f.setFamilies({QStringLiteral("Inter"), QStringLiteral("Segoe UI")});
    f.setStyleHint(QFont::SansSerif);
    f.setPixelSize(14);                                 // Harbor 14.5px
    return f;
}

// Harbor joins kicker segments with " · " (middot), never pipe bars.
QString joinDot(const QStringList& parts)
{
    return parts.join(QString::fromUtf8(" \xC2\xB7 "));
}

} // namespace

PickerHeader::PickerHeader(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("PickerHeader"));
    setAttribute(Qt::WA_TranslucentBackground);

    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(12);

    m_kicker = new QLabel(this);
    m_kicker->setObjectName(QStringLiteral("PickerKicker"));
    m_kicker->setTextFormat(Qt::PlainText);
    m_kicker->setFont(kickerFont());
    col->addWidget(m_kicker);

    m_title = new QLabel(this);
    m_title->setObjectName(QStringLiteral("PickerTitle"));
    m_title->setWordWrap(true);
    m_title->setMaximumWidth(980);
    m_title->setFont(titleFont(68));
    col->addWidget(m_title);

    m_overview = new QLabel(this);
    m_overview->setObjectName(QStringLiteral("PickerOverview"));
    m_overview->setWordWrap(true);
    m_overview->setMaximumWidth(672);   // Harbor max-w-2xl
    m_overview->setFont(overviewFont());
    col->addSpacing(8);
    col->addWidget(m_overview);

    // Colors only — fonts are set via QFont above.
    setStyleSheet(QStringLiteral(R"qss(
QLabel#PickerKicker { color: #8b909b; }
QLabel#PickerTitle { color: #f3f1ea; }
QLabel#PickerOverview { color: #aab1bd; }
)qss"));
}

void PickerHeader::setMeta(const QString& name, const QString& releaseInfo, const QStringList& genres)
{
    // Harbor movie kicker (picker-header.tsx 24-29): the row exists ONLY when
    // releaseInfo is present; then " · " + the first two genres, uppercased.
    QString kicker;
    if (!releaseInfo.isEmpty()) {
        QStringList parts{releaseInfo};
        for (int i = 0; i < genres.size() && i < 2; ++i)
            parts.push_back(genres.at(i));
        kicker = joinDot(parts).toUpper();
    }

    m_kicker->setText(kicker);
    m_kicker->setVisible(!kicker.isEmpty());
    m_title->setFont(titleFont(68));
    m_title->setText(name);
    m_overview->clear();
    m_overview->setVisible(false);
}

void PickerHeader::setEpisode(const QString& seriesName,
                              int season,
                              int episode,
                              const QString& episodeName,
                              const QString& overview)
{
    const QString title = episodeName.isEmpty()
        ? QStringLiteral("Episode %1").arg(episode)
        : episodeName;

    // Harbor episode kicker (picker-header.tsx 8-10):
    //   {name} · Season {season} · Episode {episode padded to 2}, uppercased.
    const QString kicker = joinDot({seriesName,
                                    QStringLiteral("Season %1").arg(season),
                                    QStringLiteral("Episode %1").arg(episode, 2, 10, QLatin1Char('0'))})
                               .toUpper();
    m_kicker->setText(kicker);
    m_kicker->setVisible(true);
    m_title->setFont(titleFont(64));
    m_title->setText(title);
    m_overview->setText(overview);
    m_overview->setVisible(!overview.isEmpty());
}

} // namespace tankoban
