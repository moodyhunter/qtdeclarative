// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qqmlengine_p.h>
#include <private/qqmlextensionplugin_p.h>
#include <private/qqmltypeloaderthread_p.h>

#if QT_CONFIG(qml_network)
#include <private/qqmltypeloadernetworkreplyproxy_p.h>
#endif

QT_BEGIN_NAMESPACE

QQmlTypeLoaderThread::QQmlTypeLoaderThread(QQmlTypeLoader *loader)
    : m_loader(loader)
#if QT_CONFIG(qml_network)
      , m_networkAccessManager(nullptr), m_networkReplyProxy(nullptr)
#endif // qml_network
{
    // Do that after initializing all the members.
    startup();
}

#if QT_CONFIG(qml_network)
QNetworkAccessManager *QQmlTypeLoaderThread::networkAccessManager() const
{
    Q_ASSERT(isThisThread());
    if (!m_networkAccessManager) {
        m_networkAccessManager = QQmlEnginePrivate::get(m_loader->engine())->createNetworkAccessManager(nullptr);
        m_networkReplyProxy = new QQmlTypeLoaderNetworkReplyProxy(m_loader);
    }

    return m_networkAccessManager;
}

QQmlTypeLoaderNetworkReplyProxy *QQmlTypeLoaderThread::networkReplyProxy() const
{
    Q_ASSERT(isThisThread());
    Q_ASSERT(m_networkReplyProxy); // Must call networkAccessManager() first
    return m_networkReplyProxy;
}
#endif // qml_network

void QQmlTypeLoaderThread::load(QQmlDataBlob *b)
{
    b->addref();
    callMethodInThread(&This::loadThread, b);
}

void QQmlTypeLoaderThread::loadAsync(QQmlDataBlob *b)
{
    b->addref();
    postMethodToThread(&This::loadThread, b);
}

void QQmlTypeLoaderThread::loadWithStaticData(QQmlDataBlob *b, const QByteArray &d)
{
    b->addref();
    callMethodInThread(&This::loadWithStaticDataThread, b, d);
}

void QQmlTypeLoaderThread::loadWithStaticDataAsync(QQmlDataBlob *b, const QByteArray &d)
{
    b->addref();
    postMethodToThread(&This::loadWithStaticDataThread, b, d);
}

void QQmlTypeLoaderThread::loadWithCachedUnit(QQmlDataBlob *b, const QQmlPrivate::CachedQmlUnit *unit)
{
    b->addref();
    callMethodInThread(&This::loadWithCachedUnitThread, b, unit);
}

void QQmlTypeLoaderThread::loadWithCachedUnitAsync(QQmlDataBlob *b, const QQmlPrivate::CachedQmlUnit *unit)
{
    b->addref();
    postMethodToThread(&This::loadWithCachedUnitThread, b, unit);
}

void QQmlTypeLoaderThread::callCompleted(QQmlDataBlob *b)
{
    b->addref();
#if !QT_CONFIG(thread)
    if (!isThisThread())
        postMethodToThread(&This::callCompletedMain, b);
#else
    postMethodToMain(&This::callCompletedMain, b);
#endif
}

void QQmlTypeLoaderThread::callDownloadProgressChanged(QQmlDataBlob *b, qreal p)
{
    b->addref();
#if !QT_CONFIG(thread)
    if (!isThisThread())
        postMethodToThread(&This::callDownloadProgressChangedMain, b, p);
#else
    postMethodToMain(&This::callDownloadProgressChangedMain, b, p);
#endif
}

void QQmlTypeLoaderThread::initializeEngine(QQmlExtensionInterface *iface,
                                            const char *uri)
{
    callMethodInMain(&This::initializeExtensionMain, iface, uri);
}

void QQmlTypeLoaderThread::initializeEngine(QQmlEngineExtensionInterface *iface,
                                            const char *uri)
{
    callMethodInMain(&This::initializeEngineExtensionMain, iface, uri);
}

void QQmlTypeLoaderThread::shutdownThread()
{
#if QT_CONFIG(qml_network)
    delete m_networkAccessManager;
    m_networkAccessManager = nullptr;
    delete m_networkReplyProxy;
    m_networkReplyProxy = nullptr;
#endif // qml_network
}

void QQmlTypeLoaderThread::loadThread(QQmlDataBlob *b)
{
    m_loader->loadThread(b);
    b->release();
}

void QQmlTypeLoaderThread::loadWithStaticDataThread(QQmlDataBlob *b, const QByteArray &d)
{
    m_loader->loadWithStaticDataThread(b, d);
    b->release();
}

void QQmlTypeLoaderThread::loadWithCachedUnitThread(QQmlDataBlob *b, const QQmlPrivate::CachedQmlUnit *unit)
{
    m_loader->loadWithCachedUnitThread(b, unit);
    b->release();
}

void QQmlTypeLoaderThread::callCompletedMain(QQmlDataBlob *b)
{
#ifdef DATABLOB_DEBUG
    qWarning("QQmlTypeLoaderThread: %s completed() callback", qPrintable(b->urlString()));
#endif
    b->completed();
    b->release();
}

void QQmlTypeLoaderThread::callDownloadProgressChangedMain(QQmlDataBlob *b, qreal p)
{
#ifdef DATABLOB_DEBUG
    qWarning("QQmlTypeLoaderThread: %s downloadProgressChanged(%f) callback",
             qPrintable(b->urlString()), p);
#endif
    b->downloadProgressChanged(p);
    b->release();
}

void QQmlTypeLoaderThread::initializeExtensionMain(QQmlExtensionInterface *iface,
                                                const char *uri)
{
    Q_ASSERT(m_loader->engine()->thread() == QThread::currentThread());
    iface->initializeEngine(m_loader->engine(), uri);
}

void QQmlTypeLoaderThread::initializeEngineExtensionMain(QQmlEngineExtensionInterface *iface,
                                                const char *uri)
{
    Q_ASSERT(m_loader->engine()->thread() == QThread::currentThread());
    iface->initializeEngine(m_loader->engine(), uri);
}

QT_END_NAMESPACE
