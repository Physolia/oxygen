#ifndef oxygentoolboxengine_h
#define oxygentoolboxengine_h

//////////////////////////////////////////////////////////////////////////////
// oxygentoolboxengine.h
// QToolBox engine
// -------------------
//
// Copyright (c) 2010 Hugo Pereira Da Costa <hugo.pereira@free.fr>
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
#include "oxygenwidgetstatedata.h"

namespace Oxygen
{

    //* QToolBox animation engine
    class ToolBoxEngine: public BaseEngine
    {

        Q_OBJECT

        public:

        //* constructor
        explicit ToolBoxEngine( QObject* parent ):
            BaseEngine( parent )
        {}

        //* enability
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

        //* register widget
        bool registerWidget( QWidget* );

        //* true if widget hover state is changed
        bool updateState( const QPaintDevice*, bool );

        //* true if widget is animated
        bool isAnimated( const QPaintDevice* );

        //* animation opacity
        qreal opacity( const QPaintDevice* object )
        { return isAnimated( object ) ? data( object ).data()->opacity(): AnimationData::OpacityInvalid; }

        public Q_SLOTS:

        //* remove widget from map
        bool unregisterWidget( QObject* data ) override
        {

            if( !data ) return false;

            // reinterpret_cast is safe here since only the address is used to find
            // data in the map
            return _data.unregisterWidget( reinterpret_cast<QPaintDevice*>(data) );

        }

        protected:

        //* returns data associated to widget
        PaintDeviceDataMap<WidgetStateData>::Value data( const QPaintDevice* object )
        { return _data.find( object ).data(); }

        private:

        //* map
        PaintDeviceDataMap<WidgetStateData> _data;

    };

}

#endif
