// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlcustomparser_p.h"

#include <private/qv4compileddata_p.h>
#include <private/qqmlsourcecoordinate_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

/*!
    \class QQmlCustomParser
    \brief The QQmlCustomParser class allows you to add new arbitrary types to QML.
    \internal

    By subclassing QQmlCustomParser, you can add a parser for
    building a particular type.

    The subclass must implement compile() and setCustomData(), and register
    itself in the meta type system by calling the macro:

    \code
    QML_REGISTER_CUSTOM_TYPE(Module, MajorVersion, MinorVersion, Name, TypeClass, ParserClass)
    \endcode
*/

/*
    \fn QByteArray QQmlCustomParser::compile(const QList<QQmlCustomParserProperty> & properties)

    The custom parser processes \a properties, and returns
    a QByteArray containing data meaningful only to the
    custom parser; the type engine will pass this same data to
    setCustomData() when making an instance of the data.

    Errors must be reported via the error() functions.

    The QByteArray may be cached between executions of the system, so
    it must contain correctly-serialized data (not, for example,
    pointers to stack objects).
*/

/*
    \fn void QQmlCustomParser::setCustomData(QObject *object, const QByteArray &data)

    This function sets \a object to have the properties defined
    by \a data, which is a block of data previously returned by a call
    to compile().

    Errors should be reported using qmlWarning(object).

    The \a object will be an instance of the TypeClass specified by QML_REGISTER_CUSTOM_TYPE.
*/

void QQmlCustomParser::clearErrors()
{
    exceptions.clear();
}

/*!
    Reports an error with the given \a description.

    An error is generated referring to the \a location in the source file.
*/
void QQmlCustomParser::error(const QV4::CompiledData::Location &location, const QString &description)
{
    QQmlError error;
    error.setLine(qmlConvertSourceCoordinate<quint32, int>(location.line()));
    error.setColumn(qmlConvertSourceCoordinate<quint32, int>(location.column()));
    error.setDescription(description);

    exceptions << error;
}

/*!
    If \a script is a simple enumeration expression (eg. Text.AlignLeft),
    returns the integer equivalent (eg. 1), and sets \a ok to true.

    Otherwise sets \a ok to false.

    A valid \a ok must be provided, or the function will assert.
*/
int QQmlCustomParser::evaluateEnum(const QByteArray& script, bool *ok) const
{
    Q_ASSERT_X(ok, "QQmlCustomParser::evaluateEnum", "ok must not be a null pointer");
    *ok = false;

    // we support one or two '.' in the enum phrase:
    // * <TypeName>.<EnumValue>
    // * <TypeName>.<ScopedEnumName>.<EnumValue>

    auto nextDot = [&](int dot) {
        const int nextDot = script.indexOf('.', dot + 1);
        return (nextDot == script.length() - 1) ? -1 : nextDot;
    };

    int dot = nextDot(-1);
    if (dot == -1)
        return -1;

    QString scope = QString::fromUtf8(script.left(dot));

    if (scope != QLatin1String("Qt")) {
        if (imports.isNull())
            return -1;
        QQmlType type;

        if (imports.isT1()) {
            QQmlImportNamespace *ns = nullptr;
            if (!imports.asT1()->resolveType(scope, &type, nullptr, &ns))
                return -1;
            if (!type.isValid() && ns != nullptr) {
                dot = nextDot(dot);
                if (dot == -1 || !imports.asT1()->resolveType(QString::fromUtf8(script.left(dot)),
                                                              &type, nullptr, nullptr)) {
                    return -1;
                }
            }
        } else {
            QQmlTypeNameCache::Result result = imports.asT2()->query(scope);
            if (result.isValid()) {
                type = result.type;
            } else if (result.importNamespace) {
                dot = nextDot(dot);
                if (dot != -1)
                    type = imports.asT2()->query(QString::fromUtf8(script.left(dot))).type;
            }
        }

        if (!type.isValid())
            return -1;

        const int dot2 = nextDot(dot);
        const bool dot2Valid = (dot2 != -1);
        QByteArray enumValue = script.mid(dot2Valid ? dot2 + 1 : dot + 1);
        QByteArray scopedEnumName = (dot2Valid ? script.mid(dot + 1, dot2 - dot - 1) : QByteArray());
        if (!scopedEnumName.isEmpty())
            return type.scopedEnumValue(engine, scopedEnumName, enumValue, ok);
        else
            return type.enumValue(engine, QHashedCStringRef(enumValue.constData(), enumValue.length()), ok);
    }

    QByteArray enumValue = script.mid(dot + 1);
    const QMetaObject *mo = &Qt::staticMetaObject;
    int i = mo->enumeratorCount();
    while (i--) {
        int v = mo->enumerator(i).keyToValue(enumValue.constData(), ok);
        if (*ok)
            return v;
    }
    return -1;
}

/*!
    Resolves \a name to a type, or 0 if it is not a type. This can be used
    to type-check object nodes.
*/
const QMetaObject *QQmlCustomParser::resolveType(const QString& name) const
{
    if (!imports.isT1())
        return nullptr;
    QQmlType qmltype;
    if (!imports.asT1()->resolveType(name, &qmltype, nullptr, nullptr, nullptr))
        return nullptr;
    return qmltype.metaObject();
}

QT_END_NAMESPACE
