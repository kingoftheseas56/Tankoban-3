#include "chrome/TimeLabel.h"
#include "engine/PlaybackClock.h"
#include "util/TimeFormat.h"

#include <QFont>

#include <algorithm>

TimeLabel::TimeLabel(Mode mode, QWidget* parent) : QLabel(parent), m_mode(mode)
{
    QFont f(QStringLiteral("Consolas"));
    f.setPointSizeF(9.75);                 // Harbor .hgp-time: mono 13px
    f.setStyleHint(QFont::Monospace);
    f.setWeight(QFont::Medium);
    setFont(f);
    setAlignment(Qt::AlignCenter);
    setFixedWidth(58);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setStyleSheet(mode == Mode::Start
        ? QStringLiteral("color:rgba(255,255,255,217); background:transparent;")
        : QStringLiteral("color:rgba(255,255,255,166); background:transparent;"));
    connect(&PlaybackClock::instance(), &PlaybackClock::changed, this, &TimeLabel::refresh);
    refresh();
}

void TimeLabel::setDuration(double sec)
{
    m_duration = std::max(0.0, sec);
    refresh();
}

void TimeLabel::refresh()
{
    const double pos = PlaybackClock::instance().position();
    switch (m_mode) {
    case Mode::Start:
        setText(fmtTime(pos));
        break;
    case Mode::End:
        setText(fmtTime(m_duration));
        break;
    case Mode::Remaining:
        setText(QStringLiteral("-%1").arg(fmtTime(std::max(0.0, m_duration - pos))));
        break;
    }
}
