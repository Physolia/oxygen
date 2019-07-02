#ifndef oxygentoolbar_data_h
#define oxygentoolbar_data_h

//////////////////////////////////////////////////////////////////////////////
// oxygentoolbardata.h
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

#include "oxygenanimationdata.h"
#include <QBasicTimer>

namespace Oxygen
{

    //* toolbar data
    class ToolBarData: public AnimationData
    {

        Q_OBJECT
        Q_PROPERTY( qreal opacity READ opacity WRITE setOpacity )
        Q_PROPERTY( qreal progress READ progress  WRITE setProgress )

        public:

        //* constructor
        ToolBarData( QObject* parent, QWidget* target, int duration );

        //* event filter
        bool eventFilter( QObject*, QEvent* ) override;

        //* return animation associated to action at given position, if any
        const Animation::Pointer& animation( void ) const
        { return _animation; }

        //* return animation associated to action at given position, if any
        const Animation::Pointer& progressAnimation( void ) const
        { return _progressAnimation; }

        //* duration
        void setDuration( int duration ) override
        { animation().data()->setDuration( duration ); }

        //* duration
        void setFollowMouseDuration( int duration )
        { progressAnimation().data()->setDuration( duration ); }

        //* return 'hover' rect position when widget is animated
        const QRect& animatedRect( void ) const
        { return _animatedRect; }

        //* current rect
        const QRect& currentRect( void ) const
        { return _currentRect; }

        //* timer
        const QBasicTimer& timer( void ) const
        { return _timer; }

        //* animation opacity
        qreal opacity( void ) const
        { return _opacity; }

        //* animation opacity
        void setOpacity( qreal value )
        {
            value = digitize( value );
            if( _opacity == value ) return;
            _opacity = value;
            setDirty();
        }

        //* animation progress
        qreal progress( void ) const
        { return _progress; }

        //* animation progress
        void setProgress( qreal value )
        {
            value = digitize( value );
            if( _progress == value ) return;
            _progress = value;
            updateAnimatedRect();
        }

        protected Q_SLOTS:

        //* updated animated rect
        void updateAnimatedRect( void );

        protected:

        //* timer event
        void timerEvent( QTimerEvent *) override;

        private:

        //*@name current object handling
        //@{

        //* object pointer
        /*! there is no need to guard it because the object contents is never accessed */
        using ObjectPointer = const QObject*;

        //* current object
        const ObjectPointer& currentObject( void ) const
        { return _currentObject; }

        //* current object
        void setCurrentObject( const QObject* object )
        { _currentObject = ObjectPointer( object ); }

        //* current object
        void clearCurrentObject( void )
        { _currentObject = NULL; }

        //@}

        //*@name rect handling
        //@{

        //* current rect
        void setCurrentRect( const QRect& rect )
        { _currentRect = rect; }

        //* current rect
        void clearCurrentRect( void )
        { _currentRect = QRect(); }

        //* previous rect
        const QRect& previousRect( void ) const
        { return _previousRect; }

        //* previous rect
        void setPreviousRect( const QRect& rect )
        { _previousRect = rect; }

        //* previous rect
        void clearPreviousRect( void )
        { _previousRect = QRect(); }

        //* animated rect
        void clearAnimatedRect( void )
        { _animatedRect = QRect(); }

        //@}

        //* toolbar enterEvent
        void enterEvent( const QObject* );

        //* toolbar enterEvent
        void leaveEvent( const QObject* );

        //* toolbutton added
        void childAddedEvent( QObject* );

        //* toolbutton enter event
        void childEnterEvent( const QObject* );

        private:

        //* fade animation
        Animation::Pointer _animation;

        //* progress animation
        Animation::Pointer _progressAnimation;

        //* opacity
        qreal _opacity;

        //* opacity
        qreal _progress;

        //* timer
        /*! this allows to add some delay before starting leaveEvent animation */
        QBasicTimer _timer;

        //* current object
        ObjectPointer _currentObject;

        //* current rect
        QRect _currentRect;

        //* previous rect
        QRect _previousRect;

        //* animated rect
        QRect _animatedRect;

        //* true if toolbar was entered at least once (this prevents some initialization glitches)
        bool _entered;

    };

}

#endif
