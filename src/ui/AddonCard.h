// Tankoban 3 — AddonCard (Addons marketplace).
//
// A marketplace addon card (Harbor Discover/Browse): logo, name, star count, a category,
// and an Install / Installed action. Used in the Discover rails and the Browse grid.

#pragma once

#include <QWidget>

#include "core/AddonStoreModels.h"

class QLabel;
class QPushButton;

namespace tankoban {

class AddonCard : public QWidget {
    Q_OBJECT
public:
    AddonCard(const StoreAddon& addon, bool installed, QWidget* parent = nullptr);

    void setInstalled(bool installed);
    const StoreAddon& addon() const { return m_addon; }

    static constexpr int kCardW = 300;
    static constexpr int kCardH = 158;

signals:
    void installRequested(const StoreAddon& addon);
    void openRequested(const StoreAddon& addon);

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void enterEvent(QEnterEvent* e) override;
    void leaveEvent(QEvent* e) override;

private:
    void loadLogo(const QString& url);

    StoreAddon m_addon;
    QLabel* m_logo = nullptr;
    QLabel* m_name = nullptr;
    QLabel* m_stars = nullptr;
    QLabel* m_desc = nullptr;
    QLabel* m_cat = nullptr;
    QPushButton* m_install = nullptr;
    bool m_installed = false;
};

} // namespace tankoban
