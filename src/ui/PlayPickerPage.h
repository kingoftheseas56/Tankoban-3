// Tankoban 3 - Play Picker page shell (M5).
#pragma once

#include <optional>

#include <QWidget>

#include "core/MetaDetail.h"

class QLabel;
class QPushButton;
class QWidget;

namespace tankoban {

class BackdropLayer;
class MetaService;
class PickerHeader;

class PlayPickerPage : public QWidget {
    Q_OBJECT
public:
    explicit PlayPickerPage(QWidget* parent = nullptr);

    void open(const MetaDetail& meta, std::optional<EpisodeItem> episode);

signals:
    void backRequested();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void positionBackButton();

    QString m_currentId;
    std::optional<EpisodeItem> m_currentEpisode;
    MetaService* m_meta = nullptr;
    BackdropLayer* m_backdrop = nullptr;
    PickerHeader* m_header = nullptr;
    QWidget* m_spinner = nullptr;
    QLabel* m_loading = nullptr;
    QPushButton* m_back = nullptr;
};

} // namespace tankoban
