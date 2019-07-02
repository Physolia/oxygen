#ifndef oxygentoolbarengine_h
#define oxygentoolbarengine_h

//////////////////////////////////////////////////////////////////////////////
// oxygentoolbarengine.h
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

#include "oxygenbaseengine.h"
#include "oxygendatamap.h"
#include "oxygentoolbardata.h"

namespace Oxygen
{

    //* follow-mouse toolbar animation
    class ToolBarEngine: public BaseEngine
    {

        Q_OBJECT

        public:

        //* constructor
        explicit ToolBarEngine( QObject* parent ):
            BaseEngine( parent ),
            _followMouseDuration( 150 )
        {}

        //* register toolbar
        void registerWidget( QWidget* );

        //* returns registered widgets
        WidgetList registeredWidgets( void ) const override;

        //* return true if object is animated
        bool isAnimated( const QObject* );

        //* return true if object is animated
        bool isFollowMouseAnimated( const QObject* );

        //* animation opacity
        qreal opacity( const QObject* object )
        { return isAnimated( object ) ? _data.find( object ).data()->opacity(): AnimationData::OpacityInvalid; }

        //* return 'hover' rect position when widget is animated
        QRect currentRect( const QObject* );

        //* return 'hover' rect position when widget is animated
        QRect animatedRect( const QObject* );

        //* timer
        bool isTimerActive( const QObject* );

        //* enable state
        void setEnabled( bool value ) override
        {
            BaseEngine::setEnabled( value );
            _data.setEnabled( value );
        }

        //* duration
        void setDuration( int value ) override
        {
            BaseEngine::setDuration( value );
            _data.setDuration( value );
        }

        //* duration
        int followMouseDuration( void ) const
        { return _followMouseDuration; }

        //* duration
        void setFollowMouseDuration( int duration )
        {
            _followMouseDuration = duration;
            foreach( const DataMap<ToolBarData>::Value& value, _data )
            { if( value ) value.data()->setFollowMouseDuration( duration ); }
        }

        protected Q_SLOTS:

        //* remove widget from map
        bool unregisterWidget( QObject* object ) override
        { return _data.unregisterWidget( object ); }

        private:

        //* follow mouse animation duration
        int _followMouseDuration = -1;

        //* data map
        DataMap<ToolBarData> _data;

    };

}

#endif
