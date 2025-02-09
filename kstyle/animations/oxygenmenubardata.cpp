//////////////////////////////////////////////////////////////////////////////
// oxygenmenubardata.cpp
// data container for QMenuBar animations
// -------------------
//
// SPDX-FileCopyrightText: 2009 Hugo Pereira Da Costa <hugo.pereira@free.fr>
//
// SPDX-License-Identifier: MIT
//////////////////////////////////////////////////////////////////////////////

#include "oxygenmenubardata.h"

#include <QTextStream>

namespace Oxygen
{

//______________________________________________
MenuBarData::MenuBarData(QObject *parent, QWidget *target)
    : AnimationData(parent, target)
    , _isMenu(qobject_cast<QMenu *>(target))
    , _motions(-1)
{
}

//______________________________________________
MenuBarDataV1::MenuBarDataV1(QObject *parent, QWidget *target, int duration)
    : MenuBarData(parent, target)
{
    target->installEventFilter(this);

    // setup timeLine
    _current._animation = new Animation(duration, this);
    setupAnimation(currentAnimation(), "currentOpacity");
    currentAnimation().data()->setDirection(Animation::Forward);

    _previous._animation = new Animation(duration, this);
    setupAnimation(previousAnimation(), "previousOpacity");
    previousAnimation().data()->setDirection(Animation::Backward);
}

//______________________________________________
bool MenuBarDataV1::eventFilter(QObject *object, QEvent *event)
{
    if (!(enabled() && object == target().data())) {
        return AnimationData::eventFilter(object, event);
    }

    // check event type
    switch (event->type()) {
    case QEvent::Enter: {
        // first need to call proper event processing
        // then implement transition
        object->event(event);
        enterEvent(object);
        if (_isMenu)
            _motions = -1;
        break;
    }

    case QEvent::Leave: {
        // first need to call proper event processing
        // then implement transition
        object->event(event);
        leaveEvent(object);
        break;
    }

    case QEvent::MouseMove: {
        // first need to call proper event processing
        // then implement transition
        if (!_isMenu || _motions++ > 0)
            object->event(event);
        mouseMoveEvent(object);
        break;
    }

    case QEvent::MouseButtonPress: {
        // first need to call proper event processing
        // then implement transition
        mousePressEvent(object);
        break;
    }

    default:
        break;
    }

    // always forward event
    return AnimationData::eventFilter(object, event);
}

//______________________________________________
MenuBarDataV2::MenuBarDataV2(QObject *parent, QWidget *target, int duration)
    : MenuBarData(parent, target)
    , _opacity(0)
    , _progress(0)
    , _entered(true)
{
    target->installEventFilter(this);

    _animation = new Animation(duration, this);
    animation().data()->setDirection(Animation::Forward);
    animation().data()->setStartValue(0.0);
    animation().data()->setEndValue(1.0);
    animation().data()->setTargetObject(this);
    animation().data()->setPropertyName("opacity");

    _progressAnimation = new Animation(duration, this);
    progressAnimation().data()->setDirection(Animation::Forward);
    progressAnimation().data()->setStartValue(0);
    progressAnimation().data()->setEndValue(1);
    progressAnimation().data()->setTargetObject(this);
    progressAnimation().data()->setPropertyName("progress");
    progressAnimation().data()->setEasingCurve(QEasingCurve::Linear);
}

//______________________________________________
bool MenuBarDataV2::eventFilter(QObject *object, QEvent *event)
{
    if (!enabled())
        return false;

    // check event type
    switch (event->type()) {
    case QEvent::Enter: {
        // first need to call proper event processing
        // then implement transition
        object->event(event);
        enterEvent(object);
        if (!_isMenu)
            _motions = -1;
        break;
    }

    case QEvent::Hide:
    case QEvent::Leave: {
        // first need to call proper event processing
        // then implement transition
        object->event(event);
        if (_timer.isActive())
            _timer.stop();
        _timer.start(100, this);
        break;
    }

    case QEvent::MouseMove: {
        // first need to call proper event processing
        // then implement transition
        if (!_isMenu || _motions++ > 0)
            object->event(event);
        mouseMoveEvent(object);
        break;
    }

    default:
        break;
    }

    // always forward event
    return false;
}

//____________________________________________________________
void MenuBarDataV2::updateAnimatedRect(void)
{
    // check rect validity
    if (!(currentRect().isValid() && previousRect().isValid())) {
        _animatedRect = QRect();
        return;
    }

    // compute rect located 'between' previous and current
    _animatedRect.setLeft(previousRect().left() + progress() * (currentRect().left() - previousRect().left()));
    _animatedRect.setRight(previousRect().right() + progress() * (currentRect().right() - previousRect().right()));
    _animatedRect.setTop(previousRect().top() + progress() * (currentRect().top() - previousRect().top()));
    _animatedRect.setBottom(previousRect().bottom() + progress() * (currentRect().bottom() - previousRect().bottom()));

    // trigger update
    setDirty();

    return;
}

//___________________________________________________________
void MenuBarDataV2::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != _timer.timerId())
        return AnimationData::timerEvent(event);
    _timer.stop();
    leaveEvent(target().data());
}
}
