// Tankoban 3 - Play Picker source list (M6).
// Qt translation of Harbor's src/views/play-picker/stremio-layout.tsx source-list
// lane: an addon/source filter, a quality filter, the ranked StremioRow list, and a
// pending-addons pill. Preserves StreamScorer ranking order by default; does not
// re-score or re-order. Fed by PlayPickerPage::setPicker(RankedPicker.all).
#pragma once

#include <QHash>
#include <QVector>
#include <QWidget>

#include "core/StreamModels.h"

class QHBoxLayout;
class QLabel;
class QPushButton;
class QVBoxLayout;

namespace tankoban {

class SourceFilterProxy;
class SourceListModel;
class SourceListView;
class SourceRowDelegate;

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
    // Source rows live in a Qt model/view stack: a data-only model, a ranked
    // addon/quality proxy, and a virtualized QListView painted by SourceRowDelegate.
    // Only on-screen rows are ever painted -> no per-source widgets, no synchronous
    // 500-row rebuild, no shimmer while partials stream in.
    void rebuildQualityBar();
    void refreshAddonButton();
    void showAddonMenu();
    void updatePending();

    QVector<ScoredStream> m_streams;
    Options m_options;
    QString m_addonFilter = QStringLiteral("all");
    QString m_qualityFilter = QStringLiteral("all");

    QPushButton* m_addonButton = nullptr;
    QLabel* m_addonCircle = nullptr;
    QLabel* m_addonLabel = nullptr;
    QWidget* m_qualityBar = nullptr;
    QHBoxLayout* m_qualityLayout = nullptr;
    QWidget* m_pending = nullptr;
    QLabel* m_pendingLabel = nullptr;

    // Native model/view source list: data-only model -> ranked addon/quality proxy ->
    // virtualized QListView painted by the delegate.
    SourceListModel* m_model = nullptr;
    SourceFilterProxy* m_proxy = nullptr;
    SourceListView* m_view = nullptr;
    SourceRowDelegate* m_delegate = nullptr;
};

} // namespace tankoban
