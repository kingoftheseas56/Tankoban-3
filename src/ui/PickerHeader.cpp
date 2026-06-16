// Tankoban 3 - Play Picker header (M5). See PickerHeader.h.

#include "ui/PickerHeader.h"

#include <QVBoxLayout>

namespace tankoban {

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
    col->addWidget(m_kicker);

    m_title = new QLabel(this);
    m_title->setObjectName(QStringLiteral("PickerTitle"));
    m_title->setWordWrap(true);
    m_title->setMaximumWidth(980);
    col->addWidget(m_title);

    m_overview = new QLabel(this);
    m_overview->setObjectName(QStringLiteral("PickerOverview"));
    m_overview->setWordWrap(true);
    m_overview->setMaximumWidth(680);
    col->addSpacing(8);
    col->addWidget(m_overview);

    setStyleSheet(QStringLiteral(R"qss(
QLabel#PickerKicker {
    color: #8b909b;
    font-family: "Inter", "Segoe UI", sans-serif;
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 5px;
    text-transform: uppercase;
}
QLabel#PickerTitle {
    color: #f3f1ea;
    font-family: "Fraunces", "Iowan Old Style", "Georgia", serif;
    font-size: 68px;
    font-weight: 500;
    line-height: 0.96;
}
QLabel#PickerOverview {
    color: #aab1bd;
    font-family: "Inter", "Segoe UI", sans-serif;
    font-size: 15px;
    line-height: 1.55;
}
)qss"));
}

void PickerHeader::setMeta(const QString& name, const QString& releaseInfo, const QStringList& genres)
{
    QString kicker = releaseInfo.toUpper();
    if (!genres.isEmpty()) {
        QStringList shown;
        for (int i = 0; i < genres.size() && i < 2; ++i)
            shown.push_back(genres.at(i).toUpper());
        if (!shown.isEmpty())
            kicker += (kicker.isEmpty() ? QString() : QStringLiteral("  |  ")) + shown.join(QStringLiteral("  |  "));
    }

    m_kicker->setText(kicker);
    m_kicker->setVisible(!kicker.isEmpty());
    m_title->setStyleSheet(QStringLiteral("font-size: 68px;"));
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
    m_kicker->setText(QStringLiteral("%1  |  Season %2  |  Episode %3")
                          .arg(seriesName.toUpper())
                          .arg(season)
                          .arg(episode, 2, 10, QLatin1Char('0')));
    m_kicker->setVisible(true);
    m_title->setStyleSheet(QStringLiteral("font-size: 64px;"));
    m_title->setText(title);
    m_overview->setText(overview);
    m_overview->setVisible(!overview.isEmpty());
}

} // namespace tankoban
