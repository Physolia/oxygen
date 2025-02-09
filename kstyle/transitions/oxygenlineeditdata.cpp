// krazy:excludeall=qclasses

//////////////////////////////////////////////////////////////////////////////
// oxygenlineeditdata.cpp
// data container for QLineEdit transition
// -------------------
//
// SPDX-FileCopyrightText: 2009 Hugo Pereira Da Costa <hugo.pereira@free.fr>
//
// SPDX-License-Identifier: MIT
//////////////////////////////////////////////////////////////////////////////

#include "oxygenlineeditdata.h"

#include <QDateTimeEdit>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QPainter>
#include <QSpinBox>
#include <QStyle>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QStyleOptionFrameV2>
#else
#include <QStyleOptionFrame>
#endif

namespace Oxygen
{

// use 20 milliseconds for animation lock
const int LineEditData::_lockTime = 20;

//______________________________________________________
LineEditData::LineEditData(QObject *parent, QLineEdit *target, int duration)
    : TransitionData(parent, target, duration)
    , _target(target)
    , _hasClearButton(false)
    , _edited(false)
{
    _target.data()->installEventFilter(this);

    checkClearButton();

    connect(_target.data(), SIGNAL(destroyed()), SLOT(targetDestroyed()));
    connect(_target.data(), SIGNAL(textEdited(QString)), SLOT(textEdited()));
    connect(_target.data(), SIGNAL(textChanged(QString)), SLOT(textChanged()));

    /*
    Additional signal/slot connections depending on widget's parent.
    This is needed because parents sometime disable the textChanged signal of the embedded
    QLineEdit
    */
    if (auto spinbox = qobject_cast<QSpinBox *>(_target.data()->parentWidget())) {
        connect(spinbox, &QSpinBox::textChanged, this, &LineEditData::textChanged);
    } else if (auto spinbox = qobject_cast<QDoubleSpinBox *>(_target.data()->parentWidget())) {
        connect(spinbox, &QDoubleSpinBox::textChanged, this, &LineEditData::textChanged);
    } else if (qobject_cast<QDateTimeEdit *>(_target.data()->parentWidget())) {
        connect(_target.data()->parentWidget(), SIGNAL(dateTimeChanged(QDateTime)), SLOT(textChanged()));
    }

    // update cached pixmap on selection change
    connect(_target.data(), SIGNAL(selectionChanged()), SLOT(selectionChanged()));
}

//___________________________________________________________________
bool LineEditData::eventFilter(QObject *object, QEvent *event)
{
    if (!(enabled() && object && object == _target.data())) {
        return TransitionData::eventFilter(object, event);
    }

    switch (event->type()) {
    case QEvent::Show:
    case QEvent::Resize:
    case QEvent::Move:
        transition().data()->setEndPixmap(QPixmap());
        break;

    default:
        break;
    }

    return TransitionData::eventFilter(object, event);
}

//___________________________________________________________________
void LineEditData::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == _timer.timerId()) {
        _timer.stop();
        checkClearButton();
        if (enabled() && transition() && _target && _target.data()->isVisible()) {
            setRecursiveCheck(true);
            transition().data()->setEndPixmap(transition().data()->grab(_target.data(), targetRect()));
            setRecursiveCheck(false);
        }

    } else if (event->timerId() == _animationLockTimer.timerId()) {
        unlockAnimations();

    } else
        return TransitionData::timerEvent(event);
}

//___________________________________________________________________
void LineEditData::checkClearButton(void)
{
    if (!_target)
        return;
    const QObjectList children = _target.data()->children();
    _hasClearButton = false;
    for (QObject *child : children) {
        if (child->inherits("KLineEditButton")) {
            _hasClearButton = true;
            _clearButtonRect = static_cast<QWidget *>(child)->geometry();
            break;
        }
    }

    return;
}

//___________________________________________________________________
void LineEditData::textEdited(void)
{
    _edited = true;
    if (!recursiveCheck()) {
        _timer.start(0, this);
    }
}

//___________________________________________________________________
void LineEditData::selectionChanged(void)
{
    if (!recursiveCheck()) {
        _timer.start(0, this);
    }
}

//___________________________________________________________________
void LineEditData::textChanged(void)
{
    // check whether text change was triggered manually
    // in which case do not start transition
    if (_edited) {
        _edited = false;
        return;
    }

    if (transition().data()->isAnimated()) {
        transition().data()->endAnimation();
    }

    if (isLocked()) {
        // if locked one do not start the new animation, to prevent flicker
        // instead, one hides the transition pixmap, trigger an update, and return.
        // animations are re-locked.
        transition().data()->hide();
        lockAnimations();
        _timer.start(0, this);
        return;
    }

    if (initializeAnimation()) {
        lockAnimations();
        animate();

    } else {
        transition().data()->hide();
    }
}

//___________________________________________________________________
bool LineEditData::initializeAnimation(void)
{
    if (!(enabled() && _target && _target.data()->isVisible()))
        return false;

    if (recursiveCheck())
        return false;

    QRect current(targetRect());

    transition().data()->setOpacity(0);
    transition().data()->setGeometry(current);

    if (_widgetRect.isValid() && !transition().data()->currentPixmap().isNull() && _widgetRect != current) {
        // if label geometry has changed since last animation
        // one must clone the pixmap to make it match the right
        // geometry before starting the animation.
        QPixmap pixmap(current.size());
        pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);
        p.drawPixmap(_widgetRect.topLeft() - current.topLeft(), transition().data()->currentPixmap());
        p.end();
        transition().data()->setStartPixmap(pixmap);

    } else {
        transition().data()->setStartPixmap(transition().data()->currentPixmap());
    }

    bool valid(!transition().data()->startPixmap().isNull());
    if (valid) {
        transition().data()->show();
        transition().data()->raise();
    }

    setRecursiveCheck(true);
    transition().data()->setEndPixmap(transition().data()->grab(_target.data(), targetRect()));
    setRecursiveCheck(false);

    return valid;
}

//___________________________________________________________________
bool LineEditData::animate(void)
{
    transition().data()->animate();
    return true;
}

//___________________________________________________________________
void LineEditData::targetDestroyed(void)
{
    setEnabled(false);
    _target.clear();
}
}
