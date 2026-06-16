// Tankoban 3 — StremioAddonsProvider (Addons marketplace).
//
// Client for the public stremio-addons.net index (Harbor's Browse/Discover source):
//   GET /api/v0/addons?page&limit&sort_by&order&nsfw&category&search   → list
//   GET /api/v0/rising                                                  → rising
//   GET /api/v0/categories                                              → categories
// Faithful to Harbor's src/lib/providers/stremio-addons.ts.

#pragma once

#include <QObject>
#include <QString>
#include <QVector>

#include "core/AddonStoreModels.h"

class QNetworkAccessManager;

namespace tankoban {

class StremioAddonsProvider : public QObject {
    Q_OBJECT
public:
    explicit StremioAddonsProvider(QObject* parent = nullptr);

    // mode: "top" (stars desc) | "new" (createdAt desc) | "rising" (/rising endpoint).
    // requestKey is echoed back so the caller can ignore stale responses.
    void list(const QString& requestKey, const QString& mode, const QString& category,
              const QString& search, int page, bool includeAdult);
    void fetchCategories();

signals:
    void listReady(const QString& requestKey, const StorePage& page);
    void listFailed(const QString& requestKey, const QString& error);
    void categoriesReady(const QVector<StoreCategory>& cats);

private:
    QNetworkAccessManager* m_nam = nullptr;
};

} // namespace tankoban
