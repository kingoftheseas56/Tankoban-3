// Tankoban 3 - Play Picker source row (M6).
// Qt translation of Harbor's src/views/play-picker/stremio-row.tsx: one ranked
// stream rendered as an elevated row with an addon tile, headline/description,
// format badge chips, copy-link, and a circular play button. Unsupported rows
// (hash-only / external / yt / nzb / "#") are visible but not playable.
#pragma once

#include <QWidget>

#include "core/StreamModels.h"

class QLabel;
class QHBoxLayout;
class QPushButton;

namespace tankoban {

class StremioRow : public QWidget {
    Q_OBJECT
public:
    explicit StremioRow(QWidget* parent = nullptr);

    void setStream(const ScoredStream& stream,
                   bool failed = false,
                   const QString& addonLogo = QString());

signals:
    void playRequested(const ScoredStream& stream);

private:
    void rebuildBadges(const QStringList& labels);

    ScoredStream m_stream;
    QString m_copyLink;

    QLabel* m_tile = nullptr;
    QLabel* m_headline = nullptr;
    QLabel* m_description = nullptr;
    QWidget* m_badgeRow = nullptr;
    QHBoxLayout* m_badgeLayout = nullptr;
    QLabel* m_status = nullptr;     // failed / unsupported reason
    QPushButton* m_copy = nullptr;
    QPushButton* m_play = nullptr;
    bool m_copied = false;
};

} // namespace tankoban
