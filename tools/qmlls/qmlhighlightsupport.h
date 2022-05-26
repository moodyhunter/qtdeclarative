// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef QMLCOLORINGSUPPORT_H
#define QMLCOLORINGSUPPORT_H

#include "qlanguageserver.h"
#include "qqmlcodemodel.h"

class QmlHighlightSupportPrivate;

class QmlHighlightSupport : public QLanguageServerModule
{
    Q_OBJECT
public:
    QmlHighlightSupport(QmlLsp::QQmlCodeModel *codeModel);
    ~QmlHighlightSupport();
    QString name() const override;
    void registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol) override;
    void setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                           QLspSpecification::InitializeResult &) override;
    QLspSpecification::SemanticTokens
    semanticTokens(const QLspSpecification::SemanticTokensParams &params);
    QLspSpecification::SemanticTokens
    deltaSemanticTokens(const QLspSpecification::SemanticTokensDeltaParams &params);
    QLspSpecification::SemanticTokens
    rangeSemanticTokens(const QLspSpecification::SemanticTokensRangeParams &params);

private:
    QmlLsp::QQmlCodeModel *m_codeModel;
    QMutex m_mutex;
    Q_DECLARE_PRIVATE(QmlHighlightSupport)
};

#endif // QMLCOLORINGSUPPORT_H
