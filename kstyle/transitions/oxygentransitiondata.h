#ifndef oxygentransitiondata_h
#define oxygentransitiondata_h

//////////////////////////////////////////////////////////////////////////////
// oxygentransitiondata.h
// data container for generic transitions
// -------------------
//
// SPDX-FileCopyrightText: 2009 Hugo Pereira Da Costa <hugo.pereira@free.fr>
//
// SPDX-License-Identifier: MIT
//////////////////////////////////////////////////////////////////////////////

#include "oxygentransitionwidget.h"

#include <QElapsedTimer>
#include <QObject>
#include <QTime>
#include <QWidget>

namespace Oxygen
{

//* generic data
class TransitionData : public QObject
{
    Q_OBJECT

public:
    //* constructor
    TransitionData(QObject *parent, QWidget *target, int);

    //* destructor
    ~TransitionData(void) override;

    //* enability
    virtual void setEnabled(bool value)
    {
        _enabled = value;
    }

    //* enability
    virtual bool enabled(void) const
    {
        return _enabled;
    }

    //* duration
    virtual void setDuration(int duration)
    {
        if (_transition) {
            _transition.data()->setDuration(duration);
        }
    }

    //* max render time
    void setMaxRenderTime(int value)
    {
        _maxRenderTime = value;
    }

    //* max renderTime
    const int &maxRenderTime(void) const
    {
        return _maxRenderTime;
    }

    //* start clock
    void startClock(void)
    {
        if (!_timer.isValid()) {
            _timer.start();
        } else {
            _timer.restart();
        }
    }

    //* check if rendering is too slow
    bool slow(void) const
    {
        return _timer.isValid() && _timer.elapsed() > maxRenderTime();
    }

protected Q_SLOTS:

    //* initialize animation
    virtual bool initializeAnimation(void) = 0;

    //* animate
    virtual bool animate(void) = 0;

protected:
    //* returns true if one parent matches given class name
    inline bool hasParent(const QWidget *, const char *) const;

    //* transition widget
    const TransitionWidget::Pointer &transition(void) const
    {
        return _transition;
    }

    //* used to avoid recursion when grabbing widgets
    void setRecursiveCheck(bool value)
    {
        _recursiveCheck = value;
    }

    //* used to avoid recursion when grabbing widgets
    bool recursiveCheck(void) const
    {
        return _recursiveCheck;
    }

private:
    //* enability
    bool _enabled = true;

    //* used to avoid recursion when grabbing widgets
    bool _recursiveCheck = false;

    //* timer used to detect slow rendering
    QElapsedTimer _timer;

    //* max render time
    /** used to detect slow rendering */
    int _maxRenderTime = 200;

    //* animation handling
    TransitionWidget::Pointer _transition;
};

//_____________________________________________________________________________________
bool TransitionData::hasParent(const QWidget *widget, const char *className) const
{
    if (!widget)
        return false;
    for (QWidget *parent = widget->parentWidget(); parent; parent = parent->parentWidget()) {
        if (parent->inherits(className))
            return true;
    }

    return false;
}
}

#endif
