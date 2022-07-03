// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLDOCUMENTSYMBOLSSUPPORT_H
#define QMLDOCUMENTSYMBOLSSUPPORT_H
#include "qlanguageserver.h"
#include "qqmlcodemodel.h"

class QmlDocumentSymbolsSupport : public QLanguageServerModule
{
    Q_OBJECT
public:
    QmlDocumentSymbolsSupport(QmlLsp::QQmlCodeModel *codeModel);
    ~QmlDocumentSymbolsSupport();
    QString name() const override;
    void registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol) override;
    void setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                           QLspSpecification::InitializeResult &) override;
public slots:
    void updatedSnapshot(const QByteArray &url);

private:
    QmlLsp::QQmlCodeModel *m_codeModel;
    QMutex m_mutex;
};

#endif // QMLDOCUMENTSYMBOLSSUPPORT_H
