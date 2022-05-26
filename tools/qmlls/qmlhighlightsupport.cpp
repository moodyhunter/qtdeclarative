// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "qlanguageserver.h"

#include "qmlhighlightsupport.h"
#include "private/qqmldomexternalitems_p.h"

#include "qlanguageserver.h"
#include "lspcustomtypes.h"
#include "QtQmlDom/private/qqmldomastdumper_p.h"
#include "QtQmlDom/private/qqmldomscanner_p.h"
#include "QtQmlDom/private/qqmldomcodeformatter_p.h"
#include <queue>

QT_BEGIN_NAMESPACE

using namespace QLspSpecification;
using namespace QQmlJS::AST;
using namespace QQmlJS;
using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(highlightLog, "qt.languageserver.highlighting")

struct SemanticToken
{
    SemanticToken(int deltaLine, int deltaStart, int length, SemanticTokenTypes tokenTypes)
        : deltaLine(deltaLine),
          deltaStart(deltaStart),
          length(length),
          tokenType(int(tokenType)),
          tokenModifiers(0)
    {
    }
    template<typename... Types>
    SemanticToken(int deltaLine, int deltaStart, int length, SemanticTokenTypes tokenType,
                  SemanticTokenModifiers modifier, Types... extraModifiers)
        : SemanticToken(deltaLine, deltaStart, length, tokenType, extraModifiers...)
    {
        tokenModifiers |= (1 << int(modifier));
    }
    int deltaLine = 0;
    int deltaStart = 0;
    int length = 0;
    int tokenType = 0;
    int tokenModifiers = 0;

    static std::vector<QByteArray> tokenTypesList();
    static std::vector<QByteArray> tokenModifiersList();
};

struct LineInfo
{
    QQmlJS::Dom::FormatTextStatus startState;
    QList<SemanticToken> lineTokens;
};

struct SemanticState
{
    QList<LineInfo> lines;
    QMap<int, int> lineChanges;
    void addChange(int line, int lineChange);
    void updateRange(int lineStart, int lineEnd);
};

class QmlHighlightSupportPrivate : QObjectPrivate
{
    SemanticState fullState;
    SemanticState rangeState;

    void addChange(int line, int lineChange)
    {
        fullState.addChange(line, lineChange);
        rangeState.addChange(line, lineChange);
    }
};

struct highlight
{
    quint32 offset;
    quint32 startLine;
    quint32 startColumn;
    quint32 length;
    LegendType type;
    int modifier;

    bool operator<(const highlight &other) const { return offset > other.offset; }
};

class HighlightingVisitor : public AST::Visitor
{
public:
    explicit HighlightingVisitor(UiProgram *);
    QList<int> popResult()
    {
        QList<int> result;
        while (!highlights.empty()) {
            highlight h = highlights.top();
            highlights.pop();
            if (h.startLine != privLine)
                privColumn = 1;
            result.append(h.startLine - privLine);
            result.append(h.startColumn - privColumn);
            result.append(h.length);
            result.append(h.type);
            result.append(h.modifier);
            privLine = h.startLine;
            privColumn = h.startColumn;
        }
        return result;
    }

private:
    // Ui
    // bool visit(UiProgram *) override { return true; }

    // default implementation is good enough (iterates over pragmas and imports)
    // bool visit(UiHeaderItemList *) override { return true; }
    bool visit(UiPragma *) override;
    bool visit(UiImport *) override;
    bool visit(UiPublicMember *) override;
    bool visit(UiSourceElement *) override { return true; }
    bool visit(UiObjectDefinition *) override;

    // default implementation is good enough (iterate over members)
    // bool visit(UiObjectInitializer *) override { return true; }

    // contentItem: Item{}
    bool visit(UiObjectBinding *) override;

    // prop: val
    bool visit(UiScriptBinding *) override;

    // children: [Item{}, Item{}, Item{}, Item{}]
    bool visit(UiArrayBinding *) override;

    bool visit(UiParameterList *) override;
    // bool visit(UiObjectMemberList *) override { return true; }
    bool visit(UiArrayMemberList *) override { return true; }
    // Nearly everything is a QualifiedID, don't mess with it...
    // bool visit(UiQualifiedId *) override { return true; }
    bool visit(UiEnumDeclaration *) override;
    bool visit(UiEnumMemberList *) override;
    bool visit(UiVersionSpecifier *) override { return true; }
    bool visit(UiInlineComponent *) override;
    bool visit(UiAnnotation *) override;
    bool visit(UiAnnotationList *) override { return true; }
    bool visit(UiRequired *) override { return true; }

    // QQmlJS
    bool visit(TypeExpression *) override { return true; }

    bool visit(ThisExpression *t) override;
    bool visit(IdentifierExpression *i) override;
    bool visit(NullExpression *n) override;
    bool visit(TrueLiteral *t) override;
    bool visit(FalseLiteral *f) override;
    bool visit(SuperLiteral *s) override;
    bool visit(StringLiteral *s) override;
    bool visit(TemplateLiteral *t) override;
    bool visit(NumericLiteral *n) override;
    bool visit(RegExpLiteral *r) override;

    bool visit(ArrayPattern *) override { return true; }
    bool visit(ObjectPattern *) override { return true; }
    bool visit(PatternElementList *) override { return true; }
    bool visit(PatternPropertyList *) override { return true; }
    bool visit(PatternElement *) override;
    bool visit(PatternProperty *) override;
    bool visit(Elision *) override { return true; }
    bool visit(NestedExpression *) override { return true; }
    // Handled by PropertyName
    // bool visit(IdentifierPropertyName *) override { return true; }
    // bool visit(StringLiteralPropertyName *) override { return true; }
    // bool visit(NumericLiteralPropertyName *) override { return true; }
    // bool visit(ComputedPropertyName *) override { return true; }
    bool visit(ArrayMemberExpression *) override { return true; }
    bool visit(FieldMemberExpression *) override;
    bool visit(TaggedTemplate *) override { return true; }
    bool visit(NewMemberExpression *) override;
    bool visit(NewExpression *) override { return true; }

    bool visit(CallExpression *c) override;
    void endVisit(CallExpression *) override;

    bool visit(ArgumentList *) override { return true; }
    bool visit(PostIncrementExpression *) override { return true; }
    bool visit(PostDecrementExpression *) override { return true; }
    bool visit(DeleteExpression *) override { return true; }
    bool visit(VoidExpression *) override { return true; }
    bool visit(TypeOfExpression *) override { return true; }
    bool visit(PreIncrementExpression *) override { return true; }
    bool visit(PreDecrementExpression *) override { return true; }
    bool visit(UnaryPlusExpression *) override { return true; }
    bool visit(UnaryMinusExpression *) override { return true; }
    bool visit(TildeExpression *) override { return true; }
    bool visit(NotExpression *) override { return true; }
    bool visit(BinaryExpression *) override { return true; }
    bool visit(ConditionalExpression *) override { return true; }
    bool visit(Expression *) override { return true; }
    bool visit(Block *) override { return true; }
    bool visit(StatementList *) override { return true; }
    bool visit(VariableStatement *) override;
    bool visit(VariableDeclarationList *) override { return true; }
    // A boring semicolon :)
    // bool visit(EmptyStatement *) override { return true; }
    bool visit(ExpressionStatement *) override { return true; }
    bool visit(IfStatement *) override;
    bool visit(DoWhileStatement *) override;
    bool visit(WhileStatement *) override;
    bool visit(ForStatement *) override;
    bool visit(ForEachStatement *) override;
    bool visit(ContinueStatement *) override;
    bool visit(BreakStatement *) override;
    bool visit(ReturnStatement *) override;
    bool visit(YieldExpression *) override;
    bool visit(WithStatement *) override;
    bool visit(SwitchStatement *) override;
    // Their default implementation is good enough (iterates over cases)
    // bool visit(CaseBlock *) override { return true; }
    // bool visit(CaseClauses *) override { return true; }
    bool visit(CaseClause *) override;
    bool visit(DefaultClause *) override;
    bool visit(LabelledStatement *) override;
    bool visit(ThrowStatement *) override;
    bool visit(TryStatement *) override;
    bool visit(Catch *) override;
    bool visit(Finally *) override;
    bool visit(FunctionDeclaration *) override;
    void endVisit(FunctionDeclaration *) override;

    // bool visit(FunctionExpression *) override { return true; }

    // No need, its default accessor iterates over the members.
    // bool visit(FormalParameterList *) override { return true; }
    bool visit(ClassExpression *) override { return true; }
    bool visit(ClassDeclaration *) override { return true; }
    bool visit(ClassElementList *) override { return true; }
    bool visit(Program *) override { return true; }
    bool visit(NameSpaceImport *) override { return true; }
    bool visit(ImportSpecifier *) override { return true; }
    bool visit(ImportsList *) override { return true; }
    bool visit(NamedImports *) override { return true; }
    bool visit(FromClause *) override { return true; }
    bool visit(ImportClause *) override { return true; }
    bool visit(ImportDeclaration *) override { return true; }
    bool visit(ExportSpecifier *) override { return true; }
    bool visit(ExportsList *) override { return true; }
    bool visit(ExportClause *) override { return true; }
    bool visit(ExportDeclaration *) override { return true; }
    bool visit(ESModule *) override { return true; }
    bool visit(DebuggerStatement *) override;
    bool visit(Type *) override { return true; }
    bool visit(TypeArgumentList *) override { return true; }
    bool visit(TypeAnnotation *) override;

    void throwRecursionDepthError() override { }

private:
    void addHighlight(const QQmlJS::SourceLocation &loc, LegendType type, int b);

private:
    std::priority_queue<highlight> highlights;
    quint32 privLine = 1;
    quint32 privColumn = 1;

    bool inFunctionCall = false;
    bool isInFunctionDecl = false;
};

HighlightingVisitor::HighlightingVisitor(UiProgram *prog)
{
    prog->accept(this);
}

void HighlightingVisitor::addHighlight(const QQmlJS::SourceLocation &loc, LegendType type, int b)
{
    highlight h;
    h.offset = loc.offset;
    h.startLine = loc.startLine;
    h.startColumn = loc.startColumn;
    h.length = loc.length;
    h.type = type;
    h.modifier = b;

    highlights.emplace(h);
};

bool HighlightingVisitor::visit(UiPragma *p)
{
    addHighlight(p->pragmaToken, Keyword, 0);
    return false;
}

bool HighlightingVisitor::visit(UiImport *import)
{
    addHighlight(import->importToken, Keyword, 0);

    UiQualifiedId *uri = import->importUri;
    if (uri) {
        const auto sl = combine(uri->firstSourceLocation(), uri->lastSourceLocation());
        addHighlight(sl, Module, 0);
    }

    if (!import->fileName.isEmpty())
        addHighlight(import->fileNameToken, String, 0);

    if (import->asToken.length)
        addHighlight(import->asToken, Keyword, 0);

    if (import->importIdToken.length)
        addHighlight(import->importIdToken, Module, 0);

    return false;
}

bool HighlightingVisitor::visit(UiPublicMember *p)
{
    if (p->annotations)
        p->annotations->accept(this);
    if (p->memberType)
        p->memberType->accept(this);
    if (p->parameters)
        p->parameters->accept(this);

    const auto defaultToken = p->defaultToken();
    if (defaultToken.offset)
        addHighlight(defaultToken, Keyword, 0);

    const auto readonlyToken = p->readonlyToken();
    if (readonlyToken.offset)
        addHighlight(readonlyToken, Keyword, 0);

    const auto propertyToken = p->propertyToken();
    if (propertyToken.offset)
        addHighlight(propertyToken, Keyword, 0);

    const auto requiredToken = p->requiredToken();
    if (requiredToken.offset)
        addHighlight(requiredToken, Keyword, 0);

    const auto typeModifierToken = p->typeModifierToken;
    if (typeModifierToken.offset)
        addHighlight(typeModifierToken, Keyword, 0);

    const auto typeToken = p->typeToken;
    if (typeToken.offset)
        addHighlight(typeToken, TypeType, 0);

    const auto identifierToken = p->identifierToken;
    if (identifierToken.offset)
        addHighlight(identifierToken, Property, 0);

    return true;
}

bool HighlightingVisitor::visit(UiObjectDefinition *def)
{
    qCWarning(highlightLog) << "UiObjectDefinition" << def->qualifiedTypeNameId->name;
    auto *name = def->qualifiedTypeNameId;
    addHighlight(combine(name->firstSourceLocation(), name->lastSourceLocation()), Class, 0);
    // continue visiting the initializer
    return true;
}

bool HighlightingVisitor::visit(UiEnumDeclaration *decl)
{
    addHighlight(decl->enumToken, Keyword, 0);
    addHighlight(decl->identifierToken, Enum, 0);
    // continue visiting the enum members
    return true;
}

bool HighlightingVisitor::visit(UiEnumMemberList *def)
{
    if (!def)
        return false;
    addHighlight(def->memberToken, EnumMember, 0);
    if (def->valueToken.offset)
        addHighlight(def->valueToken, Number, 0);
    return visit(def->next);
}

bool HighlightingVisitor::visit(UiInlineComponent *ic)
{
    addHighlight(ic->componentToken, Keyword, 0);
    addHighlight(ic->nameToken, TypeType, 0);
    // continue visiting the component
    return true;
}

bool HighlightingVisitor::visit(UiObjectBinding *binding)
{
    const auto typeToken = combine(binding->qualifiedTypeNameId->firstSourceLocation(),
                                   binding->qualifiedTypeNameId->lastSourceLocation());
    const auto idToken = combine(binding->qualifiedId->firstSourceLocation(),
                                 binding->qualifiedId->lastSourceLocation());

    if (binding->hasOnToken) {
        // colonToken is actually "on" token in this case, see qqmljs.g:1014
        addHighlight(typeToken, Class, 0);
        addHighlight(binding->colonToken, Keyword, 0);
        addHighlight(idToken, Property, 0);
    } else {
        addHighlight(idToken, Property, 0);
        addHighlight(typeToken, Class, 0);
    }

    if (binding->initializer)
        binding->initializer->accept(this);

    return false;
}

bool HighlightingVisitor::visit(UiScriptBinding *b)
{
    qCWarning(highlightLog) << "UiScriptBinding on line" << b->firstSourceLocation().startLine
                            << " of file" << b->qualifiedId->name;
    // Process qualified ID
    const auto sl =
            combine(b->qualifiedId->firstSourceLocation(), b->qualifiedId->lastSourceLocation());
    addHighlight(sl, Property, 0);
    // continue visiting the statement
    return true;
}

bool HighlightingVisitor::visit(UiAnnotation *anno)
{
    qCWarning(highlightLog) << "UiAnnotation on line" << anno->firstSourceLocation().startLine
                            << " of file" << anno->qualifiedTypeNameId->name;
    auto sl = combine(anno->qualifiedTypeNameId->firstSourceLocation(),
                      anno->qualifiedTypeNameId->lastSourceLocation());
    addHighlight(sl, Property, 0);
    // continue visiting the initializer
    return false;
}

bool HighlightingVisitor::visit(CallExpression *)
{
    // continue visiting the arguments, but mark do notify them
    // that this is a call expression
    inFunctionCall = true;
    return true;
}

void HighlightingVisitor::endVisit(CallExpression *)
{
    inFunctionCall = false;
}

bool HighlightingVisitor::visit(FieldMemberExpression *fme)
{
    if (inFunctionCall)
        addHighlight(fme->identifierToken, Function, 0);
    else
        addHighlight(fme->identifierToken, Variable, 0);

    inFunctionCall = false;
    return true;
}

bool HighlightingVisitor::visit(FunctionDeclaration *f)
{
    addHighlight(f->functionToken, Keyword, 0);
    addHighlight(f->identifierToken, Function, 0);
    isInFunctionDecl = true;
    // continue visiting the parameters, return type and the body
    return true;
}

void HighlightingVisitor::endVisit(FunctionDeclaration *)
{
    isInFunctionDecl = false;
}

bool HighlightingVisitor::visit(PatternElement *pe)
{
    if (pe->identifierToken.offset) {
        if (isInFunctionDecl)
            addHighlight(pe->identifierToken, Parameter, 0);
        else
            addHighlight(pe->identifierToken, Variable, 0);
    }
    return true;
}

bool HighlightingVisitor::visit(PatternProperty *pp)
{
    if (pp->name->propertyNameToken.offset)
        addHighlight(pp->name->propertyNameToken, Property, 0);
    return true;
}

bool HighlightingVisitor::visit(VariableStatement *v)
{
    addHighlight(v->declarationKindToken, Keyword, 0);
    return true;
}

bool HighlightingVisitor::visit(UiArrayBinding *a)
{
    qCWarning(highlightLog) << "UiArrayBinding on line" << a->firstSourceLocation().startLine
                            << " name" << a->qualifiedId->name;

    const auto sl =
            combine(a->qualifiedId->firstSourceLocation(), a->qualifiedId->lastSourceLocation());
    addHighlight(sl, Property, 0);
    a->members->accept(this);
    return false;
}

bool HighlightingVisitor::visit(UiParameterList *binding)
{
    if (!binding)
        return false;
    addHighlight(binding->identifierToken, Parameter, 0);
    addHighlight(binding->propertyTypeToken, TypeType, 0);
    return visit(binding->next);
}

bool HighlightingVisitor::visit(TypeAnnotation *ta)
{
    qCWarning(highlightLog) << "TypeAnnotation on line" << ta->firstSourceLocation().startLine;
    const auto sl = combine(ta->type->firstSourceLocation(), ta->type->lastSourceLocation());
    addHighlight(sl, TypeType, 0);
    return false;
}

bool HighlightingVisitor::visit(NewMemberExpression *n)
{
    // TODO: Handle base, possibly contains a module name
    addHighlight(n->newToken, Keyword, 0);
    const auto sl = combine(n->base->firstSourceLocation(), n->base->lastSourceLocation());
    addHighlight(sl, TypeType, 0);
    if (n->arguments)
        n->arguments->accept(this);
    return false;
}

bool HighlightingVisitor::visit(IfStatement *i)
{
    addHighlight(i->ifToken, Keyword, 0);
    if (i->elseToken.length)
        addHighlight(i->elseToken, Keyword, 0);
    return true;
}

bool HighlightingVisitor::visit(DoWhileStatement *d)
{
    addHighlight(d->doToken, Keyword, 0);
    addHighlight(d->whileToken, Keyword, 0);
    return true;
}

bool HighlightingVisitor::visit(WhileStatement *w)
{
    addHighlight(w->whileToken, Keyword, 0);
    return true;
}

bool HighlightingVisitor::visit(SwitchStatement *w)
{
    addHighlight(w->switchToken, Keyword, 0);
    return true;
}

bool HighlightingVisitor::visit(CaseClause *c)
{
    addHighlight(c->caseToken, Keyword, 0);
    return true;
}

bool HighlightingVisitor::visit(DefaultClause *d)
{
    addHighlight(d->defaultToken, Keyword, 0);
    return true;
}

bool HighlightingVisitor::visit(TryStatement *t)
{
    addHighlight(t->tryToken, Keyword, 0);
    return true;
}

bool HighlightingVisitor::visit(Catch *c)
{
    addHighlight(c->catchToken, Keyword, 0);
    return true;
}

bool HighlightingVisitor::visit(ThrowStatement *t)
{
    addHighlight(t->throwToken, Keyword, 0);
    return true;
}

bool HighlightingVisitor::visit(Finally *f)
{
    addHighlight(f->finallyToken, Keyword, 0);
    return true;
}

bool HighlightingVisitor::visit(DebuggerStatement *d)
{
    addHighlight(d->debuggerToken, Keyword, 0);
    return true;
}

bool HighlightingVisitor::visit(ContinueStatement *c)
{
    addHighlight(c->continueToken, Keyword, 0);
    if (c->identifierToken.length)
        addHighlight(c->identifierToken, Label, 0);
    return true;
}

bool HighlightingVisitor::visit(BreakStatement *b)
{
    addHighlight(b->breakToken, Keyword, 0);
    return true;
}

bool HighlightingVisitor::visit(ReturnStatement *r)
{
    addHighlight(r->returnToken, Keyword, 0);
    return true;
}

bool HighlightingVisitor::visit(WithStatement *w)
{
    addHighlight(w->withToken, Keyword, 0);
    return true;
}

bool HighlightingVisitor::visit(YieldExpression *y)
{
    addHighlight(y->yieldToken, Keyword, 0);
    return true;
}

bool HighlightingVisitor::visit(ForStatement *f)
{
    addHighlight(f->forToken, Keyword, 0);
    // TODO: 'var' cannot be highlighted here, need further investigation
    return true;
}

bool HighlightingVisitor::visit(ForEachStatement *f)
{
    addHighlight(f->forToken, Keyword, 0);
    addHighlight(f->inOfToken, Keyword, 0);
    return true;
}

bool HighlightingVisitor::visit(LabelledStatement *l)
{
    addHighlight(l->identifierToken, Label, 0);
    return true;
}
bool HighlightingVisitor::visit(ThisExpression *t)
{
    addHighlight(t->thisToken, Keyword, 0);
    return false;
}

bool HighlightingVisitor::visit(IdentifierExpression *i)
{
    addHighlight(i->identifierToken, Variable, 0);
    return false;
}

bool HighlightingVisitor::visit(NullExpression *n)
{
    addHighlight(n->nullToken, Keyword, 0);
    return false;
}

bool HighlightingVisitor::visit(TrueLiteral *t)
{
    addHighlight(t->trueToken, Boolean, 0);
    return false;
}

bool HighlightingVisitor::visit(FalseLiteral *f)
{
    addHighlight(f->falseToken, Boolean, 0);
    return false;
}

bool HighlightingVisitor::visit(SuperLiteral *s)
{
    addHighlight(s->superToken, Keyword, 0);
    return false;
}

bool HighlightingVisitor::visit(StringLiteral *s)
{
    addHighlight(s->literalToken, String, 0);
    return false;
}

bool HighlightingVisitor::visit(TemplateLiteral *t)
{
    addHighlight(t->literalToken, String, 0);
    if (t->expression)
        t->expression->accept(this);
    return true;
}

bool HighlightingVisitor::visit(NumericLiteral *n)
{
    addHighlight(n->literalToken, Number, 0);
    return false;
}

bool HighlightingVisitor::visit(RegExpLiteral *r)
{
    addHighlight(r->literalToken, RegExp, 0);
    return false;
}

void QmlHighlightSupport::registerHandlers(QLanguageServer *, QLanguageServerProtocol *protocol)
{
    qCDebug(highlightLog) << "Registering document color handler";
    protocol->registerSemanticTokensRequestHandler(
            [this](const QByteArray &, const SemanticTokensParams &params,
                   LSPPartialResponse<std::variant<SemanticTokens, std::nullptr_t>,
                                      SemanticTokensPartialResult> &&response) {
                response.sendResponse(semanticTokens(params));
            });
    protocol->registerSemanticTokensDeltaRequestHandler(
            [this](const QByteArray &, const SemanticTokensDeltaParams &params,
                   LSPPartialResponse<
                           std::variant<SemanticTokens, SemanticTokensDelta, std::nullptr_t>,
                           SemanticTokensDeltaPartialResult> &&response) {
                response.sendResponse(deltaSemanticTokens(params));
            });

    protocol->registerSemanticTokensRangeRequestHandler(
            [this](const QByteArray &, const SemanticTokensRangeParams &params,
                   LSPPartialResponse<std::variant<SemanticTokens, std::nullptr_t>,
                                      SemanticTokensPartialResult> &&response) {
                response.sendResponse(rangeSemanticTokens(params));
            });
}

SemanticTokens QmlHighlightSupport::semanticTokens(const SemanticTokensParams &params) { }

SemanticTokens QmlHighlightSupport::deltaSemanticTokens(const SemanticTokensDeltaParams &params) { }

SemanticTokens QmlHighlightSupport::rangeSemanticTokens(const SemanticTokensRangeParams &params) { }

QmlHighlightSupport::QmlHighlightSupport(QmlLsp::QQmlCodeModel *codeModel) : m_codeModel(codeModel)
{
    QObject::connect(m_codeModel, &QmlLsp::QQmlCodeModel::updatedSnapshot, this,
                     &QmlHighlightSupport::updatedSnapshot);
}

QmlHighlightSupport::~QmlHighlightSupport() { }

QString QmlHighlightSupport::name() const
{
    return u"QmlHighlightSupport"_s;
}

void QmlHighlightSupport::setupCapabilities(const QLspSpecification::InitializeParams &,
                                            QLspSpecification::InitializeResult &serverCapabilities)
{
    SemanticTokensOptions options;
    options.range = true;
    options.full = QJsonObject({ { "delta", true } });
    options.legend = SemanticTokensLegend{ { "keyword", "module", "class", "property", "function",
                                             "variable", "string", "comment", "number", "operator",
                                             "boolean", "enum", "regexp", "enumMember", "parameter",
                                             "type", "label" },
                                           { "normal", "bold" } };
    serverCapabilities.capabilities.semanticTokensProvider = options;
}

void QmlHighlightSupport::updatedSnapshot(const QByteArray &) { }

QList<QByteArray> SemanticToken::tokenTypesList()
{
    static std::array<SemanticTokenTypes, 22> tokenTypes = {
        SemanticTokenTypes::Namespace,     SemanticTokenTypes::Type,
        SemanticTokenTypes::Class,         SemanticTokenTypes::Enum,
        SemanticTokenTypes::Interface,     SemanticTokenTypes::Struct,
        SemanticTokenTypes::TypeParameter, SemanticTokenTypes::Parameter,
        SemanticTokenTypes::Variable,      SemanticTokenTypes::Property,
        SemanticTokenTypes::EnumMember,    SemanticTokenTypes::Event,
        SemanticTokenTypes::Function,      SemanticTokenTypes::Method,
        SemanticTokenTypes::Macro,         SemanticTokenTypes::Keyword,
        SemanticTokenTypes::Modifier,      SemanticTokenTypes::Comment,
        SemanticTokenTypes::String,        SemanticTokenTypes::Number,
        SemanticTokenTypes::Regexp,        SemanticTokenTypes::Operator
    };
    int ii = 0;
    static QList<QByteArray> res = [&]() {
        QList<QByteArray> r(tokenTypes.size());
        for (auto t : tokenTypes) {
            Q_ASSERT_X(int(t) == ii, "SemanticToken::tokenTypeList", "unexpected enum value");
            r[ii++] = QTypedJson::enumToString(t).toUtf8();
        }
        return r;
    }();
    return res;
}

QList<QByteArray> SemanticToken::tokenModifiersList()
{
    static std::array<SemanticTokenModifiers, 10> tokenModifiers = {
        SemanticTokenModifiers::Declaration,   SemanticTokenModifiers::Definition,
        SemanticTokenModifiers::Readonly,      SemanticTokenModifiers::Static,
        SemanticTokenModifiers::Deprecated,    SemanticTokenModifiers::Abstract,
        SemanticTokenModifiers::Async,         SemanticTokenModifiers::Modification,
        SemanticTokenModifiers::Documentation, SemanticTokenModifiers::DefaultLibrary
    };
    int ii = 0;
    static QList<QByteArray> res = [&]() {
        QList<QByteArray> r(tokenModifiers.size());
        for (auto t : tokenModifiers) {
            Q_ASSERT_X(int(t) == ii, "SemanticToken::tokenModifiersList", "unexpected enum value");
            r[ii++] = QTypedJson::enumToString(t).toUtf8();
        }
        return r;
    }();
    return res;
}

QT_END_NAMESPACE

void SemanticState::addChange(int line, int lineChange)
{
    if (lines.isEmpty())
        return;
    const qsizetype maxChanges = 10;
    int lineOffset = 0;
    auto it = lineChanges.cbegin();
    while (it != lineChanges.cend()) {
        if (line >= (it.key() + lineOffset)) {
            lineOffset += it.value();
            if (line <= (it.key() + lineOffset)) {
                lineChanges[it.key()] = it.value() + lineChange;
                return;
            }
        } else {
            break;
        }
    }
    if (lineChanges.size() >= maxChanges) {
        lines.clear();
        lineChanges.clear();
    } else {
        lineChanges[line + lineOffset] = lineChange;
    }
}

void SemanticState::updateRange(int lineStart, int lineEnd)
{
    if (lines.isEmpty())
        return;
    const qsizetype maxChanges = 10;
    int lineOffset = 0;
    auto it = lineChanges.cbegin();
    while (it != lineChanges.cend()) {
        if (line >= (it.key() + lineOffset)) {
            lineOffset += it.value();
            if (line <= (it.key() + lineOffset)) {
                lineChanges[it.key()] = it.value() + lineChange;
                return;
            }
        } else {
            break;
        }
    }
    if (lineChanges.size() >= maxChanges) {
        lines.clear();
        lineChanges.clear();
    } else {
        lineChanges[line + lineOffset] = lineChange;
    }
}
