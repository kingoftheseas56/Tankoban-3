// Tankoban 3 - Play Picker header (M5).
#pragma once

#include <QLabel>
#include <QStringList>
#include <QWidget>

namespace tankoban {

class PickerHeader : public QWidget {
    Q_OBJECT
public:
    explicit PickerHeader(QWidget* parent = nullptr);

    void setMeta(const QString& name, const QString& releaseInfo, const QStringList& genres);
    void setEpisode(const QString& seriesName,
                    int season,
                    int episode,
                    const QString& episodeName,
                    const QString& overview);

private:
    QLabel* m_kicker = nullptr;
    QLabel* m_title = nullptr;
    QLabel* m_overview = nullptr;
};

} // namespace tankoban
