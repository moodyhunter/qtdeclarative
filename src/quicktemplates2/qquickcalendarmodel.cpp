// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickcalendarmodel_p.h"

#include <QtCore/private/qabstractitemmodel_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype CalendarModel
    \inherits QAbstractListModel
//! \instantiates QQuickCalendarModel
    \inqmlmodule QtQuick.Controls
    \brief A calendar model.

    CalendarModel provides a way of creating a range of MonthGrid
    instances. It is typically used as a model for a ListView that uses
    MonthGrid as a delegate.

    \snippet qtquickcontrols2-calendarmodel.qml 1

    In addition to the \c index property, a list of model data roles
    are available in the context of each delegate:
    \table
        \row \li \b model.month : int \li The number of the month
        \row \li \b model.year : int \li The number of the year
    \endtable

    \include zero-based-months.qdocinc

    \sa MonthGrid, Calendar
*/

class QQuickCalendarModelPrivate : public QAbstractItemModelPrivate
{
    Q_DECLARE_PUBLIC(QQuickCalendarModel)

public:
    QQuickCalendarModelPrivate() : complete(false),
        from(1,1,1), to(275759, 9, 25), count(0)
    {
    }

    static int getCount(const QDate& from, const QDate &to);

    void populate(const QDate &from, const QDate &to, bool force = false);

    bool complete;
    QDate from;
    QDate to;
    int count;
};

int QQuickCalendarModelPrivate::getCount(const QDate& from, const QDate &to)
{
    if (!from.isValid() || !to.isValid())
        return 0;

    QDate f(from.year(), from.month(), 1);
    QDate t(to.year(), to.month(), to.daysInMonth());
    int days = f.daysTo(t);
    if (days < 0)
        return 0;

    QDate r = QDate(1, 1, 1).addDays(days);
    int years = r.year() - 1;
    int months = r.month() - 1;
    return 12 * years + months + (r.day() / t.day());
}

void QQuickCalendarModelPrivate::populate(const QDate &f, const QDate &t, bool force)
{
    Q_Q(QQuickCalendarModel);
    if (!force && f == from && t == to)
        return;

    int c = getCount(from, to);
    if (c != count) {
        q->beginResetModel();
        count = c;
        q->endResetModel();
        emit q->countChanged();
    } else {
        emit q->dataChanged(q->index(0, 0), q->index(c - 1, 0));
    }
}

QQuickCalendarModel::QQuickCalendarModel(QObject *parent) :
    QAbstractListModel(*(new QQuickCalendarModelPrivate), parent)
{
}

/*!
    \qmlproperty date QtQuick.Controls::CalendarModel::from

    This property holds the start date.
*/
QDate QQuickCalendarModel::from() const
{
    Q_D(const QQuickCalendarModel);
    return d->from;
}

void QQuickCalendarModel::setFrom(const QDate &from)
{
    Q_D(QQuickCalendarModel);
    if (d->from != from) {
        if (d->complete)
            d->populate(from, d->to);
        d->from = from;
        emit fromChanged();
    }
}

/*!
    \qmlproperty date QtQuick.Controls::CalendarModel::to

    This property holds the end date.
*/
QDate QQuickCalendarModel::to() const
{
    Q_D(const QQuickCalendarModel);
    return d->to;
}

void QQuickCalendarModel::setTo(const QDate &to)
{
    Q_D(QQuickCalendarModel);
    if (d->to != to) {
        if (d->complete)
            d->populate(d->from, to);
        d->to = to;
        emit toChanged();
    }
}

/*!
    \qmlmethod int QtQuick.Controls::CalendarModel::monthAt(int index)

    Returns the month number at the specified model \a index.
*/
int QQuickCalendarModel::monthAt(int index) const
{
    Q_D(const QQuickCalendarModel);
    return d->from.addMonths(index).month() - 1;
}

/*!
    \qmlmethod int QtQuick.Controls::CalendarModel::yearAt(int index)

    Returns the year number at the specified model \a index.
*/
int QQuickCalendarModel::yearAt(int index) const
{
    Q_D(const QQuickCalendarModel);
    return d->from.addMonths(index).year();
}

/*!
    \qmlmethod int QtQuick.Controls::CalendarModel::indexOf(Date date)

    Returns the model index of the specified \a date.
*/
int QQuickCalendarModel::indexOf(const QDate &date) const
{
    Q_D(const QQuickCalendarModel);
    return d->getCount(d->from, date) - 1;
}

/*!
    \qmlmethod int QtQuick.Controls::CalendarModel::indexOf(int year, int month)

    Returns the model index of the specified \a year and \a month.
*/
int QQuickCalendarModel::indexOf(int year, int month) const
{
    return indexOf(QDate(year, month + 1, 1));
}

QVariant QQuickCalendarModel::data(const QModelIndex &index, int role) const
{
    Q_D(const QQuickCalendarModel);
    if (index.isValid() && index.row() < d->count) {
        switch (role) {
        case MonthRole:
            return monthAt(index.row());
        case YearRole:
            return yearAt(index.row());
        default:
            break;
        }
    }
    return QVariant();
}

int QQuickCalendarModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const QQuickCalendarModel);
    if (!parent.isValid())
        return d->count;
    return 0;
}

QHash<int, QByteArray> QQuickCalendarModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[MonthRole] = QByteArrayLiteral("month");
    roles[YearRole] = QByteArrayLiteral("year");
    return roles;
}

void QQuickCalendarModel::classBegin()
{
}

void QQuickCalendarModel::componentComplete()
{
    Q_D(QQuickCalendarModel);
    d->complete = true;
    d->populate(d->from, d->to, true);
}

QT_END_NAMESPACE

#include "moc_qquickcalendarmodel_p.cpp"
