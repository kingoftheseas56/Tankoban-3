// Tankoban 3 - Play Picker source list (M6).
// Qt translation of Harbor's src/views/play-picker/stremio-layout.tsx source-list
// lane: an addon/source filter, a quality filter, the ranked StremioRow list, and a
// pending-addons pill. Preserves StreamScorer ranking order by default; does not
// re-score or re-order. Fed by PlayPickerPage::setPicker(RankedPicker.all).
#pragma once

#include <QVector>
#include <QWidget>

#include "core/StreamModels.h"

class QHBoxLayout;
class QLabel;
class QPushButton;
class QVBoxLayout;

namespace tankoban {

class StreamList : public QWidget {
    Q_OBJECT
public:
    struct Options {
        bool pipelineDone = true;
        int loadingAddonCount = 0;
        bool preserveOrder = true; // default: keep StreamScorer ranking order
    };

    explicit StreamList(QWidget* parent = nullptr);

    void setStreams(const QVector<ScoredStream>& streams, const Options& options = Options{});
    void clearStreams();

signals:
    void streamActivated(const ScoredStream& stream);

private:
    void rebuildRows();
    void rebuildQualityBar();
    void refreshAddonButton();
    void showAddonMenu();
    void updatePending();
    QVector<ScoredStream> visibleStreams() const;

    QVector<ScoredStream> m_streams;
    Options m_options;
    QString m_addonFilter = QStringLiteral("all");
    QString m_qualityFilter = QStringLiteral("all");

    QPushButton* m_addonButton = nullptr;
    QLabel* m_addonCircle = nullptr;
    QLabel* m_addonLabel = nullptr;
    QWidget* m_qualityBar = nullptr;
    QHBoxLayout* m_qualityLayout = nullptr;
    QWidget* m_rowsContainer = nullptr;
    QVBoxLayout* m_rowsLayout = nullptr;
    QWidget* m_pending = nullptr;
    QLabel* m_pendingLabel = nullptr;
};

} // namespace tankoban
