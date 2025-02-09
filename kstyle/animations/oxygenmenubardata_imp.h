#ifndef oxygenmenubardata_imp_h
#define oxygenmenubardata_imp_h

//////////////////////////////////////////////////////////////////////////////
// oxygenmenubardata_imp.h
// implements menubar data templatized methods
// -------------------
//
// SPDX-FileCopyrightText: 2009 Hugo Pereira Da Costa <hugo.pereira@free.fr>
//
// SPDX-License-Identifier: MIT
//////////////////////////////////////////////////////////////////////////////

namespace Oxygen
{

//________________________________________________________________________
template<typename T>
void MenuBarDataV1::enterEvent(const QObject *object)
{
    const T *local = qobject_cast<const T *>(object);
    if (!local)
        return;

    // if the current action is still active, one does nothing
    if (local->activeAction() == currentAction().data())
        return;

    if (currentAnimation().data()->isRunning())
        currentAnimation().data()->stop();
    clearCurrentAction();
    clearCurrentRect();
}

//________________________________________________________________________
template<typename T>
void MenuBarDataV1::leaveEvent(const QObject *object)
{
    const T *local = qobject_cast<const T *>(object);
    if (!local)
        return;

    // if the current action is still active, one does nothing
    if (local->activeAction() == currentAction().data())
        return;

    if (currentAnimation().data()->isRunning())
        currentAnimation().data()->stop();
    if (previousAnimation().data()->isRunning())
        previousAnimation().data()->stop();
    if (currentAction()) {
        setPreviousRect(currentRect());
        clearCurrentAction();
        clearCurrentRect();
        previousAnimation().data()->start();
    }

    // trigger update
    setDirty();
}

//________________________________________________________________________
template<typename T>
void MenuBarDataV1::mouseMoveEvent(const QObject *object)
{
    const T *local = qobject_cast<const T *>(object);
    if (!local)
        return;

    // check action
    if (local->activeAction() == currentAction().data())
        return;

    bool hasCurrentAction(currentAction());

    // check current action
    if (currentAction()) {
        if (currentAnimation().data()->isRunning())
            currentAnimation().data()->stop();
        if (previousAnimation().data()->isRunning()) {
            previousAnimation().data()->setCurrentTime(0);
            previousAnimation().data()->stop();
        }

        // only start fadeout effect if there is no new selected action
        // if( !activeActionValid )
        if (!local->activeAction()) {
            setPreviousRect(currentRect());
            previousAnimation().data()->start();
        }

        clearCurrentAction();
        clearCurrentRect();
    }

    // check if local current actions is valid
    bool activeActionValid(local->activeAction() && local->activeAction()->isEnabled() && !local->activeAction()->isSeparator());
    if (activeActionValid) {
        if (currentAnimation().data()->isRunning())
            currentAnimation().data()->stop();

        setCurrentAction(local->activeAction());
        setCurrentRect(local->actionGeometry(currentAction().data()));
        if (!hasCurrentAction) {
            currentAnimation().data()->start();
        }
    }
}

//________________________________________________________________________
template<typename T>
void MenuBarDataV1::mousePressEvent(const QObject *object)
{
    const T *local = qobject_cast<const T *>(object);
    if (!local)
        return;

    // check action
    if (local->activeAction() == currentAction().data())
        return;

    // check current action
    bool activeActionValid(local->activeAction() && local->activeAction()->isEnabled() && !local->activeAction()->isSeparator());
    if (currentAction() && !activeActionValid) {
        if (currentAnimation().data()->isRunning())
            currentAnimation().data()->stop();
        if (previousAnimation().data()->isRunning())
            previousAnimation().data()->stop();

        setPreviousRect(currentRect());
        previousAnimation().data()->start();

        clearCurrentAction();
        clearCurrentRect();
    }
}

//________________________________________________________________________
template<typename T>
void MenuBarDataV2::enterEvent(const QObject *object)
{
    // cast widget
    const T *local = qobject_cast<const T *>(object);
    if (!local)
        return;

    if (_timer.isActive())
        _timer.stop();

    // if the current action is still active, one does nothing
    if (currentAction() && local->activeAction() == currentAction().data())
        return;

    if (animation().data()->isRunning())
        animation().data()->stop();
    if (progressAnimation().data()->isRunning())
        progressAnimation().data()->stop();
    clearPreviousRect();
    clearAnimatedRect();

    if (local->activeAction() && local->activeAction()->isEnabled() && !local->activeAction()->isSeparator()) {
        setCurrentAction(local->activeAction());
        setCurrentRect(local->actionGeometry(currentAction().data()));
        animation().data()->setDirection(Animation::Forward);
        animation().data()->start();

    } else {
        clearCurrentAction();
        clearCurrentRect();
    }

    return;
}

//________________________________________________________________________
template<typename T>
void MenuBarDataV2::leaveEvent(const QObject *object)
{
    const T *local = qobject_cast<const T *>(object);
    if (!local)
        return;

    // if the current action is still active, one does nothing
    if (local->activeAction() && local->activeAction() == currentAction().data()) {
        return;
    }

    if (progressAnimation().data()->isRunning())
        progressAnimation().data()->stop();
    if (animation().data()->isRunning())
        animation().data()->stop();
    clearAnimatedRect();
    clearPreviousRect();
    if (currentAction()) {
        clearCurrentAction();
        animation().data()->setDirection(Animation::Backward);
        animation().data()->start();
    }

    // trigger update
    setDirty();

    return;
}

//________________________________________________________________________
template<typename T>
void MenuBarDataV2::mouseMoveEvent(const QObject *object)
{
    const T *local = qobject_cast<const T *>(object);
    if (!local)
        return;
    if (local->activeAction() == currentAction().data())
        return;

    // check if current position match another action
    if (local->activeAction() && local->activeAction()->isEnabled() && !local->activeAction()->isSeparator()) {
        if (_timer.isActive())
            _timer.stop();

        QAction *activeAction(local->activeAction());

        // update previous rect if the current action is valid
        QRect activeRect(local->actionGeometry(activeAction));
        if (currentAction()) {
            if (!progressAnimation().data()->isRunning()) {
                setPreviousRect(currentRect());

            } else if (progress() < 1 && currentRect().isValid() && previousRect().isValid()) {
                // re-calculate previous rect so that animatedRect
                // is unchanged after currentRect is updated
                // this prevents from having jumps in the animation
                qreal ratio = progress() / (1.0 - progress());
                _previousRect.adjust(ratio * (currentRect().left() - activeRect.left()),
                                     ratio * (currentRect().top() - activeRect.top()),
                                     ratio * (currentRect().right() - activeRect.right()),
                                     ratio * (currentRect().bottom() - activeRect.bottom()));
            }

            // update current action
            setCurrentAction(activeAction);
            setCurrentRect(activeRect);
            if (animation().data()->isRunning())
                animation().data()->stop();
            if (!progressAnimation().data()->isRunning())
                progressAnimation().data()->start();

        } else {
            // update current action
            setCurrentAction(activeAction);
            setCurrentRect(activeRect);
            if (!_entered) {
                _entered = true;
                if (animation().data()->isRunning())
                    animation().data()->stop();
                if (!progressAnimation().data()->isRunning())
                    progressAnimation().data()->start();

            } else {
                setPreviousRect(activeRect);
                clearAnimatedRect();
                if (progressAnimation().data()->isRunning())
                    progressAnimation().data()->stop();
                animation().data()->setDirection(Animation::Forward);
                if (!animation().data()->isRunning())
                    animation().data()->start();
            }
        }

    } else if (currentAction()) {
        _timer.start(150, this);
    }

    return;
}
}

#endif
