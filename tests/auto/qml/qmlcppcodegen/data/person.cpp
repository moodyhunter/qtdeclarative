// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "person.h"

using namespace Qt::StringLiterals;

Person::Person(QObject *parent)
    : QObject(parent), m_name(u"Bart"_s), m_shoeSize(0)
{
    m_things.append(u"thing"_s);
    m_things.append(30);
}

QString Person::name() const
{
    return m_name;
}

void Person::setName(const QString &n)
{
    if (n != m_name) {
        m_name = n;
        emit nameChanged();
    }
}

void Person::resetName()
{
    setName(u"Bart"_s);
}

int Person::shoeSize() const
{
    return m_shoeSize;
}

void Person::setShoeSize(int s)
{
    if (s != m_shoeSize) {
        m_shoeSize = s;
        emit shoeSizeChanged();
    }
}

QVariantList Person::things() const
{
    return m_things;
}

void Person::setThings(const QVariantList &things)
{
    if (m_things == things)
        return;
    m_things = things;
    emit thingsChanged();
}
