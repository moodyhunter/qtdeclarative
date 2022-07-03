// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmldocumentsymbolssupport.h"

QT_BEGIN_NAMESPACE
using namespace QLspSpecification;
using namespace QQmlJS;
using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(docSymsLog, "qt.languageserver.documentsymbols")

QmlDocumentSymbolsSupport::QmlDocumentSymbolsSupport(QmlLsp::QQmlCodeModel *codeModel)
    : m_codeModel(codeModel)
{
}

QmlDocumentSymbolsSupport::~QmlDocumentSymbolsSupport() { }

QString QmlDocumentSymbolsSupport::name() const
{
    return u"Document Symbols"_s;
}

void QmlDocumentSymbolsSupport::registerHandlers(QLanguageServer *server,
                                                 QLanguageServerProtocol *protocol)
{
    protocol->registerDocumentSymbolRequestHandler(
            [](const QByteArray &, const DocumentSymbolParams &,
               LSPPartialResponse<std::variant<QList<DocumentSymbol>, QList<SymbolInformation>,
                                               std::nullptr_t>,
                                  std::variant<QList<DocumentSymbol>, QList<SymbolInformation>>>
                       &&) {
                ;
                ;
            });
}

void QmlDocumentSymbolsSupport::setupCapabilities(
        const QLspSpecification::InitializeParams &,
        QLspSpecification::InitializeResult &serverCapabilities)
{
    serverCapabilities.capabilities.documentSymbolProvider = true;
}

void QmlDocumentSymbolsSupport::updatedSnapshot(const QByteArray &url)
{
    ;
}

QT_END_NAMESPACE
