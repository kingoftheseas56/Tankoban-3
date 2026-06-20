// Tankoban 3 — Qt Quick spike. Increment 1: prove the toolkit + the
// Home -> Series -> Sources -> Player navigation walk render in pure Qt Quick.
#include <QGuiApplication>
#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QQmlApplicationEngine>
#include <QQmlNetworkAccessManagerFactory>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QUrl>

// ── IPv4-pinned network stack ────────────────────────────────────────────────
// On this network IPv6 is a black hole: metahub publishes AAAA records but the
// IPv6 endpoint never connects, so Qt (which tries IPv6 first) blocks for the
// ~21s Windows TCP-connect timeout before falling back to IPv4 — the "images
// take 21s then all appear at once" stall. Proven: curl -6 hangs 21s, curl -4 is
// 0.26s. Fix: resolve the image host to IPv4 once and route requests to that IP,
// keeping TLS valid via SNI/peer-verify-name + the real Host header. Forces
// HTTP/1.1 so the Host header governs routing (vs HTTP/2 :authority = the IP).
class Ipv4Nam : public QNetworkAccessManager {
public:
    Ipv4Nam(QString host, QString ipv4, QObject* parent)
        : QNetworkAccessManager(parent), m_host(std::move(host)), m_ipv4(std::move(ipv4)) {}

protected:
    QNetworkReply* createRequest(Operation op, const QNetworkRequest& req,
                                 QIODevice* outgoing) override {
        QNetworkRequest r(req);
        QUrl u = r.url();
        if (!m_ipv4.isEmpty() && u.host() == m_host) {
            r.setRawHeader("Host", m_host.toUtf8());        // route on the real name
            r.setPeerVerifyName(m_host);                    // TLS SNI + cert check vs name
            r.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
            u.setHost(m_ipv4);                              // connect to the IPv4 literal
            r.setUrl(u);
        }
        return QNetworkAccessManager::createRequest(op, r, outgoing);
    }

private:
    QString m_host, m_ipv4;
};

class Ipv4NamFactory : public QQmlNetworkAccessManagerFactory {
public:
    Ipv4NamFactory(QString host, QString ipv4)
        : m_host(std::move(host)), m_ipv4(std::move(ipv4)) {}
    QNetworkAccessManager* create(QObject* parent) override {
        return new Ipv4Nam(m_host, m_ipv4, parent);
    }
private:
    QString m_host, m_ipv4;
};

static QString resolveIpv4(const QString& host)
{
    const QHostInfo info = QHostInfo::fromName(host); // blocking, pre-event-loop, ~25ms
    for (const QHostAddress& a : info.addresses())
        if (a.protocol() == QAbstractSocket::IPv4Protocol)
            return a.toString();
    return {};
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // QQuickFramebufferObject + the mpv OpenGL render API need the GL scene-graph
    // backend. Qt6 on Windows defaults to Direct3D11 — force OpenGL before any
    // QQuickWindow is created, or the mpv video node renders nothing.
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    // Direct connection (no Windows WPAD proxy hunt).
    QNetworkProxyFactory::setUseSystemConfiguration(false);
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);

    // Fusion = a clean desktop-native Controls style (vs the bare "Basic").
    QQuickStyle::setStyle(QStringLiteral("Fusion"));

    QQmlApplicationEngine engine;

    // Pin the poster/backdrop host to IPv4 (see Ipv4Nam above).
    const QString kImageHost = QStringLiteral("images.metahub.space");
    const QString v4 = resolveIpv4(kImageHost);
    if (!v4.isEmpty())
        engine.setNetworkAccessManagerFactory(new Ipv4NamFactory(kImageHost, v4));

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, [](const QUrl& url) {
            qWarning() << "[NAVDBG] objectCreationFailed ->" << url << "-> exit(-1)";
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    engine.loadFromModule("Tankoban3Quick", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
