//////////////////////////////////////////////////////////////////////////////
// oxygenmenubarengine.cpp
// stores event filters and maps widgets to timelines for animations
// -------------------
//
// Copyright (c) 2009 Hugo Pereira Da Costa <hugo.pereira@free.fr>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//////////////////////////////////////////////////////////////////////////////

#include "oxygenmenubarengine.h"
#include "oxygenmenubarengine.moc"

#include <QtCore/QEvent>

namespace Oxygen
{

    //____________________________________________________________
    bool MenuBarEngineV1::registerWidget( QWidget* widget )
    {

        if( !( enabled() && widget ) ) return false;

        // create new data class
        if( !data_.contains( widget ) ) data_.insert( widget, new MenuBarDataV1( this, widget, duration() ) );

        // connect destruction signal
        disconnect( widget, SIGNAL( destroyed( QObject* ) ), this, SLOT( unregisterWidget( QObject* ) ) );
        connect( widget, SIGNAL( destroyed( QObject* ) ), this, SLOT( unregisterWidget( QObject* ) ) );
        return true;

    }

    //____________________________________________________________
    bool MenuBarEngineV1::isAnimated( const QObject* object, const QPoint& position )
    {
        DataMap<MenuBarDataV1>::Value data( data_.find( object ) );
        if( !data ) return false;
        if( Animation::Pointer animation = data.data()->animation( position ) ) return animation.data()->isRunning();
        else return false;
    }

    //____________________________________________________________
    bool MenuBarEngineV2::registerWidget( QWidget* widget )
    {

        if( !( enabled() && widget ) ) return false;

        // create new data class
        if( !data_.contains( widget ) ) data_.insert( widget, DataMap<MenuBarDataV2>::Value( new MenuBarDataV2( this, widget, duration() ) ) );

        // connect destruction signal
        disconnect( widget, SIGNAL( destroyed( QObject* ) ), this, SLOT( unregisterWidget( QObject* ) ) );
        connect( widget, SIGNAL( destroyed( QObject* ) ), this, SLOT( unregisterWidget( QObject* ) ) );
        return true;

    }


    //____________________________________________________________
    bool MenuBarEngineV2::isAnimated( const QObject* object, const QPoint& )
    {
        if( !enabled() ) return false;
        DataMap<MenuBarDataV2>::Value data( data_.find( object ) );
        if( !data ) return false;
        if( data.data()->animation() && data.data()->animation().data()->isRunning() ) return true;
        else if( Animation::Pointer animation = data.data()->progressAnimation() ) return animation.data()->isRunning();
        else return false;

    }

    //____________________________________________________________
    QRect MenuBarEngineV2::currentRect( const QObject* object, const QPoint& )
    {
        if( !enabled() ) return QRect();
        DataMap<MenuBarDataV2>::Value data( data_.find( object ) );
        return data ? data.data()->currentRect():QRect();
    }

    //____________________________________________________________
    QRect MenuBarEngineV2::animatedRect( const QObject* object )
    {
        if( !enabled() ) return QRect();
        DataMap<MenuBarDataV2>::Value data( data_.find( object ) );
        return data ? data.data()->animatedRect():QRect();

    }

    //____________________________________________________________
    bool MenuBarEngineV2::isTimerActive( const QObject* object )
    {
        if( !enabled() ) return false;
        DataMap<MenuBarDataV2>::Value data( data_.find( object ) );
        return data ? data.data()->timer().isActive():false;
    }

}
