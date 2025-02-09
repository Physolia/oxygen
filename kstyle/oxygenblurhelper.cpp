//////////////////////////////////////////////////////////////////////////////
// oxygenblurhelper.cpp
// handle regions passed to kwin for blurring
// -------------------
//
// SPDX-FileCopyrightText: 2010 Hugo Pereira Da Costa <hugo.pereira@free.fr>
//
// Loosely inspired (and largely rewritten) from BeSpin style
// SPDX-FileCopyrightText: 2007 Thomas Luebking <thomas.luebking@web.de>
//
// SPDX-License-Identifier: MIT
//////////////////////////////////////////////////////////////////////////////

#include "oxygenblurhelper.h"

#include "oxygenstyleconfigdata.h"

#include <QEvent>
#include <QProgressBar>
#include <QPushButton>
#include <QVector>

namespace Oxygen
{

//___________________________________________________________
BlurHelper::BlurHelper(QObject *parent, StyleHelper &helper)
    : QObject(parent)
    , _helper(helper)
    , _enabled(false)
{
#if OXYGEN_HAVE_X11

    if (_helper.isX11()) {
        // create atom
        _blurAtom = _helper.createAtom(QStringLiteral("_KDE_NET_WM_BLUR_BEHIND_REGION"));
        _opaqueAtom = _helper.createAtom(QStringLiteral("_NET_WM_OPAQUE_REGION"));
    } else {
        _blurAtom = 0;
        _opaqueAtom = 0;
    }

#endif
}

//___________________________________________________________
void BlurHelper::registerWidget(QWidget *widget)
{
    // check if already registered
    if (_widgets.contains(widget))
        return;

    // install event filter
    addEventFilter(widget);

    // add to widgets list
    _widgets.insert(widget);

    // cleanup on destruction
    connect(widget, SIGNAL(destroyed(QObject *)), SLOT(widgetDestroyed(QObject *)));

    if (enabled()) {
        // schedule shadow area repaint
        _pendingWidgets.insert(widget, widget);
        update();
    }
}

//___________________________________________________________
void BlurHelper::unregisterWidget(QWidget *widget)
{
    // remove from widgets
    if (!_widgets.remove(widget))
        return;

    // remove event filter
    widget->removeEventFilter(this);

    if (isTransparent(widget))
        clear(widget);
}

//___________________________________________________________
bool BlurHelper::eventFilter(QObject *object, QEvent *event)
{
    // do nothing if not enabled
    if (!enabled())
        return false;

    switch (event->type()) {
    case QEvent::Hide: {
        QWidget *widget(qobject_cast<QWidget *>(object));
        if (widget && isOpaque(widget) && isTransparent(widget->window())) {
            QWidget *window(widget->window());
            _pendingWidgets.insert(window, window);
            update();
        }
        break;
    }

    case QEvent::Show:
    case QEvent::Resize: {
        // cast to widget and check
        QWidget *widget(qobject_cast<QWidget *>(object));
        if (!widget)
            break;
        if (isTransparent(widget)) {
            _pendingWidgets.insert(widget, widget);
            update();

        } else if (isOpaque(widget)) {
            QWidget *window(widget->window());
            if (isTransparent(window)) {
                _pendingWidgets.insert(window, window);
                update();
            }
        }

        break;
    }

    default:
        break;
    }

    // never eat events
    return false;
}

//___________________________________________________________
QRegion BlurHelper::blurRegion(QWidget *widget) const
{
    if (!widget->isVisible())
        return QRegion();

    // get main region
    QRegion region;
    if (qobject_cast<const QDockWidget *>(widget) || qobject_cast<const QMenu *>(widget) || qobject_cast<const QToolBar *>(widget)
        || widget->inherits("QComboBoxPrivateContainer")) {
        region = _helper.roundedMask(widget->rect());

    } else
        region = widget->mask().isEmpty() ? widget->rect() : widget->mask();

    // trim blur region to remove unnecessary areas
    trimBlurRegion(widget, widget, region);
    return region;
}

//___________________________________________________________
void BlurHelper::trimBlurRegion(QWidget *parent, QWidget *widget, QRegion &region) const
{
    // loop over children
    const auto children = widget->children();
    for (QObject *childObject : children) {
        QWidget *child(qobject_cast<QWidget *>(childObject));
        if (!(child && child->isVisible()))
            continue;

        if (isOpaque(child)) {
            const QPoint offset(child->mapTo(parent, QPoint(0, 0)));
            if (child->mask().isEmpty()) {
                const QRect rect(child->rect().translated(offset).adjusted(1, 1, -1, -1));
                region -= rect;

            } else
                region -= child->mask().translated(offset);

        } else {
            trimBlurRegion(parent, child, region);
        }
    }

    return;
}

//___________________________________________________________
void BlurHelper::update(QWidget *widget) const
{
#if OXYGEN_HAVE_X11
    if (!_helper.isX11())
        return;

    /*
    directly from bespin code. Supposibly prevent playing with some 'pseudo-widgets'
    that have winId matching some other -random- window
    */
    if (!(widget->testAttribute(Qt::WA_WState_Created) || widget->internalWinId())) {
        return;
    }

    const QRegion blurRegion(this->blurRegion(widget));
    const QRegion opaqueRegion = QRegion(0, 0, widget->width(), widget->height()) - blurRegion;
    if (blurRegion.isEmpty()) {
        clear(widget);

    } else {
        QVector<quint32> data;
        for (const QRect &rect : blurRegion) {
            data << rect.x() << rect.y() << rect.width() << rect.height();
        }

        xcb_change_property(_helper.connection(), XCB_PROP_MODE_REPLACE, widget->winId(), _blurAtom, XCB_ATOM_CARDINAL, 32, data.size(), data.constData());

        data.clear();
        for (const QRect &rect : opaqueRegion) {
            data << rect.x() << rect.y() << rect.width() << rect.height();
        }

        xcb_change_property(_helper.connection(), XCB_PROP_MODE_REPLACE, widget->winId(), _opaqueAtom, XCB_ATOM_CARDINAL, 32, data.size(), data.constData());
        xcb_flush(_helper.connection());
    }

    // force update
    if (widget->isVisible()) {
        widget->update();
    }

#else

    Q_UNUSED(widget)

#endif
}

//___________________________________________________________
void BlurHelper::clear(QWidget *widget) const
{
#if OXYGEN_HAVE_X11
    if (!_helper.isX11())
        return;

    xcb_delete_property(_helper.connection(), widget->winId(), _blurAtom);
    xcb_delete_property(_helper.connection(), widget->winId(), _opaqueAtom);
#else
    Q_UNUSED(widget)
#endif
}

//___________________________________________________________
bool BlurHelper::isOpaque(const QWidget *widget) const
{
    return (!widget->isWindow())
        && ((widget->autoFillBackground() && widget->palette().color(widget->backgroundRole()).alpha() == 0xff)
            || widget->testAttribute(Qt::WA_OpaquePaintEvent));
}

//___________________________________________________________
bool BlurHelper::isTransparent(const QWidget *widget) const
{
    return widget->isWindow() && widget->testAttribute(Qt::WA_TranslucentBackground) &&

        // widgets using qgraphicsview
        !(widget->graphicsProxyWidget() || widget->inherits("Plasma::Dialog")) &&

        // flags and special widgets
        (widget->testAttribute(Qt::WA_StyledBackground) || qobject_cast<const QMenu *>(widget) || qobject_cast<const QDockWidget *>(widget)
         || qobject_cast<const QToolBar *>(widget) || widget->windowType() == Qt::ToolTip)
        && _helper.hasAlphaChannel(widget);
}
}
