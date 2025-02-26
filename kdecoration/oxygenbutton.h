#ifndef oxygenbutton_h
#define oxygenbutton_h

/*
 * SPDX-FileCopyrightText: 2006, 2007 Riccardo Iaconelli <riccardo@kde.org>
 * SPDX-FileCopyrightText: 2006, 2007 Casper Boemann <cbr@boemann.dk>
 * SPDX-FileCopyrightText: 2009 Hugo Pereira Da Costa <hugo.pereira@free.fr>
 * SPDX-FileCopyrightText: 2015 David Edmundson <davidedmundson@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "oxygen.h"
#include "oxygenanimation.h"
#include "oxygendecohelper.h"
#include "oxygendecoration.h"

#include <KDecoration3/DecorationButton>

namespace Oxygen
{
class Button : public KDecoration3::DecorationButton
{
    Q_OBJECT

    //* declare active state opacity
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    //* constructor
    explicit Button(QObject *parent, const QVariantList &args);

    //* button creation
    static Button *create(KDecoration3::DecorationButtonType type, KDecoration3::Decoration *decoration, QObject *parent);

    //* render
    void paint(QPainter *painter, const QRectF &repaintRegion) override;

    //* flag
    enum Flag { FlagNone, FlagStandalone, FlagFirstInList, FlagLastInList };

    //* flag
    void setFlag(Flag value)
    {
        m_flag = value;
    }

    //* standalone buttons
    bool isStandAlone() const
    {
        return m_flag == FlagStandalone;
    }

    //* offset
    void setOffset(const QPointF &value)
    {
        m_offset = value;
    }

    //* horizontal offset, for rendering
    void setHorizontalOffset(qreal value)
    {
        m_offset.setX(value);
    }

    //* vertical offset, for rendering
    void setVerticalOffset(qreal value)
    {
        m_offset.setY(value);
    }

    //* set icon size
    void setIconSize(const QSize &value)
    {
        m_iconSize = value;
    }

    //*@name active state change animation
    //@{
    void setOpacity(qreal value)
    {
        if (m_opacity == value)
            return;
        m_opacity = value;
        update();
    }

    qreal opacity(void) const
    {
        return m_opacity;
    }

    //@}

private Q_SLOTS:

    //* apply configuration changes
    void reconfigure();

    //* animation state
    void updateAnimationState(bool);

private:
    //* draw icon
    void drawIcon(QPainter *);

    //*@name colors
    //@{

    QColor foregroundColor(const QPalette &) const;
    QColor foregroundColor(const QPalette &palette, bool active) const;

    QColor backgroundColor(const QPalette &) const;
    QColor backgroundColor(const QPalette &palette, bool active) const;

    //@}

    //* true if animation is in progress
    bool isAnimated(void) const
    {
        return m_animation->state() == QPropertyAnimation::Running;
    }

    //* true if button is active
    bool isActive(void) const;

    //*@name button properties
    //@{

    //* true if button if of close type
    bool isSpacer(void) const
    {
        return type() == KDecoration3::DecorationButtonType::Spacer;
    }

    //* true if button if of menu type
    bool isMenuButton(void) const
    {
        return type() == KDecoration3::DecorationButtonType::Menu || type() == KDecoration3::DecorationButtonType::ApplicationMenu;
    }

    //* true if button is of toggle type
    bool isToggleButton(void) const
    {
        return type() == KDecoration3::DecorationButtonType::OnAllDesktops || type() == KDecoration3::DecorationButtonType::KeepAbove
            || type() == KDecoration3::DecorationButtonType::KeepBelow;
    }

    //* true if button if of close type
    bool isCloseButton(void) const
    {
        return type() == KDecoration3::DecorationButtonType::Close;
    }

    //* true if button has decoration
    bool hasDecoration(void) const
    {
        return !isMenuButton();
    }

    //@}

    //* private constructor
    explicit Button(KDecoration3::DecorationButtonType type, Decoration *decoration, QObject *parent);

    Flag m_flag = FlagNone;

    //* glow animation
    QPropertyAnimation *m_animation;

    //* vertical offset (for rendering)
    QPointF m_offset;

    //* icon size
    QSize m_iconSize;

    //* glow intensity
    qreal m_opacity;
};

} // namespace

#endif
