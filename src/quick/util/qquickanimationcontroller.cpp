/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickanimationcontroller_p.h"
#include <QtQml/qqmlinfo.h>
#include <private/qqmlengine_p.h>

QT_BEGIN_NAMESPACE


class QQuickAnimationControllerPrivate : public QObjectPrivate, QAnimationJobChangeListener
{
    Q_DECLARE_PUBLIC(QQuickAnimationController)
public:
    QQuickAnimationControllerPrivate()
        : progress(0.0), animation(0), animationInstance(0), finalized(false) {}
    virtual void animationFinished(QAbstractAnimationJob *job);
    virtual void animationCurrentTimeChanged(QAbstractAnimationJob *job, int currentTime);


    qreal progress;
    QQuickAbstractAnimation *animation;
    QAbstractAnimationJob *animationInstance;
    bool finalized:1;

};

void QQuickAnimationControllerPrivate::animationFinished(QAbstractAnimationJob *job)
{
    Q_Q(QQuickAnimationController);
    Q_ASSERT(animationInstance && animationInstance == job);

    animationInstance->removeAnimationChangeListener(this, QAbstractAnimationJob::Completion | QAbstractAnimationJob::CurrentTime);

    if (animationInstance->direction() == QAbstractAnimationJob::Forward && progress != 1) {
        progress = 1;
        emit q->progressChanged();
    } else if (animationInstance->direction() == QAbstractAnimationJob::Backward && progress != 0) {
        progress = 0;
        emit q->progressChanged();
    }

}

void QQuickAnimationControllerPrivate::animationCurrentTimeChanged(QAbstractAnimationJob *job, int currentTime)
{
    Q_Q(QQuickAnimationController);
    Q_ASSERT(animationInstance && animationInstance == job);
    const qreal newProgress = currentTime * 1.0 / animationInstance->duration();
    if (progress != newProgress) {
        progress = newProgress;
        emit q->progressChanged();
    }
}

/*!
    \qmlclass AnimationController QQuickAnimationController
    \inqmlmodule QtQuick 2
    \ingroup qml-animation-transition
    \brief Enables manual control of animations

    Normally animations are driven by an internal timer, but the AnimationController
    allows the given \a animation to be driven by a \a progress value explicitly.
*/


QQuickAnimationController::QQuickAnimationController(QObject *parent)
: QObject(*(new QQuickAnimationControllerPrivate), parent)
{
}

QQuickAnimationController::~QQuickAnimationController()
{
    Q_D(QQuickAnimationController);
    delete d->animationInstance;
}

/*!
    \qmlproperty real QtQuick2::AnimationController::progress
    This property holds the animation progress value.

    The valid \c progress value is 0.0 to 1.0, setting values less than 0 will be converted to 0,
    setting values great than 1 will be converted to 1.
*/
qreal QQuickAnimationController::progress() const
{
    Q_D(const QQuickAnimationController);
    return d->progress;
}

void QQuickAnimationController::setProgress(qreal progress)
{
    Q_D(QQuickAnimationController);
    progress = qBound(qreal(0), progress, qreal(1));

    if (progress != d->progress) {
        d->progress = progress;
        updateProgress();
        emit progressChanged();
    }
}

/*!
    \qmlproperty real QtQuick2::AnimationController::animation
    \default

    This property holds the animation to be controlled by the AnimationController.

    Note:An animation controlled by AnimationController will always have its
         \c running and \c paused properties set to true. It can not be manually
         started or stopped (much like an animation in a Behavior can not be manually started or stopped).
*/
QQuickAbstractAnimation *QQuickAnimationController::animation() const
{
    Q_D(const QQuickAnimationController);
    return d->animation;
}

void QQuickAnimationController::classBegin()
{
    QQmlEnginePrivate *engPriv = QQmlEnginePrivate::get(qmlEngine(this));
    engPriv->registerFinalizeCallback(this, this->metaObject()->indexOfSlot("componentFinalized()"));
}


void QQuickAnimationController::setAnimation(QQuickAbstractAnimation *animation)
{
    Q_D(QQuickAnimationController);

    if (animation != d->animation) {
        if (animation) {
            if (animation->userControlDisabled()) {
                qmlInfo(this) << "QQuickAnimationController::setAnimation: the animation is controlled by others, can't be used in AnimationController.";
                return;
            }
            animation->setDisableUserControl();
        }

        if (d->animation)
            d->animation->setEnableUserControl();

        d->animation = animation;
        reload();
        emit animationChanged();
    }
}

/*!
    \qmlmethod QtQuick2::AnimationController::reload()
    \brief Reloads the animation properties

    If the animation properties changed, calling this method to reload the animation definations.
*/
void QQuickAnimationController::reload()
{
    Q_D(QQuickAnimationController);
    if (!d->finalized)
        return;

    if (!d->animation) {
        d->animationInstance = 0;
    } else {
        QQuickStateActions actions;
        QQmlProperties properties;
        QAbstractAnimationJob *oldInstance = d->animationInstance;
        d->animationInstance = d->animation->transition(actions, properties, QQuickAbstractAnimation::Forward);
        if (oldInstance && oldInstance != d->animationInstance)
            delete oldInstance;
        if (d->animationInstance) {
            d->animationInstance->setLoopCount(1);
            d->animationInstance->setDisableUserControl();
            d->animationInstance->start();
            d->animationInstance->pause();
            updateProgress();
        }
    }
}

void QQuickAnimationController::updateProgress()
{
    Q_D(QQuickAnimationController);
    if (!d->animationInstance)
        return;

    d->animationInstance->setDisableUserControl();
    d->animationInstance->start();
    QQmlAnimationTimer::unregisterAnimation(d->animationInstance);
    d->animationInstance->setCurrentTime(d->progress * d->animationInstance->duration());
}

void QQuickAnimationController::componentFinalized()
{
    Q_D(QQuickAnimationController);
    d->finalized = true;
    reload();
}

/*!
    \qmlmethod QtQuick2::AnimationController::completeToBeginning()
    \brief Finishes running the controlled animation in a backwards direction.

    After calling this method, the animation runs normally from the current progress point
    in a backwards direction to the beginning state.

    The animation controller's progress value will be automatically updated while the animation is running.

    \sa completeToEnd(), progress()
*/
void QQuickAnimationController::completeToBeginning()
{
    Q_D(QQuickAnimationController);
    if (!d->animationInstance)
        return;

    if (d->progress == 0)
        return;

    d->animationInstance->addAnimationChangeListener(d, QAbstractAnimationJob::Completion | QAbstractAnimationJob::CurrentTime);
    d->animationInstance->setDirection(QAbstractAnimationJob::Backward);

    //Disable and then enable user control to trigger the animation instance's state change
    d->animationInstance->setDisableUserControl();
    d->animationInstance->setEnableUserControl();
    d->animationInstance->start();
}

/*!
    \qmlmethod QtQuick2::AnimationController::completeToEnd()
    \brief Finishes running the controlled animation in a forwards direction.

    After calling this method, the animation runs normally from the current progress point
    in a forwards direction to the end state.

    The animation controller's progress value will be automatically updated while the animation is running.

    \sa completeToBeginning(), progress()
*/
void QQuickAnimationController::completeToEnd()
{
    Q_D(QQuickAnimationController);
    if (!d->animationInstance)
        return;

    if (d->progress == 1)
        return;

    d->animationInstance->addAnimationChangeListener(d, QAbstractAnimationJob::Completion | QAbstractAnimationJob::CurrentTime);
    d->animationInstance->setDirection(QAbstractAnimationJob::Forward);

    //Disable and then enable user control to trigger the animation instance's state change
    d->animationInstance->setDisableUserControl();
    d->animationInstance->setEnableUserControl();
    d->animationInstance->start();
}



QT_END_NAMESPACE


