// Tankoban 3 - Play Picker page shell (M5).
#pragma once

#include <optional>

#include <QWidget>

#include "core/MetaDetail.h"
#include "core/StreamModels.h"

class QFrame;
class QLabel;
class QPushButton;
class QScrollArea;
class QWidget;

namespace tankoban {

class BackdropLayer;
class MetaService;
class PickerHeader;
class StreamList;

class PlayPickerPage : public QWidget {
    Q_OBJECT
public:
    explicit PlayPickerPage(QWidget* parent = nullptr);

    void open(const MetaDetail& meta, std::optional<EpisodeItem> episode);

    // M6: feed the source list. setPicker shows the ranked StremioRow list; setLoading
    // shows the M5 loading placeholder. Real ranked data is wired in a later milestone (M9).
    void setPicker(const RankedPicker& picker, bool pipelineDone = true,
                   int loadingAddonCount = 0);
    void setLoading(const QString& message = QStringLiteral("Loading sources..."));

signals:
    void backRequested();
    void streamSelected(const ScoredStream& stream);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void positionBackButton();
    void scheduleBackdropRecomposite();

    QString m_currentId;
    std::optional<EpisodeItem> m_currentEpisode;
    MetaService* m_meta = nullptr;
    BackdropLayer* m_backdrop = nullptr;
    PickerHeader* m_header = nullptr;
    QScrollArea* m_scroll = nullptr;
    QFrame* m_loadingPanel = nullptr;
    QWidget* m_spinner = nullptr;
    QLabel* m_loading = nullptr;
    StreamList* m_streamList = nullptr;
    QPushButton* m_back = nullptr;
};

} // namespace tankoban
