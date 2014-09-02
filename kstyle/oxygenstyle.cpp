// krazy:excludeall=qclasses

//////////////////////////////////////////////////////////////////////////////
// oxygenstyle.cpp
// Oxygen widget style for KDE 4
// -------------------
//
// Copyright ( C ) 2009-2010 Hugo Pereira Da Costa <hugo.pereira@free.fr>
// Copyright ( C ) 2008 Long Huynh Huu <long.upcase@googlemail.com>
// Copyright ( C ) 2007-2008 Casper Boemann <cbr@boemann.dk>
// Copyright ( C ) 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>
// Copyright ( C ) 2003-2005 Sandro Giessl <sandro@giessl.com>
//
// based on the KDE style "dotNET":
// Copyright ( C ) 2001-2002, Chris Lee <clee@kde.org>
// Carsten Pfeiffer <pfeiffer@kde.org>
// Karol Szwed <gallium@kde.org>
// Drawing routines completely reimplemented from KDE3 HighColor, which was
// originally based on some stuff from the KDE2 HighColor.
//
// based on drawing routines of the style "Keramik":
// Copyright ( c ) 2002 Malte Starostik <malte@kde.org>
// ( c ) 2002,2003 Maksim Orlovich <mo002j@mail.rochester.edu>
// based on the KDE3 HighColor Style
// Copyright ( C ) 2001-2002 Karol Szwed <gallium@kde.org>
// ( C ) 2001-2002 Fredrik H?glund <fredrik@kde.org>
// Drawing routines adapted from the KDE2 HCStyle,
// Copyright ( C ) 2000 Daniel M. Duley <mosfet@kde.org>
// ( C ) 2000 Dirk Mueller <mueller@kde.org>
// ( C ) 2001 Martijn Klingens <klingens@kde.org>
// Progressbar code based on KStyle,
// Copyright ( C ) 2001-2002 Karol Szwed <gallium@kde.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License version 2 as published by the Free Software Foundation.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; see the file COPYING.LIB.  If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
// Boston, MA 02110-1301, USA.
//////////////////////////////////////////////////////////////////////////////

#include "oxygenstyle.h"
#include "oxygenstyle.moc"

#include "oxygen.h"
#include "oxygenanimations.h"
#include "oxygenblurhelper.h"
#include "oxygenframeshadow.h"
#include "oxygenmdiwindowshadow.h"
#include "oxygenmnemonics.h"
#include "oxygenshadowhelper.h"
#include "oxygensplitterproxy.h"
#include "oxygenstyleconfigdata.h"
#include "oxygentransitions.h"
#include "oxygenwidgetexplorer.h"
#include "oxygenwindowmanager.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDBusConnection>
#include <QDial>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QGraphicsView>
#include <QGroupBox>
#include <QItemDelegate>
#include <QLayout>
#include <QLineEdit>
#include <QLoggingCategory>
#include <QMainWindow>
#include <QMdiSubWindow>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QSpinBox>
#include <QSplitterHandle>
#include <QStyleOption>
#include <QTextEdit>
#include <QToolBar>
#include <QToolBox>
#include <QToolButton>

#include <KColorUtils>

#include <cmath>

#define USE_KDE4 0

Q_LOGGING_CATEGORY(OXYGEN, "oxygen")

namespace OxygenPrivate
{

    /*!
    tabBar data class needed for
    the rendering of tabbars when
    one tab is being drawn
    */
    class TabBarData: public QObject
    {

        public:

        //! constructor
        explicit TabBarData( Oxygen::Style* parent ):
            QObject( parent ),
            _style( parent ),
            _dirty( false )
        {}

        //! destructor
        virtual ~TabBarData( void )
        {}

        //! assign target tabBar
        void lock( const QWidget* widget )
        { _tabBar = widget; }

        //! true if tabbar is locked
        bool locks( const QWidget* widget ) const
        { return _tabBar && _tabBar.data() == widget; }

        //! set dirty
        void setDirty( const bool& value = true )
        { _dirty = value; }

        //! release
        void release( void )
        { _tabBar.clear(); }

        //! draw tabBarBase
        virtual void drawTabBarBaseControl( const QStyleOptionTab*, QPainter*, const QWidget* );

        private:

        //! pointer to parent style object
        Oxygen::WeakPointer<const Oxygen::Style> _style;

        //! pointer to target tabBar
        Oxygen::WeakPointer<const QWidget> _tabBar;

        //! if true, will paint on next TabBarTabShapeControlCall
        bool _dirty;

    };

    //! needed to have spacing added to items in combobox
    class ComboBoxItemDelegate: public QItemDelegate
    {

        public:

        //! constructor
        ComboBoxItemDelegate( QAbstractItemView* parent ):
            QItemDelegate( parent ),
            _proxy( parent->itemDelegate() ),
            _itemMargin( Oxygen::Metrics::ItemView_ItemMarginWidth )
        {}

        //! destructor
        virtual ~ComboBoxItemDelegate( void )
        {}

        //! paint
        void paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
        {
            // call either proxy or parent class
            if( _proxy ) _proxy.data()->paint( painter, option, index );
            else QItemDelegate::paint( painter, option, index );
        }

        //! size hint for index
        virtual QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
        {

            // get size from either proxy or parent class
            QSize size( _proxy ?
                _proxy.data()->sizeHint( option, index ):
                QItemDelegate::sizeHint( option, index ) );

            // adjust and return
            if( size.isValid() ) { size.rheight() += _itemMargin*2; }
            return size;

        }

        private:

        //! proxy
        Oxygen::WeakPointer<QAbstractItemDelegate> _proxy;

        //! margin
        int _itemMargin;

    };

}

namespace Oxygen
{

    //! toplevel manager
    class TopLevelManager: public QObject
    {
        public:

        //! constructor
        TopLevelManager( QObject* parent, const StyleHelper& helper ):
            QObject( parent ),
            _helper( helper )
        {}

        //! event filter
        virtual bool eventFilter(QObject* object, QEvent* event )
        {

            // cast to QWidget
            QWidget *widget = static_cast<QWidget*>( object );
            if( event->type() == QEvent::Show && _helper.hasDecoration( widget ) )
            {
                _helper.setHasBackgroundGradient( widget->winId(), true );
                _helper.setHasBackgroundPixmap( widget->winId(), _helper.hasBackgroundPixmap() );
            }

            return false;
        }

        private:

        //! helper
        const StyleHelper& _helper;

    };

    //______________________________________________________________
    Style::Style( void ):
        _addLineButtons( DoubleButton ),
        _subLineButtons( SingleButton ),
        _singleButtonHeight( 14 ),
        _doubleButtonHeight( 28 ),
        _helper( new StyleHelper( StyleConfigData::self()->sharedConfig() ) ),
        _shadowHelper( new ShadowHelper( this, *_helper ) ),
        _animations( new Animations( this ) ),
        _transitions( new Transitions( this ) ),
        _windowManager( new WindowManager( this ) ),
        _topLevelManager( new TopLevelManager( this, *_helper ) ),
        _frameShadowFactory( new FrameShadowFactory( this ) ),
        _mdiWindowShadowFactory( new MdiWindowShadowFactory( this, *_helper ) ),
        _mnemonics( new Mnemonics( this ) ),
        _blurHelper( new BlurHelper( this, *_helper ) ),
        _widgetExplorer( new WidgetExplorer( this ) ),
        _tabBarData( new OxygenPrivate::TabBarData( this ) ),
        _splitterFactory( new SplitterFactory( this ) ),
        _frameFocusPrimitive( nullptr ),
        SH_ArgbDndWindow( newStyleHint( QStringLiteral( "SH_ArgbDndWindow" ) ) ),
        CE_CapacityBar( newControlElement( QStringLiteral( "CE_CapacityBar" ) ) )

    {

        // use DBus connection to update on oxygen configuration change
        QDBusConnection dbus = QDBusConnection::sessionBus();
        dbus.connect( QString(),
            QStringLiteral( "/OxygenStyle" ),
            QStringLiteral( "org.kde.Oxygen.Style" ),
            QStringLiteral( "reparseConfiguration" ), this, SLOT(configurationChanged()) );

        // enable debugging
        QLoggingCategory::setFilterRules(QStringLiteral("oxygen.debug = false"));

        // call the slot directly; this initial call will set up things that also
        // need to be reset when the system palette changes
        loadConfiguration();

    }

    //______________________________________________________________
    Style::~Style( void )
    {
        // _shadowHelper is a child of us, but its destructor uses _helper so we
        // delete it manually to ensure it is deleted *before* _helper is
        // deleted
        delete _shadowHelper;
        delete _helper;
    }

    //______________________________________________________________
    void Style::polish( QWidget* widget )
    {
        if( !widget ) return;

        // register widget to animations
        _animations->registerWidget( widget );
        _transitions->registerWidget( widget );
        _windowManager->registerWidget( widget );
        _frameShadowFactory->registerWidget( widget, *_helper );
        _mdiWindowShadowFactory->registerWidget( widget );
        _shadowHelper->registerWidget( widget );
        _splitterFactory->registerWidget( widget );

        // scroll areas
        if( QAbstractScrollArea* scrollArea = qobject_cast<QAbstractScrollArea*>( widget ) )
        { polishScrollArea( scrollArea ); }

        // several widgets set autofill background to false, which effectively breaks the background
        // gradient rendering. Instead of patching all concerned applications,
        // we change the background here
        if( widget->inherits( "MessageList::Core::Widget" ) )
        { widget->setAutoFillBackground( false ); }

        // adjust layout for K3B themed headers
        // FIXME: to be removed when fixed upstream
        if( widget->inherits( "K3b::ThemedHeader" ) && widget->layout() )
        {
            widget->layout()->setMargin( 0 );
            _frameShadowFactory->setHasContrast( widget, true );
        }

        // adjust flags for windows and dialogs
        switch( widget->windowFlags() & Qt::WindowType_Mask )
        {

            case Qt::Window:
            case Qt::Dialog:

            // set background as styled
            widget->setAttribute( Qt::WA_StyledBackground );
            widget->installEventFilter( _topLevelManager );

            break;

            default: break;

        }

        // enforce translucency for drag and drop window
        if( widget->testAttribute( Qt::WA_X11NetWmWindowTypeDND ) && _helper->compositingActive() )
        {
            widget->setAttribute( Qt::WA_TranslucentBackground );
            widget->clearMask();
        }

        if(
            qobject_cast<QAbstractItemView*>( widget )
            || qobject_cast<QAbstractSpinBox*>( widget )
            || qobject_cast<QCheckBox*>( widget )
            || qobject_cast<QComboBox*>( widget )
            || qobject_cast<QDial*>( widget )
            || qobject_cast<QLineEdit*>( widget )
            || qobject_cast<QPushButton*>( widget )
            || qobject_cast<QRadioButton*>( widget )
            || qobject_cast<QScrollBar*>( widget )
            || qobject_cast<QSlider*>( widget )
            || qobject_cast<QSplitterHandle*>( widget )
            || qobject_cast<QTabBar*>( widget )
            || qobject_cast<QTextEdit*>( widget )
            || qobject_cast<QToolButton*>( widget )
            )
        { widget->setAttribute( Qt::WA_Hover ); }

        // transparent tooltips
        if( widget->inherits( "QTipLabel" ) )
        {
            widget->setAttribute( Qt::WA_TranslucentBackground );

            #ifdef Q_WS_WIN
            //FramelessWindowHint is needed on windows to make WA_TranslucentBackground work properly
            widget->setWindowFlags( widget->windowFlags() | Qt::FramelessWindowHint );
            #endif
        }

        if( QAbstractItemView *itemView = qobject_cast<QAbstractItemView*>( widget ) )
        {

            // enable hover effects in itemviews' viewport
            itemView->viewport()->setAttribute( Qt::WA_Hover );


        } else if( QAbstractScrollArea* scrollArea = qobject_cast<QAbstractScrollArea*>( widget ) ) {

            // enable hover effect in sunken scrollareas that support focus
            if( scrollArea->frameShadow() == QFrame::Sunken && widget->focusPolicy()&Qt::StrongFocus )
            { widget->setAttribute( Qt::WA_Hover ); }

        } else if( QGroupBox* groupBox = qobject_cast<QGroupBox*>( widget ) )  {

            // checkable group boxes
            if( groupBox->isCheckable() )
            { groupBox->setAttribute( Qt::WA_Hover ); }

        } else if( qobject_cast<QAbstractButton*>( widget ) && qobject_cast<QDockWidget*>( widget->parent() ) ) {

            widget->setAttribute( Qt::WA_Hover );

        } else if( qobject_cast<QAbstractButton*>( widget ) && qobject_cast<QToolBox*>( widget->parent() ) ) {

            widget->setAttribute( Qt::WA_Hover );

        }



        /*
        add extra margins for widgets in toolbars
        this allows to preserve alignment with respect to actions
        */
        if( qobject_cast<QToolBar*>( widget->parent() ) )
        { widget->setContentsMargins( 0,0,0,1 ); }

        if( qobject_cast<QToolButton*>( widget ) )
        {
            if( qobject_cast<QToolBar*>( widget->parent() ) )
            {
                // this hack is needed to have correct text color
                // rendered in toolbars. This does not really update nicely when changing styles
                // but is the best I can do for now since setting the palette color at painting
                // time is not doable
                QPalette palette( widget->palette() );
                palette.setColor( QPalette::Disabled, QPalette::ButtonText, palette.color( QPalette::Disabled, QPalette::WindowText ) );
                palette.setColor( QPalette::Active, QPalette::ButtonText, palette.color( QPalette::Active, QPalette::WindowText ) );
                palette.setColor( QPalette::Inactive, QPalette::ButtonText, palette.color( QPalette::Inactive, QPalette::WindowText ) );
                widget->setPalette( palette );
            }

            widget->setBackgroundRole( QPalette::NoRole );

        } else if( qobject_cast<QMenuBar*>( widget ) ) {

            widget->setBackgroundRole( QPalette::NoRole );

        } else if( widget->inherits( "KMultiTabBar" ) ) {

            // kMultiTabBar margins are set to unity for alignment
            // with ( usually sunken ) neighbor frames
            widget->setContentsMargins( 1, 1, 1, 1 );

        } else if( qobject_cast<QToolBar*>( widget ) ) {

            widget->setBackgroundRole( QPalette::NoRole );
            addEventFilter( widget );

        } else if( qobject_cast<QTabBar*>( widget ) ) {

            addEventFilter( widget );

        } else if( widget->inherits( "QTipLabel" ) ) {

            widget->setBackgroundRole( QPalette::NoRole );
            widget->setAttribute( Qt::WA_TranslucentBackground );

            #ifdef Q_WS_WIN
            //FramelessWindowHint is needed on windows to make WA_TranslucentBackground work properly
            widget->setWindowFlags( widget->windowFlags() | Qt::FramelessWindowHint );
            #endif

        } else if( qobject_cast<QScrollBar*>( widget ) ) {

            widget->setAttribute( Qt::WA_OpaquePaintEvent, false );

            // when painted in konsole, one needs to paint the window background below
            // the scrollarea, otherwise an ugly flat background is used
            if( widget->parent() && widget->parent()->inherits( "Konsole::TerminalDisplay" ) )
            { addEventFilter( widget ); }

        } else if( qobject_cast<QDockWidget*>( widget ) ) {

            widget->setBackgroundRole( QPalette::NoRole );
            widget->setContentsMargins( 3,3,3,3 );
            addEventFilter( widget );

        } else if( qobject_cast<QMdiSubWindow*>( widget ) ) {

            widget->setAutoFillBackground( false );
            addEventFilter( widget );

        } else if( qobject_cast<QToolBox*>( widget ) ) {

            widget->setBackgroundRole( QPalette::NoRole );
            widget->setAutoFillBackground( false );
            widget->setContentsMargins( 5,5,5,5 );
            addEventFilter( widget );

        } else if( widget->parentWidget() && widget->parentWidget()->parentWidget() && qobject_cast<QToolBox*>( widget->parentWidget()->parentWidget()->parentWidget() ) ) {

            widget->setBackgroundRole( QPalette::NoRole );
            widget->setAutoFillBackground( false );
            widget->parentWidget()->setAutoFillBackground( false );

        } else if( qobject_cast<QMenu*>( widget ) ) {

            widget->setAttribute( Qt::WA_TranslucentBackground );
            #ifdef Q_WS_WIN
            //FramelessWindowHint is needed on windows to make WA_TranslucentBackground work properly
            widget->setWindowFlags( widget->windowFlags() | Qt::FramelessWindowHint );
            #endif

        } else if( QComboBox *comboBox = qobject_cast<QComboBox*>( widget ) ) {

            QAbstractItemView *itemView( comboBox->view() );
            if( itemView && itemView->itemDelegate() && itemView->itemDelegate()->inherits( "QComboBoxDelegate" ) )
            { itemView->setItemDelegate( new OxygenPrivate::ComboBoxItemDelegate( itemView ) ); }

        } else if( widget->inherits( "QComboBoxPrivateContainer" ) ) {

            addEventFilter( widget );
            widget->setAttribute( Qt::WA_TranslucentBackground );
            #ifdef Q_WS_WIN
            //FramelessWindowHint is needed on windows to make WA_TranslucentBackground work properly
            widget->setWindowFlags( widget->windowFlags() | Qt::FramelessWindowHint );
            #endif

        } else if( qobject_cast<QFrame*>( widget ) && widget->parent() && widget->parent()->inherits( "KTitleWidget" ) ) {

            widget->setAutoFillBackground( false );
            widget->setBackgroundRole( QPalette::Window );

        }

        // base class polishing
        ParentStyleClass::polish( widget );

    }

    //_______________________________________________________________
    void Style::unpolish( QWidget* widget )
    {

        // register widget to animations
        _animations->unregisterWidget( widget );
        _transitions->unregisterWidget( widget );
        _windowManager->unregisterWidget( widget );
        _frameShadowFactory->unregisterWidget( widget );
        _mdiWindowShadowFactory->unregisterWidget( widget );
        _shadowHelper->unregisterWidget( widget );
        _splitterFactory->unregisterWidget( widget );
        _blurHelper->unregisterWidget( widget );

        // event filters
        switch( widget->windowFlags() & Qt::WindowType_Mask )
        {

            case Qt::Window:
            case Qt::Dialog:
            widget->removeEventFilter( this );
            widget->setAttribute( Qt::WA_StyledBackground, false );
            break;

            default:
            break;

        }

        // checkable group boxes
        if( QGroupBox* groupBox = qobject_cast<QGroupBox*>( widget ) )
        {
            if( groupBox->isCheckable() )
            { groupBox->setAttribute( Qt::WA_Hover, false ); }
        }

        // hover flags
        if(
            qobject_cast<QAbstractItemView*>( widget )
            || qobject_cast<QAbstractSpinBox*>( widget )
            || qobject_cast<QCheckBox*>( widget )
            || qobject_cast<QComboBox*>( widget )
            || qobject_cast<QDial*>( widget )
            || qobject_cast<QLineEdit*>( widget )
            || qobject_cast<QPushButton*>( widget )
            || qobject_cast<QRadioButton*>( widget )
            || qobject_cast<QScrollBar*>( widget )
            || qobject_cast<QSlider*>( widget )
            || qobject_cast<QSplitterHandle*>( widget )
            || qobject_cast<QTabBar*>( widget )
            || qobject_cast<QTextEdit*>( widget )
            || qobject_cast<QToolButton*>( widget )
            )
        { widget->setAttribute( Qt::WA_Hover, false ); }

        // checkable group boxes
        if( QGroupBox* groupBox = qobject_cast<QGroupBox*>( widget ) )
        {
            if( groupBox->isCheckable() )
            { groupBox->setAttribute( Qt::WA_Hover, false ); }
        }

        if( qobject_cast<QMenuBar*>( widget )
            || qobject_cast<QToolBar*>( widget )
            || ( widget && qobject_cast<QToolBar *>( widget->parent() ) )
            || qobject_cast<QToolBox*>( widget ) )
        {
            widget->setBackgroundRole( QPalette::Button );
            widget->removeEventFilter( this );
            widget->clearMask();
        }

        if( qobject_cast<QTabBar*>( widget ) )
        {

            widget->removeEventFilter( this );

        } else if( widget->inherits( "QTipLabel" ) ) {

            widget->setAttribute( Qt::WA_PaintOnScreen, false );
            widget->setAttribute( Qt::WA_NoSystemBackground, false );
            widget->clearMask();

        } else if( qobject_cast<QScrollBar*>( widget ) ) {

            widget->setAttribute( Qt::WA_OpaquePaintEvent );

        } else if( qobject_cast<QDockWidget*>( widget ) ) {

            widget->setContentsMargins( 0,0,0,0 );
            widget->clearMask();

        } else if( qobject_cast<QToolBox*>( widget ) ) {

            widget->setBackgroundRole( QPalette::Button );
            widget->setContentsMargins( 0,0,0,0 );
            widget->removeEventFilter( this );

        } else if( qobject_cast<QMenu*>( widget ) ) {

            widget->setAttribute( Qt::WA_PaintOnScreen, false );
            widget->setAttribute( Qt::WA_NoSystemBackground, false );
            widget->clearMask();

        } else if( widget->inherits( "QComboBoxPrivateContainer" ) ) widget->removeEventFilter( this );

        ParentStyleClass::unpolish( widget );

    }

    //______________________________________________________________
    int Style::pixelMetric( PixelMetric metric, const QStyleOption* option, const QWidget* widget ) const
    {

        // handle special cases
        switch( metric )
        {

            case PM_DefaultFrameWidth:
            if( qobject_cast<const QLineEdit*>( widget ) ) return Metrics::LineEdit_FrameWidth;
            else if( option && option->styleObject && option->styleObject->inherits( "QQuickStyleItem" ) )
            {
                const QString &elementType = option->styleObject->property( "elementType" ).toString();
                if( elementType == QLatin1String( "edit" ) || elementType == QLatin1String( "spinbox" ) )
                {

                    return Metrics::LineEdit_FrameWidth;

                } else if( elementType == QLatin1String( "combobox" ) ) {

                    return Metrics::ComboBox_FrameWidth;

                } else {

                    return Metrics::Frame_FrameWidth;

                }

            }

            // fallback
            return Metrics::Frame_FrameWidth;

            case PM_ComboBoxFrameWidth:
            {
                const QStyleOptionComboBox* comboBoxOption( qstyleoption_cast< const QStyleOptionComboBox*>( option ) );
                return comboBoxOption && comboBoxOption->editable ? Metrics::LineEdit_FrameWidth : Metrics::ComboBox_FrameWidth;
            }

            case PM_SpinBoxFrameWidth:
            return Metrics::SpinBox_FrameWidth;

            case PM_ToolBarFrameWidth:
            return Metrics::ToolBar_FrameWidth;

            case PM_ToolTipLabelFrameWidth:
            return Metrics::ToolTip_FrameWidth;

            // layout

            case PM_LayoutLeftMargin:
            case PM_LayoutTopMargin:
            case PM_LayoutRightMargin:
            case PM_LayoutBottomMargin:
            {
                /*
                use either Child margin or TopLevel margin,
                depending on  widget type
                */
                if( ( option && ( option->state & QStyle::State_Window ) ) || ( widget && widget->isWindow() ) )
                {

                    return Metrics::Layout_TopLevelMarginWidth;

                } else {

                    return Metrics::Layout_ChildMarginWidth;

                }

            }

            case PM_LayoutHorizontalSpacing: return Metrics::Layout_DefaultSpacing;
            case PM_LayoutVerticalSpacing: return Metrics::Layout_DefaultSpacing;

            // buttons
            case PM_ButtonMargin:
            {
                /* HACK: needs special case for kcalc buttons, to prevent the application to set too small margins */
                if( widget && widget->inherits( "KCalcButton" ) ) return Metrics::Button_MarginWidth + 4;
                else return Metrics::Button_MarginWidth;
            }

            // buttons
            case PM_ButtonDefaultIndicator: return 0;
            case PM_ButtonShiftHorizontal: return 0;
            case PM_ButtonShiftVertical: return 0;

            // menubars
            case PM_MenuBarPanelWidth: return 0;
            case PM_MenuBarHMargin: return 0;
            case PM_MenuBarVMargin: return 0;
            case PM_MenuBarItemSpacing: return 0;
            case PM_MenuDesktopFrameWidth: return 0;

            // menu buttons
            case PM_MenuButtonIndicator: return Metrics::MenuButton_IndicatorWidth;

            // toolbars
            case PM_ToolBarHandleExtent: return Metrics::ToolBar_HandleExtent;
            case PM_ToolBarSeparatorExtent: return Metrics::ToolBar_SeparatorWidth;
            case PM_ToolBarExtensionExtent:
            return pixelMetric( PM_SmallIconSize, option, widget ) + 2*Metrics::ToolButton_MarginWidth;

            case PM_ToolBarItemMargin: return 0;
            case PM_ToolBarItemSpacing: return Metrics::ToolBar_ItemSpacing;

            // tabbars
            case PM_TabBarTabShiftVertical: return 0;
            case PM_TabBarTabShiftHorizontal: return 0;
            case PM_TabBarTabOverlap: return Metrics::TabBar_TabOverlap;
            case PM_TabBarBaseOverlap: return Metrics::TabBar_BaseOverlap;
            case PM_TabBarTabHSpace: return 2*Metrics::TabBar_TabMarginWidth;
            case PM_TabBarTabVSpace: return 2*Metrics::TabBar_TabMarginHeight;

            // scrollbars
            case PM_ScrollBarExtent: return StyleConfigData::scrollBarWidth() + 2;
            case PM_ScrollBarSliderMin: return Metrics::ScrollBar_MinSliderHeight;

            // title bar
            case PM_TitleBarHeight: return 2*Metrics::TitleBar_MarginWidth + pixelMetric( PM_SmallIconSize, option, widget );

            // sliders
            case PM_SliderThickness: return Metrics::Slider_ControlThickness;
            case PM_SliderControlThickness: return Metrics::Slider_ControlThickness;
            case PM_SliderLength: return Metrics::Slider_ControlThickness;

            // checkboxes and radio buttons
            case PM_IndicatorWidth: return Metrics::CheckBox_Size;
            case PM_IndicatorHeight: return Metrics::CheckBox_Size;
            case PM_ExclusiveIndicatorWidth: return Metrics::CheckBox_Size;
            case PM_ExclusiveIndicatorHeight: return Metrics::CheckBox_Size;

            // list heaaders
            case PM_HeaderMarkSize: return Metrics::Header_ArrowSize;
            case PM_HeaderMargin: return Metrics::Header_MarginWidth;

            // dock widget
            // return 0 here, since frame is handled directly in polish
            case PM_DockWidgetFrameWidth: return 0;
            case PM_DockWidgetTitleMargin: return Metrics::Frame_FrameWidth;
            case PM_DockWidgetTitleBarButtonMargin: return Metrics::ToolButton_MarginWidth;

            case PM_SplitterWidth: return Metrics::Splitter_SplitterWidth;
            case PM_DockWidgetSeparatorExtent: return Metrics::Splitter_SplitterWidth;

            // spacing between widget and scrollbars
            case PM_ScrollView_ScrollBarSpacing:
            if( const QFrame* frame = qobject_cast<const QFrame*>( widget ) )
            {

                const bool framed( frame->frameShape() != QFrame::NoFrame );
                return framed ? -1:0;

            } else return -1;

            // fallback
            default: return ParentStyleClass::pixelMetric( metric, option, widget );

        }

    }

    //______________________________________________________________
    int Style::styleHint( StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* returnData ) const
    {

        /*
        special cases, that cannot be registered in styleHint map,
        because of conditional statements
        */
        switch( hint )
        {

            case SH_RubberBand_Mask:
            {

                if( QStyleHintReturnMask *mask = qstyleoption_cast<QStyleHintReturnMask*>( returnData ) )
                {

                    mask->region = option->rect;

                    // need to check on widget before removing inner region
                    // in order to still preserve rubberband in MainWindow and QGraphicsView
                    // in QMainWindow because it looks better
                    // in QGraphicsView because the painting fails completely otherwise
                    if( widget && (
                        qobject_cast<const QAbstractItemView*>( widget->parent() ) ||
                        qobject_cast<const QGraphicsView*>( widget->parent() ) ||
                        qobject_cast<const QMainWindow*>( widget->parent() ) ) )
                    { return true; }

                    // also check if widget's parent is some itemView viewport
                    if( widget && widget->parent() &&
                        qobject_cast<const QAbstractItemView*>( widget->parent()->parent() ) &&
                        static_cast<const QAbstractItemView*>( widget->parent()->parent() )->viewport() == widget->parent() )
                    { return true; }

                    // mask out center
                    mask->region -= insideMargin( option->rect, 1 );

                    return true;
                }
                return false;
            }

            case SH_ToolTip_Mask:
            case SH_Menu_Mask:
            {

                if( !_helper->hasAlphaChannel( widget ) && ( !widget || widget->isWindow() ) )
                {

                    // mask should be set only if compositing is disabled
                    if( QStyleHintReturnMask *mask = qstyleoption_cast<QStyleHintReturnMask *>( returnData ) )
                    { mask->region = _helper->roundedMask( option->rect ); }

                }

                return true;

            }

            // mouse tracking
            case SH_ComboBox_ListMouseTracking: return true;
            case SH_MenuBar_MouseTracking: return true;
            case SH_Menu_MouseTracking: return true;

            // menus
            case SH_Menu_SubMenuPopupDelay: return 150;
            case SH_Menu_SloppySubMenus: return true;

            #if QT_VERSION >= 0x050000
            case SH_Menu_SupportsSections: return true;
            #endif

            case SH_TitleBar_NoBorder: return false;
            case SH_GroupBox_TextLabelVerticalAlignment: return Qt::AlignVCenter;
            case SH_ScrollBar_MiddleClickAbsolutePosition: return true;
            case SH_ScrollView_FrameOnlyAroundContents: return true;
            case SH_FormLayoutFormAlignment: return Qt::AlignLeft | Qt::AlignTop;
            case SH_FormLayoutLabelAlignment: return Qt::AlignRight;
            case SH_FormLayoutFieldGrowthPolicy: return QFormLayout::ExpandingFieldsGrow;
            case SH_FormLayoutWrapPolicy: return QFormLayout::DontWrapRows;
            case SH_MessageBox_TextInteractionFlags: return Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse;
            case SH_RequestSoftwareInputPanel: return RSIP_OnMouseClick;

            case SH_ProgressDialog_CenterCancelButton:
            case SH_MessageBox_CenterButtons:
            return false;

            default: return ParentStyleClass::styleHint( hint, option, widget, returnData );
        }

    }

    //______________________________________________________________
    QRect Style::subElementRect( SubElement element, const QStyleOption* option, const QWidget* widget ) const
    {

        switch( element )
        {

            case SE_PushButtonContents: return pushButtonContentsRect( option, widget );
            case SE_CheckBoxContents: return checkBoxContentsRect( option, widget );
            case SE_RadioButtonContents: return checkBoxContentsRect( option, widget );
            case SE_LineEditContents: return lineEditContentsRect( option, widget );
            case SE_ProgressBarGroove: return progressBarGrooveRect( option, widget );
            case SE_ProgressBarContents: return progressBarContentsRect( option, widget );
            case SE_ProgressBarLabel: return defaultSubElementRect( option, widget );
            case SE_HeaderArrow: return headerArrowRect( option, widget );
            case SE_HeaderLabel: return headerLabelRect( option, widget );
            case SE_TabWidgetTabBar: return tabWidgetTabBarRect( option, widget );
            case SE_TabWidgetTabContents: return tabWidgetTabContentsRect( option, widget );
            case SE_TabWidgetTabPane: return tabWidgetTabPaneRect( option, widget );
            case SE_TabWidgetLeftCorner: return tabWidgetCornerRect( SE_TabWidgetLeftCorner, option, widget );
            case SE_TabWidgetRightCorner: return tabWidgetCornerRect( SE_TabWidgetRightCorner, option, widget );
            case SE_ToolBoxTabContents: return toolBoxTabContentsRect( option, widget );

            default: return ParentStyleClass::subElementRect( element, option, widget );

        }

    }

    //______________________________________________________________
    QRect Style::subControlRect( ComplexControl element, const QStyleOptionComplex* option, SubControl subControl, const QWidget* widget ) const
    {

        switch( element )
        {

            case CC_GroupBox: return groupBoxSubControlRect( option, subControl, widget );
            case CC_ToolButton: return toolButtonSubControlRect( option, subControl, widget );
            case CC_ComboBox: return comboBoxSubControlRect( option, subControl, widget );
            case CC_SpinBox: return spinBoxSubControlRect( option, subControl, widget );
            case CC_ScrollBar: return scrollBarSubControlRect( option, subControl, widget );
            case CC_Slider: return sliderSubControlRect( option, subControl, widget );

            // fallback
            default: return ParentStyleClass::subControlRect( element, option, subControl, widget );

        }

    }

    //______________________________________________________________
    QSize Style::sizeFromContents( ContentsType element, const QStyleOption* option, const QSize& size, const QWidget* widget ) const
    {

        switch( element )
        {
            case CT_CheckBox: return checkBoxSizeFromContents( option, size, widget );
            case CT_RadioButton: return checkBoxSizeFromContents( option, size, widget );
            case CT_LineEdit: return lineEditSizeFromContents( option, size, widget );
            case CT_ComboBox: return comboBoxSizeFromContents( option, size, widget );
            case CT_SpinBox: return spinBoxSizeFromContents( option, size, widget );
            case CT_Slider: return sliderSizeFromContents( option, size, widget );
            case CT_PushButton: return pushButtonSizeFromContents( option, size, widget );
            case CT_ToolButton: return toolButtonSizeFromContents( option, size, widget );
            case CT_MenuBar: return defaultSizeFromContents( option, size, widget );
            case CT_MenuBarItem: return menuBarItemSizeFromContents( option, size, widget );
            case CT_MenuItem: return menuItemSizeFromContents( option, size, widget );
            case CT_TabWidget: return tabWidgetSizeFromContents( option, size, widget );
            case CT_TabBarTab: return tabBarTabSizeFromContents( option, size, widget );
            case CT_HeaderSection: return headerSectionSizeFromContents( option, size, widget );
            case CT_ItemViewItem: return itemViewItemSizeFromContents( option, size, widget );

            default: return ParentStyleClass::sizeFromContents( element, option, size, widget );
        }

    }

    //______________________________________________________________
    QStyle::SubControl Style::hitTestComplexControl( ComplexControl control, const QStyleOptionComplex* option, const QPoint& point, const QWidget* widget ) const
    {
        switch( control )
        {
            case CC_ScrollBar:
            {

                QRect grooveRect = scrollBarSubControlRect( option, SC_ScrollBarGroove, widget );
                if( grooveRect.contains( point ) )
                {
                    //Must be either page up/page down, or just click on the slider.
                    //Grab the slider to compare
                    QRect sliderRect = scrollBarSubControlRect( option, SC_ScrollBarSlider, widget );

                    if( sliderRect.contains( point ) ) return SC_ScrollBarSlider;
                    else if( preceeds( point, sliderRect, option ) ) return SC_ScrollBarSubPage;
                    else return SC_ScrollBarAddPage;

                }

                //This is one of the up/down buttons. First, decide which one it is.
                if( preceeds( point, grooveRect, option ) )
                {

                    if( _subLineButtons == DoubleButton )
                    {
                        QRect buttonRect = scrollBarInternalSubControlRect( option, SC_ScrollBarSubLine );
                        return scrollBarHitTest( buttonRect, point, option );
                    } else return SC_ScrollBarSubLine;

                }

                if( _addLineButtons == DoubleButton )
                {

                    QRect buttonRect = scrollBarInternalSubControlRect( option, SC_ScrollBarAddLine );
                    return scrollBarHitTest( buttonRect, point, option );

                } else return SC_ScrollBarAddLine;
            }

            default: return ParentStyleClass::hitTestComplexControl( control, option, point, widget );
        }

    }

    //______________________________________________________________
    void Style::drawPrimitive( PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        StylePrimitive fcn( nullptr );
        switch( element )
        {

            // register primitives for which nothing is done
            case PE_FrameStatusBar: fcn = &Style::emptyPrimitive; break;

            case PE_Frame: fcn = &Style::drawFramePrimitive; break;

            // frame focus primitive is set at run time, in configurationChanged
            case PE_FrameFocusRect: fcn = _frameFocusPrimitive; break;

            case PE_FrameGroupBox: fcn = &Style::drawFrameGroupBoxPrimitive; break;
            case PE_FrameLineEdit: fcn = &Style::drawFrameLineEditPrimitive; break;
            case PE_FrameMenu: fcn = &Style::drawFrameMenuPrimitive; break;

            // TabBar
            case PE_FrameTabBarBase: fcn = &Style::drawFrameTabBarBasePrimitive; break;
            case PE_FrameTabWidget: fcn = &Style::drawFrameTabWidgetPrimitive; break;
            case PE_FrameWindow: fcn = &Style::drawFrameWindowPrimitive; break;
            case PE_IndicatorTabClose: fcn = &Style::drawIndicatorTabClosePrimitive; break;

            // arrows
            case PE_IndicatorArrowUp: fcn = &Style::drawIndicatorArrowUpPrimitive; break;
            case PE_IndicatorArrowDown: fcn = &Style::drawIndicatorArrowDownPrimitive; break;
            case PE_IndicatorArrowLeft: fcn = &Style::drawIndicatorArrowLeftPrimitive; break;
            case PE_IndicatorArrowRight: fcn = &Style::drawIndicatorArrowRightPrimitive; break;
            case PE_IndicatorDockWidgetResizeHandle: fcn = &Style::drawIndicatorDockWidgetResizeHandlePrimitive; break;
            case PE_IndicatorHeaderArrow: fcn = &Style::drawIndicatorHeaderArrowPrimitive; break;
            case PE_PanelButtonCommand: fcn = &Style::drawPanelButtonCommandPrimitive; break;
            case PE_PanelButtonTool: fcn = &Style::drawPanelButtonToolPrimitive; break;
            case PE_PanelItemViewItem: fcn = &Style::drawPanelItemViewItemPrimitive; break;
            case PE_PanelMenu: fcn = &Style::drawPanelMenuPrimitive; break;
            case PE_PanelScrollAreaCorner: fcn = &Style::drawPanelScrollAreaCornerPrimitive; break;
            case PE_PanelTipLabel: fcn = &Style::drawPanelTipLabelPrimitive; break;
            case PE_IndicatorMenuCheckMark: fcn = &Style::drawIndicatorMenuCheckMarkPrimitive; break;
            case PE_IndicatorBranch: fcn = &Style::drawIndicatorBranchPrimitive; break;
            case PE_IndicatorButtonDropDown: fcn = &Style::drawIndicatorButtonDropDownPrimitive; break;
            case PE_IndicatorCheckBox: fcn = &Style::drawIndicatorCheckBoxPrimitive; break;
            case PE_IndicatorRadioButton: fcn = &Style::drawIndicatorRadioButtonPrimitive; break;
            case PE_IndicatorTabTear: fcn = &Style::drawIndicatorTabTearPrimitive; break;
            case PE_IndicatorToolBarHandle: fcn = &Style::drawIndicatorToolBarHandlePrimitive; break;
            case PE_IndicatorToolBarSeparator: fcn = &Style::drawIndicatorToolBarSeparatorPrimitive; break;
            case PE_Widget: fcn = &Style::drawWidgetPrimitive; break;

            // fallback
            default: break;

        }

        painter->save();

        // call function if implemented
        if( !( fcn && ( this->*fcn )( option, painter, widget ) ) )
        { ParentStyleClass::drawPrimitive( element, option, painter, widget ); }

        painter->restore();

    }

    //______________________________________________________________
    void Style::drawControl( ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        StyleControl fcn( nullptr );
        #if !USE_KDE4
        if( element == CE_CapacityBar )
        {

            fcn = &Style::drawProgressBarControl;

        } else
        #endif
        switch( element ) {

            case CE_ComboBoxLabel: break;
            case CE_DockWidgetTitle: fcn = &Style::drawDockWidgetTitleControl; break;
            case CE_HeaderEmptyArea: fcn = &Style::drawHeaderEmptyAreaControl; break;
            case CE_HeaderLabel: break;
            case CE_HeaderSection: fcn = &Style::drawHeaderSectionControl; break;
            case CE_MenuBarEmptyArea: fcn = &Style::emptyControl; break;
            case CE_MenuBarItem: fcn = &Style::drawMenuBarItemControl; break;
            case CE_MenuItem: fcn = &Style::drawMenuItemControl; break;
            case CE_ProgressBar: fcn = &Style::drawProgressBarControl; break;
            case CE_ProgressBarContents: fcn = &Style::drawProgressBarContentsControl; break;
            case CE_ProgressBarGroove: fcn = &Style::drawProgressBarGrooveControl; break;
            case CE_ProgressBarLabel: fcn = &Style::drawProgressBarLabelControl; break;

            /*
            for CE_PushButtonBevel the only thing that is done is draw the PanelButtonCommand primitive
            since the prototypes are identical we register the second directly in the control map: fcn = without
            using an intermediate function
            */
            case CE_PushButtonBevel: fcn = &Style::drawPanelButtonCommandPrimitive; break;
            case CE_PushButtonLabel: fcn = &Style::drawPushButtonLabelControl; break;

            case CE_RubberBand: fcn = &Style::drawRubberBandControl; break;
            case CE_ScrollBarSlider: fcn = &Style::drawScrollBarSliderControl; break;
            case CE_ScrollBarAddLine: fcn = &Style::drawScrollBarAddLineControl; break;
            case CE_ScrollBarSubLine: fcn = &Style::drawScrollBarSubLineControl; break;

            // these two are handled directly in CC_ScrollBar
            case CE_ScrollBarAddPage: fcn = &Style::emptyControl; break;
            case CE_ScrollBarSubPage: fcn = &Style::emptyControl; break;

            case CE_ShapedFrame: fcn = &Style::drawShapedFrameControl; break;
            case CE_SizeGrip: fcn = &Style::emptyControl; break;
            case CE_Splitter: fcn = &Style::drawSplitterControl; break;
            case CE_TabBarTabLabel: fcn = &Style::drawTabBarTabLabelControl; break;

            // default tab style is 'SINGLE'
            case CE_TabBarTabShape: fcn = &Style::drawTabBarTabShapeControl; break;

            case CE_ToolBar: fcn = &Style::drawToolBarControl; break;
            case CE_ToolBoxTabLabel: fcn = &Style::drawToolBoxTabLabelControl; break;
            case CE_ToolBoxTabShape: fcn = &Style::drawToolBoxTabShapeControl; break;
            case CE_ToolButtonLabel: fcn = &Style::drawToolButtonLabelControl; break;

            default: break;

        }

        painter->save();

        // call function if implemented
        if( !( fcn && ( this->*fcn )( option, painter, widget ) ) )
        { ParentStyleClass::drawControl( element, option, painter, widget ); }

        painter->restore();

    }

    //______________________________________________________________
    void Style::drawComplexControl( ComplexControl element, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget ) const
    {

        StyleComplexControl fcn( nullptr );
        switch( element )
        {
            case CC_GroupBox: break;
            case CC_ToolButton: fcn = &Style::drawToolButtonComplexControl; break;
            case CC_ComboBox: fcn = &Style::drawComboBoxComplexControl; break;
            case CC_SpinBox: fcn = &Style::drawSpinBoxComplexControl; break;
            case CC_Slider: fcn = &Style::drawSliderComplexControl; break;
            case CC_Dial: fcn = &Style::drawDialComplexControl; break;
            case CC_ScrollBar: fcn = &Style::drawScrollBarComplexControl; break;
            case CC_TitleBar: fcn = &Style::drawTitleBarComplexControl; break;

            // fallback
            default: break;
        }

        painter->save();

        // call function if implemented
        if( !( fcn && ( this->*fcn )( option, painter, widget ) ) )
        { ParentStyleClass::drawComplexControl( element, option, painter, widget ); }

        painter->restore();

    }


    //___________________________________________________________________________________
    void Style::drawItemText(
        QPainter* painter, const QRect& rect, int flags, const QPalette& palette, bool enabled,
        const QString &text, QPalette::ColorRole textRole ) const
    {

        // hide mnemonics if requested
        if( !_mnemonics->enabled() && ( flags&Qt::TextShowMnemonic ) && !( flags&Qt::TextHideMnemonic ) )
        {
            flags &= ~Qt::TextShowMnemonic;
            flags |= Qt::TextHideMnemonic;
        }

        if( _animations->widgetEnabilityEngine().enabled() )
        {

            /*
            check if painter engine is registered to WidgetEnabilityEngine, and animated
            if yes, merge the palettes. Note: a static_cast is safe here, since only the address
            of the pointer is used, not the actual content.
            */
            const QWidget* widget( static_cast<const QWidget*>( painter->device() ) );
            if( _animations->widgetEnabilityEngine().isAnimated( widget, AnimationEnable ) )
            {

                const QPalette copy( _helper->mergePalettes( palette, _animations->widgetEnabilityEngine().opacity( widget, AnimationEnable ) ) );
                return ParentStyleClass::drawItemText( painter, rect, flags, copy, enabled, text, textRole );

            }

        }

        // fallback
        return ParentStyleClass::drawItemText( painter, rect, flags, palette, enabled, text, textRole );

    }


    //_____________________________________________________________________
    bool Style::eventFilter( QObject *object, QEvent *event )
    {

        if( QTabBar* tabBar = qobject_cast<QTabBar*>( object ) ) { return eventFilterTabBar( tabBar, event ); }
        if( QToolBar* toolBar = qobject_cast<QToolBar*>( object ) ) { return eventFilterToolBar( toolBar, event ); }
        if( QDockWidget* dockWidget = qobject_cast<QDockWidget*>( object ) ) { return eventFilterDockWidget( dockWidget, event ); }
        if( QToolBox* toolBox = qobject_cast<QToolBox*>( object ) ) { return eventFilterToolBox( toolBox, event ); }
        if( QMdiSubWindow* subWindow = qobject_cast<QMdiSubWindow*>( object ) ) { return eventFilterMdiSubWindow( subWindow, event ); }
        if( QScrollBar* scrollBar = qobject_cast<QScrollBar*>( object ) ) { return eventFilterScrollBar( scrollBar, event ); }

        // cast to QWidget
        QWidget *widget = static_cast<QWidget*>( object );
        if( widget->inherits( "QComboBoxPrivateContainer" ) ) { return eventFilterComboBoxContainer( widget, event ); }

        // fallback
        return ParentStyleClass::eventFilter( object, event );

    }

    //_________________________________________________________
    bool Style::eventFilterComboBoxContainer( QWidget* widget, QEvent* event )
    {
        switch( event->type() )
        {

            case QEvent::Show:
            case QEvent::Resize:
            {
                if( !_helper->hasAlphaChannel( widget ) ) widget->setMask( _helper->roundedMask( widget->rect() ) );
                else widget->clearMask();
                return false;
            }

            case QEvent::Paint:
            {

                QPainter painter( widget );
                QPaintEvent *paintEvent = static_cast<QPaintEvent*>( event );
                painter.setClipRegion( paintEvent->region() );

                const QRect r( widget->rect() );
                const QColor color( widget->palette().color( widget->window()->backgroundRole() ) );
                const bool hasAlpha( _helper->hasAlphaChannel( widget ) );

                if( hasAlpha )
                {

                    TileSet *tileSet( _helper->roundCorner( color ) );
                    tileSet->render( r, &painter );
                    painter.setCompositionMode( QPainter::CompositionMode_SourceOver );
                    painter.setClipRegion( _helper->roundedMask( r.adjusted( 1, 1, -1, -1 ) ), Qt::IntersectClip );

                }

                // background
                _helper->renderMenuBackground( &painter, paintEvent->rect(), widget, widget->palette() );

                // frame
                if( hasAlpha ) painter.setClipping( false );

                _helper->drawFloatFrame( &painter, r, color, !hasAlpha );
                return false;

            }
            default: return false;
        }
    }

    //____________________________________________________________________________
    bool Style::eventFilterDockWidget( QDockWidget* dockWidget, QEvent* event )
    {
        switch( event->type() )
        {
            case QEvent::Show:
            case QEvent::Resize:
            {
                // make sure mask is appropriate
                if( dockWidget->isFloating() )
                {
                    if( _helper->compositingActive() ) dockWidget->setMask( _helper->roundedMask( dockWidget->rect().adjusted( 1, 1, -1, -1 ) ) );
                    else dockWidget->setMask( _helper->roundedMask( dockWidget->rect() ) );
                } else dockWidget->clearMask();
                return false;
            }

            case QEvent::Paint:
            {
                QPainter painter( dockWidget );
                QPaintEvent *paintEvent = static_cast<QPaintEvent*>( event );
                painter.setClipRegion( paintEvent->region() );

                const QColor color( dockWidget->palette().color( QPalette::Window ) );
                const QRect r( dockWidget->rect() );
                if( dockWidget->isWindow() )
                {

                    _helper->renderWindowBackground( &painter, r, dockWidget, color );

                    #ifndef Q_WS_WIN
                    _helper->drawFloatFrame( &painter, r, color, !_helper->compositingActive() );
                    #endif

                } else {

                    // need to draw window background for autoFilled dockWidgets for better rendering
                    if( dockWidget->autoFillBackground() )
                    { _helper->renderWindowBackground( &painter, r, dockWidget, color ); }

                    // adjust color
                    QColor top( _helper->backgroundColor( color, dockWidget, r.topLeft() ) );
                    QColor bottom( _helper->backgroundColor( color, dockWidget, r.bottomLeft() ) );
                    TileSet *tileSet = _helper->dockFrame( top, bottom );
                    tileSet->render( r, &painter );

                }

                return false;
            }

            default: return false;

        }

    }

    //____________________________________________________________________________
    bool Style::eventFilterMdiSubWindow( QMdiSubWindow* subWindow, QEvent* event )
    {

        if( event->type() == QEvent::Paint )
        {

            QPainter painter( subWindow );
            QRect clip( static_cast<QPaintEvent*>( event )->rect() );
            if( subWindow->isMaximized() ) _helper->renderWindowBackground( &painter, clip, subWindow, subWindow->palette() );
            else {

                painter.setClipRect( clip );

                const QRect r( subWindow->rect() );
                TileSet *tileSet( _helper->roundCorner( subWindow->palette().color( subWindow->backgroundRole() ) ) );
                tileSet->render( r, &painter );

                painter.setClipRegion( _helper->roundedMask( r.adjusted( 1, 1, -1, -1 ) ), Qt::IntersectClip );
                _helper->renderWindowBackground( &painter, clip, subWindow, subWindow, subWindow->palette(), 0, 58 );

            }

        }

        // continue with normal painting
        return false;

    }

    //_________________________________________________________
    bool Style::eventFilterScrollBar( QWidget* widget, QEvent* event )
    {

        if( event->type() == QEvent::Paint )
        {
            QPainter painter( widget );
            painter.setClipRegion( static_cast<QPaintEvent*>( event )->region() );
            _helper->renderWindowBackground( &painter, widget->rect(), widget,widget->palette() );
        }

        return false;
    }

    //_____________________________________________________________________
    bool Style::eventFilterTabBar( QWidget* widget, QEvent* event )
    {
        if( event->type() == QEvent::Paint && _tabBarData->locks( widget ) )
        {
            /*
            this makes sure that tabBar base is drawn ( and drawn only once )
            every time a replaint is triggered by dragging a tab around
            */
            _tabBarData->setDirty();
        }

        return false;
    }

    //_____________________________________________________________________
    bool Style::eventFilterToolBar( QToolBar* toolBar, QEvent* event )
    {
        switch( event->type() )
        {
            case QEvent::Show:
            case QEvent::Resize:
            {

                // make sure mask is appropriate
                if( toolBar->isFloating() )
                {

                    // TODO: should not be needed
                    toolBar->setMask( _helper->roundedMask( toolBar->rect() ) );

                } else toolBar->clearMask();
                return false;

            }

            case QEvent::Paint:
            {

                QPainter painter( toolBar );
                QPaintEvent *paintEvent = static_cast<QPaintEvent*>( event );
                painter.setClipRegion( paintEvent->region() );

                const QRect r( toolBar->rect() );
                const QColor color( toolBar->palette().window().color() );

                // default painting when not qrealing
                if( !toolBar->isFloating() )
                {

                    // background has to be rendered explicitly
                    // when one of the parent has autofillBackground set to true
                    if( _helper->checkAutoFillBackground( toolBar ) )
                    { _helper->renderWindowBackground( &painter, r, toolBar, color ); }

                    return false;

                } else {

                    // background
                    _helper->renderWindowBackground( &painter, r, toolBar, color );

                    if( toolBar->isMovable() )
                    {
                        // remaining painting: need to add handle
                        // this is copied from QToolBar::paintEvent
                        QStyleOptionToolBar opt;
                        opt.initFrom( toolBar );
                        if( toolBar->orientation() == Qt::Horizontal )
                        {

                            opt.rect = visualRect( &opt, QRect( r.topLeft(), QSize( 8, r.height() ) ) );
                            opt.state |= QStyle::State_Horizontal;

                        } else {

                            opt.rect = visualRect( &opt, QRect( r.topLeft(), QSize( r.width(), 8 ) ) );

                        }

                        drawIndicatorToolBarHandlePrimitive( &opt, &painter, toolBar );

                    }

                    #ifndef Q_WS_WIN
                    if( _helper->compositingActive() ) _helper->drawFloatFrame( &painter, r.adjusted( -1, -1, 1, 1 ), color, false );
                    else _helper->drawFloatFrame( &painter, r, color, true );
                    #endif

                    // do not propagate
                    return true;

                }

            }
            default: return false;
        }

    }

    //____________________________________________________________________________
    bool Style::eventFilterToolBox( QToolBox* toolBox, QEvent* event )
    {

        if( event->type() == QEvent::Paint )
        {
            if( toolBox->frameShape() != QFrame::NoFrame )
            {

                const QRect r( toolBox->rect() );
                const StyleOptions styleOptions( NoFill );

                QPainter painter( toolBox );
                painter.setClipRegion( static_cast<QPaintEvent*>( event )->region() );
                renderSlab( &painter, r, toolBox->palette().color( QPalette::Button ), styleOptions );

            }
        }

        return false;
    }

    //___________________________________________________________________________________________________________________
    QRect Style::pushButtonContentsRect( const QStyleOption* option, const QWidget* ) const
    { return insideMargin( option->rect, Metrics::Frame_FrameWidth ); }

    //___________________________________________________________________________________________________________________
    QRect Style::checkBoxContentsRect( const QStyleOption* option, const QWidget* ) const
    { return visualRect( option, option->rect.adjusted( Metrics::CheckBox_Size + Metrics::CheckBox_ItemSpacing, 0, 0, 0 ) ); }

    //___________________________________________________________________________________________________________________
    QRect Style::lineEditContentsRect( const QStyleOption* option, const QWidget* widget ) const
    {
        // cast option and check
        const QStyleOptionFrame* frameOption( qstyleoption_cast<const QStyleOptionFrame*>( option ) );
        if( !frameOption ) return option->rect;

        // check flatness
        const bool flat( frameOption->lineWidth == 0 );
        if( flat ) return option->rect;

        // copy rect and take out margins
        QRect rect( option->rect );

        // take out margins if there is enough room
        const int frameWidth( pixelMetric( PM_DefaultFrameWidth, option, widget ) );
        if( rect.height() > option->fontMetrics.height() + 2*frameWidth ) return insideMargin( rect, frameWidth );
        else return rect;
    }

    //____________________________________________________________________
    QRect Style::progressBarGrooveRect( const QStyleOption* option, const QWidget* ) const
    {
        const QRect rect( option->rect );
        const QStyleOptionProgressBarV2 *progressBarOption2( qstyleoption_cast<const QStyleOptionProgressBarV2 *>( option ) );
        const bool horizontal( !progressBarOption2 || progressBarOption2->orientation == Qt::Horizontal );
        if( horizontal ) return rect.adjusted( 1, 0, -1, 0 );
        else return rect.adjusted( 0, 1, 0, -1 );
    }

    //____________________________________________________________________
    QRect Style::progressBarContentsRect( const QStyleOption* option, const QWidget* widget ) const
    {

        // cast option and check
        const QStyleOptionProgressBar* progressBarOption( qstyleoption_cast<const QStyleOptionProgressBar*>( option ) );
        if( !progressBarOption ) return QRect();

        // get orientation
        const QStyleOptionProgressBarV2* progressBarOption2( qstyleoption_cast<const QStyleOptionProgressBarV2*>( option ) );
        const bool horizontal( !progressBarOption2 || progressBarOption2->orientation == Qt::Horizontal );

        // check inverted appearance
        const bool inverted( progressBarOption2 ? progressBarOption2->invertedAppearance : false );

        // get groove rect
        const QRect rect( progressBarGrooveRect( option, widget ) );

        // get progress
        qreal progress = progressBarOption->progress - progressBarOption->minimum;
        const bool busy = ( progressBarOption->minimum == 0 && progressBarOption->maximum == 0 );
        if( busy ) progress = _animations->busyIndicatorEngine().value();
        if( !( progress || busy ) ) return QRect();

        const int steps = qMax( progressBarOption->maximum  - progressBarOption->minimum, 1 );

        //Calculate width fraction
        qreal widthFrac( busy ?  Metrics::ProgressBar_BusyIndicatorSize/100.0 : progress/steps );
        widthFrac = qMin( (qreal)1.0, widthFrac );

        // And now the pixel width
        const int indicatorSize( widthFrac*( horizontal ? rect.width():rect.height() ) );

        // do nothing if indicator size is too small
        if( indicatorSize < 4 ) return QRect();
        QRect indicatorRect;
        if( busy )
        {

            // The space around which we move around...
            int remSize = ( ( 1.0 - widthFrac )*( horizontal ? rect.width():rect.height() ) );
            remSize = qMax( remSize, 1 );

            int pstep =  remSize*2*progress;
            if( pstep > remSize )
            { pstep = -( pstep - 2*remSize ); }

            if( horizontal ) {

                indicatorRect = QRect( inverted ? (rect.right() - pstep - indicatorSize + 1) : (rect.left() + pstep), rect.top(), indicatorSize, rect.height() );
                indicatorRect = visualRect( option->direction, rect, indicatorRect );

            } else {

                indicatorRect = QRect( rect.left(), inverted ? (rect.bottom() - pstep - indicatorSize + 1) : (rect.top() + pstep), rect.width(), indicatorSize );

            }

        } else {

            if( horizontal )
            {

                indicatorRect = QRect( inverted ? (rect.right() - indicatorSize + 1) : rect.left(), rect.top(), indicatorSize, rect.height() );
                indicatorRect = visualRect( option->direction, rect, indicatorRect );

            } else {

                indicatorRect = QRect( rect.left(), inverted ? rect.top() : (rect.bottom()- indicatorSize + 1), rect.width(), indicatorSize );

            }
        }

        // adjust
        indicatorRect.adjust( 1, 1, -1, -1 );
        return indicatorRect;

    }

    //___________________________________________________________________________________________________________________
    QRect Style::headerArrowRect( const QStyleOption* option, const QWidget* ) const
    {

        // cast option and check
        const QStyleOptionHeader* headerOption( qstyleoption_cast<const QStyleOptionHeader*>( option ) );
        if( !headerOption ) return option->rect;

        // check if arrow is necessary
        if( headerOption->sortIndicator == QStyleOptionHeader::None ) return QRect();

        QRect arrowRect( insideMargin( option->rect, Metrics::Header_MarginWidth ) );
        arrowRect.setLeft( arrowRect.right() - Metrics::Header_ArrowSize + 1 );

        return visualRect( option, arrowRect );

    }

    //___________________________________________________________________________________________________________________
    QRect Style::headerLabelRect( const QStyleOption* option, const QWidget* ) const
    {

        // cast option and check
        const QStyleOptionHeader* headerOption( qstyleoption_cast<const QStyleOptionHeader*>( option ) );
        if( !headerOption ) return option->rect;

        // check if arrow is necessary
        QRect labelRect( insideMargin( option->rect, Metrics::Header_MarginWidth ) );
        if( headerOption->sortIndicator == QStyleOptionHeader::None ) return labelRect;

        labelRect.adjust( 0, 0, -Metrics::Header_ArrowSize-Metrics::Header_ItemSpacing, 0 );
        return visualRect( option, labelRect );

    }

    //____________________________________________________________________
    QRect Style::tabWidgetTabBarRect( const QStyleOption* option, const QWidget* widget ) const
    {

        // cast option and check
        const QStyleOptionTabWidgetFrame* tabOption = qstyleoption_cast<const QStyleOptionTabWidgetFrame*>( option );
        if( !tabOption ) return option->rect;

        // do nothing if tabbar is hidden
        const QSize tabBarSize( tabOption->tabBarSize );
        if( tabBarSize.isEmpty() ) return option->rect;

        QRect rect( option->rect );
        QRect tabBarRect( QPoint(0, 0), tabBarSize );

        Qt::Alignment tabBarAlignment( styleHint( SH_TabBar_Alignment, option, widget ) );

        // horizontal positioning
        const bool verticalTabs( isVerticalTab( tabOption->shape ) );
        if( verticalTabs )
        {

            tabBarRect.setHeight( qMin( tabBarRect.height(), rect.height() - 2 ) );
            if( tabBarAlignment == Qt::AlignCenter ) tabBarRect.moveTop( rect.top() + ( rect.height() - tabBarRect.height() )/2 );
            else tabBarRect.moveTop( rect.top()+1 );

        } else {

            // adjust rect to deal with corner buttons
            // need to properly deal with reverse layout
            const bool reverseLayout( option->direction == Qt::RightToLeft );
            if( !tabOption->leftCornerWidgetSize.isEmpty() )
            {
                const QRect buttonRect( subElementRect( SE_TabWidgetLeftCorner, option, widget ) );
                if( reverseLayout ) rect.setRight( buttonRect.left() - 1 );
                else rect.setLeft( buttonRect.width() );
            }

            if( !tabOption->rightCornerWidgetSize.isEmpty() )
            {
                const QRect buttonRect( subElementRect( SE_TabWidgetRightCorner, option, widget ) );
                if( reverseLayout ) rect.setLeft( buttonRect.width() );
                else rect.setRight( buttonRect.left() - 1 );
            }

            tabBarRect.setWidth( qMin( tabBarRect.width(), rect.width() - 2 ) );
            if( tabBarAlignment == Qt::AlignCenter ) tabBarRect.moveLeft( rect.left() + (rect.width() - tabBarRect.width())/2 );
            else {

                tabBarRect.moveLeft( rect.left() + 1 );
                tabBarRect = visualRect( option, tabBarRect );
            }
        }

        // vertical positioning
        switch( tabOption->shape )
        {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
            tabBarRect.moveTop( rect.top()+1 );
            break;

            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
            tabBarRect.moveBottom( rect.bottom()-1 );
            break;

            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
            tabBarRect.moveLeft( rect.left()+1 );
            break;

            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
            tabBarRect.moveRight( rect.right()-1 );
            break;

            default: break;

        }

        return tabBarRect;

    }

    //____________________________________________________________________
    QRect Style::tabWidgetTabContentsRect( const QStyleOption* option, const QWidget* widget ) const
    {

        // cast option and check
        // cast option and check
        const QStyleOptionTabWidgetFrame* tabOption = qstyleoption_cast<const QStyleOptionTabWidgetFrame*>( option );
        if( !tabOption ) return option->rect;

        // do nothing if tabbar is hidden
        if( tabOption->tabBarSize.isEmpty() ) return option->rect;
        const QRect rect = tabWidgetTabPaneRect( option, widget );

        const bool documentMode( tabOption->lineWidth == 0 );
        if( documentMode )
        {

            // add margin only to the relevant side
            switch( tabOption->shape )
            {
                case QTabBar::RoundedNorth:
                case QTabBar::TriangularNorth:
                return rect.adjusted( 0, Metrics::TabWidget_MarginWidth, 0, 0 );

                case QTabBar::RoundedSouth:
                case QTabBar::TriangularSouth:
                return rect.adjusted( 0, 0, 0, -Metrics::TabWidget_MarginWidth );

                case QTabBar::RoundedWest:
                case QTabBar::TriangularWest:
                return rect.adjusted( Metrics::TabWidget_MarginWidth, 0, 0, 0 );

                case QTabBar::RoundedEast:
                case QTabBar::TriangularEast:
                return rect.adjusted( 0, 0, -Metrics::TabWidget_MarginWidth, 0 );

                default: return rect;
            }

        } else return insideMargin( rect, Metrics::TabWidget_MarginWidth );

    }

    //____________________________________________________________________
    QRect Style::tabWidgetTabPaneRect( const QStyleOption* option, const QWidget* ) const
    {

        const QStyleOptionTabWidgetFrame* tabOption = qstyleoption_cast<const QStyleOptionTabWidgetFrame*>( option );
        if( !tabOption || tabOption->tabBarSize.isEmpty() ) return option->rect;

        const int overlap = Metrics::TabBar_BaseOverlap - 1;
        const QSize tabBarSize( tabOption->tabBarSize - QSize( overlap, overlap ) );

        QRect rect( option->rect );
        switch( tabOption->shape )
        {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
            rect.adjust( 0, tabBarSize.height(), 0, 0 );
            break;

            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
            rect.adjust( 0, 0, 0, -tabBarSize.height() );
            break;

            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
            rect.adjust( tabBarSize.width(), 0, 0, 0 );
            break;

            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
            rect.adjust( 0, 0, -tabBarSize.width(), 0 );
            break;

            default: break;
        }

        return rect;

    }

    //____________________________________________________________________
    QRect Style::tabWidgetCornerRect( SubElement element, const QStyleOption* option, const QWidget* ) const
    {

        // cast option and check
        const QStyleOptionTabWidgetFrame* tabOption = qstyleoption_cast<const QStyleOptionTabWidgetFrame*>( option );
        if( !tabOption ) return option->rect;

        // do nothing if tabbar is hidden
        const QSize tabBarSize( tabOption->tabBarSize );
        if( tabBarSize.isEmpty() ) return QRect();

        // do nothing for vertical tabs
        const bool verticalTabs( isVerticalTab( tabOption->shape ) );
        if( verticalTabs ) return QRect();

        const QRect rect( option->rect );
        QRect cornerRect( QPoint( 0, 0 ), QSize( tabBarSize.height(), tabBarSize.height() + 1 ) );

        if( element == SE_TabWidgetRightCorner ) cornerRect.moveRight( rect.right() );
        else cornerRect.moveLeft( rect.left() );

        switch( tabOption->shape )
        {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
            cornerRect.moveTop( rect.top() );
            break;

            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
            cornerRect.moveBottom( rect.bottom() );
            break;

            default: break;
        }

        // return cornerRect;
        cornerRect = visualRect( option, cornerRect );
        return cornerRect;

    }

    //____________________________________________________________________
    QRect Style::toolBoxTabContentsRect( const QStyleOption* option, const QWidget* widget ) const
    {

        // cast option and check
        const QStyleOptionToolBox* toolBoxOption( qstyleoption_cast<const QStyleOptionToolBox *>( option ) );
        if( !toolBoxOption ) return option->rect;

        // copy rect
        const QRect& rect( option->rect );

        int contentsWidth(0);
        if( !toolBoxOption->icon.isNull() )
        {
            const int iconSize( pixelMetric( QStyle::PM_SmallIconSize, option, widget ) );
            contentsWidth += iconSize;

            if( !toolBoxOption->text.isEmpty() ) contentsWidth += Metrics::ToolBox_TabItemSpacing;
        }

        if( !toolBoxOption->text.isEmpty() )
        {

            const int textWidth = toolBoxOption->fontMetrics.size( _mnemonics->textFlags(), toolBoxOption->text ).width();
            contentsWidth += textWidth;

        }

        contentsWidth = qMin( contentsWidth, rect.width() );
        contentsWidth = qMax( contentsWidth, int(Metrics::ToolBox_TabMinWidth) );
        return centerRect( rect, contentsWidth, rect.height() );

    }

    //______________________________________________________________
    QRect Style::groupBoxSubControlRect( const QStyleOptionComplex* option, SubControl subControl, const QWidget* widget ) const
    {

        QRect rect = option->rect;
        switch( subControl )
        {

            case SC_GroupBoxFrame: return rect;

            case SC_GroupBoxContents:
            {

                // cast option and check
                const QStyleOptionGroupBox *groupBoxOption = qstyleoption_cast<const QStyleOptionGroupBox*>( option );
                if( !groupBoxOption ) break;

                // take out frame width
                rect = insideMargin( rect, Metrics::Frame_FrameWidth );

                // get state
                const bool checkable( groupBoxOption->subControls & QStyle::SC_GroupBoxCheckBox );
                const bool emptyText( groupBoxOption->text.isEmpty() );

                // calculate title height
                int titleHeight( 0 );
                if( !emptyText ) titleHeight = groupBoxOption->fontMetrics.height();
                if( checkable ) titleHeight = qMax( titleHeight, int(Metrics::CheckBox_Size) );

                // add margin
                if( titleHeight > 0 ) titleHeight += 2*Metrics::GroupBox_TitleMarginWidth;

                rect.adjust( 0, titleHeight, 0, 0 );
                return rect;

            }

            case SC_GroupBoxCheckBox:
            case SC_GroupBoxLabel:
            {

                // cast option and check
                const QStyleOptionGroupBox *groupBoxOption = qstyleoption_cast<const QStyleOptionGroupBox*>( option );
                if( !groupBoxOption ) break;

                // take out frame width
                rect = insideMargin( rect, Metrics::Frame_FrameWidth );

                const bool emptyText( groupBoxOption->text.isEmpty() );
                const bool checkable( groupBoxOption->subControls & QStyle::SC_GroupBoxCheckBox );

                // calculate title height
                int titleHeight( 0 );
                int titleWidth( 0 );
                if( !emptyText )
                {
                    const QFontMetrics fontMetrics = option->fontMetrics;
                    titleHeight = qMax( titleHeight, fontMetrics.height() );
                    titleWidth += fontMetrics.size( _mnemonics->textFlags(), groupBoxOption->text ).width();
                }

                if( checkable )
                {
                    titleHeight = qMax( titleHeight, int(Metrics::CheckBox_Size) );
                    titleWidth += Metrics::CheckBox_Size;
                    if( !emptyText ) titleWidth += Metrics::CheckBox_ItemSpacing;
                }

                // adjust height
                QRect titleRect( rect );
                titleRect.setHeight( titleHeight );
                titleRect.translate( 0, Metrics::GroupBox_TitleMarginWidth );

                // center
                titleRect = centerRect( titleRect, titleWidth, titleHeight );

                if( subControl == SC_GroupBoxCheckBox )
                {

                    // vertical centering
                    titleRect = centerRect( titleRect, titleWidth, Metrics::CheckBox_Size );

                    // horizontal positioning
                    const QRect subRect( titleRect.topLeft(), QSize( Metrics::CheckBox_Size, titleRect.height() ) );
                    return visualRect( option->direction, titleRect, subRect );

                } else {

                    // vertical centering
                    QFontMetrics fontMetrics = option->fontMetrics;
                    titleRect = centerRect( titleRect, titleWidth, fontMetrics.height() );

                    // horizontal positioning
                    QRect subRect( titleRect );
                    if( checkable ) subRect.adjust( Metrics::CheckBox_Size + Metrics::CheckBox_ItemSpacing, 0, 0, 0 );
                    return visualRect( option->direction, titleRect, subRect );

                }

            }

            default: break;

        }

        return ParentStyleClass::subControlRect( CC_GroupBox, option, subControl, widget );
    }

    //___________________________________________________________________________________________________________________
    QRect Style::toolButtonSubControlRect( const QStyleOptionComplex* option, SubControl subControl, const QWidget* widget ) const
    {

        // cast option and check
        const QStyleOptionToolButton* toolButtonOption = qstyleoption_cast<const QStyleOptionToolButton*>( option );
        if( !toolButtonOption ) return ParentStyleClass::subControlRect( CC_ToolButton, option, subControl, widget );

        const bool hasPopupMenu( toolButtonOption->features & QStyleOptionToolButton::MenuButtonPopup );
        const bool hasInlineIndicator( toolButtonOption->features & QStyleOptionToolButton::HasMenu && !hasPopupMenu );

        // store rect
        const QRect& rect( option->rect );
        const int menuButtonWidth( Metrics::MenuButton_IndicatorWidth );
        switch( subControl )
        {
            case SC_ToolButtonMenu:
            {

                // check fratures
                if( !(hasPopupMenu || hasInlineIndicator ) ) return QRect();

                // check features
                QRect menuRect( rect );
                menuRect.setLeft( rect.right() - menuButtonWidth + 1 );
                if( hasInlineIndicator )
                { menuRect.setTop( menuRect.bottom() - menuButtonWidth + 1 ); }

                return visualRect( option, menuRect );
            }

            case SC_ToolButton:
            {

                if( hasPopupMenu )
                {

                    QRect contentsRect( rect );
                    contentsRect.setRight( rect.right() - menuButtonWidth );
                    return visualRect( option, contentsRect );

                } else return rect;

            }

            default: return QRect();

        }

    }

    //___________________________________________________________________________________________________________________
    QRect Style::comboBoxSubControlRect( const QStyleOptionComplex* option, SubControl subControl, const QWidget* widget ) const
    {
        // cast option and check
        const QStyleOptionComboBox *comboBoxOption( qstyleoption_cast<const QStyleOptionComboBox*>( option ) );
        if( !comboBoxOption ) return ParentStyleClass::subControlRect( CC_ComboBox, option, subControl, widget );

        const bool editable( comboBoxOption->editable );
        const bool flat( editable && !comboBoxOption->frame );

        // copy rect
        QRect rect( option->rect );

        switch( subControl )
        {
            case SC_ComboBoxFrame: return flat ? rect : QRect();
            case SC_ComboBoxListBoxPopup: return rect;

            case SC_ComboBoxArrow:
            {

                // take out frame width
                if( !flat ) rect = insideMargin( rect, Metrics::Frame_FrameWidth );

                QRect arrowRect(
                    rect.right() - Metrics::MenuButton_IndicatorWidth + 1,
                    rect.top(),
                    Metrics::MenuButton_IndicatorWidth,
                    rect.height() );

                arrowRect = centerRect( arrowRect, Metrics::MenuButton_IndicatorWidth, Metrics::MenuButton_IndicatorWidth );
                return visualRect( option, arrowRect );

            }

            case SC_ComboBoxEditField:
            {

                QRect labelRect;
                const int frameWidth( pixelMetric( PM_ComboBoxFrameWidth, option, widget ) );
                labelRect = QRect(
                    rect.left(), rect.top(),
                    rect.width() - Metrics::MenuButton_IndicatorWidth,
                    rect.height() );

                // remove margins
                if( !flat && rect.height() > option->fontMetrics.height() + 2*frameWidth )
                { labelRect.adjust( frameWidth, frameWidth, 0, -frameWidth ); }

                return visualRect( option, labelRect );

            }

            default: break;

        }

        return ParentStyleClass::subControlRect( CC_ComboBox, option, subControl, widget );

    }

    //___________________________________________________________________________________________________________________
    QRect Style::spinBoxSubControlRect( const QStyleOptionComplex* option, SubControl subControl, const QWidget* widget ) const
    {

        // cast option and check
        const QStyleOptionSpinBox *spinBoxOption( qstyleoption_cast<const QStyleOptionSpinBox*>( option ) );
        if( !spinBoxOption ) return ParentStyleClass::subControlRect( CC_SpinBox, option, subControl, widget );
        const bool flat( !spinBoxOption->frame );

        // copy rect
        QRect rect( option->rect );

        switch( subControl )
        {
            case SC_SpinBoxFrame: return flat ? QRect():rect;

            case SC_SpinBoxUp:
            case SC_SpinBoxDown:
            {

                // take out frame width
                if( !flat && rect.height() >= 2*Metrics::Frame_FrameWidth + Metrics::SpinBox_ArrowButtonWidth ) rect = insideMargin( rect, Metrics::Frame_FrameWidth );

                QRect arrowRect;
                arrowRect = QRect(
                    rect.right() - Metrics::SpinBox_ArrowButtonWidth + 1,
                    rect.top(),
                    Metrics::SpinBox_ArrowButtonWidth,
                    rect.height() );

                const int arrowHeight( qMin( rect.height(), int(Metrics::SpinBox_ArrowButtonWidth) ) );
                arrowRect = centerRect( arrowRect, Metrics::SpinBox_ArrowButtonWidth, arrowHeight );
                arrowRect.setHeight( arrowHeight/2 );
                if( subControl == SC_SpinBoxDown ) arrowRect.translate( 0, arrowHeight/2 );

                return visualRect( option, arrowRect );

            }

            case SC_SpinBoxEditField:
            {

                QRect labelRect;
                labelRect = QRect(
                    rect.left(), rect.top(),
                    rect.width() - Metrics::SpinBox_ArrowButtonWidth,
                    rect.height() );

                // remove right side line editor margins
                const int frameWidth( pixelMetric( PM_SpinBoxFrameWidth, option, widget ) );
                if( !flat && labelRect.height() > option->fontMetrics.height() + 2*frameWidth )
                { labelRect.adjust( frameWidth, frameWidth, 0, -frameWidth ); }

                return visualRect( option, labelRect );

            }

            default: break;

        }

        return ParentStyleClass::subControlRect( CC_SpinBox, option, subControl, widget );

    }

    //___________________________________________________________________________________________________________________
    QRect Style::scrollBarInternalSubControlRect( const QStyleOptionComplex* option, SubControl subControl ) const
    {

        const QRect& rect = option->rect;
        const State& state( option->state );
        const bool horizontal( state & State_Horizontal );

        switch( subControl )
        {

            case SC_ScrollBarSubLine:
            {
                int majorSize( scrollBarButtonHeight( _subLineButtons ) );
                if( horizontal ) return visualRect( option, QRect( rect.left(), rect.top(), majorSize, rect.height() ) );
                else return visualRect( option, QRect( rect.left(), rect.top(), rect.width(), majorSize ) );
            }

            case SC_ScrollBarAddLine:
            {
                int majorSize( scrollBarButtonHeight( _addLineButtons ) );
                if( horizontal ) return visualRect( option, QRect( rect.right() - majorSize + 1, rect.top(), majorSize, rect.height() ) );
                else return visualRect( option, QRect( rect.left(), rect.bottom() - majorSize + 1, rect.width(), majorSize ) );
            }

            default: return QRect();

        }
    }

    //___________________________________________________________________________________________________________________
    QRect Style::scrollBarSubControlRect( const QStyleOptionComplex* option, SubControl subControl, const QWidget* widget ) const
    {
        // cast option and check
        const QStyleOptionSlider* sliderOption( qstyleoption_cast<const QStyleOptionSlider*>( option ) );
        if( !sliderOption ) return ParentStyleClass::subControlRect( CC_ScrollBar, option, subControl, widget );

        // get relevant state
        const State& state( option->state );
        const bool horizontal( state & State_Horizontal );

        switch( subControl )
        {

            case SC_ScrollBarSubLine:
            case SC_ScrollBarAddLine:
            return scrollBarInternalSubControlRect( option, subControl );

            case SC_ScrollBarGroove:
            {
                QRect topRect = visualRect( option, scrollBarInternalSubControlRect( option, SC_ScrollBarSubLine ) );
                QRect bottomRect = visualRect( option, scrollBarInternalSubControlRect( option, SC_ScrollBarAddLine ) );

                QPoint topLeftCorner;
                QPoint botRightCorner;

                if( horizontal )
                {

                    topLeftCorner  = QPoint( topRect.right() + 1, topRect.top() );
                    botRightCorner = QPoint( bottomRect.left()  - 1, topRect.bottom() );

                } else {

                    topLeftCorner  = QPoint( topRect.left(),  topRect.bottom() + 1 );
                    botRightCorner = QPoint( topRect.right(), bottomRect.top() - 1 );

                }

                // define rect
                return visualRect( option, QRect( topLeftCorner, botRightCorner )  );

            }

            case SC_ScrollBarSlider:
            {

                // We handle RTL here to unreflect things if need be
                QRect groove = visualRect( option, scrollBarSubControlRect( option, SC_ScrollBarGroove, widget ) );

                if( sliderOption->minimum == sliderOption->maximum ) return groove;

                //Figure out how much room we have..
                int space( horizontal ? groove.width() : groove.height() );

                //Calculate the portion of this space that the slider should take up.
                int sliderSize = space * qreal( sliderOption->pageStep ) / ( sliderOption->maximum - sliderOption->minimum + sliderOption->pageStep );
                sliderSize = qMax( sliderSize, static_cast<int>(Metrics::ScrollBar_MinSliderHeight ) );
                sliderSize = qMin( sliderSize, space );

                space -= sliderSize;
                if( space <= 0 ) return groove;

                int pos = qRound( qreal( sliderOption->sliderPosition - sliderOption->minimum )/ ( sliderOption->maximum - sliderOption->minimum )*space );
                if( sliderOption->upsideDown ) pos = space - pos;
                if( horizontal ) return visualRect( option, QRect( groove.left() + pos, groove.top(), sliderSize, groove.height() ) );
                else return visualRect( option, QRect( groove.left(), groove.top() + pos, groove.width(), sliderSize ) );
            }

            case SC_ScrollBarSubPage:
            {

                //We do visualRect here to unreflect things if need be
                QRect slider = visualRect( option, scrollBarSubControlRect( option, SC_ScrollBarSlider, widget ) );
                QRect groove = visualRect( option, scrollBarSubControlRect( option, SC_ScrollBarGroove, widget ) );

                if( horizontal ) return visualRect( option, QRect( groove.left(), groove.top(), slider.left() - groove.left(), groove.height() ) );
                else return visualRect( option, QRect( groove.left(), groove.top(), groove.width(), slider.top() - groove.top() ) );
            }

            case SC_ScrollBarAddPage:
            {

                //We do visualRect here to unreflect things if need be
                QRect slider = visualRect( option, scrollBarSubControlRect( option, SC_ScrollBarSlider, widget ) );
                QRect groove = visualRect( option, scrollBarSubControlRect( option, SC_ScrollBarGroove, widget ) );

                if( horizontal ) return visualRect( option, QRect( slider.right() + 1, groove.top(), groove.right() - slider.right(), groove.height() ) );
                else return visualRect( option, QRect( groove.left(), slider.bottom() + 1, groove.width(), groove.bottom() - slider.bottom() ) );

            }

            default: return ParentStyleClass::subControlRect( CC_ScrollBar, option, subControl, widget );;
        }
    }


    //___________________________________________________________________________________________________________________
    QRect Style::sliderSubControlRect( const QStyleOptionComplex* option, SubControl subControl, const QWidget* widget ) const
    {

        // cast option and check
        const QStyleOptionSlider* sliderOption( qstyleoption_cast<const QStyleOptionSlider*>( option ) );
        if( !sliderOption ) return ParentStyleClass::subControlRect( CC_Slider, option, subControl, widget );

        switch( subControl )
        {
            case SC_SliderGroove:
            {

                // direction
                const bool horizontal( sliderOption->orientation == Qt::Horizontal );

                // get base class rect
                QRect grooveRect( ParentStyleClass::subControlRect( CC_Slider, option, subControl, widget ) );
                grooveRect = insideMargin( grooveRect, pixelMetric( PM_DefaultFrameWidth, option, widget ) );

                // centering
                if( horizontal ) grooveRect = centerRect( grooveRect, grooveRect.width(), Metrics::Slider_GrooveThickness ).adjusted( 3, 0, -3, 0 );
                else grooveRect = centerRect( grooveRect, Metrics::Slider_GrooveThickness, grooveRect.height() ).adjusted( 0, 3, 0, -3 );
                return grooveRect;

            }

            case SC_SliderHandle:
            {

                QRect handleRect( ParentStyleClass::subControlRect( CC_Slider, option, subControl, widget ) );
                handleRect = centerRect( handleRect, Metrics::Slider_ControlThickness, Metrics::Slider_ControlThickness );
                return handleRect;

            }

            default: return ParentStyleClass::subControlRect( CC_Slider, option, subControl, widget );
        }

    }

    //______________________________________________________________
    QSize Style::checkBoxSizeFromContents( const QStyleOption*, const QSize& contentsSize, const QWidget* ) const
    {
        // get contents size
        QSize size( contentsSize );

        // add focus height
        size = expandSize( size, 0, Metrics::CheckBox_FocusMarginWidth );

        // make sure there is enough height for indicator
        size.setHeight( qMax( size.height(), int(Metrics::CheckBox_Size) ) );

        // Add space for the indicator and the icon
        size.rwidth() += Metrics::CheckBox_Size + Metrics::CheckBox_ItemSpacing;

        // also add extra space, to leave room to the right of the label
        size.rwidth() += Metrics::CheckBox_ItemSpacing;

        return size;

    }

    //______________________________________________________________
    QSize Style::lineEditSizeFromContents( const QStyleOption* option, const QSize& contentsSize, const QWidget* widget ) const
    {
        // cast option and check
        const QStyleOptionFrame* frameOption( qstyleoption_cast<const QStyleOptionFrame*>( option ) );
        if( !frameOption ) return contentsSize;

        const bool flat( frameOption->lineWidth == 0 );
        const int frameWidth( pixelMetric( PM_DefaultFrameWidth, option, widget ) );
        return flat ? contentsSize : expandSize( contentsSize, frameWidth );
    }

    //______________________________________________________________
    QSize Style::comboBoxSizeFromContents( const QStyleOption* option, const QSize& contentsSize, const QWidget* widget ) const
    {

        // cast option and check
        const QStyleOptionComboBox* comboBoxOption( qstyleoption_cast<const QStyleOptionComboBox*>( option ) );
        if( !comboBoxOption ) return contentsSize;

        // copy size
        QSize size( contentsSize );

        // add relevant margin
        const bool flat( !comboBoxOption->frame );
        const int frameWidth( pixelMetric( PM_ComboBoxFrameWidth, option, widget ) );
        if( !flat ) size = expandSize( size, frameWidth );

        // make sure there is enough height for the button
        size.setHeight( qMax( size.height(), int(Metrics::MenuButton_IndicatorWidth) ) );

        // add button width and spacing
        size.rwidth() += Metrics::MenuButton_IndicatorWidth;

        return size;

    }

    //______________________________________________________________
    QSize Style::spinBoxSizeFromContents( const QStyleOption* option, const QSize& contentsSize, const QWidget* widget ) const
    {

        // cast option and check
        const QStyleOptionSpinBox *spinBoxOption( qstyleoption_cast<const QStyleOptionSpinBox*>( option ) );
        if( !spinBoxOption ) return contentsSize;

        const bool flat( !spinBoxOption->frame );

        // copy size
        QSize size( contentsSize );

        // add editor margins
        const int frameWidth( pixelMetric( PM_SpinBoxFrameWidth, option, widget ) );
        if( !flat ) size = expandSize( size, frameWidth );

        // make sure there is enough height for the button
        size.setHeight( qMax( size.height(), int(Metrics::SpinBox_ArrowButtonWidth) ) );

        // add button width and spacing
        size.rwidth() += Metrics::SpinBox_ArrowButtonWidth;

        return size;

    }

    //______________________________________________________________
    QSize Style::sliderSizeFromContents( const QStyleOption* option, const QSize& contentsSize, const QWidget* ) const
    {

        // cast option and check
        const QStyleOptionSlider *sliderOption( qstyleoption_cast<const QStyleOptionSlider*>( option ) );
        if( !sliderOption ) return contentsSize;

        // store tick position and orientation
        const QSlider::TickPosition& tickPosition( sliderOption->tickPosition );
        const bool horizontal( sliderOption->orientation == Qt::Horizontal );
        const bool disableTicks( !StyleConfigData::sliderDrawTickMarks() );

        /*
        Qt adds its own tick length directly inside QSlider.
        Take it out and replace by ours, if needed
        */
        const int tickLength( disableTicks ? 0 : (
            Metrics::Slider_TickLength + Metrics::Slider_TickMarginWidth +
            (Metrics::Slider_GrooveThickness - Metrics::Slider_ControlThickness)/2 ) );

        const int builtInTickLength( 5 );
        if( tickPosition == QSlider::NoTicks ) return contentsSize;

        QSize size( contentsSize );
        if( horizontal )
        {

            if(tickPosition & QSlider::TicksAbove) size.rheight() += tickLength - builtInTickLength;
            if(tickPosition & QSlider::TicksBelow) size.rheight() += tickLength - builtInTickLength;

        } else {

            if(tickPosition & QSlider::TicksAbove) size.rwidth() += tickLength - builtInTickLength;
            if(tickPosition & QSlider::TicksBelow) size.rwidth() += tickLength - builtInTickLength;

        }

        return size;

    }

    //______________________________________________________________
    QSize Style::pushButtonSizeFromContents( const QStyleOption* option, const QSize& contentsSize, const QWidget* widget ) const
    {

        // cast option and check
        const QStyleOptionButton* buttonOption( qstyleoption_cast<const QStyleOptionButton*>( option ) );
        if( !buttonOption ) return contentsSize;

        QSize size( contentsSize );

        // add space for arrow
        if( buttonOption->features & QStyleOptionButton::HasMenu )
        {
            size.rheight() += 2*Metrics::Button_MarginWidth;
            size.setHeight( qMax( size.height(), int( Metrics::MenuButton_IndicatorWidth ) ) );
            size.rwidth() += Metrics::Button_MarginWidth;

            if( !( buttonOption->icon.isNull() && buttonOption->text.isEmpty() ) )
            { size.rwidth() += Metrics::Button_ItemSpacing; }

        }  else size = expandSize( size, Metrics::Button_MarginWidth );

        // add space for icon
        if( !buttonOption->icon.isNull() )
        {

            QSize iconSize = buttonOption->iconSize;
            if( !iconSize.isValid() ) iconSize = QSize( pixelMetric( PM_SmallIconSize, option, widget ), pixelMetric( PM_SmallIconSize, option, widget ) );

            size.setHeight( qMax( size.height(), iconSize.height() ) );

            if( !buttonOption->text.isEmpty() )
            { size.rwidth() += Metrics::Button_ItemSpacing; }

        }

        // make sure buttons have a minimum width
        if( !buttonOption->text.isEmpty() )
        { size.rwidth() = qMax( size.rwidth(), int( Metrics::Button_MinWidth ) ); }

        // finally add margins
        return expandSize( size, Metrics::Frame_FrameWidth );

    }

    //______________________________________________________________
    QSize Style::toolButtonSizeFromContents( const QStyleOption* option, const QSize& contentsSize, const QWidget* ) const
    {

        // cast option and check
        const QStyleOptionToolButton* toolButtonOption = qstyleoption_cast<const QStyleOptionToolButton*>( option );
        if( !toolButtonOption ) return contentsSize;

        // copy size
        QSize size = contentsSize;

        // get relevant state flags
        const State& state( option->state );
        const bool autoRaise( state & State_AutoRaise );
        const bool hasPopupMenu( toolButtonOption->subControls & SC_ToolButtonMenu );
        const bool hasInlineIndicator( toolButtonOption->features & QStyleOptionToolButton::HasMenu && !hasPopupMenu );
        const int marginWidth( autoRaise ? Metrics::ToolButton_MarginWidth : Metrics::Button_MarginWidth + Metrics::Frame_FrameWidth );

        if( hasInlineIndicator ) size.rwidth() += Metrics::ToolButton_InlineIndicatorWidth;
        size = expandSize( size, marginWidth );

        return size;

    }

    //______________________________________________________________
    QSize Style::menuBarItemSizeFromContents( const QStyleOption*, const QSize& contentsSize, const QWidget* ) const
    { return expandSize( contentsSize, Metrics::MenuBarItem_MarginWidth, Metrics::MenuBarItem_MarginHeight ); }

    //______________________________________________________________
    QSize Style::menuItemSizeFromContents( const QStyleOption* option, const QSize& contentsSize, const QWidget* widget ) const
    {

        // cast option and check
        const QStyleOptionMenuItem* menuItemOption = qstyleoption_cast<const QStyleOptionMenuItem*>( option );
        if( !menuItemOption ) return contentsSize;

        // First, we calculate the intrinsic size of the item.
        // this must be kept consistent with what's in drawMenuItemControl
        QSize size( contentsSize );
        switch( menuItemOption->menuItemType )
        {

            case QStyleOptionMenuItem::Normal:
            case QStyleOptionMenuItem::DefaultItem:
            case QStyleOptionMenuItem::SubMenu:
            {

                const int iconWidth( menuItemOption->maxIconWidth );
                int leftColumnWidth( iconWidth );

                // add space with respect to text
                leftColumnWidth += Metrics::MenuItem_ItemSpacing;

                // add checkbox indicator width
                if( menuItemOption->menuHasCheckableItems )
                { leftColumnWidth += Metrics::CheckBox_Size + Metrics::MenuItem_ItemSpacing; }

                // add spacing for accelerator
                /*
                Note:
                The width of the accelerator itself is not included here since
                Qt will add that on separately after obtaining the
                sizeFromContents() for each menu item in the menu to be shown
                ( see QMenuPrivate::calcActionRects() )
                */
                const bool hasAccelerator( menuItemOption->text.indexOf( QLatin1Char( '\t' ) ) >= 0 );
                if( hasAccelerator ) size.rwidth() += Metrics::MenuItem_AcceleratorSpace;

                // right column
                const int rightColumnWidth = Metrics::MenuButton_IndicatorWidth + Metrics::MenuItem_ItemSpacing;
                size.rwidth() += leftColumnWidth + rightColumnWidth;

                // make sure height is large enough for icon and arrow
                size.setHeight( qMax( size.height(), int(Metrics::MenuButton_IndicatorWidth) ) );
                size.setHeight( qMax( size.height(), int(Metrics::CheckBox_Size) ) );
                size.setHeight( qMax( size.height(), iconWidth ) );
                return expandSize( size, Metrics::MenuItem_MarginWidth );

            }

            case QStyleOptionMenuItem::Separator:
            {

                if( menuItemOption->text.isEmpty() && menuItemOption->icon.isNull() )
                {

                    return expandSize( QSize(0,1), Metrics::MenuItem_MarginWidth );


                } else {

                    // build toolbutton option
                    const QStyleOptionToolButton toolButtonOption( separatorMenuItemOption( menuItemOption, widget ) );

                    // make sure height is large enough for icon and text
                    const int iconWidth( menuItemOption->maxIconWidth );
                    const int textHeight( menuItemOption->fontMetrics.height() );
                    if( !menuItemOption->icon.isNull() ) size.setHeight( qMax( size.height(), iconWidth ) );
                    if( !menuItemOption->text.isEmpty() ) size.setHeight( qMax( size.height(), textHeight ) );

                    return sizeFromContents( CT_ToolButton, &toolButtonOption, size, widget );

                }

            }

            // for all other cases, return input
            default: return contentsSize;
        }

    }

    //______________________________________________________________
    QSize Style::tabWidgetSizeFromContents( const QStyleOption* option, const QSize& contentsSize, const QWidget* ) const
    {

        // cast option and check
        const QStyleOptionTabWidgetFrame* tabOption = qstyleoption_cast<const QStyleOptionTabWidgetFrame*>( option );
        if( !tabOption ) return expandSize( contentsSize, Metrics::Frame_FrameWidth );

        // tab orientation
        const bool verticalTabs( tabOption && isVerticalTab( tabOption->shape ) );

        // need to reduce the size in the tabbar direction, due to a bug in QTabWidget::minimumSize
        return verticalTabs ?
            expandSize( contentsSize, Metrics::Frame_FrameWidth, Metrics::Frame_FrameWidth - 1 ):
            expandSize( contentsSize, Metrics::Frame_FrameWidth - 1, Metrics::Frame_FrameWidth );

    }

    //______________________________________________________________
    QSize Style::tabBarTabSizeFromContents( const QStyleOption* option, const QSize& contentsSize, const QWidget* ) const
    {

        const QStyleOptionTab *tabOption( qstyleoption_cast<const QStyleOptionTab*>( option ) );

        // add margins
        QSize size( contentsSize );

        // compare to minimum size
        const bool verticalTabs( tabOption && isVerticalTab( tabOption ) );
        if( verticalTabs )
        {

            size = expandSize( size, Metrics::TabBar_TabMarginHeight, Metrics::TabBar_TabMarginWidth );
            size = size.expandedTo( QSize( Metrics::TabBar_TabMinHeight, Metrics::TabBar_TabMinWidth ) );

        } else {

            size = expandSize( size, Metrics::TabBar_TabMarginWidth, Metrics::TabBar_TabMarginHeight );
            size = size.expandedTo( QSize( Metrics::TabBar_TabMinWidth, Metrics::TabBar_TabMinHeight ) );

        }

        return size;

    }

    //______________________________________________________________
    QSize Style::headerSectionSizeFromContents( const QStyleOption* option, const QSize& contentsSize, const QWidget* ) const
    {

        // cast option and check
        const QStyleOptionHeader* headerOption( qstyleoption_cast<const QStyleOptionHeader*>( option ) );
        if( !headerOption ) return contentsSize;

        // get text size
        const bool horizontal( headerOption->orientation == Qt::Horizontal );
        const bool hasText( !headerOption->text.isEmpty() );
        const bool hasIcon( !headerOption->icon.isNull() );

        const QSize textSize( hasText ? headerOption->fontMetrics.size( 0, headerOption->text ) : QSize() );
        const QSize iconSize( hasIcon ? QSize( 22,22 ) : QSize() );

        // contents width
        int contentsWidth( 0 );
        if( hasText ) contentsWidth += textSize.width();
        if( hasIcon )
        {
            contentsWidth += iconSize.width();
            if( hasText ) contentsWidth += Metrics::Header_ItemSpacing;
        }

        // contents height
        int contentsHeight( headerOption->fontMetrics.height() );
        if( hasIcon ) contentsHeight = qMax( contentsHeight, iconSize.height() );

        if( horizontal )
        {
            // also add space for icon
            contentsWidth += Metrics::Header_ArrowSize + Metrics::Header_ItemSpacing;
            contentsHeight = qMax( contentsHeight, int(Metrics::Header_ArrowSize) );
        }

        // update contents size, add margins and return
        const QSize size( contentsSize.expandedTo( QSize( contentsWidth, contentsHeight ) ) );
        return expandSize( size, Metrics::Header_MarginWidth );

    }

    //______________________________________________________________
    QSize Style::itemViewItemSizeFromContents( const QStyleOption* option, const QSize& contentsSize, const QWidget* widget ) const
    {
        // call base class
        QSize size( ParentStyleClass::sizeFromContents( CT_ItemViewItem, option, contentsSize, widget ) );

        // add margins
        return expandSize( size, Metrics::ItemView_ItemMarginWidth );

    }

    //___________________________________________________________________________________
    bool Style::drawFramePrimitive( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        // copy rect and palette
        const QRect& rect( option->rect );
        const QPalette& palette( option->palette );

        // store state
        const State& state( option->state );
        const bool enabled( state & State_Enabled );
        const bool isQtQuickControl = !widget && option && option->styleObject && option->styleObject->inherits( "QQuickStyleItem" );
        const bool isInputWidget( ( widget && widget->testAttribute( Qt::WA_Hover ) )
                                  || ( isQtQuickControl && option->styleObject->property( "elementType" ).toString() == QStringLiteral( "edit") ) );

        // hover
        const bool hoverHighlight( enabled && isInputWidget && ( state & State_MouseOver ) );

        // focus
        bool focusHighlight( false );
        if( enabled && ( state & State_HasFocus ) ) focusHighlight = true;

        // assume focus takes precedence over hover
        _animations->lineEditEngine().updateState( widget, AnimationFocus, focusHighlight );
        _animations->lineEditEngine().updateState( widget, AnimationHover, hoverHighlight && !focusHighlight );

        if( state & State_Sunken )
        {
            const QRect local( rect );
            qreal opacity( -1 );
            AnimationMode mode = AnimationNone;
            if( enabled && _animations->lineEditEngine().isAnimated( widget, AnimationFocus ) )
            {

                opacity = _animations->lineEditEngine().opacity( widget, AnimationFocus  );
                mode = AnimationFocus;

            } else if( enabled && _animations->lineEditEngine().isAnimated( widget, AnimationHover ) ) {

                opacity = _animations->lineEditEngine().opacity( widget, AnimationHover );
                mode = AnimationHover;

            }

            if( _frameShadowFactory->isRegistered( widget ) )
            {

                _frameShadowFactory->updateState( widget, focusHighlight, hoverHighlight, opacity, mode );

            } else {

                HoleOptions options( 0 );
                if( focusHighlight ) options |= HoleFocus;
                if( hoverHighlight ) options |= HoleHover;

                _helper->renderHole(
                    painter, palette.color( QPalette::Window ), local, options,
                    opacity, mode, TileSet::Ring );

            }

        } else if( state & State_Raised ) {

            const QRect local( rect );
            renderSlab( painter, local, palette.color( QPalette::Background ), NoFill );

        }

        return true;
    }

    //___________________________________________________________________________________
    bool Style::drawFrameLineEditPrimitive( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        // copy rect and palette
        const QRect& rect( option->rect );
        const QPalette& palette( option->palette );

        // store state
        const State& state( option->state );
        const bool enabled( state & State_Enabled );

        // hover
        const bool hoverHighlight( enabled && ( state & State_MouseOver ) );

        // focus
        bool focusHighlight( false );
        if( enabled && ( state & State_HasFocus ) ) focusHighlight = true;

        // assume focus takes precedence over hover
        _animations->lineEditEngine().updateState( widget, AnimationFocus, focusHighlight );
        _animations->lineEditEngine().updateState( widget, AnimationHover, hoverHighlight && !focusHighlight );

        const QRect local( rect );
        qreal opacity( -1 );
        AnimationMode mode = AnimationNone;
        if( enabled && _animations->lineEditEngine().isAnimated( widget, AnimationFocus ) )
        {

            opacity = _animations->lineEditEngine().opacity( widget, AnimationFocus  );
            mode = AnimationFocus;

        } else if( enabled && _animations->lineEditEngine().isAnimated( widget, AnimationHover ) ) {

            opacity = _animations->lineEditEngine().opacity( widget, AnimationHover );
            mode = AnimationHover;

        }

        HoleOptions options( 0 );
        if( focusHighlight ) options |= HoleFocus;
        if( hoverHighlight ) options |= HoleHover;

        painter->setPen( Qt::NoPen );
        painter->setBrush( palette.color( QPalette::Base ) );
        _helper->fillHole( *painter, rect );
        _helper->renderHole(
            painter, palette.color( QPalette::Window ), local, options,
            opacity, mode, TileSet::Ring );

        return true;
    }

    //___________________________________________________________________________________
    bool Style::drawFrameFocusRectPrimitive( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        if( !widget ) return true;

        // no focus indicator on buttons, since it is rendered elsewhere
        if( qobject_cast< const QAbstractButton*>( widget ) )
        { return true; }

        const State& state( option->state );
        const QRect rect( option->rect.adjusted( 0, 0, 0, -1 ) );
        const QPalette& palette( option->palette );

        if( rect.width() < 10 ) return true;

        QLinearGradient lg( rect.bottomLeft(), rect.bottomRight() );

        lg.setColorAt( 0.0, Qt::transparent );
        lg.setColorAt( 1.0, Qt::transparent );
        if( state & State_Selected )
        {

            lg.setColorAt( 0.2, palette.color( QPalette::BrightText ) );
            lg.setColorAt( 0.8, palette.color( QPalette::BrightText ) );

        } else {

            lg.setColorAt( 0.2, palette.color( QPalette::Text ) );
            lg.setColorAt( 0.8, palette.color( QPalette::Text ) );

        }

        painter->setRenderHint( QPainter::Antialiasing, false );
        painter->setPen( QPen( lg, 1 ) );
        painter->drawLine( rect.bottomLeft(), rect.bottomRight() );

        return true;

    }

    //______________________________________________________________
    bool Style::drawFrameGroupBoxPrimitive( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        // cast option and check
        const QStyleOptionFrame *frameOption( qstyleoption_cast<const QStyleOptionFrame*>( option ) );
        if( !frameOption ) return true;

        // no frame for flat groupboxes
        QStyleOptionFrameV2 frameOption2( *frameOption );
        if( frameOption2.features & QStyleOptionFrameV2::Flat ) return true;

        // normal frame
        const QPalette& palette( option->palette );
        const QRect& rect( option->rect );
        const QColor base( _helper->backgroundColor( palette.color( QPalette::Window ), widget, rect.center() ) );

        painter->save();
        painter->setRenderHint( QPainter::Antialiasing );
        painter->setPen( Qt::NoPen );

        QLinearGradient innerGradient( 0, rect.top()-rect.height()+12, 0, rect.bottom()+rect.height()-19 );
        QColor light( _helper->calcLightColor( base ) );
        light.setAlphaF( 0.4 ); innerGradient.setColorAt( 0.0, light );
        light.setAlphaF( 0.0 ); innerGradient.setColorAt( 1.0, light );
        painter->setBrush( innerGradient );
        painter->setClipRect( rect.adjusted( 0, 0, 0, -19 ) );
        _helper->fillSlab( *painter, rect );

        painter->setClipping( false );
        _helper->slope( base, 0.0 )->render( rect, painter );

        painter->restore();
        return true;

    }

    //___________________________________________________________________________________
    bool Style::drawFrameMenuPrimitive( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {
        // only draw frame for ( expanded ) toolbars
        // do nothing for other cases
        if( qobject_cast<const QToolBar*>( widget ) )
        {

            _helper->renderWindowBackground( painter, option->rect, widget, option->palette );
            _helper->drawFloatFrame( painter, option->rect, option->palette.window().color(), true );

        } else if( option->styleObject && option->styleObject->inherits( "QQuickItem" ) ) {

            // QtQuick Control case
            _helper->drawFloatFrame( painter, option->rect, option->palette.window().color(), true );

        }

        return true;

    }

    //___________________________________________________________________________________
    bool Style::drawFrameTabBarBasePrimitive( const QStyleOption* option, QPainter* painter, const QWidget* ) const
    {

        // tabbar frame used either for 'separate' tabbar, or in 'document mode'

        // cast option and check
        const QStyleOptionTabBarBase* tabOption( qstyleoption_cast<const QStyleOptionTabBarBase*>( option ) );
        if( !tabOption ) return true;

        if( tabOption->tabBarRect.isValid() )
        {

            // if tabBar rect is valid, all the frame is handled in tabBarTabShapeControl
            // nothing to be done here.
            // on the other hand, invalid tabBarRect corresponds to buttons in tabbars ( e.g. corner buttons ),
            // and the appropriate piece of frame needs to be drawn
            return true;

        }

        // store palette and rect
        const QPalette& palette( option->palette );
        const QRect& r( option->rect );

        QRect frameRect( r );
        SlabRect slab;
        switch( tabOption->shape )
        {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
            {
                frameRect.adjust( -7, 0, 7, 0 );
                frameRect.translate( 0, 4 );
                slab = SlabRect( frameRect, TileSet::Top );
                break;
            }

            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
            {
                frameRect.adjust( -7, 0, 7, 0 );
                frameRect.translate( 0, -4 );
                slab = SlabRect( frameRect, TileSet::Bottom );
                break;
            }

            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
            {
                frameRect.adjust( 0, -7, 0, 7 + 1 );
                frameRect.translate( 5, 0 );
                slab = SlabRect( frameRect, TileSet::Left );
                break;
            }

            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
            {
                frameRect.adjust( 0, -7, 0, 7 + 1 );
                frameRect.translate( -5, 0 );
                slab = SlabRect( frameRect, TileSet::Right );
                break;
            }

            default: return true;
        }

        // render registered slabs
        renderSlab( painter, slab, palette.color( QPalette::Window ), NoFill );

        return true;
    }

    //___________________________________________________________________________________
    bool Style::drawFrameTabWidgetPrimitive( const QStyleOption* option, QPainter* painter, const QWidget* ) const
    {

        // cast option and check
        const QStyleOptionTabWidgetFrame* tabOption( qstyleoption_cast<const QStyleOptionTabWidgetFrame*>( option ) );
        if( !tabOption ) return true;

        const QRect& r( option->rect );
        const QPalette& palette( option->palette );
        const bool reverseLayout( option->direction == Qt::RightToLeft );

        /*
        no frame is drawn when tabbar is empty.
        this is consistent with the tabWidgetTabContents subelementRect
        */
        if( tabOption->tabBarSize.isEmpty() ) return true;

        // get tabbar dimentions
        const int w( tabOption->tabBarSize.width() );
        const int h( tabOption->tabBarSize.height() );

        // left corner widget
        const int lw( tabOption->leftCornerWidgetSize.width() );
        const int lh( tabOption->leftCornerWidgetSize.height() );

        // right corner
        const int rw( tabOption->rightCornerWidgetSize.width() );
        const int rh( tabOption->rightCornerWidgetSize.height() );

        // list of slabs to be drawn
        SlabRectList slabs;

        // expand rect by glow width.
        QRect baseSlabRect( insideMargin( r, 0 ) );

        // render the three free sides
        switch( tabOption->shape )
        {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
            {
                // main slab
                slabs << SlabRect( baseSlabRect, ( TileSet::Ring & ~TileSet::Top ) );

                // top
                if( reverseLayout )
                {

                    // left side
                    QRect slabRect( baseSlabRect );
                    slabRect.setRight( qMax( slabRect.right() - w - lw, slabRect.left() + rw ) + 7 );
                    slabRect.setHeight( 7 );
                    slabs << SlabRect( slabRect, TileSet::TopLeft );

                    // right side
                    if( rw > 0 )
                    {
                        QRect slabRect( baseSlabRect );
                        slabRect.setLeft( slabRect.right() - rw - 7 );
                        slabRect.setHeight( 7 );
                        slabs << SlabRect( slabRect, TileSet::TopRight );
                    }

                } else {

                    // left side
                    if( lw > 0 )
                    {

                        QRect slabRect( baseSlabRect );
                        slabRect.setRight( baseSlabRect.left() + lw + 7 );
                        slabRect.setHeight( 7 );
                        slabs << SlabRect( slabRect, TileSet::TopLeft );

                    }


                    // right side
                    QRect slabRect( baseSlabRect );
                    slabRect.setLeft( qMin( slabRect.left() + w + lw + 1, slabRect.right() - rw ) -7 );
                    slabRect.setHeight( 7 );
                    slabs << SlabRect( slabRect, TileSet::TopRight );

                }
                break;
            }

            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
            {

                slabs << SlabRect( baseSlabRect, TileSet::Ring & ~TileSet::Bottom );

                if( reverseLayout )
                {

                    // left side
                    QRect slabRect( baseSlabRect );
                    slabRect.setRight( qMax( slabRect.right() - w - lw, slabRect.left() + rw ) + 7 );
                    slabRect.setTop( slabRect.bottom() - 7 );
                    slabs << SlabRect( slabRect, TileSet::BottomLeft );

                    // right side
                    if( rw > 0 )
                    {
                        QRect slabRect( baseSlabRect );
                        slabRect.setLeft( slabRect.right() - rw - 7 );
                        slabRect.setTop( slabRect.bottom() - 7 );
                        slabs << SlabRect( slabRect, TileSet::BottomRight );
                    }

                } else {

                    // left side
                    if( lw > 0 )
                    {

                        QRect slabRect( baseSlabRect );
                        slabRect.setRight( baseSlabRect.left() + lw + 7 );
                        slabRect.setTop( slabRect.bottom() - 7 );
                        slabs << SlabRect( slabRect, TileSet::BottomLeft );

                    }

                    // right side
                    QRect slabRect( baseSlabRect );
                    slabRect.setLeft( qMin( slabRect.left() + w + lw + 1, slabRect.right() - rw ) -7 );
                    slabRect.setTop( slabRect.bottom() - 7 );
                    slabs << SlabRect( slabRect, TileSet::BottomRight );

                }

                break;
            }

            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
            {
                slabs << SlabRect( baseSlabRect, TileSet::Ring & ~TileSet::Left );

                // top side
                if( lh > 0 )
                {

                    QRect slabRect( baseSlabRect );
                    slabRect.setBottom( baseSlabRect.top() + lh + 7 + 1 );
                    slabRect.setWidth( 7 );
                    slabs << SlabRect( slabRect, TileSet::TopLeft );

                }

                // bottom side
                QRect slabRect( baseSlabRect );
                slabRect.setTop( qMin( slabRect.top() + h + lh, slabRect.bottom() - rh ) -7 + 1 );
                slabRect.setWidth( 7 );
                slabs << SlabRect( slabRect, TileSet::BottomLeft );

                break;
            }

            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
            {
                slabs << SlabRect( baseSlabRect, TileSet::Ring & ~TileSet::Right );

                // top side
                if( lh > 0 )
                {

                    QRect slabRect( baseSlabRect );
                    slabRect.setBottom( baseSlabRect.top() + lh + 7 + 1 );
                    slabRect.setLeft( slabRect.right()-7 );
                    slabs << SlabRect( slabRect, TileSet::TopRight );

                }

                // bottom side
                QRect slabRect( baseSlabRect );
                slabRect.setTop( qMin( slabRect.top() + h + lh, slabRect.bottom() - rh ) -7 + 1 );
                slabRect.setLeft( slabRect.right()-7 );
                slabs << SlabRect( slabRect, TileSet::BottomRight );
                break;
            }

            break;

            default: break;
        }

        // render registered slabs
        foreach( const SlabRect& slab, slabs )
        { renderSlab( painter, slab, palette.color( QPalette::Window ), NoFill ); }

        return true;

    }

    //___________________________________________________________________________________
    bool Style::drawFrameWindowPrimitive( const QStyleOption* option, QPainter* painter, const QWidget* ) const
    {

        const QRect& r( option->rect );
        const QPalette& palette( option->palette );
        _helper->drawFloatFrame( painter, r, palette.window().color(), false );

        return true;

    }
    //___________________________________________________________________________________
    bool Style::drawIndicatorTabClosePrimitive( const QStyleOption* option, QPainter* painter, const QWidget* ) const
    {
        if( _tabCloseIcon.isNull() )
        {
            // load the icon on-demand: in the constructor, KDE is not yet ready to find it!
            _tabCloseIcon = QIcon::fromTheme( QStringLiteral( "dialog-close" ) );
            if( _tabCloseIcon.isNull() ) return false;
        }

        const int size( pixelMetric(QStyle::PM_SmallIconSize) );
        QIcon::Mode mode;
        if( option->state & State_Enabled )
        {
            if( option->state & State_Raised ) mode = QIcon::Active;
            else mode = QIcon::Normal;
        } else mode = QIcon::Disabled;

        if( !(option->state & State_Raised)
            && !(option->state & State_Sunken)
            && !(option->state & QStyle::State_Selected))
            mode = QIcon::Disabled;

        QIcon::State state = option->state & State_Sunken ? QIcon::On:QIcon::Off;
        QPixmap pixmap = _tabCloseIcon.pixmap(size, mode, state);
        drawItemPixmap( painter, option->rect, Qt::AlignCenter, pixmap );
        return true;
    }

    //___________________________________________________________________________________
    bool Style::drawIndicatorArrowPrimitive( ArrowOrientation orientation, const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {
        const QRect rect( option->rect );
        const QPalette& palette( option->palette );
        const State& state( option->state );
        const bool enabled( state & State_Enabled );
        const bool mouseOver( enabled && ( state & State_MouseOver ) );

        // define gradient and polygon for drawing arrow
        const QPolygonF arrow = genericArrow( orientation, ArrowNormal );

        const qreal penThickness = 1.6;
        const qreal offset( qMin( penThickness, qreal( 1.0 ) ) );

        QColor color;
        const QToolButton* toolButton( qobject_cast<const QToolButton*>( widget ) );
        if( toolButton && toolButton->arrowType() != Qt::NoArrow )
        {

            // set color properly
            color = (toolButton->autoRaise() ? palette.color( QPalette::WindowText ):palette.color( QPalette::ButtonText ) );

        } else if( mouseOver ) {

            color = _helper->viewHoverBrush().brush( palette ).color();

        } else {

            color = palette.color( QPalette::WindowText );

        }

        painter->translate( QRectF( rect ).center() );
        painter->setRenderHint( QPainter::Antialiasing );

        painter->translate( 0,offset );
        const QColor background = palette.color( QPalette::Window );
        painter->setPen( QPen( _helper->calcLightColor( background ), penThickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
        painter->drawPolyline( arrow );
        painter->translate( 0,-offset );

        painter->setPen( QPen( _helper->decoColor( background, color ) , penThickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
        painter->drawPolyline( arrow );

        return true;
    }

    //___________________________________________________________________________________
    bool Style::drawIndicatorHeaderArrowPrimitive( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {
        const QStyleOptionHeader *headerOption = qstyleoption_cast<const QStyleOptionHeader *>( option );
        const State& state( option->state );

        // arrow orientation
        ArrowOrientation orientation( ArrowNone );
        if( state & State_UpArrow || ( headerOption && headerOption->sortIndicator==QStyleOptionHeader::SortUp ) ) orientation = ArrowUp;
        else if( state & State_DownArrow || ( headerOption && headerOption->sortIndicator==QStyleOptionHeader::SortDown ) ) orientation = ArrowDown;
        if( orientation == ArrowNone ) return true;

        // invert arrows if requested by (hidden) options
        if( StyleConfigData::viewInvertSortIndicator() ) orientation = (orientation == ArrowUp) ? ArrowDown:ArrowUp;

        // flags, rect and palette
        const QRect& rect( option->rect );
        const QPalette& palette( option->palette );
        const bool enabled( state & State_Enabled );
        const bool mouseOver( enabled && ( state & State_MouseOver ) );

        _animations->headerViewEngine().updateState( widget, rect.topLeft(), mouseOver );
        const bool animated( enabled && _animations->headerViewEngine().isAnimated( widget, rect.topLeft() ) );

        // define gradient and polygon for drawing arrow
        const QPolygonF arrow = genericArrow( orientation, ArrowNormal );
        QColor color = palette.color( QPalette::WindowText );
        const QColor background = palette.color( QPalette::Window );
        const QColor highlight( _helper->viewHoverBrush().brush( palette ).color() );
        const qreal penThickness = 1.6;
        const qreal offset( qMin( penThickness, qreal( 1.0 ) ) );

        if( animated )
        {

            const qreal opacity( _animations->headerViewEngine().opacity( widget, rect.topLeft() ) );
            color = KColorUtils::mix( color, highlight, opacity );

        } else if( mouseOver ) color = highlight;

        painter->translate( QRectF(rect).center() );
        painter->translate( 0, 1 );
        painter->setRenderHint( QPainter::Antialiasing );

        painter->translate( 0,offset );
        painter->setPen( QPen( _helper->calcLightColor( background ), penThickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
        painter->drawPolyline( arrow );
        painter->translate( 0,-offset );

        painter->setPen( QPen( _helper->decoColor( background, color ) , penThickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
        painter->drawPolyline( arrow );

        return true;
    }

    //______________________________________________________________
    bool Style::drawPanelButtonCommandPrimitive( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        // copy rect and palette
        const QRect& rect( option->rect );
        const QPalette& palette( option->palette );

        // store state
        const State& state( option->state );
        const bool enabled( state & State_Enabled );
        const bool mouseOver( enabled && ( state & State_MouseOver ) );
        const bool hasFocus( enabled && ( state & State_HasFocus ) );

        StyleOptions styleOptions = 0;
        if( state & ( State_On|State_Sunken ) ) styleOptions |= Sunken;
        if( state & State_HasFocus ) styleOptions |= Focus;
        if( enabled && ( state & State_MouseOver ) ) styleOptions |= Hover;

        // update animation state
        _animations->widgetStateEngine().updateState( widget, AnimationHover, mouseOver );
        _animations->widgetStateEngine().updateState( widget, AnimationFocus, hasFocus && !mouseOver );

        // store animation state
        const bool hoverAnimated( _animations->widgetStateEngine().isAnimated( widget, AnimationHover ) );
        const bool focusAnimated( _animations->widgetStateEngine().isAnimated( widget, AnimationFocus ) );
        const qreal hoverOpacity( _animations->widgetStateEngine().opacity( widget, AnimationHover ) );
        const qreal focusOpacity( _animations->widgetStateEngine().opacity( widget, AnimationFocus ) );

        // decide if widget must be rendered flat.
        /*
        The decision is made depending on
        - whether the "flat" flag is set in the option
        - whether the widget is hight enough to render both icons and normal margins
        Note: in principle one should also check for the button text height
        */

        const QStyleOptionButton* buttonOption( qstyleoption_cast< const QStyleOptionButton* >( option ) );
        bool flat = ( buttonOption && (
            buttonOption->features.testFlag( QStyleOptionButton::Flat ) ||
            ( ( !buttonOption->icon.isNull() ) && sizeFromContents( CT_PushButton, option, buttonOption->iconSize, widget ).height() > rect.height() ) ) );

        if( flat )
        {

            if( !( styleOptions & Sunken ) )
            {
                // hover rect
                if( enabled && hoverAnimated )
                {

                    QColor glow( _helper->alphaColor( _helper->viewFocusBrush().brush( QPalette::Active ).color(), hoverOpacity ) );
                    _helper->slitFocused( glow )->render( rect, painter );

                } else if( mouseOver ) {

                    _helper->slitFocused( _helper->viewFocusBrush().brush( QPalette::Active ).color() )->render( rect, painter );

                }

            } else {

                HoleOptions options( 0 );
                if( mouseOver ) options |= HoleHover;

                // flat pressed-down buttons do not get focus effect,
                // consistently with tool buttons
                if( enabled && hoverAnimated )
                {

                    _helper->renderHole( painter, palette.color( QPalette::Window ), rect, options, hoverOpacity, AnimationHover, TileSet::Ring );

                } else {

                    _helper->renderHole( painter, palette.color( QPalette::Window ), rect, options );

                }

            }

        } else {

            // match color to the window background
            QColor buttonColor( _helper->backgroundColor( palette.color( QPalette::Button ), widget, rect.center() ) );

            // merge button color with highlight in case of default button
            if( enabled && buttonOption && (buttonOption->features&QStyleOptionButton::DefaultButton) )
            {
                const QColor tintColor( _helper->calcLightColor( buttonColor ) );
                buttonColor = KColorUtils::mix( buttonColor, tintColor, 0.5 );
            }

            if( enabled && hoverAnimated && !( styleOptions & Sunken ) )
            {

                renderButtonSlab( painter, rect, buttonColor, styleOptions, hoverOpacity, AnimationHover, TileSet::Ring );

            } else if( enabled && !mouseOver && focusAnimated && !( styleOptions & Sunken ) ) {

                renderButtonSlab( painter, rect, buttonColor, styleOptions, focusOpacity, AnimationFocus, TileSet::Ring );

            } else {

                renderButtonSlab( painter, rect, buttonColor, styleOptions );

            }

        }

        return true;

    }

    //______________________________________________________________
    bool Style::drawTabBarPanelButtonToolPrimitive( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        const QPalette& palette( option->palette );
        QRect rect( option->rect );

        // adjust rect depending on shape
        const QTabBar* tabBar( static_cast<const QTabBar*>( widget->parent() ) );
        switch( tabBar->shape() )
        {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
            rect.adjust( 0, 0, 0, -6 );
            break;

            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
            rect.adjust( 0, 6, 0, 0 );
            break;

            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
            rect.adjust( 0, 0, -6, 0 );
            break;

            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
            rect.adjust( 6, 0, 0, 0 );
            break;

            default: break;

        }

        const QPalette local( widget->parentWidget() ? widget->parentWidget()->palette() : palette );

        // check whether parent has autofill background flag
        const QWidget* parent = _helper->checkAutoFillBackground( widget );
        if( parent && !qobject_cast<const QDockWidget*>( parent ) ) painter->fillRect( rect, parent->palette().color( parent->backgroundRole() ) );
        else _helper->renderWindowBackground( painter, rect, widget, local );

        return true;
    }

    //______________________________________________________________
    bool Style::drawPanelButtonToolPrimitive( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        // copy rect and palette
        const QRect& rect( option->rect );
        const QPalette& palette( option->palette );

        // store state
        const State& state( option->state );
        const bool enabled( state & State_Enabled );
        const bool mouseOver( enabled && ( state & State_MouseOver ) );
        const bool hasFocus( enabled && ( state & State_HasFocus ) );
        const bool reverseLayout( option->direction == Qt::RightToLeft );
        const bool autoRaised( state & State_AutoRaise );

        // check whether toolbutton is in toolbar
        const bool isInToolBar( widget && qobject_cast<const QToolBar*>( widget->parent() ) );

        // toolbar engine
        const bool toolBarAnimated( isInToolBar && widget && ( _animations->toolBarEngine().isAnimated( widget->parentWidget() ) || _animations->toolBarEngine().isFollowMouseAnimated( widget->parentWidget() ) ) );
        const QRect animatedRect( ( isInToolBar && widget ) ? _animations->toolBarEngine().animatedRect( widget->parentWidget() ):QRect() );
        const QRect childRect( ( widget && widget->parentWidget() ) ? rect.translated( widget->mapToParent( QPoint( 0,0 ) ) ):QRect() );
        const QRect currentRect(  widget ? _animations->toolBarEngine().currentRect( widget->parentWidget() ):QRect() );
        const bool current( isInToolBar && widget && widget->parentWidget() && currentRect.intersects( rect.translated( widget->mapToParent( QPoint( 0,0 ) ) ) ) );
        const bool toolBarTimerActive( isInToolBar && widget && _animations->toolBarEngine().isTimerActive( widget->parentWidget() ) );
        const qreal toolBarOpacity( ( isInToolBar && widget ) ? _animations->toolBarEngine().opacity( widget->parentWidget() ):0 );

        // toolbutton engine
        if( isInToolBar && !toolBarAnimated )
        {

            _animations->widgetStateEngine().updateState( widget, AnimationHover, mouseOver );

        } else {

            // mouseOver has precedence over focus
            _animations->widgetStateEngine().updateState( widget, AnimationHover, mouseOver );
            _animations->widgetStateEngine().updateState( widget, AnimationFocus, hasFocus && !mouseOver );

        }

        bool hoverAnimated( _animations->widgetStateEngine().isAnimated( widget, AnimationHover ) );
        bool focusAnimated( _animations->widgetStateEngine().isAnimated( widget, AnimationFocus ) );

        qreal hoverOpacity( _animations->widgetStateEngine().opacity( widget, AnimationHover ) );
        qreal focusOpacity( _animations->widgetStateEngine().opacity( widget, AnimationFocus ) );

        // non autoraised tool buttons get same slab as regular buttons
        if( widget && !autoRaised )
        {

            StyleOptions styleOptions;

            // "normal" parent, and non "autoraised" ( that is: always raised ) buttons
            if( state & ( State_On|State_Sunken ) ) styleOptions |= Sunken;
            if( state & State_HasFocus ) styleOptions |= Focus;
            if( enabled && ( state & State_MouseOver ) ) styleOptions |= Hover;

            TileSet::Tiles tiles( TileSet::Ring );

            // adjust tiles and rect in case of menubutton
            const QToolButton* toolButton = qobject_cast<const QToolButton*>( widget );
            const bool hasPopupMenu( toolButton && toolButton->popupMode() == QToolButton::MenuButtonPopup );

            if( hasPopupMenu )
            {
                if( reverseLayout ) tiles &= ~TileSet::Left;
                else tiles &= ~TileSet::Right;
            }

            // adjust opacity and animation mode
            qreal opacity( -1 );
            AnimationMode mode( AnimationNone );
            if( enabled && hoverAnimated )
            {
                opacity = hoverOpacity;
                mode = AnimationHover;

            } else if( enabled && !hasFocus && focusAnimated ) {

                opacity = focusOpacity;
                mode = AnimationFocus;

            }

            // match button color to window background
            const QColor buttonColor( _helper->backgroundColor( palette.color( QPalette::Button ), widget, rect.center() ) );

            // render slab
            renderButtonSlab( painter, rect, buttonColor, styleOptions, opacity, mode, tiles );

            return true;

        }

        // normal ( auto-raised ) toolbuttons
        if( state & ( State_Sunken|State_On ) )
        {

            {

                // fill hole
                qreal opacity = 1.0;
                const qreal bias = 0.75;
                if( enabled && hoverAnimated )
                {

                    opacity = 1.0 - bias*hoverOpacity;

                } else if( toolBarAnimated && enabled && animatedRect.isNull() && current  ) {

                    opacity = 1.0 - bias*toolBarOpacity;

                } else if( enabled && (( toolBarTimerActive && current ) || mouseOver ) ) {

                    opacity = 1.0 - bias;

                }

                if( opacity > 0 )
                {
                    QColor color( _helper->backgroundColor( _helper->calcMidColor( palette.color( QPalette::Window ) ), widget, rect.center() ) );
                    color = _helper->alphaColor( color, opacity );
                    painter->save();
                    painter->setRenderHint( QPainter::Antialiasing );
                    painter->setPen( Qt::NoPen );
                    painter->setBrush( color );
                    painter->drawRoundedRect( rect.adjusted( 1, 1, -1, -1 ), 3.5, 3.5 );
                    painter->restore();
                }

            }


            HoleOptions options( HoleContrast );
            if( hasFocus && enabled ) options |= HoleFocus;
            if( mouseOver && enabled ) options |= HoleHover;

            if( enabled && hoverAnimated )
            {

                _helper->renderHole( painter, palette.color( QPalette::Window ), rect, options, hoverOpacity, AnimationHover, TileSet::Ring );

            } else if( toolBarAnimated ) {

                if( enabled && animatedRect.isNull() && current  )
                {

                    _helper->renderHole( painter, palette.color( QPalette::Window ), rect, options, toolBarOpacity, AnimationHover, TileSet::Ring );

                } else {

                    _helper->renderHole( painter, palette.color( QPalette::Window ), rect, HoleContrast );

                }

            } else if( toolBarTimerActive && current ) {

                _helper->renderHole( painter, palette.color( QPalette::Window ), rect, options | HoleHover );

            } else {

                _helper->renderHole( painter, palette.color( QPalette::Window ), rect, options );

            }

        } else {

            if( enabled && hoverAnimated )
            {

                QColor glow( _helper->alphaColor( _helper->viewFocusBrush().brush( QPalette::Active ).color(), hoverOpacity ) );
                _helper->slitFocused( glow )->render( rect, painter );

            } else if( toolBarAnimated ) {

                if( enabled && animatedRect.isNull() && current )
                {
                    QColor glow( _helper->alphaColor( _helper->viewFocusBrush().brush( QPalette::Active ).color(), toolBarOpacity ) );
                    _helper->slitFocused( glow )->render( rect, painter );
                }

            } else if( hasFocus || mouseOver || ( toolBarTimerActive && current ) ) {

                _helper->slitFocused( _helper->viewFocusBrush().brush( QPalette::Active ).color() )->render( rect, painter );

            }

        }

        return true;
    }

    //___________________________________________________________________________________
    bool Style::drawPanelScrollAreaCornerPrimitive( const QStyleOption*, QPainter*, const QWidget* widget ) const
    {
        // disable painting of PE_PanelScrollAreaCorner
        // the default implementation fills the rect with the window background color
        // which does not work for windows that have gradients.
        // unfortunately, this does not work when scrollbars are children of QWebView,
        // in which case, false is returned, in order to fall back to the parent style implementation
        return !( widget && widget->inherits( "QWebView" ) );
    }

    //___________________________________________________________________________________
    bool Style::drawPanelMenuPrimitive( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        // do nothing if menu is embedded in another widget
        // this corresponds to having a transparent background
        if( widget && !widget->isWindow() ) return true;

        const QStyleOptionMenuItem* menuItemOption( qstyleoption_cast<const QStyleOptionMenuItem*>( option ) );
        if( !( menuItemOption && widget ) ) return true;
        const QRect& r = menuItemOption->rect;
        const QColor color = menuItemOption->palette.color( widget->window()->backgroundRole() );

        const bool hasAlpha( _helper->hasAlphaChannel( widget ) );
        if( hasAlpha )
        {

            painter->setCompositionMode( QPainter::CompositionMode_Source );
            TileSet *tileSet( _helper->roundCorner( color ) );
            tileSet->render( r, painter );

            painter->setCompositionMode( QPainter::CompositionMode_SourceOver );
            painter->setClipRegion( _helper->roundedMask( r.adjusted( 1, 1, -1, -1 ) ), Qt::IntersectClip );

        }

        _helper->renderMenuBackground( painter, r, widget, menuItemOption->palette );

        if( hasAlpha ) painter->setClipping( false );
        _helper->drawFloatFrame( painter, r, color, !hasAlpha );

        return true;

    }

    //___________________________________________________________________________________
    bool Style::drawPanelTipLabelPrimitive( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        // force registration of widget
        if( widget && widget->window() )
        { _shadowHelper->registerWidget( widget->window(), true ); }

        const QRect& rect( option->rect );
        const QColor color( option->palette.brush( QPalette::ToolTipBase ).color() );
        QColor topColor( _helper->backgroundTopColor( color ) );
        QColor bottomColor( _helper->backgroundBottomColor( color ) );

        // make tooltip semi transparents when possible
        // alpha is copied from "kdebase/apps/dolphin/tooltips/filemetadatatooltip.cpp"
        const bool hasAlpha( _helper->hasAlphaChannel( widget ) );
        if( hasAlpha && StyleConfigData::toolTipTransparent() )
        {
            if( widget && widget->window() )
            { _blurHelper->registerWidget( widget->window() ); }
            topColor.setAlpha( 220 );
            bottomColor.setAlpha( 220 );
        }

        QLinearGradient gradient( 0, rect.top(), 0, rect.bottom() );
        gradient.setColorAt( 0, topColor );
        gradient.setColorAt( 1, bottomColor );

        // contrast pixmap
        QLinearGradient gradient2( 0, rect.top(), 0, rect.bottom() );
        gradient2.setColorAt( 0.5, _helper->calcLightColor( bottomColor ) );
        gradient2.setColorAt( 0.9, bottomColor );

        painter->save();

        if( hasAlpha )
        {
            painter->setRenderHint( QPainter::Antialiasing );

            QRectF local( rect );
            local.adjust( 0.5, 0.5, -0.5, -0.5 );

            painter->setPen( Qt::NoPen );
            painter->setBrush( gradient );
            painter->drawRoundedRect( local, 4, 4 );

            painter->setBrush( Qt::NoBrush );
            painter->setPen( QPen( gradient2, 1.1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
            painter->drawRoundedRect( local, 3.5, 3.5 );

        } else {

            painter->setPen( Qt::NoPen );
            painter->setBrush( gradient );
            painter->drawRect( rect );

            painter->setBrush( Qt::NoBrush );
            painter->setPen( QPen( gradient2, 1.1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
            painter->drawRect( rect );

        }

        painter->restore();

        return true;

    }

    //___________________________________________________________________________________
    bool Style::drawPanelItemViewItemPrimitive( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        // cast option and check
        const QStyleOptionViewItemV4 *viewItemOption = qstyleoption_cast<const QStyleOptionViewItemV4*>( option );
        if( !viewItemOption ) return false;

        // try cast widget
        const QAbstractItemView *view = qobject_cast<const QAbstractItemView *>( widget );

        // store palette and rect
        const QPalette& palette( option->palette );
        QRect rect( option->rect );

        // store flags
        const State& state( option->state );
        const bool mouseOver( ( state & State_MouseOver ) && (!view || view->selectionMode() != QAbstractItemView::NoSelection) );
        const bool selected( state & State_Selected );
        const bool enabled( state & State_Enabled );
        const bool active( state & State_Active );
        const bool hasCustomBackground( viewItemOption->backgroundBrush.style() != Qt::NoBrush && !selected );
        const bool hasSolidBackground( !hasCustomBackground || viewItemOption->backgroundBrush.style() == Qt::SolidPattern );

        if( !mouseOver && !selected && !hasCustomBackground && !( viewItemOption->features & QStyleOptionViewItemV2::Alternate ) )
        { return true; }

        QPalette::ColorGroup colorGroup;
        if( enabled ) colorGroup = active ? QPalette::Normal : QPalette::Inactive;
        else colorGroup = QPalette::Disabled;

        QColor color;
        if( hasCustomBackground && hasSolidBackground ) color = viewItemOption->backgroundBrush.color();
        else color = palette.color( colorGroup, QPalette::Highlight );

        if( mouseOver && !hasCustomBackground )
        {
            if( !selected ) color.setAlphaF( 0.2 );
            else color = color.lighter( 110 );
        }

        if( viewItemOption && ( viewItemOption->features & QStyleOptionViewItemV2::Alternate ) )
        { painter->fillRect( option->rect, palette.brush( colorGroup, QPalette::AlternateBase ) ); }

        if( !mouseOver && !selected && !hasCustomBackground )
        { return true; }

        if( hasCustomBackground && !hasSolidBackground )
        {

            const QPointF oldBrushOrigin = painter->brushOrigin();
            painter->setBrushOrigin( viewItemOption->rect.topLeft() );
            painter->setBrush( viewItemOption->backgroundBrush );
            painter->setPen( Qt::NoPen );
            painter->drawRect( viewItemOption->rect );
            painter->setBrushOrigin( oldBrushOrigin );

        } else {

            // get selection tileset
            TileSet *tileSet( _helper->selection( color, rect.height(), hasCustomBackground ) );

            bool roundedLeft  = false;
            bool roundedRight = false;
            if( viewItemOption )
            {

                roundedLeft  = ( viewItemOption->viewItemPosition == QStyleOptionViewItemV4::Beginning );
                roundedRight = ( viewItemOption->viewItemPosition == QStyleOptionViewItemV4::End );
                if( viewItemOption->viewItemPosition == QStyleOptionViewItemV4::OnlyOne ||
                    viewItemOption->viewItemPosition == QStyleOptionViewItemV4::Invalid ||
                    ( view && view->selectionBehavior() != QAbstractItemView::SelectRows ) )
                {
                    roundedLeft  = true;
                    roundedRight = true;
                }

            }

            const bool reverseLayout( option->direction == Qt::RightToLeft );

            // define tiles
            TileSet::Tiles tiles( TileSet::Center );
            if( !reverseLayout ? roundedLeft : roundedRight ) tiles |= TileSet::Left;
            if( !reverseLayout ? roundedRight : roundedLeft ) tiles |= TileSet::Right;

            // adjust rect and render
            rect = tileSet->adjust( rect, tiles );
            if( rect.isValid() ) tileSet->render( rect, painter, tiles );

        }

        return true;
    }

    //___________________________________________________________________________________
    bool Style::drawIndicatorMenuCheckMarkPrimitive( const QStyleOption *option, QPainter *painter, const QWidget * ) const
    {
        const QRect& rect( option->rect );
        const State& state( option->state );
        const QPalette& palette( option->palette );
        const bool enabled( state & State_Enabled );

        StyleOptions styleOptions( NoFill );
        if( !enabled ) styleOptions |= Disabled;

        renderCheckBox( painter, rect, palette, styleOptions, CheckOn );
        return true;

    }

    //___________________________________________________________________________________
    bool Style::drawIndicatorBranchPrimitive( const QStyleOption* option, QPainter* painter, const QWidget* ) const
    {

        // copy rect and palette
        const QRect& rect( option->rect );
        const QPalette& palette( option->palette );

        // state
        const State& state( option->state );
        const bool reverseLayout( option->direction == Qt::RightToLeft );

        //draw expander
        int expanderAdjust = 0;
        if( state & State_Children )
        {

            int sizeLimit = qMin( rect.width(), rect.height() );
            const bool expanderOpen( state & State_Open );

            // make sure size limit is odd
            expanderAdjust = sizeLimit/2 + 1;

            // flags
            const bool enabled( state & State_Enabled );
            const bool mouseOver( enabled && ( state & State_MouseOver ) );

            // color
            const QColor expanderColor( mouseOver ? _helper->viewHoverBrush().brush( palette ).color():palette.color( QPalette::Text ) );

            // get arrow size from option
            ArrowSize size = ArrowSmall;
            qreal penThickness( 1.2 );
            switch( StyleConfigData::viewTriangularExpanderSize() )
            {
                case StyleConfigData::TE_TINY:
                size = ArrowTiny;
                break;

                default:
                case StyleConfigData::TE_SMALL:
                size = ArrowSmall;
                break;

                case StyleConfigData::TE_NORMAL:
                penThickness = 1.6;
                size = ArrowNormal;
                break;

            }

            // get arrows polygon
            QPolygonF arrow;
            if( expanderOpen ) arrow = genericArrow( ArrowDown, size );
            else arrow = genericArrow( reverseLayout ? ArrowLeft:ArrowRight, size );

            // render
            painter->save();
            painter->translate( QRectF( rect ).center() );
            painter->setPen( QPen( expanderColor, penThickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
            painter->setRenderHint( QPainter::Antialiasing );
            painter->drawPolyline( arrow );
            painter->restore();

        }


        // tree branches
        if( !StyleConfigData::viewDrawTreeBranchLines() ) return true;

        const QPoint center( rect.center() );
        const QColor lineColor( KColorUtils::mix( palette.color( QPalette::Text ), palette.color( QPalette::Background ), 0.8 ) );
        painter->setRenderHint( QPainter::Antialiasing, false );
        painter->setPen( lineColor );
        if( state & ( State_Item | State_Children | State_Sibling ) )
        {
            const QLine line( QPoint( center.x(), rect.top() ), QPoint( center.x(), center.y() - expanderAdjust ) );
            painter->drawLine( line );
        }

        //The right/left ( depending on dir ) line gets drawn if we have an item
        if( state & State_Item )
        {
            const QLine line = reverseLayout ?
                QLine( QPoint( rect.left(), center.y() ), QPoint( center.x() - expanderAdjust, center.y() ) ):
                QLine( QPoint( center.x() + expanderAdjust, center.y() ), QPoint( rect.right(), center.y() ) );
            painter->drawLine( line );

        }

        //The bottom if we have a sibling
        if( state & State_Sibling )
        {
            const QLine line( QPoint( center.x(), center.y() + expanderAdjust ), QPoint( center.x(), rect.bottom() ) );
            painter->drawLine( line );
        }

        return true;
    }

    //___________________________________________________________________________________
    bool Style::drawIndicatorButtonDropDownPrimitive( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        // cast option and check
        const QStyleOptionToolButton* toolButtonOption( qstyleoption_cast<const QStyleOptionToolButton*>( option ) );
        if( !toolButtonOption ) return true;

        // copy palette and rect
        const QPalette& palette( option->palette );
        const QRect& rect( option->rect );

        // store state
        const State& state( option->state );
        const bool autoRaise( state & State_AutoRaise );

        // do nothing for autoraise buttons
        if( autoRaise || !(toolButtonOption->subControls & SC_ToolButtonMenu) ) return true;

        // store state
        const bool enabled( state & State_Enabled );
        const bool hasFocus( enabled && ( state & State_HasFocus ) );
        const bool mouseOver( enabled && ( state & State_MouseOver ) );
        const bool sunken( enabled && ( state & State_Sunken ) );
        const bool reverseLayout( option->direction == Qt::RightToLeft );

        // match button color to window background
        const QColor highlight( _helper->viewHoverBrush().brush( palette ).color() );
        const QColor background( _helper->backgroundColor( palette.color( QPalette::Button ), widget, rect.center() ) );
        StyleOptions styleOptions = 0;

        // handle animations
        // mouseOver has precedence over focus
        _animations->widgetStateEngine().updateState( widget, AnimationHover, mouseOver );
        _animations->widgetStateEngine().updateState( widget, AnimationFocus, hasFocus && !mouseOver );

        const bool hoverAnimated( _animations->widgetStateEngine().isAnimated( widget, AnimationHover ) );
        const bool focusAnimated( _animations->widgetStateEngine().isAnimated( widget, AnimationFocus ) );

        const qreal hoverOpacity( _animations->widgetStateEngine().opacity( widget, AnimationHover ) );
        const qreal focusOpacity( _animations->widgetStateEngine().opacity( widget, AnimationFocus ) );

        if( hasFocus ) styleOptions |= Focus;
        if( mouseOver ) styleOptions |= Hover;

        // adjust opacity and animation mode
        qreal opacity( -1 );
        AnimationMode mode( AnimationNone );
        if( enabled && hoverAnimated )
        {

            opacity = hoverOpacity;
            mode = AnimationHover;

        } else if( enabled && !hasFocus && focusAnimated ) {

            opacity = focusOpacity;
            mode = AnimationFocus;

        }

        // paint frame
        TileSet::Tiles tiles( TileSet::Ring );
        if( state & ( State_On|State_Sunken ) ) styleOptions |= Sunken;
        if( reverseLayout ) tiles &= ~TileSet::Right;
        else tiles &= ~TileSet::Left;

        painter->setClipRect( rect, Qt::IntersectClip );
        renderButtonSlab( painter, rect, background, styleOptions, opacity, mode, tiles );

        // draw separating vertical line
        const QColor color( palette.color( QPalette::Button ) );
        const QColor light =_helper->alphaColor( _helper->calcLightColor( color ), 0.6 );
        QColor dark = _helper->calcDarkColor( color );
        dark.setAlpha( 200 );

        const int top( rect.top()+ (sunken ? 3:2) );
        const int bottom( rect.bottom()-4 );

        painter->setPen( QPen( light,1 ) );

        if( reverseLayout )
        {

            painter->drawLine( rect.right()+1, top+1, rect.right()+1, bottom );
            painter->drawLine( rect.right()-1, top+2, rect.right()-1, bottom );
            painter->setPen( dark );
            painter->drawLine( rect.right(), top, rect.right(), bottom );

        } else {

            painter->drawLine( rect.left()-1, top+1, rect.left()-1, bottom-1 );
            painter->drawLine( rect.left()+1, top+1, rect.left()+1, bottom-1 );
            painter->setPen( dark );
            painter->drawLine( rect.left(), top, rect.left(), bottom );

        }

        return true;

    }

    //___________________________________________________________________________________
    bool Style::drawIndicatorCheckBoxPrimitive( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        // get rect
        const QRect& rect( option->rect );
        const State& state( option->state );
        const bool enabled( state & State_Enabled );
        const bool mouseOver( enabled && ( state & State_MouseOver ) );
        const bool hasFocus( state & State_HasFocus );

        StyleOptions styleOptions( 0 );
        if( !enabled ) styleOptions |= Disabled;
        if( mouseOver ) styleOptions |= Hover;
        if( hasFocus ) styleOptions |= Focus;

        // get checkbox state
        CheckBoxState checkBoxState;
        if( state & State_NoChange ) checkBoxState = CheckTriState;
        else if( state & State_Sunken ) checkBoxState = CheckSunken;
        else if( state & State_On ) checkBoxState = CheckOn;
        else checkBoxState = CheckOff;

        // match button color to window background
        QPalette palette( option->palette );
        palette.setColor(
            QPalette::Button,
            _helper->backgroundColor(
            palette.color( QPalette::Button ), widget, rect.center() ) );

        // mouseOver has precedence over focus
        _animations->widgetStateEngine().updateState( widget, AnimationHover, mouseOver );
        _animations->widgetStateEngine().updateState( widget, AnimationFocus, hasFocus&&!mouseOver );

        if( enabled && _animations->widgetStateEngine().isAnimated( widget, AnimationHover ) )
        {

            const qreal opacity( _animations->widgetStateEngine().opacity( widget, AnimationHover ) );
            renderCheckBox( painter, rect, palette, styleOptions, checkBoxState, opacity, AnimationHover );

        } else if( enabled && !hasFocus && _animations->widgetStateEngine().isAnimated( widget, AnimationFocus ) ) {

            const qreal opacity( _animations->widgetStateEngine().opacity( widget, AnimationFocus ) );
            renderCheckBox( painter, rect, palette, styleOptions, checkBoxState, opacity, AnimationFocus );

        } else renderCheckBox( painter, rect, palette, styleOptions, checkBoxState );

        return true;
    }

    //___________________________________________________________________________________
    bool Style::drawIndicatorRadioButtonPrimitive( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        // copy rect
        const QRect& rect( option->rect );

        // store state
        const State& state( option->state );
        const bool enabled( state & State_Enabled );
        const bool mouseOver( enabled && ( state & State_MouseOver ) );
        const bool hasFocus( state & State_HasFocus );

        StyleOptions styleOptions( 0 );
        if( !enabled ) styleOptions |= Disabled;
        if( mouseOver ) styleOptions |= Hover;
        if( hasFocus ) styleOptions |= Focus;

        // match button color to window background
        QPalette palette( option->palette );
        palette.setColor( QPalette::Button, _helper->backgroundColor( palette.color( QPalette::Button ), widget, rect.center() ) );

        // mouseOver has precedence over focus
        _animations->widgetStateEngine().updateState( widget, AnimationHover, mouseOver );
        _animations->widgetStateEngine().updateState( widget, AnimationFocus, hasFocus && !mouseOver );

        CheckBoxState checkBoxState;
        if( state & State_Sunken ) checkBoxState = CheckSunken;
        else if( state & State_On ) checkBoxState = CheckOn;
        else checkBoxState = CheckOff;

        if( enabled && _animations->widgetStateEngine().isAnimated( widget, AnimationHover ) )
        {

            const qreal opacity( _animations->widgetStateEngine().opacity( widget, AnimationHover ) );
            renderRadioButton( painter, rect, palette, styleOptions, checkBoxState, opacity, AnimationHover );

        } else if( enabled && _animations->widgetStateEngine().isAnimated( widget, AnimationFocus ) ) {

            const qreal opacity( _animations->widgetStateEngine().opacity( widget, AnimationFocus ) );
            renderRadioButton( painter, rect, palette, styleOptions, checkBoxState, opacity, AnimationFocus );

        } else renderRadioButton( painter, rect, palette, styleOptions, checkBoxState );

        return true;

    }


    //___________________________________________________________________________________
    bool Style::drawIndicatorTabTearPrimitive( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        const QStyleOptionTab* tabOption( qstyleoption_cast<const QStyleOptionTab*>( option ) );
        if( !tabOption ) return true;

        const QRect& r( option->rect );
        const QPalette& palette( option->palette );
        const bool reverseLayout( option->direction == Qt::RightToLeft );

        // in fact with current version of Qt ( 4.6.0 ) the cast fails and document mode is always false
        // this will hopefully be fixed in later versions
        const QStyleOptionTabV3* tabOptionV3( qstyleoption_cast<const QStyleOptionTabV3*>( option ) );
        bool documentMode( tabOptionV3 ? tabOptionV3->documentMode : false );

        const QTabWidget *tabWidget = ( widget && widget->parentWidget() ) ? qobject_cast<const QTabWidget *>( widget->parentWidget() ) : NULL;
        documentMode |= ( tabWidget ? tabWidget->documentMode() : true );

        QRect gradientRect( r );
        switch( tabOption->shape )
        {

            case QTabBar::TriangularNorth:
            case QTabBar::RoundedNorth:
            gradientRect.adjust( 0, 0, 0, -5 );
            if( !reverseLayout ) gradientRect.translate( 0,0 );
            break;

            case QTabBar::TriangularSouth:
            case QTabBar::RoundedSouth:
            gradientRect.adjust( 0, 5, 0, 0 );
            if( !reverseLayout ) gradientRect.translate( 0,0 );
            break;

            case QTabBar::TriangularWest:
            case QTabBar::RoundedWest:
            gradientRect.adjust( 0, 0, -5, 0 );
            gradientRect.translate( 0,0 );
            break;

            case QTabBar::TriangularEast:
            case QTabBar::RoundedEast:
            gradientRect.adjust( 5, 0, 0, 0 );
            gradientRect.translate( 0,0 );
            break;

            default: return true;
        }

        // fade tabbar
        QPixmap pixmap( gradientRect.size() );
        {
            pixmap.fill( Qt::transparent );
            QPainter painter( &pixmap );

            const bool verticalTabs( isVerticalTab( tabOption ) );

            int width = 0;
            int height = 0;
            if( verticalTabs ) height = gradientRect.height();
            else width = gradientRect.width();

            QLinearGradient grad;
            if( reverseLayout && !verticalTabs ) grad = QLinearGradient( 0, 0, width, height );
            else grad = QLinearGradient( width, height, 0, 0 );

            grad.setColorAt( 0, Qt::transparent );
            grad.setColorAt( 0.6, Qt::black );

            if( widget )
            { _helper->renderWindowBackground( &painter, pixmap.rect(), widget, palette ); }
            painter.setCompositionMode( QPainter::CompositionMode_DestinationAtop );
            painter.fillRect( pixmap.rect(), QBrush( grad ) );
            painter.end();
        }

        // draw pixmap
        painter->drawPixmap( gradientRect.topLeft() + QPoint( 0,-1 ), pixmap );

        return true;
    }

    //___________________________________________________________________________________
    bool Style::drawIndicatorToolBarHandlePrimitive( const QStyleOption* option, QPainter* painter, const QWidget* ) const
    {
        const State& state( option->state );
        const bool horizontal( state & State_Horizontal );
        const QRect& r( option->rect );
        const QPalette& palette( option->palette );
        int counter( 1 );

        if( horizontal )
        {

            const int center( r.left()+r.width()/2 );
            for( int j = r.top()+2; j <= r.bottom()-3; j+=3, ++counter )
            {
                if( counter%2 == 0 ) _helper->renderDot( painter, QPoint( center+1, j ), palette.color( QPalette::Background ) );
                else _helper->renderDot( painter, QPoint( center-2, j ), palette.color( QPalette::Background ) );
            }

        } else {

            const int center( r.top()+r.height()/2 );
            for( int j = r.left()+2; j <= r.right()-3; j+=3, ++counter )
            {
                if( counter%2 == 0 ) _helper->renderDot( painter, QPoint( j, center+1 ), palette.color( QPalette::Background ) );
                else _helper->renderDot( painter, QPoint( j, center-2 ), palette.color( QPalette::Background ) );
            }
        }

        return true;

    }

    //___________________________________________________________________________________
    bool Style::drawIndicatorToolBarSeparatorPrimitive( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {
        /*
        do nothing if disabled from options
        also need to check if widget is a combobox, because of Qt hack using 'toolbar' separator primitive
        for rendering separators in comboboxes
        */
        if( !( StyleConfigData::toolBarDrawItemSeparator() || qobject_cast<const QComboBox*>( widget ) ) )
        { return true; }

        // store rect and palette
        const QRect& rect( option->rect );
        const QPalette& palette( option->palette );

        // store state
        const State& state( option->state );
        const bool separatorIsVertical( state & State_Horizontal );

        // define color and render
        const QColor color( palette.color( QPalette::Window ) );
        if( separatorIsVertical ) _helper->drawSeparator( painter, rect, color, Qt::Vertical );
        else _helper->drawSeparator( painter, rect, color, Qt::Horizontal );

        return true;
    }

    //___________________________________________________________________________________
    bool Style::drawWidgetPrimitive( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        // check widget and attributes
        if( !widget || !widget->testAttribute( Qt::WA_StyledBackground ) || widget->testAttribute( Qt::WA_NoSystemBackground ) ) return false;
        if( !( ( widget->windowFlags() & Qt::WindowType_Mask ) & ( Qt::Window|Qt::Dialog ) ) ) return false;
        if( !widget->isWindow() ) return false;

        // normal "window" background
        const QPalette& palette( option->palette );

        // do not render background if palette brush has a texture (pixmap or image)
        const QBrush brush( palette.brush( widget->backgroundRole() ) );
        if( !( brush.texture().isNull() && brush.textureImage().isNull() ) )
        { return false; }

        _helper->renderWindowBackground( painter, option->rect, widget, palette );
        return true;

    }

    //___________________________________________________________________________________
    bool Style::drawDockWidgetTitleControl( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        // cast option and check
        const QStyleOptionDockWidget* dockWidgetOption = qstyleoption_cast<const QStyleOptionDockWidget*>( option );
        if( !dockWidgetOption ) return true;

        const QPalette& palette( option->palette );
        const State& state( option->state );
        const bool enabled( state & State_Enabled );
        const bool reverseLayout( option->direction == Qt::RightToLeft );

        // cast to v2 to check vertical bar
        const QStyleOptionDockWidgetV2 *v2 = qstyleoption_cast<const QStyleOptionDockWidgetV2*>( option );
        const bool verticalTitleBar( v2 ? v2->verticalTitleBar : false );

        const QRect buttonRect( subElementRect( dockWidgetOption->floatable ? SE_DockWidgetFloatButton : SE_DockWidgetCloseButton, option, widget ) );

        // get rectangle and adjust to properly accounts for buttons
        QRect r( insideMargin( dockWidgetOption->rect, Metrics::Frame_FrameWidth ) );
        if( verticalTitleBar )
        {

            if( buttonRect.isValid() ) r.setTop( buttonRect.bottom()+1 );

        } else if( reverseLayout ) {

            if( buttonRect.isValid() ) r.setLeft( buttonRect.right()+1 );
            r.adjust( 0,0,-4,0 );

        } else {

            if( buttonRect.isValid() ) r.setRight( buttonRect.left()-1 );
            r.adjust( 4,0,0,0 );

        }

        QString title( dockWidgetOption->title );
        QString tmpTitle = title;

        // this is quite suboptimal
        // and does not really work
        if( tmpTitle.contains( QLatin1Char( '&' ) ) )
        {
            int pos = tmpTitle.indexOf( QLatin1Char( '&' ) );
            if( !( tmpTitle.size()-1 > pos && tmpTitle.at( pos+1 ) == QLatin1Char( '&' ) ) ) tmpTitle.remove( pos, 1 );

        }

        int tw = dockWidgetOption->fontMetrics.width( tmpTitle );
        int width = verticalTitleBar ? r.height() : r.width();
        if( width < tw ) title = dockWidgetOption->fontMetrics.elidedText( title, Qt::ElideRight, width, Qt::TextShowMnemonic );

        if( verticalTitleBar )
        {

            QSize s = r.size();
            s.transpose();
            r.setSize( s );

            painter->save();
            painter->translate( r.left(), r.top() + r.width() );
            painter->rotate( -90 );
            painter->translate( -r.left(), -r.top() );
            drawItemText( painter, r, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic, palette, enabled, title, QPalette::WindowText );
            painter->restore();


        } else {

            drawItemText( painter, r, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic, palette, enabled, title, QPalette::WindowText );

        }

        return true;


    }

    //___________________________________________________________________________________
    bool Style::drawHeaderEmptyAreaControl( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        // use the same background as in drawHeaderPrimitive
        QPalette palette( option->palette );

        if( widget && _animations->widgetEnabilityEngine().isAnimated( widget, AnimationEnable ) )
        { palette = _helper->mergePalettes( palette, _animations->widgetEnabilityEngine().opacity( widget, AnimationEnable )  ); }

        const bool horizontal( option->state & QStyle::State_Horizontal );
        const bool reverseLayout( option->direction == Qt::RightToLeft );
        renderHeaderBackground( option->rect, palette, painter, widget, horizontal, reverseLayout );

        return true;

    }

    //___________________________________________________________________________________
    bool Style::drawHeaderSectionControl( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        const QRect& r( option->rect );
        const QPalette& palette( option->palette );

        const QStyleOptionHeader* headerOption( qstyleoption_cast<const QStyleOptionHeader *>( option ) );
        if( !headerOption ) return true;

        const bool horizontal( headerOption->orientation == Qt::Horizontal );
        const bool reverseLayout( option->direction == Qt::RightToLeft );
        const bool isFirst( horizontal && ( headerOption->position == QStyleOptionHeader::Beginning ) );
        const bool isCorner( widget && widget->inherits( "QTableCornerButton" ) );

        // corner header lines
        if( isCorner )
        {

            if( widget ) _helper->renderWindowBackground( painter, r, widget, palette );
            else painter->fillRect( r, palette.color( QPalette::Window ) );
            if( reverseLayout ) renderHeaderLines( r, palette, painter, TileSet::BottomLeft );
            else renderHeaderLines( r, palette, painter, TileSet::BottomRight );

        } else renderHeaderBackground( r, palette, painter, widget, horizontal, reverseLayout );

        // dots
        const QColor color( palette.color( QPalette::Window ) );
        if( horizontal )
        {

            if( headerOption->section != 0 || isFirst )
            {
                const int center( r.center().y() );
                const int pos( reverseLayout ? r.left()+1 : r.right()-1 );
                _helper->renderDot( painter, QPoint( pos, center-3 ), color );
                _helper->renderDot( painter, QPoint( pos, center ), color );
                _helper->renderDot( painter, QPoint( pos, center+3 ), color );
            }

        } else {

            const int center( r.center().x() );
            const int pos( r.bottom()-1 );
            _helper->renderDot( painter, QPoint( center-3, pos ), color );
            _helper->renderDot( painter, QPoint( center, pos ), color );
            _helper->renderDot( painter, QPoint( center+3, pos ), color );

        }

        return true;

    }

    //___________________________________________________________________________________
    bool Style::drawMenuBarItemControl( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        const QStyleOptionMenuItem* menuOption = qstyleoption_cast<const QStyleOptionMenuItem*>( option );
        if( !menuOption ) return true;

        const State& state( option->state );
        const bool enabled( state & State_Enabled );

        const QRect& r( option->rect );
        const QPalette& palette( option->palette );

        if( enabled )
        {
            const bool active( state & State_Selected );
            const bool animated( _animations->menuBarEngine().isAnimated( widget, r.topLeft() ) );
            const qreal opacity( _animations->menuBarEngine().opacity( widget, r.topLeft() ) );
            const QRect currentRect( _animations->menuBarEngine().currentRect( widget, r.topLeft() ) );
            const QRect animatedRect( _animations->menuBarEngine().animatedRect( widget ) );

            const bool intersected( animatedRect.intersects( r ) );
            const bool current( currentRect.contains( r.topLeft() ) );
            const bool timerIsActive( _animations->menuBarEngine().isTimerActive( widget ) );

            // do nothing in case of empty intersection between animated rect and current
            if( ( intersected || !animated || animatedRect.isNull() ) && ( active || animated || timerIsActive ) )
            {

                QColor color( _helper->calcMidColor( palette.color( QPalette::Window ) ) );
                if( StyleConfigData::menuHighlightMode() != StyleConfigData::MM_DARK )
                {

                    if( state & State_Sunken )
                    {

                        if( StyleConfigData::menuHighlightMode() == StyleConfigData::MM_STRONG ) color = palette.color( QPalette::Highlight );
                        else color = KColorUtils::mix( color, KColorUtils::tint( color, palette.color( QPalette::Highlight ), 0.6 ) );

                    } else {

                        if( StyleConfigData::menuHighlightMode() == StyleConfigData::MM_STRONG ) color = KColorUtils::tint( color, _helper->viewHoverBrush().brush( palette ).color() );
                        else color = KColorUtils::mix( color, KColorUtils::tint( color, _helper->viewHoverBrush().brush( palette ).color() ) );
                    }

                } else color = _helper->backgroundColor( color, widget, r.center() );

                // drawing
                if( animated && intersected )
                {

                    _helper->holeFlat( color, 0.0 )->render( animatedRect.adjusted( 1,1,-1,-1 ), painter, TileSet::Full );

                } else if( timerIsActive && current ) {

                    _helper->holeFlat( color, 0.0 )->render( r.adjusted( 1,1,-1,-1 ), painter, TileSet::Full );

                } else if( animated && current ) {

                    color.setAlphaF( opacity );
                    _helper->holeFlat( color, 0.0 )->render( r.adjusted( 1,1,-1,-1 ), painter, TileSet::Full );

                } else if( active ) {

                    _helper->holeFlat( color, 0.0 )->render( r.adjusted( 1,1,-1,-1 ), painter, TileSet::Full );

                }

            }

        }

        // text
        QPalette::ColorRole role( QPalette::WindowText );
        if( StyleConfigData::menuHighlightMode() == StyleConfigData::MM_STRONG && ( state & State_Sunken ) && enabled )
        { role = QPalette::HighlightedText; }

        drawItemText( painter, r, Qt::AlignCenter | Qt::TextShowMnemonic, palette, enabled, menuOption->text, role );

        return true;

    }

    //___________________________________________________________________________________
    bool Style::drawMenuItemControl( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        // cast option and check
        const QStyleOptionMenuItem* menuItemOption = qstyleoption_cast<const QStyleOptionMenuItem*>( option );
        if( !menuItemOption ) return true;
        if( menuItemOption->menuItemType == QStyleOptionMenuItem::EmptyArea ) return true;

        // copy rect and palette
        const QRect& rect( option->rect );
        const QPalette& palette( option->palette );

        // render background
        renderMenuItemBackground( option, painter, widget );

        // store state
        const State& state( option->state );
        const bool enabled( state & State_Enabled );
        const bool selected( enabled && (state & State_Selected) );
        const bool sunken( enabled && (state & (State_On|State_Sunken) ) );
        const bool hasFocus( enabled && ( state & State_HasFocus ) );
        const bool mouseOver( enabled && ( state & State_MouseOver ) );
        const bool reverseLayout( option->direction == Qt::RightToLeft );

        // Active indicator
        if( selected )
        {

            // check if there is a 'sliding' animation in progress, in which case, do nothing
            const QRect animatedRect( _animations->menuEngine().animatedRect( widget ) );
            if( animatedRect.isNull() )
            {

                const bool animated( _animations->menuEngine().isAnimated( widget, Current ) );
                const QRect currentRect( _animations->menuEngine().currentRect( widget, Current ) );
                const bool intersected( currentRect.contains( rect.topLeft() ) );

                const QColor color( _helper->menuBackgroundColor( _helper->calcMidColor( palette.color( QPalette::Window ) ), widget, rect.center() ) );

                if( animated && intersected ) renderMenuItemRect( option, rect, color, palette, painter, _animations->menuEngine().opacity( widget, Current ) );
                else renderMenuItemRect( option, rect, color, palette, painter );

            }

        }

        // get rect available for contents
        QRect contentsRect( insideMargin( rect,  Metrics::MenuItem_MarginWidth ) );

        // deal with separators
        if( menuItemOption->menuItemType == QStyleOptionMenuItem::Separator )
        {

            // normal separator
            if( menuItemOption->text.isEmpty() && menuItemOption->icon.isNull() )
            {

                // in all other cases draw regular separator
                const QColor color( _helper->menuBackgroundColor( palette.color( QPalette::Window ), widget, rect.center() ) );
                _helper->drawSeparator( painter, rect, color, Qt::Horizontal );
                return true;

            } else {

                // separator can have a title and an icon
                // in that case they are rendered as sunken flat toolbuttons
                QStyleOptionToolButton toolButtonOption( separatorMenuItemOption( menuItemOption, widget ) );
                toolButtonOption.state = State_On|State_Sunken|State_Enabled;
                drawComplexControl( CC_ToolButton, &toolButtonOption, painter, widget );
                return true;

            }

        }

        // define relevant rectangles
        // checkbox
        QRect checkBoxRect;
        if( menuItemOption->menuHasCheckableItems )
        {
            checkBoxRect = QRect( contentsRect.left(), contentsRect.top() + (contentsRect.height()-Metrics::CheckBox_Size)/2 - 1, Metrics::CheckBox_Size, Metrics::CheckBox_Size );
            contentsRect.setLeft( checkBoxRect.right() + Metrics::MenuItem_ItemSpacing + 1 );
        }

        // render checkbox indicator
        const CheckBoxState checkBoxState( menuItemOption->checked ? CheckOn:CheckOff );
        if( menuItemOption->checkType == QStyleOptionMenuItem::NonExclusive )
        {

            checkBoxRect = visualRect( option, checkBoxRect );

            StyleOptions styleOptions( 0 );
            styleOptions |= Sunken;
            if( !enabled ) styleOptions |= Disabled;
            if( mouseOver ) styleOptions |= Hover;
            if( hasFocus ) styleOptions |= Focus;

            QPalette localPalette( palette );
            localPalette.setColor( QPalette::Window, _helper->menuBackgroundColor( palette.color( QPalette::Window ), widget, rect.topLeft() ) );
            renderCheckBox( painter, checkBoxRect, localPalette, styleOptions, checkBoxState );

        } else if( menuItemOption->checkType == QStyleOptionMenuItem::Exclusive ) {

            checkBoxRect = visualRect( option, checkBoxRect );

            StyleOptions styleOptions( 0 );
            if( !enabled ) styleOptions |= Disabled;
            if( mouseOver ) styleOptions |= Hover;
            if( hasFocus ) styleOptions |= Focus;

            QPalette localPalette( palette );
            localPalette.setColor( QPalette::Window, _helper->menuBackgroundColor( palette.color( QPalette::Window ), widget, rect.topLeft() ) );
            renderRadioButton( painter, checkBoxRect, localPalette, styleOptions, checkBoxState );

        }

        // icon
        const int iconWidth( menuItemOption->maxIconWidth );

        QRect iconRect( contentsRect.left(), contentsRect.top() + (contentsRect.height()-iconWidth)/2, iconWidth, iconWidth );
        contentsRect.setLeft( iconRect.right() + Metrics::MenuItem_ItemSpacing + 1 );

        if( !menuItemOption->icon.isNull() )
        {

            const QSize iconSize( pixelMetric( PM_SmallIconSize, option, widget ), pixelMetric( PM_SmallIconSize, option, widget ) );
            iconRect = centerRect( iconRect, iconSize );
            iconRect = visualRect( option, iconRect );

            // icon mode
            QIcon::Mode mode;
            if( selected ) mode = QIcon::Active;
            else if( enabled ) mode = QIcon::Normal;
            else mode = QIcon::Disabled;

            // icon state
            const QIcon::State iconState( sunken ? QIcon::On:QIcon::Off );
            const QPixmap icon = menuItemOption->icon.pixmap( iconRect.size(), mode, iconState );
            painter->drawPixmap( iconRect, icon );

        }

        // text role
        const QPalette::ColorRole textRole( ( selected && StyleConfigData::menuHighlightMode() == StyleConfigData::MM_STRONG ) ?
            QPalette::HighlightedText:
            QPalette::WindowText );

        // arrow
        if( menuItemOption->menuItemType == QStyleOptionMenuItem::SubMenu )
        {

            QRect arrowRect( contentsRect.right() - Metrics::MenuButton_IndicatorWidth + 1, contentsRect.top() + (contentsRect.height()-Metrics::MenuButton_IndicatorWidth)/2, Metrics::MenuButton_IndicatorWidth, Metrics::MenuButton_IndicatorWidth );
            contentsRect.setRight( arrowRect.left() -  Metrics::MenuItem_ItemSpacing - 1 );

            const qreal penThickness = 1.6;
            const QColor color = palette.color( textRole );
            const QColor background = palette.color( QPalette::Window );

            // get arrow shape
            QPolygonF arrow = genericArrow( option->direction == Qt::LeftToRight ? ArrowRight : ArrowLeft, ArrowNormal );

            painter->save();
            painter->translate( QRectF( arrowRect ).center() );
            painter->setRenderHint( QPainter::Antialiasing );

            // white reflection
            const qreal offset( qMin( penThickness, qreal( 1.0 ) ) );
            painter->translate( 0,offset );
            painter->setPen( QPen( _helper->calcLightColor( background ), penThickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
            painter->drawPolyline( arrow );
            painter->translate( 0,-offset );

            painter->setPen( QPen( _helper->decoColor( background, color ) , penThickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
            painter->drawPolyline( arrow );
            painter->restore();

        }

        QRect textRect = contentsRect;
        if( !menuItemOption->text.isEmpty() )
        {

            // adjust textRect
            QString text = menuItemOption->text;
            textRect = centerRect( textRect, textRect.width(), option->fontMetrics.size( _mnemonics->textFlags(), text ).height() );
            textRect = visualRect( option, textRect );

            // set font
            painter->setFont( menuItemOption->font );

            // locate accelerator and render
            const int tabPosition( text.indexOf( QLatin1Char( '\t' ) ) );
            if( tabPosition >= 0 )
            {
                QString accelerator( text.mid( tabPosition + 1 ) );
                text = text.left( tabPosition );
                drawItemText( painter, textRect, Qt::AlignRight | Qt::AlignVCenter | _mnemonics->textFlags(), palette, enabled, accelerator, QPalette::WindowText );
            }

            // render text
            const int textFlags( Qt::AlignVCenter | (reverseLayout ? Qt::AlignRight : Qt::AlignLeft ) | _mnemonics->textFlags() );

            textRect = option->fontMetrics.boundingRect( textRect, textFlags, text );
            drawItemText( painter, textRect, textFlags, palette, enabled, text, textRole );

        }

        return true;
    }

    //___________________________________________________________________________________
    bool Style::drawProgressBarControl( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        const QStyleOptionProgressBar* progressBarOption( qstyleoption_cast<const QStyleOptionProgressBar*>( option ) );
        if( !progressBarOption ) return true;

        QStyleOptionProgressBarV2 progressBarOption2 = *progressBarOption;
        progressBarOption2.rect = subElementRect( SE_ProgressBarGroove, progressBarOption, widget );
        drawProgressBarGrooveControl( &progressBarOption2, painter, widget );

        // enable busy animations
        #if QT_VERSION >= 0x050000
        const QObject* styleObject( widget ? widget:progressBarOption->styleObject );
        #else
        const QObject* styleObject( widget );
        #endif

        if( styleObject && _animations->busyIndicatorEngine().enabled() )
        {

            #if QT_VERSION >= 0x050000
            // register QML object if defined
            if( !widget && progressBarOption->styleObject )
            { _animations->busyIndicatorEngine().registerWidget( progressBarOption->styleObject ); }
            #endif

            _animations->busyIndicatorEngine().setAnimated( styleObject, progressBarOption->maximum == 0 && progressBarOption->minimum == 0 );

        }

        if( _animations->busyIndicatorEngine().isAnimated( styleObject ) )
        { progressBarOption2.progress = _animations->busyIndicatorEngine().value(); }

        // render contents
        progressBarOption2.rect = subElementRect( SE_ProgressBarContents, progressBarOption, widget );
        drawProgressBarContentsControl( &progressBarOption2, painter, widget );

        // render text
        const bool textVisible( progressBarOption->textVisible );
        const bool busy( progressBarOption->minimum == 0 && progressBarOption->maximum == 0 );
        if( textVisible && !busy )
        {
            progressBarOption2.rect = subElementRect( SE_ProgressBarLabel, progressBarOption, widget );
            drawProgressBarLabelControl( &progressBarOption2, painter, widget );
        }

        return true;

    }

    //___________________________________________________________________________________
    bool Style::drawProgressBarContentsControl( const QStyleOption* option, QPainter* painter, const QWidget* ) const
    {

        // cast option and check
        const QStyleOptionProgressBar* progressBarOption = qstyleoption_cast<const QStyleOptionProgressBar*>( option );
        if( !progressBarOption ) return true;

        // get orientation
        const QStyleOptionProgressBarV2* progressBarOption2 = qstyleoption_cast<const QStyleOptionProgressBarV2*>( option );
        const bool horizontal( !progressBarOption2 || progressBarOption2->orientation == Qt::Horizontal );

        // copy rect and palette
        const QRect& rect( option->rect );
        const QPalette& palette( option->palette );

        // make sure rect is large enough
        if( rect.isValid() )
        {
            // calculate dimension
            int dimension( 20 );
            if( progressBarOption2 ) dimension = qMax( 5, horizontal ? rect.height() : rect.width() );
            TileSet* tileSet( _helper->progressBarIndicator( palette, dimension ) );
            tileSet->render( rect, painter, TileSet::Full );
        }

        return true;

    }

    //___________________________________________________________________________________
    bool Style::drawProgressBarGrooveControl( const QStyleOption* option, QPainter* painter, const QWidget* ) const
    {

        const QStyleOptionProgressBarV2 *progressBarOption = qstyleoption_cast<const QStyleOptionProgressBarV2 *>( option );
        const Qt::Orientation orientation( progressBarOption? progressBarOption->orientation : Qt::Horizontal );
        renderScrollBarHole( painter, option->rect, option->palette.color( QPalette::Window ), orientation );

        return true;
    }

    //___________________________________________________________________________________
    bool Style::drawProgressBarLabelControl( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        // cast option and check
        const QStyleOptionProgressBar* progressBarOption = qstyleoption_cast<const QStyleOptionProgressBar*>( option );
        if( !progressBarOption ) return true;

        // copy rect and palette
        const QRect& rect( option->rect );
        const QPalette& palette( option->palette );

        // store state
        const State& state( option->state );
        const bool enabled( state & State_Enabled );
        const bool reverseLayout = ( option->direction == Qt::RightToLeft );

        // get orientation
        const QStyleOptionProgressBarV2* progressBarOption2 = qstyleoption_cast<const QStyleOptionProgressBarV2*>( option );
        const bool horizontal = !progressBarOption2 || progressBarOption2->orientation == Qt::Horizontal;

        // check inverted appearance
        const bool inverted( progressBarOption2 ? progressBarOption2->invertedAppearance : false );

        // rotate label for vertical layout
        QTransform transform;
        if( !horizontal )
        {
            if( reverseLayout ) transform.rotate( -90 );
            else transform.rotate( 90 );
        }

        painter->setTransform( transform );
        const QRect progressRect( transform.inverted().mapRect( subElementRect( SE_ProgressBarContents, progressBarOption, widget ) ) );
        QRect textRect( transform.inverted().mapRect( rect ) );

        Qt::Alignment hAlign( ( progressBarOption->textAlignment == Qt::AlignLeft ) ? Qt::AlignHCenter : progressBarOption->textAlignment );

        /*
        Figure out the geometry of the indicator.
        This is copied from drawProgressBarContentsControl
        */
        if( progressRect.isValid() )
        {
            // first pass ( normal )
            QRect textClipRect( textRect );

            if( horizontal )
            {

                if( (reverseLayout && !inverted) || (inverted && !reverseLayout) ) textClipRect.setRight( progressRect.left() );
                else textClipRect.setLeft( progressRect.right() + 1 );

            } else if( (reverseLayout && !inverted) || (inverted && !reverseLayout) ) textClipRect.setLeft( progressRect.right() + 1 );
            else textClipRect.setRight( progressRect.left() );

            painter->setClipRect( textClipRect );
            drawItemText( painter, textRect, Qt::AlignVCenter | hAlign, palette, enabled, progressBarOption->text, QPalette::WindowText );

            // second pass ( highlighted )
            painter->setClipRect( progressRect );
            drawItemText( painter, textRect, Qt::AlignVCenter | hAlign, palette, enabled, progressBarOption->text, QPalette::HighlightedText );

        } else {

            drawItemText( painter, textRect, Qt::AlignVCenter | hAlign, palette, enabled, progressBarOption->text, QPalette::WindowText );

        }

        return true;
    }

    //___________________________________________________________________________________
    bool Style::drawPushButtonLabelControl( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        // cast option and check
        const QStyleOptionButton* buttonOption( qstyleoption_cast<const QStyleOptionButton*>( option ) );
        if( !buttonOption ) return true;

        // copy rect and palette
        const QRect& rect( option->rect );
        const QPalette& palette( option->palette );

        // state
        const State& state( option->state );
        const bool enabled( state & State_Enabled );
        const bool sunken( ( state & State_On ) || ( state & State_Sunken ) );
        const bool mouseOver( enabled && (option->state & State_MouseOver) );
        const bool flat( buttonOption->features & QStyleOptionButton::Flat );

        // content
        const bool hasIcon( !buttonOption->icon.isNull() );
        const bool hasText( !buttonOption->text.isEmpty() );

        // contents
        QRect contentsRect( rect );

        // color role
        const QPalette::ColorRole textRole( flat ? QPalette::WindowText:QPalette::ButtonText );

        // menu arrow
        if( buttonOption->features & QStyleOptionButton::HasMenu )
        {

            // define rect
            QRect arrowRect( contentsRect );
            arrowRect.setLeft( contentsRect.right() - Metrics::MenuButton_IndicatorWidth + 1 );
            arrowRect = centerRect( arrowRect, Metrics::MenuButton_IndicatorWidth, Metrics::MenuButton_IndicatorWidth );

            contentsRect.setRight( arrowRect.left() - Metrics::Button_ItemSpacing - 1  );
            contentsRect.adjust( Metrics::Button_MarginWidth, 0, 0, 0 );

            arrowRect = visualRect( option, arrowRect );

            // arrow
            const qreal penThickness = 1.6;
            QPolygonF arrow = genericArrow( ArrowDown, ArrowNormal );

            const QColor color = palette.color( flat ? QPalette::WindowText:QPalette::ButtonText );
            const QColor background = palette.color( flat ? QPalette::Window:QPalette::Button );

            painter->save();
            painter->translate( QRectF( arrowRect ).center() );
            painter->setRenderHint( QPainter::Antialiasing );

            const qreal offset( qMin( penThickness, qreal( 1.0 ) ) );
            painter->translate( 0,offset );
            painter->setPen( QPen( _helper->calcLightColor(  background ), penThickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
            painter->drawPolyline( arrow );
            painter->translate( 0,-offset );

            painter->setPen( QPen( _helper->decoColor( background, color ) , penThickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
            painter->drawPolyline( arrow );
            painter->restore();

        } else contentsRect.adjust( Metrics::Button_MarginWidth, 0, -Metrics::Button_MarginWidth, 0 );

        // icon size
        QSize iconSize( buttonOption->iconSize );
        if( !iconSize.isValid() )
        {
            const int metric( pixelMetric( PM_SmallIconSize, option, widget ) );
            iconSize = QSize( metric, metric );
        }

        // text size
        const int textFlags( _mnemonics->textFlags() | Qt::AlignCenter );
        const QSize textSize( option->fontMetrics.size( textFlags, buttonOption->text ) );

        // adjust text and icon rect based on options
        QRect iconRect;
        QRect textRect;

        if( hasText && !hasIcon ) textRect = contentsRect;
        else if( hasIcon && !hasText ) iconRect = contentsRect;
        else {

            const int contentsWidth( iconSize.width() + textSize.width() + Metrics::Button_ItemSpacing );
            iconRect = QRect( QPoint( contentsRect.left() + (contentsRect.width() - contentsWidth )/2, contentsRect.top() + (contentsRect.height() - iconSize.height())/2 ), iconSize );
            textRect = QRect( QPoint( iconRect.right() + Metrics::ToolButton_ItemSpacing + 1, contentsRect.top() + (contentsRect.height() - textSize.height())/2 ), textSize );

        }

        // handle right to left
        if( iconRect.isValid() ) iconRect = visualRect( option, iconRect );
        if( textRect.isValid() ) textRect = visualRect( option, textRect );

        // make sure there is enough room for icon
        if( iconRect.isValid() ) iconRect = centerRect( iconRect, iconSize );

        // render icon
        if( hasIcon && iconRect.isValid() ) {

            // icon state and mode
            const QIcon::State iconState( sunken ? QIcon::On : QIcon::Off );
            QIcon::Mode iconMode;
            if( !enabled ) iconMode = QIcon::Disabled;
            else if( mouseOver && flat ) iconMode = QIcon::Active;
            else iconMode = QIcon::Normal;

            const QPixmap pixmap = buttonOption->icon.pixmap( iconSize, iconMode, iconState );
            drawItemPixmap( painter, iconRect, Qt::AlignCenter, pixmap );

        }

        // render text
        if( hasText && textRect.isValid() )
        { drawItemText( painter, textRect, textFlags, palette, enabled, buttonOption->text, textRole ); }

        return true;

    }

    //___________________________________________________________________________________
    bool Style::drawRubberBandControl( const QStyleOption* option, QPainter* painter, const QWidget* ) const
    {

        const QPalette& palette( option->palette );
        const QRect rect( option->rect );

        QColor color = palette.color( QPalette::Highlight );
        painter->setPen( KColorUtils::mix( color, palette.color( QPalette::Active, QPalette::WindowText ) ) );
        color.setAlpha( 50 );
        painter->setBrush( color );
        painter->setClipRegion( rect );
        painter->drawRect( rect.adjusted( 0, 0, -1, -1 ) );
        return true;

    }

    //___________________________________________________________________________________
    bool Style::drawScrollBarSliderControl( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        // cast option and check
        const QStyleOptionSlider *sliderOption = qstyleoption_cast<const QStyleOptionSlider *>( option );
        if( !sliderOption ) return true;

        // store rect and palette
        QRect rect( option->rect );
        const QPalette& palette( option->palette );

        // store state
        const State& state( option->state );
        const Qt::Orientation orientation( (state & State_Horizontal) ? Qt::Horizontal : Qt::Vertical );
        const bool enabled( state & State_Enabled );
        const bool mouseOver( enabled && ( state & State_MouseOver ) );

        // update animations
        _animations->scrollBarEngine().updateState( widget, enabled && ( sliderOption->activeSubControls & SC_ScrollBarSlider ) );
        const bool animated( enabled && _animations->scrollBarEngine().isAnimated( widget, SC_ScrollBarSlider ) );

        if( orientation == Qt::Horizontal ) rect.adjust( 0, 1, 0, -1 );
        else rect.adjust( 1, 0, -1, 0 );

        // render
        if( animated ) renderScrollBarHandle( painter, rect, palette, orientation, mouseOver, _animations->scrollBarEngine().opacity( widget, SC_ScrollBarSlider ) );
        else renderScrollBarHandle( painter, rect, palette, orientation, mouseOver );

        return true;
    }

    //___________________________________________________________________________________
    bool Style::drawScrollBarAddLineControl( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        // do nothing if no buttons are defined
        if( _addLineButtons == NoButton ) return true;

        // cast option and check
        const QStyleOptionSlider* sliderOption( qstyleoption_cast<const QStyleOptionSlider*>( option ) );
        if( !sliderOption ) return true;

        const State& state( option->state );
        const bool horizontal( state & State_Horizontal );
        const bool reverseLayout( option->direction == Qt::RightToLeft );

        // colors
        const QPalette& palette( option->palette );
        const QColor background( palette.color( QPalette::Window ) );

        // adjust rect, based on number of buttons to be drawn
        const QRect rect( scrollBarInternalSubControlRect( sliderOption, SC_ScrollBarAddLine ) );

        QColor color;
        QStyleOptionSlider copy( *sliderOption );
        if( _addLineButtons == DoubleButton )
        {

            if( horizontal )
            {

                //Draw the arrows
                const QSize halfSize( rect.width()/2, rect.height() );
                const QRect leftSubButton( rect.topLeft(), halfSize );
                const QRect rightSubButton( leftSubButton.topRight() + QPoint( 1, 0 ), halfSize );

                copy.rect = leftSubButton;
                color = scrollBarArrowColor( &copy,  reverseLayout ? SC_ScrollBarAddLine:SC_ScrollBarSubLine, widget );
                renderScrollBarArrow( painter, leftSubButton, color, background, ArrowLeft );

                copy.rect = rightSubButton;
                color = scrollBarArrowColor( &copy,  reverseLayout ? SC_ScrollBarSubLine:SC_ScrollBarAddLine, widget );
                renderScrollBarArrow( painter, rightSubButton, color, background, ArrowRight );

            } else {

                const QSize halfSize( rect.width(), rect.height()/2 );
                const QRect topSubButton( rect.topLeft(), halfSize );
                const QRect botSubButton( topSubButton.bottomLeft() + QPoint( 0, 1 ), halfSize );

                copy.rect = topSubButton;
                color = scrollBarArrowColor( &copy, SC_ScrollBarSubLine, widget );
                renderScrollBarArrow( painter, topSubButton, color, background, ArrowUp );

                copy.rect = botSubButton;
                color = scrollBarArrowColor( &copy, SC_ScrollBarAddLine, widget );
                renderScrollBarArrow( painter, botSubButton, color, background, ArrowDown );

            }

        } else if( _addLineButtons == SingleButton ) {

            copy.rect = rect;
            color = scrollBarArrowColor( &copy,  SC_ScrollBarAddLine, widget );
            if( horizontal ) renderScrollBarArrow( painter, rect, color, background, reverseLayout ? ArrowLeft : ArrowRight );
            else renderScrollBarArrow( painter, rect, color, background, ArrowDown );

        }

        return true;
    }

    //___________________________________________________________________________________
    bool Style::drawScrollBarSubLineControl( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        // do nothing if no buttons are set
        if( _subLineButtons == NoButton ) return true;

        // cast option and check
        const QStyleOptionSlider* sliderOption( qstyleoption_cast<const QStyleOptionSlider*>( option ) );
        if( !sliderOption ) return true;

        const State& state( option->state );
        const bool horizontal( state & State_Horizontal );
        const bool reverseLayout( option->direction == Qt::RightToLeft );

        // colors
        const QPalette& palette( option->palette );
        const QColor background( palette.color( QPalette::Window ) );


        // adjust rect, based on number of buttons to be drawn
        QRect rect( scrollBarInternalSubControlRect( sliderOption, SC_ScrollBarSubLine ) );

        QColor color;
        QStyleOptionSlider copy( *sliderOption );
        if( _subLineButtons == DoubleButton )
        {

            if( horizontal )
            {

                //Draw the arrows
                const QSize halfSize( rect.width()/2, rect.height() );
                const QRect leftSubButton( rect.topLeft(), halfSize );
                const QRect rightSubButton( leftSubButton.topRight() + QPoint( 1, 0 ), halfSize );

                copy.rect = leftSubButton;
                color = scrollBarArrowColor( &copy,  reverseLayout ? SC_ScrollBarAddLine:SC_ScrollBarSubLine, widget );
                renderScrollBarArrow( painter, leftSubButton, color, background, ArrowLeft );

                copy.rect = rightSubButton;
                color = scrollBarArrowColor( &copy,  reverseLayout ? SC_ScrollBarSubLine:SC_ScrollBarAddLine, widget );
                renderScrollBarArrow( painter, rightSubButton, color, background, ArrowRight );

            } else {

                const QSize halfSize( rect.width(), rect.height()/2 );
                const QRect topSubButton( rect.topLeft(), halfSize );
                const QRect botSubButton( topSubButton.bottomLeft() + QPoint( 0, 1 ), halfSize );

                copy.rect = topSubButton;
                color = scrollBarArrowColor( &copy, SC_ScrollBarSubLine, widget );
                renderScrollBarArrow( painter, topSubButton, color, background, ArrowUp );

                copy.rect = botSubButton;
                color = scrollBarArrowColor( &copy, SC_ScrollBarAddLine, widget );
                renderScrollBarArrow( painter, botSubButton, color, background, ArrowDown );

            }

        } else if( _subLineButtons == SingleButton ) {

            copy.rect = rect;
            color = scrollBarArrowColor( &copy,  SC_ScrollBarSubLine, widget );
            if( horizontal ) renderScrollBarArrow( painter, rect, color, background, reverseLayout ? ArrowRight : ArrowLeft );
            else renderScrollBarArrow( painter, rect, color, background, ArrowUp );

        }

        return true;
    }

    //___________________________________________________________________________________
    bool Style::drawShapedFrameControl( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        // cast option and check
        const QStyleOptionFrameV3* frameOption = qstyleoption_cast<const QStyleOptionFrameV3*>( option );
        if( !frameOption ) return false;

        switch( frameOption->frameShape )
        {

            case QFrame::Box:
            {
                if( option->state & State_Sunken ) return true;
                else break;
            }

            case QFrame::HLine:
            {
                const QColor color( _helper->backgroundColor( option->palette.color( QPalette::Window ), widget, option->rect.center() ) );
                _helper->drawSeparator( painter, option->rect, color, Qt::Horizontal );
                return true;
            }

            case QFrame::VLine:
            {
                const QColor color( _helper->backgroundColor( option->palette.color( QPalette::Window ), widget, option->rect.center() ) );
                _helper->drawSeparator( painter, option->rect, color, Qt::Vertical );
                return true;
            }

            default: break;

        }

        return false;

    }

    //___________________________________________________________________________________
    bool Style::drawTabBarTabLabelControl( const QStyleOption* option, QPainter* painter, const QWidget* ) const
    {

        const QStyleOptionTab *tabOption = qstyleoption_cast< const QStyleOptionTab* >( option );
        if( !tabOption ) return true;

        // add extra offset for selected tas
        QStyleOptionTabV3 tabOptionV3( *tabOption );

        const bool selected( option->state & State_Selected );

        // get rect
        QRect r( option->rect );

        // handle selection and orientation
        /*
        painter is rotated and translated to deal with various orientations
        rect is translated to 0,0, and possibly transposed
        */
        switch( tabOptionV3.shape )
        {


            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
            {
                if( selected ) r.translate( 0, -1 );
                painter->translate( r.topLeft() );
                r.moveTopLeft( QPoint( 0,0 ) );
                break;

            }

            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
            {
                if( selected ) r.translate( 0, 1 );
                painter->translate( r.topLeft() );
                r.moveTopLeft( QPoint( 0,0 ) );
                break;

            }

            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
            {

                if( selected ) r.translate( -1, 0 );
                painter->translate( r.bottomLeft() );
                painter->rotate( -90 );
                r = QRect( QPoint( 0, 0 ), QSize( r.height(), r.width() ) );
                break;

            }

            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
            {

                if( selected ) r.translate( 1, 0 );
                painter->translate( r.topRight() );
                painter->rotate( 90 );
                r = QRect( QPoint( 0, 0 ), QSize( r.height(), r.width() ) );
                break;

            }

            default: break;

        }

        // make room for left and right widgets
        // left widget
        const bool verticalTabs( isVerticalTab( tabOption ) );
        const bool hasLeftButton( !( option->direction == Qt::RightToLeft ? tabOptionV3.rightButtonSize.isEmpty():tabOptionV3.leftButtonSize.isEmpty() ) );
        const bool hasRightButton( !( option->direction == Qt::RightToLeft ? tabOptionV3.leftButtonSize.isEmpty():tabOptionV3.rightButtonSize.isEmpty() ) );

        if( hasLeftButton )
        { r.setLeft( r.left() + 4 + ( verticalTabs ? tabOptionV3.leftButtonSize.height() : tabOptionV3.leftButtonSize.width() ) ); }

        // make room for left and right widgets
        // left widget
        if( hasRightButton )
        { r.setRight( r.right() - 4 - ( verticalTabs ? tabOptionV3.rightButtonSize.height() : tabOptionV3.rightButtonSize.width() ) ); }

        // compute textRect and iconRect
        // now that orientation is properly dealt with, everything is handled as a 'north' orientation
        QRect textRect;
        QRect iconRect;

        if( tabOptionV3.icon.isNull() )
        {

            textRect = r.adjusted( 6, 0, -6, 0 );

        } else {

            const QSize& iconSize( tabOptionV3.iconSize );
            iconRect = centerRect( r, iconSize );
            if( !tabOptionV3.text.isEmpty() )
            {

                iconRect.moveLeft( r.left() + 8 );
                textRect = r;
                textRect.setLeft( iconRect.right()+3 );
                textRect.setRight( r.right() - 6 );
            }

        }

        if( !verticalTabs )
        {
            textRect = visualRect(option->direction, r, textRect );
            iconRect = visualRect(option->direction, r, iconRect );
        }

        // render icon
        if( !iconRect.isNull() )
        {

            // not sure why this is necessary
            if( tabOptionV3.shape == QTabBar::RoundedNorth || tabOptionV3.shape == QTabBar::TriangularNorth )
            { iconRect.translate( 0, -1 ); }

            const QPixmap tabIcon = tabOptionV3.icon.pixmap(
                tabOptionV3.iconSize,
                ( tabOptionV3.state & State_Enabled ) ? QIcon::Normal : QIcon::Disabled,
                ( tabOptionV3.state & State_Selected ) ? QIcon::On : QIcon::Off );

            painter->drawPixmap( iconRect.topLeft(), tabIcon );
        }

        // render text
        if( !textRect.isNull() )
        {

            const QPalette& palette( option->palette );
            const QString& text( tabOptionV3.text );
            const bool enabled( option->state & State_Enabled );
            const int alignment( Qt::AlignCenter|Qt::TextShowMnemonic );
            drawItemText( painter, textRect, alignment, palette, enabled, text, QPalette::WindowText );

        }

        return true;

    }

    //___________________________________________________________________________________
    bool Style::drawTabBarTabShapeControl( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        const QStyleOptionTab* tabOption( qstyleoption_cast<const QStyleOptionTab*>( option ) );
        if( !tabOption ) return true;

        const State& state( option->state );
        const QRect& r( option->rect );
        const QPalette& palette( option->palette );

        const bool enabled( state & State_Enabled );
        const bool selected( state & State_Selected );
        const bool reverseLayout( option->direction == Qt::RightToLeft );

        // tab position and flags
        const QStyleOptionTab::TabPosition& position = tabOption->position;
        bool isFirst( position == QStyleOptionTab::OnlyOneTab || position == QStyleOptionTab::Beginning );
        bool isLast( position == QStyleOptionTab::OnlyOneTab || position == QStyleOptionTab::End );
        bool isLeftOfSelected( tabOption->selectedPosition == QStyleOptionTab::NextIsSelected );
        bool isRightOfSelected( tabOption->selectedPosition == QStyleOptionTab::PreviousIsSelected );

        // document mode
        const QStyleOptionTabV3 *tabOptionV3 = qstyleoption_cast<const QStyleOptionTabV3 *>( option );
        bool documentMode = tabOptionV3 ? tabOptionV3->documentMode : false;
        const QTabWidget *tabWidget = ( widget && widget->parentWidget() ) ? qobject_cast<const QTabWidget *>( widget->parentWidget() ) : NULL;
        documentMode |= ( tabWidget ? tabWidget->documentMode() : true );

        // this is needed to complete the base frame when there are widgets in tabbar
        const QTabBar* tabBar( qobject_cast<const QTabBar*>( widget ) );
        const QRect tabBarRect( tabBar ? insideMargin( tabBar->rect(), 0 ):QRect() );

        // check if tab is being dragged
        const bool isDragged( selected && painter->device() != tabBar );

        // hover and animation flags
        /* all are disabled when tabBar is locked ( drag in progress ) */
        const bool tabBarLocked( _tabBarData->locks( tabBar ) );
        const bool mouseOver( enabled && !tabBarLocked && ( state & State_MouseOver ) );

        // animation state
        _animations->tabBarEngine().updateState( widget, r.topLeft(), mouseOver );
        const bool animated( enabled && !selected && !tabBarLocked && _animations->tabBarEngine().isAnimated( widget, r.topLeft() ) );

        // handle base frame painting, for tabbars in which tab is being dragged
        _tabBarData->drawTabBarBaseControl( tabOption, painter, widget );
        if( selected && tabBar && isDragged ) _tabBarData->lock( tabBar );
        else if( selected  && _tabBarData->locks( tabBar ) ) _tabBarData->release();

        // corner widgets
        const bool hasLeftCornerWidget( tabOption->cornerWidgets & QStyleOptionTab::LeftCornerWidget );
        const bool hasRightCornerWidget( tabOption->cornerWidgets & QStyleOptionTab::RightCornerWidget );

        // true if widget is aligned to the frame
        /* need to check for 'isRightOfSelected' because for some reason the isFirst flag is set when active tab is being moved */
        const bool isFrameAligned( !documentMode && isFirst && !hasLeftCornerWidget && !isRightOfSelected && !isDragged );
        isFirst &= !isRightOfSelected;
        isLast &= !isLeftOfSelected;

        // swap flags based on reverse layout, so that they become layout independent
        const bool verticalTabs( isVerticalTab( tabOption ) );
        if( reverseLayout && !verticalTabs )
        {
            qSwap( isFirst, isLast );
            qSwap( isLeftOfSelected, isRightOfSelected );
        }

        const qreal radius = 5;

        // part of the tab in which the text is drawn
        QRect tabRect( r );
        QPainterPath path;

        // connection to the frame
        SlabRectList slabs;

        // highlighted slab ( if any )
        SlabRect highlightSlab;

        switch( tabOption->shape )
        {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
            {

                // part of the tab in which the text is drawn
                // larger tabs when selected
                if( selected ) tabRect.adjust( 0, -1, 0, 2 );
                else tabRect.adjust( 0, 3, 1, -7 + 1 );

                // connection to the main frame
                if( selected )
                {

                    // do nothing if dragged
                    if( isDragged ) break;

                    // left side
                    if( isFrameAligned && !reverseLayout )
                    {

                        QRect frameRect( r );
                        frameRect.setLeft( frameRect.left() );
                        frameRect.setRight( tabRect.left() + 7 );
                        frameRect.setTop( tabRect.bottom() - 13 );
                        frameRect.setBottom( frameRect.bottom() + 7 - 1 );
                        slabs << SlabRect( frameRect, TileSet::Left );

                    } else {

                        QRect frameRect( r );
                        frameRect.setLeft( frameRect.left() - 7 );
                        frameRect.setRight( tabRect.left() + 7 + 3 );
                        frameRect.setTop( r.bottom() - 7 );
                        slabs << SlabRect( frameRect, TileSet::Top );
                    }

                    // right side
                    if( isFrameAligned && reverseLayout )
                    {

                        QRect frameRect( r );
                        frameRect.setLeft( tabRect.right() - 7 );
                        frameRect.setRight( frameRect.right() );
                        frameRect.setTop( tabRect.bottom() - 13 );
                        frameRect.setBottom( frameRect.bottom() + 7 - 1 );
                        slabs << SlabRect( frameRect, TileSet::Right );

                    } else {

                        QRect frameRect( r );
                        frameRect.setLeft( tabRect.right() - 7 - 3 );
                        frameRect.setRight( frameRect.right() + 7 );
                        frameRect.setTop( r.bottom() - 7 );
                        slabs << SlabRect( frameRect, TileSet::Top );

                    }

                    // extra base, to extend below inactive tabs and buttons
                    if( tabBar )
                    {
                        if( r.left() > tabBarRect.left() + 1 )
                        {
                            QRect frameRect( r );
                            frameRect.setLeft( tabBarRect.left() - 7 + 1 );
                            frameRect.setRight( r.left() + 7 - 1 );
                            frameRect.setTop( r.bottom() - 7 );
                            if( documentMode || reverseLayout ) slabs << SlabRect( frameRect, TileSet::Top );
                            else slabs << SlabRect( frameRect, TileSet::TopLeft );

                        }

                        if( r.right() < tabBarRect.right() - 1 )
                        {

                            QRect frameRect( r );
                            frameRect.setLeft( r.right() - 7 + 1 );
                            frameRect.setRight( tabBarRect.right() + 7 - 1 );
                            frameRect.setTop( r.bottom() - 7 );
                            if( documentMode || !reverseLayout ) slabs << SlabRect( frameRect, TileSet::Top );
                            else slabs << SlabRect( frameRect, TileSet::TopRight );
                        }
                    }

                } else {

                    // adjust sides when slab is adjacent to selected slab
                    if( isLeftOfSelected ) tabRect.setRight( tabRect.right() + 2 );
                    else if( isRightOfSelected ) tabRect.setLeft( tabRect.left() - 2 );

                    if( isFirst )
                    {

                        if( isFrameAligned ) path.moveTo( tabRect.bottomLeft() + QPoint( 0, 2 ) );
                        else path.moveTo( tabRect.bottomLeft() );
                        path.lineTo( tabRect.topLeft() + QPointF( 0, radius ) );
                        path.quadTo( tabRect.topLeft(), tabRect.topLeft() + QPoint( radius, 0 ) );
                        path.lineTo( tabRect.topRight() );
                        path.lineTo( tabRect.bottomRight() );


                    } else if( isLast ) {

                        tabRect.adjust( 0, 0, -1, 0 );
                        path.moveTo( tabRect.bottomLeft() );
                        path.lineTo( tabRect.topLeft() );
                        path.lineTo( tabRect.topRight() - QPointF( radius, 0 ) );
                        path.quadTo( tabRect.topRight(), tabRect.topRight() + QPointF( 0, radius ) );
                        if( isFrameAligned ) path.lineTo( tabRect.bottomRight() + QPointF( 0, 2 ) );
                        else path.lineTo( tabRect.bottomRight() );

                    } else {

                        path.moveTo( tabRect.bottomLeft() );
                        path.lineTo( tabRect.topLeft() );
                        path.lineTo( tabRect.topRight() );
                        path.lineTo( tabRect.bottomRight() );

                    }

                    // highlight
                    QRect highlightRect( tabRect.left(), tabRect.bottom()-1, tabRect.width(), 7 );
                    if( isFrameAligned && isFirst ) highlightSlab = SlabRect( highlightRect.adjusted( -2, 0, 7 + 1, 0 ), TileSet::TopLeft );
                    else if( isFrameAligned && isLast ) highlightSlab = SlabRect( highlightRect.adjusted( -7, 0, 2, 0 ), TileSet::TopRight );
                    else highlightSlab = SlabRect( highlightRect.adjusted( -7, 0, 7, 0 ), TileSet::Top );

                }

                break;

            }

            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
            {

                // larger tabs when selected
                if( selected ) tabRect.adjust( 0, -2, 0, 1 );
                else tabRect.adjust( 0, 7 - 1, 1, -3 );

                // connection to the main frame
                if( selected )
                {

                    // do nothing if dragged
                    if( isDragged ) break;

                    // left side
                    if( isFrameAligned && !reverseLayout )
                    {

                        QRect frameRect( r );
                        frameRect.setLeft( frameRect.left() );
                        frameRect.setRight( tabRect.left() + 7 );
                        frameRect.setTop( frameRect.top() - 7 + 1 );
                        frameRect.setBottom( tabRect.top() + 13 );
                        slabs << SlabRect( frameRect, TileSet::Left );

                    } else {

                        QRect frameRect( r );
                        frameRect.setLeft( frameRect.left() - 7 );
                        frameRect.setRight( tabRect.left() + 7 + 3 );
                        frameRect.setBottom( r.top() + 7 );
                        slabs << SlabRect( frameRect, TileSet::Bottom );
                    }

                    // right side
                    if( isFrameAligned && reverseLayout )
                    {

                        QRect frameRect( r );
                        frameRect.setLeft( tabRect.right() - 7 );
                        frameRect.setRight( frameRect.right() );
                        frameRect.setTop( frameRect.top() - 7 + 1 );
                        frameRect.setBottom( tabRect.top() + 13 );
                        slabs << SlabRect( frameRect, TileSet::Right );

                    } else {

                        QRect frameRect( r );
                        frameRect.setLeft( tabRect.right() - 7 - 3 );
                        frameRect.setRight( frameRect.right() + 7 );
                        frameRect.setBottom( r.top() + 7 );
                        slabs << SlabRect( frameRect, TileSet::Bottom );

                    }

                    // extra base, to extend below tabbar buttons
                    if( tabBar )
                    {
                        if( r.left() > tabBarRect.left() + 1 )
                        {
                            QRect frameRect( r );
                            frameRect.setLeft( tabBarRect.left() - 7 + 1 );
                            frameRect.setRight( r.left() + 7 - 1 );
                            frameRect.setBottom( r.top() + 7 );
                            if( documentMode || reverseLayout ) slabs << SlabRect( frameRect, TileSet::Bottom );
                            else slabs << SlabRect( frameRect, TileSet::BottomLeft );
                        }

                        if( r.right() < tabBarRect.right() - 1 )
                        {

                            QRect frameRect( r );
                            frameRect.setLeft( r.right() - 7 + 1 );
                            frameRect.setRight( tabBarRect.right() + 7 - 1 );
                            frameRect.setBottom( r.top() + 7 );
                            if( documentMode || !reverseLayout ) slabs << SlabRect( frameRect, TileSet::Bottom );
                            else slabs << SlabRect( frameRect, TileSet::BottomRight );
                        }

                    }

                } else {

                    // adjust sides when slab is adjacent to selected slab
                    if( isLeftOfSelected ) tabRect.setRight( tabRect.right() + 2 );
                    else if( isRightOfSelected ) tabRect.setLeft( tabRect.left() - 2 );

                    if( isFirst )
                    {

                        if( isFrameAligned ) path.moveTo( tabRect.topLeft() - QPoint( 0, 2 ) );
                        else path.moveTo( tabRect.topLeft() );
                        path.lineTo( tabRect.bottomLeft() - QPointF( 0, radius ) );
                        path.quadTo( tabRect.bottomLeft(), tabRect.bottomLeft() + QPoint( radius, 0 ) );
                        path.lineTo( tabRect.bottomRight() );
                        path.lineTo( tabRect.topRight() );

                    } else if( isLast ) {

                        tabRect.adjust( 0, 0, -1, 0 );
                        path.moveTo( tabRect.topLeft() );
                        path.lineTo( tabRect.bottomLeft() );
                        path.lineTo( tabRect.bottomRight() - QPointF( radius, 0 ) );
                        path.quadTo( tabRect.bottomRight(), tabRect.bottomRight() - QPointF( 0, radius ) );
                        if( isFrameAligned ) path.lineTo( tabRect.topRight() - QPointF( 0, 2 ) );
                        else path.lineTo( tabRect.topRight() );

                    } else {

                        path.moveTo( tabRect.topLeft() );
                        path.lineTo( tabRect.bottomLeft() );
                        path.lineTo( tabRect.bottomRight() );
                        path.lineTo( tabRect.topRight() );

                    }

                    // highlight
                    QRect highlightRect( tabRect.left(), tabRect.top()-5, tabRect.width(), 7 );
                    if( isFrameAligned && isFirst ) highlightSlab = SlabRect( highlightRect.adjusted( -2, 0, 7 + 1, 0 ), TileSet::BottomLeft );
                    else if( isFrameAligned && isLast ) highlightSlab = SlabRect( highlightRect.adjusted( -7, 0, 2, 0 ), TileSet::BottomRight );
                    else highlightSlab = SlabRect( highlightRect.adjusted( -7, 0, 7, 0 ), TileSet::Bottom );

                }

                break;

            }

            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
            {

                // larger tabs when selected
                if( selected ) tabRect.adjust( -1, 0, 2, 0 );
                else tabRect.adjust( 3, 0, -7 + 1, 1 );

                // connection to the main frame
                if( selected )
                {

                    // do nothing if dragged
                    if( isDragged ) break;

                    // top side
                    if( isFrameAligned )
                    {

                        QRect frameRect( r );
                        frameRect.setLeft( tabRect.right() - 13 );
                        frameRect.setRight( frameRect.right() + 7 - 1 );
                        frameRect.setTop( frameRect.top() );
                        frameRect.setBottom( tabRect.top() + 7 );
                        slabs << SlabRect( frameRect, TileSet::Top );

                    } else {

                        QRect frameRect( r );
                        frameRect.setLeft( r.right() - 7 );
                        frameRect.setTop( frameRect.top() - 7 );
                        frameRect.setBottom( tabRect.top() + 7 + 3 );
                        slabs << SlabRect( frameRect, TileSet::Left );
                    }

                    // bottom side
                    QRect frameRect( r );
                    frameRect.setLeft( r.right() - 7 );
                    frameRect.setTop( tabRect.bottom() - 7 - 3 );
                    frameRect.setBottom( frameRect.bottom() + 7 );
                    slabs << SlabRect( frameRect, TileSet::Left );

                    // extra base, to extend below tabbar buttons
                    if( tabBar )
                    {
                        if( r.top() > tabBarRect.top() + 1 )
                        {

                            QRect frameRect( r );
                            frameRect.setTop( tabBarRect.top() - 7 + 1 );
                            frameRect.setBottom( r.top() + 7 - 1 );
                            frameRect.setLeft( r.right() - 7 );
                            if( documentMode ) slabs << SlabRect( frameRect, TileSet::Left );
                            else slabs << SlabRect( frameRect, TileSet::TopLeft );
                        }

                        if( r.bottom() < tabBarRect.bottom() - 1 )
                        {

                            QRect frameRect( r );
                            frameRect.setTop( r.bottom() - 7 + 1 );
                            if( hasRightCornerWidget && documentMode ) frameRect.setBottom( tabBarRect.bottom() + 7 - 1 );
                            else frameRect.setBottom( tabBarRect.bottom() + 7 );
                            frameRect.setLeft( r.right() - 7 );
                            slabs << SlabRect( frameRect, TileSet::Left );

                        }
                    }

                } else {

                    // adjust sides when slab is adjacent to selected slab
                    if( isLeftOfSelected ) tabRect.setBottom( tabRect.bottom() + 2 );
                    else if( isRightOfSelected ) tabRect.setTop( tabRect.top() - 2 );

                    if( isFirst )
                    {

                        if( isFrameAligned ) path.moveTo( tabRect.topRight() + QPoint( 2, 0 ) );
                        else path.moveTo( tabRect.topRight() );
                        path.lineTo( tabRect.topLeft() + QPointF( radius, 0 ) );
                        path.quadTo( tabRect.topLeft(), tabRect.topLeft() + QPoint( 0, radius ) );
                        path.lineTo( tabRect.bottomLeft() );
                        path.lineTo( tabRect.bottomRight() );

                    } else if( isLast ) {

                        path.moveTo( tabRect.topRight() );
                        path.lineTo( tabRect.topLeft() );
                        path.lineTo( tabRect.bottomLeft() - QPointF( 0, radius ) );
                        path.quadTo( tabRect.bottomLeft(), tabRect.bottomLeft() + QPointF( radius, 0 ) );
                        path.lineTo( tabRect.bottomRight() );

                    } else {

                        path.moveTo( tabRect.topRight() );
                        path.lineTo( tabRect.topLeft() );
                        path.lineTo( tabRect.bottomLeft() );
                        path.lineTo( tabRect.bottomRight() );

                    }

                    // highlight
                    QRect highlightRect( tabRect.right()-1, tabRect.top(), 7, tabRect.height() );
                    if( isFrameAligned && isFirst ) highlightSlab = SlabRect( highlightRect.adjusted( 0, -2, 0, 7 + 1 ), TileSet::TopLeft );
                    else if( isFrameAligned && isLast ) highlightSlab = SlabRect( highlightRect.adjusted( 0, -7, 0, 2 ), TileSet::BottomLeft );
                    else highlightSlab = SlabRect( highlightRect.adjusted( 0, -7 + 1, 0, 7 + 1 ), TileSet::Left );

                }

                break;
            }

            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
            {

                // larger tabs when selected
                if( selected ) tabRect.adjust( -2, 0, 1, 0 );
                else tabRect.adjust( 7 - 1, 0, -3, 1 );

                // connection to the main frame
                if( selected )
                {

                    // do nothing if dragged
                    if( isDragged ) break;

                    // top side
                    if( isFrameAligned )
                    {

                        QRect frameRect( r );
                        frameRect.setLeft( frameRect.left() - 7 + 1 );
                        frameRect.setRight( tabRect.left() + 13 );
                        frameRect.setTop( frameRect.top() );
                        frameRect.setBottom( tabRect.top() + 7 );
                        slabs << SlabRect( frameRect, TileSet::Top );

                    } else {

                        QRect frameRect( r );
                        frameRect.setRight( r.left() + 7 );
                        frameRect.setTop( frameRect.top() - 7 );
                        frameRect.setBottom( tabRect.top() + 7 + 3 );
                        slabs << SlabRect( frameRect, TileSet::Right );
                    }

                    // bottom side
                    QRect frameRect( r );
                    frameRect.setRight( r.left() + 7 );
                    frameRect.setTop( tabRect.bottom() - 7 - 3 );
                    frameRect.setBottom( frameRect.bottom() + 7 );
                    slabs << SlabRect( frameRect, TileSet::Right );

                    // extra base, to extend below tabbar buttons
                    if( tabBar )
                    {
                        if( r.top() > tabBarRect.top() + 1 )
                        {

                            QRect frameRect( r );
                            frameRect.setTop( tabBarRect.top() - 7 + 1 );
                            frameRect.setBottom( r.top() + 7 - 1 );
                            frameRect.setRight( r.left() + 7 );
                            if( documentMode ) slabs << SlabRect( frameRect, TileSet::Right );
                            else slabs << SlabRect( frameRect, TileSet::TopRight );
                        }

                        if( r.bottom() < tabBarRect.bottom() - 1 )
                        {

                            QRect frameRect( r );
                            frameRect.setTop( r.bottom() - 7 + 1 );
                            if( hasRightCornerWidget && documentMode ) frameRect.setBottom( tabBarRect.bottom() + 7 - 1 );
                            else frameRect.setBottom( tabBarRect.bottom() + 7 );
                            frameRect.setRight( r.left() + 7 );
                            slabs << SlabRect( frameRect, TileSet::Right );

                        }
                    }

                } else {

                    // adjust sides when slab is adjacent to selected slab
                    if( isLeftOfSelected ) tabRect.setBottom( tabRect.bottom() + 2 );
                    else if( isRightOfSelected ) tabRect.setTop( tabRect.top() - 2 );

                    if( isFirst )
                    {

                        if( isFrameAligned ) path.moveTo( tabRect.topLeft() - QPoint( 2, 0 ) );
                        else path.moveTo( tabRect.topLeft() );
                        path.lineTo( tabRect.topRight() - QPointF( radius, 0 ) );
                        path.quadTo( tabRect.topRight(), tabRect.topRight() + QPoint( 0, radius ) );
                        path.lineTo( tabRect.bottomRight() );
                        path.lineTo( tabRect.bottomLeft() );

                    } else if( isLast ) {

                        path.moveTo( tabRect.topLeft() );
                        path.lineTo( tabRect.topRight() );
                        path.lineTo( tabRect.bottomRight() - QPointF( 0, radius ) );
                        path.quadTo( tabRect.bottomRight(), tabRect.bottomRight() - QPointF( radius, 0 ) );
                        path.lineTo( tabRect.bottomLeft() );

                    } else {

                        path.moveTo( tabRect.topLeft() );
                        path.lineTo( tabRect.topRight() );
                        path.lineTo( tabRect.bottomRight() );
                        path.lineTo( tabRect.bottomLeft() );

                    }

                    // highlight
                    QRect highlightRect( tabRect.left()-5, tabRect.top(), 7, tabRect.height() );
                    if( isFrameAligned && isFirst ) highlightSlab = SlabRect( highlightRect.adjusted( 0, -2, 0, 7 + 1 ), TileSet::TopRight );
                    else if( isFrameAligned && isLast ) highlightSlab = SlabRect( highlightRect.adjusted( 0, -7, 0, 2 ), TileSet::BottomRight );
                    else highlightSlab = SlabRect( highlightRect.adjusted( 0, -7 + 1, 0, 7 + 1 ), TileSet::Right );

                }

                break;
            }

            default: break;
        }

        const QColor color( palette.color( QPalette::Window ) );

        // render connections to frame
        // extra care must be taken care of so that no slab
        // extends beyond tabWidget frame, if any
        const QRect tabWidgetRect( tabWidget ?
            insideMargin( tabWidget->rect(), 0 ).translated( -widget->geometry().topLeft() ) :
            QRect() );

        foreach( SlabRect slab, slabs ) // krazy:exclude=foreach
        {
            adjustSlabRect( slab, tabWidgetRect, documentMode, verticalTabs );
            renderSlab( painter, slab, color, NoFill );
        }

        //  adjust clip rect and render tab
        if( tabBar )
        {
            painter->save();
            painter->setClipRegion( tabBarClipRegion( tabBar ) );
        }

        // fill tab
        if( selected )
        {

            // render window background in case of dragged tabwidget
            if( isDragged ) fillTabBackground( painter, tabRect, color, tabOption->shape, widget );

            // slab options
            StyleOptions selectedTabOptions( NoFill );
            TileSet::Tiles tiles( tilesByShape( tabOption->shape ) );
            renderSlab( painter, tabRect, color, selectedTabOptions, tiles );
            fillTab( painter, tabRect, color, tabOption->shape, selected );

        } else {

            const QColor backgroundColor = _helper->backgroundColor( color, widget, r.center() );
            const QColor midColor = _helper->alphaColor( _helper->calcDarkColor( backgroundColor ), 0.4 );
            const QColor darkColor = _helper->alphaColor( _helper->calcDarkColor( backgroundColor ), 0.6 );

            painter->save();
            painter->translate( 0.5, 0.5 );
            painter->setRenderHints( QPainter::Antialiasing );
            painter->setPen( darkColor );
            painter->setBrush( midColor );
            painter->drawPath( path );
            painter->restore();

        }

        // restore clip region
        if( tabBar ) painter->restore();

        // hovered highlight
        if( ( animated || mouseOver ) && highlightSlab._r.isValid() )
        {

            const qreal opacity( _animations->tabBarEngine().opacity( widget, r.topLeft() ) );
            const StyleOptions hoverTabOptions( NoFill | Hover );
            adjustSlabRect( highlightSlab, tabWidgetRect, documentMode, verticalTabs );

            // pass an invalid color to have only the glow painted
            if( animated ) renderSlab( painter, highlightSlab, QColor(), hoverTabOptions, opacity, AnimationHover );
            else renderSlab( painter, highlightSlab, QColor(), hoverTabOptions );

        }


        return true;

    }

    //___________________________________________________________________________________
    bool Style::drawToolBarControl( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        const QRect& r( option->rect );

        // when timeLine is running draw border event if not hovered
        const bool toolBarAnimated( _animations->toolBarEngine().isFollowMouseAnimated( widget ) );
        const QRect animatedRect( _animations->toolBarEngine().animatedRect( widget ) );
        const bool toolBarIntersected( toolBarAnimated && animatedRect.intersects( r ) );
        if( toolBarIntersected )
        { _helper->slitFocused( _helper->viewFocusBrush().brush( QPalette::Active ).color() )->render( animatedRect, painter ); }

        // draw nothing otherwise ( toolbars are transparent )

        return true;

    }

    //___________________________________________________________________________________
    bool Style::drawToolBoxTabLabelControl( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        const QStyleOptionToolBox* toolBoxOption( qstyleoption_cast<const QStyleOptionToolBox *>( option ) );
        const bool enabled( toolBoxOption->state & State_Enabled );
        const bool selected( toolBoxOption->state & State_Selected );
        QPixmap pm(
            toolBoxOption->icon.pixmap( pixelMetric( QStyle::PM_SmallIconSize, toolBoxOption, widget ),
            enabled ? QIcon::Normal : QIcon::Disabled ) );

        const QRect cr( toolBoxTabContentsRect( toolBoxOption, widget ) );
        QRect tr;
        QRect ir;
        int ih( 0 );

        if( pm.isNull() )  tr = cr.adjusted( -1, 0, -8, 0 );
        else {

            int iw = pm.width() + 4;
            ih = pm.height();
            ir = QRect( cr.left() - 1, cr.top(), iw + 2, ih );
            tr = QRect( ir.right(), cr.top(), cr.width() - ir.right() - 4, cr.height() );

        }

        if( selected )
        {
            QFont f( painter->font() );
            f.setBold( true );
            painter->setFont( f );
        }

        QString txt( toolBoxOption->fontMetrics.elidedText( toolBoxOption->text, Qt::ElideRight, tr.width() ) );

        if( ih ) painter->drawPixmap( ir.left(), ( toolBoxOption->rect.height() - ih ) / 2, pm );

        int alignment( Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic );
        drawItemText( painter, tr, alignment, toolBoxOption->palette, enabled, txt, QPalette::WindowText );

        return true;
    }

    //___________________________________________________________________________________
    bool Style::drawToolBoxTabShapeControl( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {
        // copy rect and palette
        const QRect& rect( option->rect );
        const QPalette& palette( option->palette );

        // copy state
        const State& state( option->state );
        const bool enabled( state & State_Enabled );
        const bool selected( state & State_Selected );
        const bool mouseOver( enabled && !selected && ( state & State_MouseOver ) );
        const bool reverseLayout( option->direction == Qt::RightToLeft );

        // cast option and check
        const QStyleOptionToolBoxV2 *v2 = qstyleoption_cast<const QStyleOptionToolBoxV2 *>( option );
        if( v2 && v2->position == QStyleOptionToolBoxV2::Beginning && selected ) return true;


        /*
        the proper widget ( the toolbox tab ) is not passed as argument by Qt.
        What is passed is the toolbox directly. To implement animations properly,
        the painter->device() is used instead
        */
        bool animated( false );
        qreal opacity( AnimationData::OpacityInvalid );
        if( enabled )
        {
            // try retrieve button from painter device.
            if( QPaintDevice* device = painter->device() )
            {
                _animations->toolBoxEngine().updateState( device, mouseOver );
                animated = _animations->toolBoxEngine().isAnimated( device );
                opacity = _animations->toolBoxEngine().opacity( device );
            }

        }

        // save colors for shadow
        /* important: option returns a wrong color. We use the widget's palette when widget is set */
        const QColor color( widget ? widget->palette().color( widget->backgroundRole() ) : palette.color( QPalette::Window ) );
        const QColor dark( _helper->calcDarkColor( color ) );
        QList<QColor> colors;
        colors.push_back( _helper->calcLightColor( color ) );

        if( mouseOver || animated )
        {

            QColor highlight = _helper->viewHoverBrush().brush( palette ).color();
            if( animated )
            {

                colors.push_back( KColorUtils::mix( dark, highlight, opacity ) );
                colors.push_back( _helper->alphaColor( highlight, 0.2*opacity ) );

            } else {

                colors.push_back( highlight );
                colors.push_back( _helper->alphaColor( highlight, 0.2 ) );

            }

        } else colors.push_back( dark );

        // create path
        painter->save();
        QPainterPath path;
        const int y( rect.height()*15/100 );
        if( reverseLayout )
        {

            path.moveTo( rect.left()+52, rect.top() );
            path.cubicTo( QPointF( rect.left()+50-8, rect.top() ), QPointF( rect.left()+50-10, rect.top()+y ), QPointF( rect.left()+50-10, rect.top()+y ) );
            path.lineTo( rect.left()+18+9, rect.bottom()-y );
            path.cubicTo( QPointF( rect.left()+18+9, rect.bottom()-y ), QPointF( rect.left()+19+6, rect.bottom()-1-0.3 ), QPointF( rect.left()+19, rect.bottom()-1-0.3 ) );
            painter->setClipRect( QRect( rect.left()+21, rect.top(), 28, rect.height() ) );

        } else {

            path.moveTo( rect.right()-52, rect.top() );
            path.cubicTo( QPointF( rect.right()-50+8, rect.top() ), QPointF( rect.right()-50+10, rect.top()+y ), QPointF( rect.right()-50+10, rect.top()+y ) );
            path.lineTo( rect.right()-18-9, rect.bottom()-y );
            path.cubicTo( QPointF( rect.right()-18-9, rect.bottom()-y ), QPointF( rect.right()-19-6, rect.bottom()-1-0.3 ), QPointF( rect.right()-19, rect.bottom()-1-0.3 ) );
            painter->setClipRect( QRect( rect.right()-48, rect.top(), 32, rect.height() ) );

        }


        // paint
        painter->setRenderHint( QPainter::Antialiasing, true );
        painter->translate( 0,2 );
        foreach( const QColor& color, colors )
        {
            painter->setPen( color );
            painter->drawPath( path );
            painter->translate( 0,-1 );
        }
        painter->restore();

        painter->save();
        painter->setRenderHint( QPainter::Antialiasing, false );
        painter->translate( 0,2 );
        foreach( const QColor& color, colors )
        {
            painter->setPen( color );
            if( reverseLayout ) {
                painter->drawLine( rect.left()+50-1, rect.top(), rect.right(), rect.top() );
                painter->drawLine( rect.left()+20, rect.bottom()-2, rect.left(), rect.bottom()-2 );
            } else {
                painter->drawLine( rect.left(), rect.top(), rect.right()-50+1, rect.top() );
                painter->drawLine( rect.right()-20, rect.bottom()-2, rect.right(), rect.bottom()-2 );
            }
            painter->translate( 0,-1 );
        }

        painter->restore();
        return true;

    }

    //___________________________________________________________________________________
    bool Style::drawToolButtonLabelControl( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {

        // cast option and check
        const QStyleOptionToolButton* toolButtonOption( qstyleoption_cast<const QStyleOptionToolButton*>(option) );

        // copy rect and palette
        const QRect& rect = option->rect;
        const QPalette& palette = option->palette;

        // state
        const State& state( option->state );
        const bool enabled( state & State_Enabled );
        const bool sunken( ( state & State_On ) || ( state & State_Sunken ) );
        const bool mouseOver( enabled && (option->state & State_MouseOver) );
        const bool flat( state & State_AutoRaise );

        const bool hasArrow( toolButtonOption->features & QStyleOptionToolButton::Arrow );
        const bool hasIcon( !( hasArrow || toolButtonOption->icon.isNull() ) );
        const bool hasText( !toolButtonOption->text.isEmpty() );

        // icon size
        const QSize iconSize( toolButtonOption->iconSize );

        // text size
        int textFlags( _mnemonics->textFlags() );
        const QSize textSize( option->fontMetrics.size( textFlags, toolButtonOption->text ) );

        // adjust text and icon rect based on options
        QRect iconRect;
        QRect textRect;

        if( hasText && ( !(hasArrow||hasIcon) || toolButtonOption->toolButtonStyle == Qt::ToolButtonTextOnly ) )
        {

            // text only
            textRect = rect;
            textFlags |= Qt::AlignCenter;

        } else if( (hasArrow||hasIcon) && (!hasText || toolButtonOption->toolButtonStyle == Qt::ToolButtonIconOnly ) ) {

            // icon only
            iconRect = rect;

        } else if( toolButtonOption->toolButtonStyle == Qt::ToolButtonTextUnderIcon ) {

            const int contentsHeight( iconSize.height() + textSize.height() + Metrics::ToolButton_ItemSpacing );
            iconRect = QRect( QPoint( rect.left() + (rect.width() - iconSize.width())/2, rect.top() + (rect.height() - contentsHeight)/2 ), iconSize );
            textRect = QRect( QPoint( rect.left() + (rect.width() - textSize.width())/2, iconRect.bottom() + Metrics::ToolButton_ItemSpacing + 1 ), textSize );
            textFlags |= Qt::AlignCenter;

        } else {

            const int contentsWidth( iconSize.width() + textSize.width() + Metrics::ToolButton_ItemSpacing );
            iconRect = QRect( QPoint( rect.left() + (rect.width() - contentsWidth )/2, rect.top() + (rect.height() - iconSize.height())/2 ), iconSize );
            textRect = QRect( QPoint( iconRect.right() + Metrics::ToolButton_ItemSpacing + 1, rect.top() + (rect.height() - textSize.height())/2 ), textSize );

            // handle right to left layouts
            iconRect = visualRect( option, iconRect );
            textRect = visualRect( option, textRect );

            textFlags |= Qt::AlignLeft | Qt::AlignVCenter;

        }

        // make sure there is enough room for icon
        if( iconRect.isValid() ) iconRect = centerRect( iconRect, iconSize );

        // render arrow or icon
        if( hasArrow && iconRect.isValid() )
        {

            QStyleOptionToolButton copy( *toolButtonOption );
            copy.rect = iconRect;
            switch( toolButtonOption->arrowType )
            {
                case Qt::LeftArrow: drawPrimitive( PE_IndicatorArrowLeft, &copy, painter, widget ); break;
                case Qt::RightArrow: drawPrimitive( PE_IndicatorArrowRight, &copy, painter, widget ); break;
                case Qt::UpArrow: drawPrimitive( PE_IndicatorArrowUp, &copy, painter, widget ); break;
                case Qt::DownArrow: drawPrimitive( PE_IndicatorArrowDown, &copy, painter, widget ); break;
                default: break;
            }

        } else if( hasIcon && iconRect.isValid() ) {

            // icon state and mode
            const QIcon::State iconState( sunken ? QIcon::On : QIcon::Off );
            QIcon::Mode iconMode;
            if( !enabled ) iconMode = QIcon::Disabled;
            else if( mouseOver && flat ) iconMode = QIcon::Active;
            else iconMode = QIcon::Normal;

            const QPixmap pixmap = toolButtonOption->icon.pixmap( iconSize, iconMode, iconState );
            drawItemPixmap( painter, iconRect, Qt::AlignCenter, pixmap );

        }

        // render text
        if( hasText && textRect.isValid() )
        {

            const QPalette::ColorRole textRole( flat ? QPalette::WindowText: QPalette::ButtonText );

            painter->setFont(toolButtonOption->font);
            drawItemText( painter, textRect, textFlags, palette, enabled, toolButtonOption->text, textRole );

        }

        return true;

    }

    //______________________________________________________________
    bool Style::drawComboBoxComplexControl( const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget ) const
    {


        // cast option and check
        const QStyleOptionComboBox* comboBoxOption( qstyleoption_cast<const QStyleOptionComboBox*>( option ) );
        if( !comboBoxOption ) return true;

        // rect and palette
        const QRect& rect( option->rect );
        const QPalette& palette( option->palette );

        // state
        const State& state( option->state );
        const bool enabled( state & State_Enabled );
        const bool mouseOver( enabled && ( state & State_MouseOver ) );
        const bool hasFocus( state & State_HasFocus );
        const bool editable( comboBoxOption->editable );
        const bool sunken( state & (State_On|State_Sunken) );
        const bool flat( !comboBoxOption->frame );

        // frame
        if( comboBoxOption->subControls & SC_ComboBoxFrame )
        {


            // style options
            StyleOptions styleOptions = 0;
            if( mouseOver ) styleOptions |= Hover;
            if( hasFocus ) styleOptions |= Focus;
            if( sunken && !editable ) styleOptions |= Sunken;

            const QColor inputColor( palette.color( QPalette::Base ) );
            const QRect editField( subControlRect( CC_ComboBox, comboBoxOption, SC_ComboBoxEditField, widget ) );

            if( editable )
            {

                // editable combobox. Make it look like a LineEdit
                // focus takes precedence over hover
                _animations->lineEditEngine().updateState( widget, AnimationFocus, hasFocus );
                _animations->lineEditEngine().updateState( widget, AnimationHover, mouseOver && !hasFocus );

                // input area
                painter->save();
                painter->setRenderHint( QPainter::Antialiasing );
                painter->setPen( Qt::NoPen );
                painter->setBrush( inputColor );

                if( flat )
                {

                    // adjust rect to match frameLess editors
                    painter->fillRect( rect, inputColor );
                    painter->restore();

                } else {

                    _helper->fillHole( *painter, rect );
                    painter->restore();

                    HoleOptions options( 0 );
                    if( hasFocus && enabled ) options |= HoleFocus;
                    if( mouseOver && enabled ) options |= HoleHover;

                    const QColor color( palette.color( QPalette::Window ) );
                    if( enabled && _animations->lineEditEngine().isAnimated( widget, AnimationFocus ) )
                    {

                        _helper->renderHole( painter, color, rect, options, _animations->lineEditEngine().opacity( widget, AnimationFocus ), AnimationFocus, TileSet::Ring );

                    } else if( enabled && _animations->lineEditEngine().isAnimated( widget, AnimationHover ) ) {

                        _helper->renderHole( painter, color, rect, options, _animations->lineEditEngine().opacity( widget, AnimationHover ), AnimationHover, TileSet::Ring );

                    } else {

                        _helper->renderHole( painter, color, rect, options );

                    }

                }

            } else {

                // non editable combobox. Make it look like a PushButton
                // hover takes precedence over focus
                _animations->lineEditEngine().updateState( widget, AnimationHover, mouseOver );
                _animations->lineEditEngine().updateState( widget, AnimationFocus, hasFocus && !mouseOver );

                // store animation state
                const bool hoverAnimated( _animations->lineEditEngine().isAnimated( widget, AnimationHover ) );
                const bool focusAnimated( _animations->lineEditEngine().isAnimated( widget, AnimationFocus ) );
                const qreal hoverOpacity( _animations->lineEditEngine().opacity( widget, AnimationHover ) );
                const qreal focusOpacity( _animations->lineEditEngine().opacity( widget, AnimationFocus ) );

                // blend button color to the background
                const QColor buttonColor( _helper->backgroundColor( palette.color( QPalette::Button ), widget, rect.center() ) );
                const QRect slabRect( rect );

                if( flat )
                {

                    if( !( styleOptions & Sunken ) )
                    {
                        // hover rect
                        if( enabled && hoverAnimated )
                        {

                            QColor glow( _helper->alphaColor( _helper->viewFocusBrush().brush( QPalette::Active ).color(), hoverOpacity ) );
                            _helper->slitFocused( glow )->render( rect, painter );

                        } else if( mouseOver ) {

                            _helper->slitFocused( _helper->viewFocusBrush().brush( QPalette::Active ).color() )->render( rect, painter );

                        }

                    } else {

                        HoleOptions options( HoleContrast );
                        if( mouseOver && enabled ) options |= HoleHover;

                        // flat pressed-down buttons do not get focus effect,
                        // consistently with tool buttons
                        if( enabled && hoverAnimated )
                        {

                            _helper->renderHole( painter, palette.color( QPalette::Window ), rect, options, hoverOpacity, AnimationHover, TileSet::Ring );

                        } else {

                            _helper->renderHole( painter, palette.color( QPalette::Window ), rect, options );

                        }

                    }

                } else {

                    if( enabled && hoverAnimated )
                    {

                        renderButtonSlab( painter, slabRect, buttonColor, styleOptions, hoverOpacity, AnimationHover, TileSet::Ring );

                    } else if( enabled && focusAnimated ) {

                        renderButtonSlab( painter, slabRect, buttonColor, styleOptions, focusOpacity, AnimationFocus, TileSet::Ring );

                    } else {

                        renderButtonSlab( painter, slabRect, buttonColor, styleOptions );

                    }

                }

            }

        }

        if( comboBoxOption->subControls & SC_ComboBoxArrow )
        {

            const QComboBox* comboBox = qobject_cast<const QComboBox*>( widget );
            const bool empty( comboBox && !comboBox->count() );

            QColor color;
            QColor background;
            bool drawContrast( true );

            if( comboBoxOption->editable )
            {

                if( enabled && empty ) color = palette.color( QPalette::Disabled,  QPalette::Text );
                else {

                    // check animation state
                    const bool subControlHover( enabled && mouseOver && comboBoxOption->activeSubControls&SC_ComboBoxArrow );
                    _animations->comboBoxEngine().updateState( widget, AnimationHover, subControlHover  );

                    const bool animated( enabled && _animations->comboBoxEngine().isAnimated( widget, AnimationHover ) );
                    const qreal opacity( _animations->comboBoxEngine().opacity( widget, AnimationHover ) );

                    if( animated )
                    {

                        QColor highlight = _helper->viewHoverBrush().brush( palette ).color();
                        color = KColorUtils::mix( palette.color( QPalette::Text ), highlight, opacity );

                    } else if( subControlHover ) {

                        color = _helper->viewHoverBrush().brush( palette ).color();

                    } else {

                        color = palette.color( QPalette::Text );

                    }

                }

                background = palette.color( QPalette::Background );

                if( enabled ) drawContrast = false;

            } else {

                // foreground color
                const QPalette::ColorRole role( flat ? QPalette::WindowText : QPalette::ButtonText );
                if( enabled && empty ) color = palette.color( QPalette::Disabled,  role );
                else color  = palette.color( role );

                // background color
                background = palette.color( flat ? QPalette::Window : QPalette::Button );

            }

            // draw the arrow
            QRect arrowRect = comboBoxSubControlRect( option, SC_ComboBoxArrow, widget );
            const QPolygonF arrow( genericArrow( ArrowDown, ArrowNormal ) );
            const qreal penThickness = 1.6;

            painter->save();
            painter->translate( QRectF( arrowRect ).center() );
            painter->setRenderHint( QPainter::Antialiasing );

            if( drawContrast )
            {

                const qreal offset( qMin( penThickness, qreal( 1.0 ) ) );
                painter->translate( 0,offset );
                painter->setPen( QPen( _helper->calcLightColor( palette.color( QPalette::Window ) ), penThickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
                painter->drawPolyline( arrow );
                painter->translate( 0,-offset );

            }

            painter->setPen( QPen( _helper->decoColor( background, color ) , penThickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
            painter->drawPolyline( arrow );
            painter->restore();

        }

        return true;

    }

    //______________________________________________________________
    bool Style::drawDialComplexControl( const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget ) const
    {

        const State& state( option->state );
        const bool enabled = state & State_Enabled;
        const bool mouseOver( enabled && ( state & State_MouseOver ) );
        const bool hasFocus( enabled && ( state & State_HasFocus ) );
        const bool sunken( state & ( State_On|State_Sunken ) );

        StyleOptions styleOptions = 0;
        if( sunken ) styleOptions |= Sunken;
        if( hasFocus ) styleOptions |= Focus;
        if( mouseOver ) styleOptions |= Hover;

        // mouseOver has precedence over focus
        _animations->widgetStateEngine().updateState( widget, AnimationHover, mouseOver );
        _animations->widgetStateEngine().updateState( widget, AnimationFocus, hasFocus && !mouseOver );

        const QRect rect( option->rect );
        const QPalette &palette( option->palette );
        const QColor buttonColor( _helper->backgroundColor( palette.color( QPalette::Button ), widget, rect.center() ) );

        if( enabled && _animations->widgetStateEngine().isAnimated( widget, AnimationHover ) && !( styleOptions & Sunken ) )
        {

            qreal opacity( _animations->widgetStateEngine().opacity( widget, AnimationHover ) );
            renderDialSlab( painter, rect, buttonColor, option, styleOptions, opacity, AnimationHover );

        } else if( enabled && !mouseOver && _animations->widgetStateEngine().isAnimated( widget, AnimationFocus ) && !( styleOptions & Sunken ) ) {

            qreal opacity( _animations->widgetStateEngine().opacity( widget, AnimationFocus ) );
            renderDialSlab( painter, rect, buttonColor, option, styleOptions, opacity, AnimationFocus );

        } else {

            renderDialSlab( painter, rect, buttonColor, option, styleOptions );

        }

        return true;

    }

    //______________________________________________________________
    bool Style::drawScrollBarComplexControl( const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget ) const
    {

        // render full groove directly, rather than using the addPage and subPage control element methods
        if( option->subControls & SC_ScrollBarGroove )
        {
            // retrieve groove rectangle
            QRect grooveRect( subControlRect( CC_ScrollBar, option, SC_ScrollBarGroove, widget ) );

            const QPalette& palette( option->palette );
            const QColor color( palette.color( QPalette::Window ) );
            const State& state( option->state );
            const bool horizontal( state & State_Horizontal );

            if( horizontal ) grooveRect = centerRect( grooveRect, grooveRect.width(), StyleConfigData::scrollBarWidth() );
            else grooveRect = centerRect( grooveRect, StyleConfigData::scrollBarWidth(), grooveRect.height() );

            // render
            renderScrollBarHole( painter, grooveRect, color, Qt::Horizontal );


        }

        // call base class primitive
        ParentStyleClass::drawComplexControl( CC_ScrollBar, option, painter, widget );
        return true;
    }

    //______________________________________________________________
    bool Style::drawSliderComplexControl( const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget ) const
    {

        // cast option and check
        const QStyleOptionSlider *sliderOption( qstyleoption_cast<const QStyleOptionSlider *>( option ) );
        if( !sliderOption ) return true;


        // copy rect and palette
        const QRect& rect( option->rect );
        const QPalette& palette( option->palette );

        // copy state
        const State& state( option->state );
        const bool enabled( state & State_Enabled );
        const bool mouseOver( enabled && ( state & State_MouseOver ) );
        const bool hasFocus( state & State_HasFocus );

        // direction
        const bool horizontal( sliderOption->orientation == Qt::Horizontal );

        if( sliderOption->subControls & SC_SliderTickmarks )
        {
            const bool upsideDown( sliderOption->upsideDown );
            const int tickPosition( sliderOption->tickPosition );
            const int available( pixelMetric( PM_SliderSpaceAvailable, option, widget ) );
            int interval = sliderOption->tickInterval;
            if( interval < 1 ) interval = sliderOption->pageStep;
            if( interval >= 1 )
            {
                const int fudge( pixelMetric( PM_SliderLength, option, widget ) / 2 );
                int current( sliderOption->minimum );

                // store tick lines
                const QRect grooveRect( subControlRect( CC_Slider, sliderOption, SC_SliderGroove, widget ) );
                QList<QLine> tickLines;
                if( horizontal )
                {

                    if( tickPosition & QSlider::TicksAbove ) tickLines.append( QLine( rect.left(), grooveRect.top() - Metrics::Slider_TickMarginWidth, rect.left(), grooveRect.top() - Metrics::Slider_TickMarginWidth - Metrics::Slider_TickLength ) );
                    if( tickPosition & QSlider::TicksBelow ) tickLines.append( QLine( rect.left(), grooveRect.bottom() + Metrics::Slider_TickMarginWidth, rect.left(), grooveRect.bottom() + Metrics::Slider_TickMarginWidth + Metrics::Slider_TickLength ) );

                } else {

                    if( tickPosition & QSlider::TicksAbove ) tickLines.append( QLine( grooveRect.left() - Metrics::Slider_TickMarginWidth, rect.top(), grooveRect.left() - Metrics::Slider_TickMarginWidth - Metrics::Slider_TickLength, rect.top() ) );
                    if( tickPosition & QSlider::TicksBelow ) tickLines.append( QLine( grooveRect.right() + Metrics::Slider_TickMarginWidth, rect.top(), grooveRect.right() + Metrics::Slider_TickMarginWidth + Metrics::Slider_TickLength, rect.top() ) );

                }

                // colors
                QColor base( _helper->backgroundColor( palette.color( QPalette::Window ), widget, rect.center() ) );
                base = _helper->calcDarkColor( base );
                painter->setPen( base );

                while( current <= sliderOption->maximum )
                {

                    // calculate positions and draw lines
                    int position( sliderPositionFromValue( sliderOption->minimum, sliderOption->maximum, current, available ) + fudge );
                    foreach( const QLine& tickLine, tickLines )
                    {
                        if( horizontal ) painter->drawLine( tickLine.translated( upsideDown ? (rect.width() - position) : position, 0 ) );
                        else painter->drawLine( tickLine.translated( 0, upsideDown ? (rect.height() - position):position ) );
                    }

                    // go to next position
                    current += interval;

                }
            }
        }

        // groove
        if( sliderOption->subControls & SC_SliderGroove )
        {
            // get rect
            QRect grooveRect( subControlRect( CC_Slider, sliderOption, SC_SliderGroove, widget ) );

            // render
            _helper->scrollHole( palette.color( QPalette::Window ), sliderOption->orientation, true )->render( grooveRect, painter, TileSet::Full );

        }

        // handle
        if( sliderOption->subControls & SC_SliderHandle )
        {

            // get rect and center
            QRect handleRect( subControlRect( CC_Slider, sliderOption, SC_SliderHandle, widget ) );

            const bool handleActive( sliderOption->activeSubControls & SC_SliderHandle );
            StyleOptions styleOptions( 0 );
            if( hasFocus ) styleOptions |= Focus;
            if( handleActive && mouseOver ) styleOptions |= Hover;

            _animations->sliderEngine().updateState( widget, enabled && handleActive );
            const qreal opacity( _animations->sliderEngine().opacity( widget ) );

            const QColor color( _helper->backgroundColor( palette.color( QPalette::Button ), widget, handleRect.center() ) );
            const QColor glow( slabShadowColor( styleOptions, opacity, AnimationHover ) );

            const bool sunken( state & (State_On|State_Sunken) );
            painter->drawPixmap( handleRect.topLeft(), _helper->sliderSlab( color, glow, sunken, 0.0 ) );

        }

        return true;
    }

    //______________________________________________________________
    bool Style::drawSpinBoxComplexControl( const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget ) const
    {

        // cast option and check
        const QStyleOptionSpinBox *spinBoxOption = qstyleoption_cast<const QStyleOptionSpinBox *>( option );
        if( !spinBoxOption ) return true;

        // copy rect and palette
        const QRect& rect( option->rect );
        const QPalette& palette( option->palette );

        // store state
        const State& state( option->state );
        const bool enabled( state & State_Enabled );
        const bool mouseOver( enabled && ( state & State_MouseOver ) );
        const bool hasFocus( state & State_HasFocus );
        const QColor inputColor( palette.color( QPalette::Base ) );

        if( spinBoxOption->subControls & SC_SpinBoxFrame )
        {

            painter->save();
            painter->setRenderHint( QPainter::Antialiasing );
            painter->setPen( Qt::NoPen );
            painter->setBrush( inputColor );

            if( !spinBoxOption->frame )
            {
                // frameless spinbox
                // frame is adjusted to have the same dimensions as a frameless editor
                painter->fillRect( rect, inputColor );
                painter->restore();

            } else {

                // normal spinbox
                _helper->fillHole( *painter, rect );
                painter->restore();

                HoleOptions options( 0 );
                if( hasFocus && enabled ) options |= HoleFocus;
                if( mouseOver && enabled ) options |= HoleHover;

                QColor local( palette.color( QPalette::Window ) );
                _animations->lineEditEngine().updateState( widget, AnimationHover, mouseOver );
                _animations->lineEditEngine().updateState( widget, AnimationFocus, hasFocus );
                if( enabled && _animations->lineEditEngine().isAnimated( widget, AnimationFocus ) )
                {

                    _helper->renderHole( painter, local, rect, options, _animations->lineEditEngine().opacity( widget, AnimationFocus ), AnimationFocus, TileSet::Ring );

                } else if( enabled && _animations->lineEditEngine().isAnimated( widget, AnimationHover ) ) {

                    _helper->renderHole( painter, local, rect, options, _animations->lineEditEngine().opacity( widget, AnimationHover ), AnimationHover, TileSet::Ring );

                } else {

                    _helper->renderHole( painter, local, rect, options );

                }

            }
        }

        if( spinBoxOption->subControls & SC_SpinBoxUp ) renderSpinBoxArrow( painter, spinBoxOption, widget, SC_SpinBoxUp );
        if( spinBoxOption->subControls & SC_SpinBoxDown ) renderSpinBoxArrow( painter, spinBoxOption, widget, SC_SpinBoxDown );

        return true;

    }

    //______________________________________________________________
    bool Style::drawTitleBarComplexControl( const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget ) const
    {
        const QStyleOptionTitleBar *tb( qstyleoption_cast<const QStyleOptionTitleBar *>( option ) );
        if( !tb ) return true;

        const State& state( option->state );
        const bool enabled( state & State_Enabled );
        const bool active( enabled && ( tb->titleBarState & Qt::WindowActive ) );

        // draw title text
        {
            QRect textRect = subControlRect( CC_TitleBar, tb, SC_TitleBarLabel, widget );

            // enable state transition
            _animations->widgetEnabilityEngine().updateState( widget, AnimationEnable, active );

            // make sure palette has the correct color group
            QPalette palette( option->palette );

            if( _animations->widgetEnabilityEngine().isAnimated( widget, AnimationEnable ) )
            { palette = _helper->mergePalettes( palette, _animations->widgetEnabilityEngine().opacity( widget, AnimationEnable )  ); }

            palette.setCurrentColorGroup( active ? QPalette::Active: QPalette::Disabled );
            ParentStyleClass::drawItemText( painter, textRect, Qt::AlignCenter, palette, active, tb->text, QPalette::WindowText );

        }


        // menu button
        if( ( tb->subControls & SC_TitleBarSysMenu ) && ( tb->titleBarFlags & Qt::WindowSystemMenuHint ) && !tb->icon.isNull() )
        {

            const QRect br = subControlRect( CC_TitleBar, tb, SC_TitleBarSysMenu, widget );
            tb->icon.paint( painter, br );

        }

        if( ( tb->subControls & SC_TitleBarMinButton ) && ( tb->titleBarFlags & Qt::WindowMinimizeButtonHint ) )
        { renderTitleBarButton( painter, tb, widget, SC_TitleBarMinButton ); }

        if( ( tb->subControls & SC_TitleBarMaxButton ) && ( tb->titleBarFlags & Qt::WindowMaximizeButtonHint ) )
        { renderTitleBarButton( painter, tb, widget, SC_TitleBarMaxButton ); }

        if( ( tb->subControls & SC_TitleBarCloseButton ) )
        { renderTitleBarButton( painter, tb, widget, SC_TitleBarCloseButton ); }

        if( ( tb->subControls & SC_TitleBarNormalButton ) &&
            ( ( ( tb->titleBarFlags & Qt::WindowMinimizeButtonHint ) &&
            ( tb->titleBarState & Qt::WindowMinimized ) ) ||
            ( ( tb->titleBarFlags & Qt::WindowMaximizeButtonHint ) &&
            ( tb->titleBarState & Qt::WindowMaximized ) ) ) )
        { renderTitleBarButton( painter, tb, widget, SC_TitleBarNormalButton ); }

        if( tb->subControls & SC_TitleBarShadeButton )
        { renderTitleBarButton( painter, tb, widget, SC_TitleBarShadeButton ); }

        if( tb->subControls & SC_TitleBarUnshadeButton )
        { renderTitleBarButton( painter, tb, widget, SC_TitleBarUnshadeButton ); }

        if( ( tb->subControls & SC_TitleBarContextHelpButton ) && ( tb->titleBarFlags & Qt::WindowContextHelpButtonHint ) )
        { renderTitleBarButton( painter, tb, widget, SC_TitleBarContextHelpButton ); }

        return true;
    }


    //______________________________________________________________
    bool Style::drawToolButtonComplexControl( const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget ) const
    {

        // check autoRaise state
        const State& state( option->state );
        const bool isInToolBar( widget && qobject_cast<QToolBar*>( widget->parent() ) );

        // get rect and palette
        const QRect& rect( option->rect );
        const QStyleOptionToolButton *toolButtonOption( qstyleoption_cast<const QStyleOptionToolButton *>( option ) );
        if( !toolButtonOption ) return true;

        const bool enabled( state & State_Enabled );
        const bool mouseOver( enabled && ( state & State_MouseOver ) );
        const bool hasFocus( enabled && ( state & State_HasFocus ) );
        const bool sunken( state & ( State_Sunken|State_On ) );
        const bool autoRaise( state & State_AutoRaise );

        if( isInToolBar )
        {

            _animations->widgetStateEngine().updateState( widget, AnimationHover, mouseOver );

        } else {

            // mouseOver has precedence over focus
            _animations->widgetStateEngine().updateState( widget, AnimationHover, mouseOver );
            _animations->widgetStateEngine().updateState( widget, AnimationFocus, hasFocus&&!mouseOver );

        }

        // toolbar animation
        QWidget* parent( widget ? widget->parentWidget():0 );
        const bool toolBarAnimated( isInToolBar && _animations->toolBarEngine().isAnimated( parent ) );
        const QRect animatedRect( _animations->toolBarEngine().animatedRect( parent ) );
        const QRect currentRect( _animations->toolBarEngine().currentRect( parent ) );
        const bool current( isInToolBar && currentRect.intersects( rect.translated( widget->mapToParent( QPoint( 0,0 ) ) ) ) );
        const bool toolBarTimerActive( isInToolBar && _animations->toolBarEngine().isTimerActive( widget->parentWidget() ) );

        // normal toolbutton animation
        const bool hoverAnimated( _animations->widgetStateEngine().isAnimated( widget, AnimationHover ) );
        const bool focusAnimated( _animations->widgetStateEngine().isAnimated( widget, AnimationFocus ) );

        // detect buttons in tabbar, for which special rendering is needed
        const bool inTabBar( widget && qobject_cast<const QTabBar*>( widget->parentWidget() ) );

        // local copy of option
        QStyleOptionToolButton copy( *toolButtonOption );

        const bool hasPopupMenu( toolButtonOption->subControls & SC_ToolButtonMenu );
        const bool hasInlineIndicator( toolButtonOption->features & QStyleOptionToolButton::HasMenu && !hasPopupMenu );

        const QRect buttonRect( subControlRect( CC_ToolButton, option, SC_ToolButton, widget ) );
        const QRect menuRect( subControlRect( CC_ToolButton, option, SC_ToolButtonMenu, widget ) );

        // frame
        const bool drawFrame(
            (enabled && !( mouseOver || hasFocus || sunken ) &&
            (hoverAnimated || ( focusAnimated && !hasFocus ) || ( ( ( toolBarAnimated && animatedRect.isNull() )||toolBarTimerActive ) && current )) ) ||
            (toolButtonOption->subControls & SC_ToolButton) );
        if( drawFrame )
        {
            copy.rect = buttonRect;
            if( inTabBar ) drawTabBarPanelButtonToolPrimitive( &copy, painter, widget );
            else drawPrimitive( PE_PanelButtonTool, &copy, painter, widget);
        }

        if( hasPopupMenu )
        {

            copy.rect = menuRect;
            if( !autoRaise )
            {
                drawPrimitive( PE_IndicatorButtonDropDown, &copy, painter, widget );
                copy.state &= ~(State_MouseOver|State_HasFocus);
            }

            drawPrimitive( PE_IndicatorArrowDown, &copy, painter, widget );

        } else if( hasInlineIndicator ) {

            copy.rect = menuRect;
            copy.state &= ~(State_MouseOver|State_HasFocus);
            drawPrimitive( PE_IndicatorArrowDown, &copy, painter, widget );

        }

        // label
        copy.state = state;
        copy.rect = buttonRect;
        drawControl( CE_ToolButtonLabel, &copy, painter, widget );

        return true;

    }

    //_____________________________________________________________________
    void Style::configurationChanged()
    {
        // reload config (reparses oxygenrc)
        StyleConfigData::self()->load();

        _shadowHelper->reparseCacheConfig();

        _helper->invalidateCaches();

        loadConfiguration();
    }

    //_____________________________________________________________________
    void Style::loadConfiguration()
    {
        // set helper configuration
        _helper->loadConfig();

        // background gradient
        _helper->setUseBackgroundGradient( StyleConfigData::useBackgroundGradient() );

        // background pixmap
        _helper->setBackgroundPixmap( StyleConfigData::backgroundPixmap() );

        // update top level window hints
        foreach( QWidget* widget, qApp->topLevelWidgets() )
        {
            // make sure widget has a valid WId
            if( !(widget->testAttribute(Qt::WA_WState_Created) || widget->internalWinId() ) ) continue;

            // make sure widget has a decoration
            if( !_helper->hasDecoration( widget ) ) continue;

            // update flags
            _helper->setHasBackgroundGradient( widget->winId(), true );
            _helper->setHasBackgroundPixmap( widget->winId(), _helper->hasBackgroundPixmap() );
        }

        // update caches size
        int cacheSize( StyleConfigData::cacheEnabled() ?
            StyleConfigData::maxCacheSize():0 );

        _helper->setMaxCacheSize( cacheSize );

        // always enable blur helper
        _blurHelper->setEnabled( true );

        // reinitialize engines
        _animations->setupEngines();
        _transitions->setupEngines();
        _windowManager->initialize();
        _shadowHelper->loadConfig();

        // mnemonics
        _mnemonics->setMode( StyleConfigData::mnemonicsMode() );

        // widget explorer
        _widgetExplorer->setEnabled( StyleConfigData::widgetExplorerEnabled() );
        _widgetExplorer->setDrawWidgetRects( StyleConfigData::drawWidgetRects() );

        // splitter proxy
        _splitterFactory->setEnabled( StyleConfigData::splitterProxyEnabled() );

        // scrollbar button dimentions.
        /* it has to be reinitialized here because scrollbar width might have changed */
        _noButtonHeight = 0;
        _singleButtonHeight = qMax( StyleConfigData::scrollBarWidth() * 7 / 10, 14 );
        _doubleButtonHeight = 2*_singleButtonHeight;

        // scrollbar buttons
        switch( StyleConfigData::scrollBarAddLineButtons() )
        {
            case 0: _addLineButtons = NoButton; break;
            case 1: _addLineButtons = SingleButton; break;

            default:
            case 2: _addLineButtons = DoubleButton; break;
        }

        switch( StyleConfigData::scrollBarSubLineButtons() )
        {
            case 0: _subLineButtons = NoButton; break;
            case 1: _subLineButtons = SingleButton; break;

            default:
            case 2: _subLineButtons = DoubleButton; break;
        }

        // frame focus
        if( StyleConfigData::viewDrawFocusIndicator() ) _frameFocusPrimitive = &Style::drawFrameFocusRectPrimitive;
        else _frameFocusPrimitive = &Style::emptyPrimitive;
    }

    //____________________________________________________________________
    QIcon Style::standardIconImplementation(
        StandardPixmap standardPixmap,
        const QStyleOption *option,
        const QWidget *widget ) const
    {

        // MDI windows buttons
        // get button color ( unfortunately option and widget might not be set )
        QColor buttonColor;
        QColor iconColor;
        if( option )
        {

            buttonColor = option->palette.window().color();
            iconColor   = option->palette.windowText().color();

        } else if( widget ) {

            buttonColor = widget->palette().window().color();
            iconColor   = widget->palette().windowText().color();

        } else if( qApp ) {

            // might not have a QApplication
            buttonColor = QPalette().window().color();
            iconColor   = QPalette().windowText().color();

        } else {

            // KCS is always safe
            buttonColor = KColorScheme( QPalette::Active, KColorScheme::Window, _helper->config() ).background().color();
            iconColor   = KColorScheme( QPalette::Active, KColorScheme::Window, _helper->config() ).foreground().color();

        }

        // contrast
        const QColor contrast( _helper->calcLightColor( buttonColor ) );

        switch( standardPixmap )
        {

            case SP_TitleBarNormalButton:
            {
                QPixmap pixmap(
                    pixelMetric( QStyle::PM_SmallIconSize, 0, 0 ),
                    pixelMetric( QStyle::PM_SmallIconSize, 0, 0 ) );

                pixmap.fill( Qt::transparent );

                QPainter painter( &pixmap );
                renderTitleBarButton( &painter, pixmap.rect(), buttonColor, iconColor, SC_TitleBarNormalButton );

                return QIcon( pixmap );

            }

            case SP_TitleBarShadeButton:
            {
                QPixmap pixmap(
                    pixelMetric( QStyle::PM_SmallIconSize, 0, 0 ),
                    pixelMetric( QStyle::PM_SmallIconSize, 0, 0 ) );

                pixmap.fill( Qt::transparent );
                QPainter painter( &pixmap );
                renderTitleBarButton( &painter, pixmap.rect(), buttonColor, iconColor, SC_TitleBarShadeButton );

                return QIcon( pixmap );
            }

            case SP_TitleBarUnshadeButton:
            {
                QPixmap pixmap(
                    pixelMetric( QStyle::PM_SmallIconSize, 0, 0 ),
                    pixelMetric( QStyle::PM_SmallIconSize, 0, 0 ) );

                pixmap.fill( Qt::transparent );
                QPainter painter( &pixmap );
                renderTitleBarButton( &painter, pixmap.rect(), buttonColor, iconColor, SC_TitleBarUnshadeButton );

                return QIcon( pixmap );
            }

            case SP_TitleBarCloseButton:
            case SP_DockWidgetCloseButton:
            {
                QPixmap pixmap(
                    pixelMetric( QStyle::PM_SmallIconSize, 0, 0 ),
                    pixelMetric( QStyle::PM_SmallIconSize, 0, 0 ) );

                pixmap.fill( Qt::transparent );
                QPainter painter( &pixmap );
                renderTitleBarButton( &painter, pixmap.rect(), buttonColor, iconColor, SC_TitleBarCloseButton );

                return QIcon( pixmap );

            }

            case SP_ToolBarHorizontalExtensionButton:
            {

                QPixmap pixmap( pixelMetric( QStyle::PM_SmallIconSize,0,0 ), pixelMetric( QStyle::PM_SmallIconSize,0,0 ) );
                pixmap.fill( Qt::transparent );
                QPainter painter( &pixmap );
                painter.setRenderHints( QPainter::Antialiasing );
                painter.setBrush( Qt::NoBrush );

                painter.translate( QRectF( pixmap.rect() ).center() );

                const bool reverseLayout( option && option->direction == Qt::RightToLeft );
                QPolygonF arrow = genericArrow( reverseLayout ? ArrowLeft:ArrowRight, ArrowTiny );

                const qreal width( 1.1 );
                painter.translate( 0, 0.5 );
                painter.setBrush( Qt::NoBrush );
                painter.setPen( QPen( _helper->calcLightColor( buttonColor ), width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
                painter.drawPolyline( arrow );

                painter.translate( 0,-1 );
                painter.setBrush( Qt::NoBrush );
                painter.setPen( QPen( iconColor, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
                painter.drawPolyline( arrow );

                return QIcon( pixmap );
            }

            case SP_ToolBarVerticalExtensionButton:
            {
                QPixmap pixmap( pixelMetric( QStyle::PM_SmallIconSize,0,0 ), pixelMetric( QStyle::PM_SmallIconSize,0,0 ) );
                pixmap.fill( Qt::transparent );
                QPainter painter( &pixmap );
                painter.setRenderHints( QPainter::Antialiasing );
                painter.setBrush( Qt::NoBrush );

                painter.translate( QRectF( pixmap.rect() ).center() );

                QPolygonF arrow = genericArrow( ArrowDown, ArrowTiny );

                const qreal width( 1.1 );
                painter.translate( 0, 0.5 );
                painter.setBrush( Qt::NoBrush );
                painter.setPen( QPen( _helper->calcLightColor( buttonColor ), width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
                painter.drawPolyline( arrow );

                painter.translate( 0,-1 );
                painter.setBrush( Qt::NoBrush );
                painter.setPen( QPen( iconColor, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
                painter.drawPolyline( arrow );

                return QIcon( pixmap );
            }

            default:
            // do not cache parent style icon, since it may change at runtime
            #if QT_VERSION >= 0x050000
            return  ParentStyleClass::standardIcon( standardPixmap, option, widget );
            #else
            return  ParentStyleClass::standardIconImplementation( standardPixmap, option, widget );
            #endif

        }
    }

    //______________________________________________________________
    void Style::polishScrollArea( QAbstractScrollArea* scrollArea ) const
    {

        if( !scrollArea ) return;

        // HACK: add exception for KPIM transactionItemView, which is an overlay widget
        // and must have filled background. This is a temporary workaround until a more
        // robust solution is found.
        if( scrollArea->inherits( "KPIM::TransactionItemView" ) )
        {
            // also need to make the scrollarea background plain ( using autofill background )
            // so that optional vertical scrollbar background is not transparent either.
            // TODO: possibly add an event filter to use the "normal" window background
            // instead of something flat.
            scrollArea->setAutoFillBackground( true );
            return;
        }

        // check frame style and background role
        if( !(scrollArea->frameShape() == QFrame::NoFrame || scrollArea->backgroundRole() == QPalette::Window ) )
        { return; }

        // get viewport and check background role
        QWidget* viewport( scrollArea->viewport() );
        if( !( viewport && viewport->backgroundRole() == QPalette::Window ) ) return;

        // change viewport autoFill background.
        // do the same for children if the background role is QPalette::Window
        viewport->setAutoFillBackground( false );
        QList<QWidget*> children( viewport->findChildren<QWidget*>() );
        foreach( QWidget* child, children )
        {
            if( child->parent() == viewport && child->backgroundRole() == QPalette::Window )
            { child->setAutoFillBackground( false ); }
        }

    }

    //_______________________________________________________________
    QRegion Style::tabBarClipRegion( const QTabBar* tabBar ) const
    {
        // need to mask-out arrow buttons, if visible.
        QRegion mask( tabBar->rect() );
        foreach( const QObject* child, tabBar->children() )
        {
            const QToolButton* toolButton( qobject_cast<const QToolButton*>( child ) );
            if( toolButton && toolButton->isVisible() ) mask -= toolButton->geometry();
        }

        return mask;

    }

    //_________________________________________________________________________________
    void Style::renderDialSlab( QPainter *painter, const QRect& r, const QColor &color, const QStyleOption *option, StyleOptions styleOptions, qreal opacity, AnimationMode mode ) const
    {

        // cast option
        const QStyleOptionSlider* sliderOption( qstyleoption_cast<const QStyleOptionSlider*>( option ) );
        if( !sliderOption ) return;

        // adjust rect to be square, and centered
        const int dimension( qMin( r.width(), r.height() ) );
        const QRect rect( centerRect( r, dimension, dimension ) );

        // calculate glow color
        const QColor glow( slabShadowColor( styleOptions, opacity, mode ) );

        // get main slab
        QPixmap pix( _helper->dialSlab( color, glow, 0.0, dimension ) );
        const qreal baseOffset( 3.5 );

        const QColor light( _helper->calcLightColor( color ) );
        const QColor shadow( _helper->calcShadowColor( color ) );

        QPainter p( &pix );
        p.setPen( Qt::NoPen );
        p.setRenderHints( QPainter::Antialiasing );

        // indicator
        const qreal angle( dialAngle( sliderOption, sliderOption->sliderPosition ) );
        QPointF center( pix.rect().center() );
        const int sliderWidth( dimension/6 );
        const qreal radius( 0.5*( dimension - 2*sliderWidth ) );
        center += QPointF( radius*cos( angle ), -radius*sin( angle ) );

        QRectF sliderRect( 0, 0, sliderWidth, sliderWidth );
        sliderRect.moveCenter( center );

        // outline circle
        const qreal offset( 0.3 );
        QLinearGradient lg( 0, baseOffset, 0, baseOffset + 2*sliderRect.height() );
        p.setBrush( light );
        p.setPen( Qt::NoPen );
        p.drawEllipse( sliderRect.translated( 0, offset ) );

        // mask
        p.setPen( Qt::NoPen );
        p.save();
        p.setCompositionMode( QPainter::CompositionMode_DestinationOut );
        p.setBrush( QBrush( Qt::black ) );
        p.drawEllipse( sliderRect );
        p.restore();

        // shadow
        p.translate( sliderRect.topLeft() );
        _helper->drawInverseShadow( p, shadow.darker( 200 ), 0.0, sliderRect.width(), 0.0 );

        // glow
        if( glow.isValid() ) _helper->drawInverseGlow( p, glow, 0.0, sliderRect.width(),  sliderRect.width() );

        p.end();

        painter->drawPixmap( rect.topLeft(), pix );

        return;

    }

    //____________________________________________________________________________________
    void Style::renderButtonSlab( QPainter *painter, QRect rect, const QColor &color, StyleOptions options, qreal opacity,
        AnimationMode mode,
        TileSet::Tiles tiles ) const
    {

        // check rect
        if( !rect.isValid() ) return;

        // edges
        // for slabs, hover takes precedence over focus ( other way around for holes )
        // but in any case if the button is sunken we don't show focus nor hover
        TileSet *tile(0L);
        if( options & Sunken )
        {
            tile = _helper->slabSunken( color );

        } else {

            QColor glow = slabShadowColor( options, opacity, mode );
            tile = _helper->slab( color, glow, 0.0 );

        }

        // adjust rect to account for missing tiles
        if( tile ) rect = tile->adjust( rect, tiles );

        // fill
        if( !( options & NoFill ) ) _helper->fillButtonSlab( *painter, rect, color, options&Sunken );

        // render slab
        if( tile ) tile->render( rect, painter, tiles );

    }

    //____________________________________________________________________________________
    void Style::renderSlab(
        QPainter *painter, QRect rect,
        const QColor &color,
        StyleOptions options, qreal opacity,
        AnimationMode mode,
        TileSet::Tiles tiles ) const
    {

        // check rect
        if( !rect.isValid() ) return;

        // fill
        if( !( options & NoFill ) )
        {
            painter->save();
            painter->setRenderHint( QPainter::Antialiasing );
            painter->setPen( Qt::NoPen );

            if( _helper->calcShadowColor( color ).value() > color.value() && ( options & Sunken ) )
            {

                QLinearGradient innerGradient( 0, rect.top(), 0, rect.bottom() + rect.height() );
                innerGradient.setColorAt( 0.0, color );
                innerGradient.setColorAt( 1.0, _helper->calcLightColor( color ) );
                painter->setBrush( innerGradient );

            } else {

                QLinearGradient innerGradient( 0, rect.top() - rect.height(), 0, rect.bottom() );
                innerGradient.setColorAt( 0.0, _helper->calcLightColor( color ) );
                innerGradient.setColorAt( 1.0, color );
                painter->setBrush( innerGradient );

            }

            _helper->fillSlab( *painter, rect );

            painter->restore();
        }

        // edges
        // for slabs, hover takes precedence over focus ( other way around for holes )
        // but in any case if the button is sunken we don't show focus nor hover
        TileSet *tile( 0 );
        if( ( options & Sunken ) && color.isValid() )
        {
            tile = _helper->slabSunken( color );

        } else {

            // calculate proper glow color based on current settings and opacity
            const QColor glow( slabShadowColor( options, opacity, mode ) );
            if( color.isValid() || glow.isValid() ) tile = _helper->slab( color, glow , 0.0 );
            else return;

        }

        // render tileset
        if( tile ) tile->render( rect, painter, tiles );

    }

    //______________________________________________________________________________________________________________________________
    void Style::fillTabBackground( QPainter* painter, const QRect &r, const QColor &color, QTabBar::Shape shape, const QWidget* widget ) const
    {

        // filling
        QRect fillRect( r );
        switch( shape )
        {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
            fillRect.adjust( 4, 4, -4, -6 );
            break;

            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
            fillRect.adjust( 4, 4, -4, -4 );
            break;

            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
            fillRect.adjust( 4, 3, -5, -5 );
            break;

            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
            fillRect.adjust( 5, 3, -4, -5 );
            break;

            default: return;

        }

        if( widget ) _helper->renderWindowBackground( painter, fillRect, widget, color );
        else painter->fillRect( fillRect, color );

    }

    //______________________________________________________________________________________________________________________________
    void Style::fillTab( QPainter* painter, const QRect &r, const QColor &color, QTabBar::Shape shape, bool active ) const
    {

        const QColor dark( _helper->calcDarkColor( color ) );
        const QColor shadow( _helper->calcShadowColor( color ) );
        const QColor light( _helper->calcLightColor( color ) );
        const QRect fillRect( r.adjusted( 4, 3,-4,-5 ) );

        QLinearGradient highlight;
        switch( shape )
        {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
            highlight = QLinearGradient( fillRect.topLeft(), fillRect.bottomLeft() );
            break;

            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
            highlight = QLinearGradient( fillRect.bottomLeft(), fillRect.topLeft() );
            break;

            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
            highlight = QLinearGradient( fillRect.topRight(), fillRect.topLeft() );
            break;

            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
            highlight = QLinearGradient( fillRect.topLeft(), fillRect.topRight() );
            break;

            default: return;

        }

        if( active ) {

            highlight.setColorAt( 0.0, _helper->alphaColor( light, 0.5 ) );
            highlight.setColorAt( 0.1, _helper->alphaColor( light, 0.5 ) );
            highlight.setColorAt( 0.25, _helper->alphaColor( light, 0.3 ) );
            highlight.setColorAt( 0.5, _helper->alphaColor( light, 0.2 ) );
            highlight.setColorAt( 0.75, _helper->alphaColor( light, 0.1 ) );
            highlight.setColorAt( 0.9, Qt::transparent );

        } else {

            // inactive
            highlight.setColorAt( 0.0, _helper->alphaColor( light, 0.1 ) );
            highlight.setColorAt( 0.4, _helper->alphaColor( dark, 0.5 ) );
            highlight.setColorAt( 0.8, _helper->alphaColor( dark, 0.4 ) );
            highlight.setColorAt( 0.9, Qt::transparent );

        }

        painter->setRenderHints( QPainter::Antialiasing );
        painter->setPen( Qt::NoPen );

        painter->setBrush( highlight );
        painter->drawRoundedRect( fillRect, 2, 2 );

    }

    //____________________________________________________________________________________________________
    void Style::renderSpinBoxArrow( QPainter* painter, const QStyleOptionSpinBox* option, const QWidget* widget, const SubControl& subControl ) const
    {

        const QPalette& palette( option->palette );
        const State& state( option->state );

        // enable state
        bool enabled( state & State_Enabled );

        // check steps enable step
        const bool atLimit(
            (subControl == SC_SpinBoxUp && !(option->stepEnabled & QAbstractSpinBox::StepUpEnabled )) ||
            (subControl == SC_SpinBoxDown && !(option->stepEnabled & QAbstractSpinBox::StepDownEnabled ) ) );

        // update enabled state accordingly
        enabled &= !atLimit;

        // update mouse-over effect
        const bool mouseOver( enabled && ( state & State_MouseOver ) );

        // check animation state
        const bool subControlHover( enabled && mouseOver && ( option->activeSubControls & subControl ) );
        _animations->spinBoxEngine().updateState( widget, subControl, subControlHover );

        const bool animated( enabled && _animations->spinBoxEngine().isAnimated( widget, subControl ) );
        const qreal opacity( _animations->spinBoxEngine().opacity( widget, subControl ) );

        QColor color;
        if( animated )
        {

            QColor highlight = _helper->viewHoverBrush().brush( palette ).color();
            color = KColorUtils::mix( palette.color( QPalette::Text ), highlight, opacity );

        } else if( subControlHover ) {

            color = _helper->viewHoverBrush().brush( palette ).color();

        } else if( atLimit ) {

            color = palette.color( QPalette::Disabled, QPalette::Text );

        } else {

            color = palette.color( QPalette::Text );

        }

        const qreal penThickness = 1.6;
        const QColor background = palette.color( QPalette::Background );

        const QPolygonF arrow( genericArrow( ( subControl == SC_SpinBoxUp ) ? ArrowUp:ArrowDown, ArrowNormal ) );
        const QRect arrowRect( subControlRect( CC_SpinBox, option, subControl, widget ) );

        painter->save();
        painter->translate( QRectF( arrowRect ).center() );
        painter->setRenderHint( QPainter::Antialiasing );

        painter->setPen( QPen( _helper->decoColor( background, color ) , penThickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
        painter->drawPolyline( arrow );
        painter->restore();

        return;

    }

    //___________________________________________________________________________________
    void Style::renderSplitter( const QStyleOption* option, QPainter* painter, const QWidget* widget, bool horizontal ) const
    {

        const QPalette& palette( option->palette );
        const QRect& r( option->rect );
        const State& state( option->state );
        const bool enabled( state & State_Enabled );
        const bool mouseOver( enabled && ( state & ( State_MouseOver|State_Sunken ) ) );

        // get orientation
        const Qt::Orientation orientation( horizontal ? Qt::Horizontal : Qt::Vertical );

        bool animated( false );
        qreal opacity( AnimationData::OpacityInvalid );

        if( enabled )
        {
            if( qobject_cast<const QMainWindow*>( widget ) )
            {

                _animations->dockSeparatorEngine().updateRect( widget, r, orientation, mouseOver );
                animated = _animations->dockSeparatorEngine().isAnimated( widget, r, orientation );
                opacity = animated ? _animations->dockSeparatorEngine().opacity( widget, orientation ) : AnimationData::OpacityInvalid;

            } else if( QPaintDevice* device = painter->device() ) {

                /*
                try update QSplitterHandle using painter device, because Qt passes
                QSplitter as the widget to the QStyle primitive.
                */
                _animations->splitterEngine().updateState( device, mouseOver );
                animated = _animations->splitterEngine().isAnimated( device );
                opacity = _animations->splitterEngine().opacity( device );

            }
        }

        // get base color
        const QColor color = palette.color( QPalette::Background );

        if( horizontal )
        {
            const int hCenter = r.center().x();
            const int h = r.height();

            if( animated || mouseOver )
            {
                const QColor highlight = _helper->alphaColor( _helper->calcLightColor( color ),0.5*( animated ? opacity:1.0 ) );
                const qreal a( r.height() > 30 ? 10.0/r.height():0.1 );
                QLinearGradient lg( 0, r.top(), 0, r.bottom() );
                lg.setColorAt( 0, Qt::transparent );
                lg.setColorAt( a, highlight );
                lg.setColorAt( 1.0-a, highlight );
                lg.setColorAt( 1, Qt::transparent );
                painter->fillRect( r, lg );
            }

            const int ngroups( qMax( 1,h / 250 ) );
            int center( ( h - ( ngroups-1 ) * 250 ) /2 + r.top() );
            for( int k = 0; k < ngroups; k++, center += 250 )
            {
                _helper->renderDot( painter, QPoint( hCenter, center-3 ), color );
                _helper->renderDot( painter, QPoint( hCenter, center ), color );
                _helper->renderDot( painter, QPoint( hCenter, center+3 ), color );
            }

        } else {

            const int vCenter( r.center().y() );
            const int w( r.width() );
            if( animated || mouseOver )
            {
                const QColor highlight( _helper->alphaColor( _helper->calcLightColor( color ),0.5*( animated ? opacity:1.0 ) ) );
                const qreal a( r.width() > 30 ? 10.0/r.width():0.1 );
                QLinearGradient lg( r.left(), 0, r.right(), 0 );
                lg.setColorAt( 0, Qt::transparent );
                lg.setColorAt( a, highlight );
                lg.setColorAt( 1.0-a, highlight );
                lg.setColorAt( 1, Qt::transparent );
                painter->fillRect( r, lg );

            }

            const int ngroups( qMax( 1, w / 250 ) );
            int center = ( w - ( ngroups-1 ) * 250 ) /2 + r.left();
            for( int k = 0; k < ngroups; k++, center += 250 )
            {
                _helper->renderDot( painter, QPoint( center-3, vCenter ), color );
                _helper->renderDot( painter, QPoint( center, vCenter ), color );
                _helper->renderDot( painter, QPoint( center+3, vCenter ), color );
            }

        }

    }

    //____________________________________________________________________________________________________
    void Style::renderTitleBarButton( QPainter* painter, const QStyleOptionTitleBar* option, const QWidget* widget, const SubControl& subControl ) const
    {

        const QRect r = subControlRect( CC_TitleBar, option, subControl, widget );
        if( !r.isValid() ) return;

        QPalette palette = option->palette;

        const State& state( option->state );
        const bool enabled( state & State_Enabled );
        const bool active( enabled && ( option->titleBarState & Qt::WindowActive ) );

        // enable state transition
        _animations->widgetEnabilityEngine().updateState( widget, AnimationEnable, active );
        if( _animations->widgetEnabilityEngine().isAnimated( widget, AnimationEnable ) )
        { palette = _helper->mergePalettes( palette, _animations->widgetEnabilityEngine().opacity( widget, AnimationEnable )  ); }

        const bool sunken( state & State_Sunken );
        const bool mouseOver( ( !sunken ) && widget && r.translated( widget->mapToGlobal( QPoint( 0,0 ) ) ).contains( QCursor::pos() ) );

        _animations->mdiWindowEngine().updateState( widget, subControl, enabled && mouseOver );
        const bool animated( enabled && _animations->mdiWindowEngine().isAnimated( widget, subControl ) );
        const qreal opacity( _animations->mdiWindowEngine().opacity( widget, subControl ) );

        // contrast color
        const QColor base =option->palette.color( QPalette::Active, QPalette::Window );

        // icon color
        QColor color;
        if( animated )
        {

            const QColor base( palette.color( active ? QPalette::Active : QPalette::Disabled, QPalette::WindowText ) );
            const QColor glow( subControl == SC_TitleBarCloseButton ?
                _helper->viewNegativeTextBrush().brush( palette ).color():
                _helper->viewHoverBrush().brush( palette ).color() );

            color = KColorUtils::mix( base, glow, opacity );

        } else if( mouseOver ) {

            color = ( subControl == SC_TitleBarCloseButton ) ?
                _helper->viewNegativeTextBrush().brush( palette ).color():
                _helper->viewHoverBrush().brush( palette ).color();

        } else {

            color = palette.color( active ? QPalette::Active : QPalette::Disabled, QPalette::WindowText );

        }

        // rendering
        renderTitleBarButton( painter, r, base, color, subControl );

    }

    //____________________________________________________________________________________________________
    void Style::renderTitleBarButton( QPainter* painter, const QRect& rect, const QColor& base, const QColor& color, const SubControl& subControl ) const
    {

        painter->save();
        painter->setRenderHints( QPainter::Antialiasing );
        painter->setBrush( Qt::NoBrush );

        painter->drawPixmap( rect, _helper->dockWidgetButton( base, true, rect.width() ) );

        const qreal width( 1.1 );

        // contrast
        painter->translate( 0, 0.5 );
        painter->setPen( QPen( _helper->calcLightColor( base ), width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
        renderTitleBarIcon( painter, rect, subControl );

        // main icon painting
        painter->translate( 0,-1 );
        painter->setPen( QPen( color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
        renderTitleBarIcon( painter, rect, subControl );

        painter->restore();

    }

    //____________________________________________________________________________________
    void Style::renderTitleBarIcon( QPainter *painter, const QRect &r, const SubControl& subControl ) const
    {

        painter->save();

        painter->translate( r.topLeft() );
        painter->scale( qreal( r.width() )/16, qreal( r.height() )/16 );

        switch( subControl )
        {
            case SC_TitleBarContextHelpButton:
            {
                painter->drawArc( 6, 4, 3, 3, 135*16, -180*16 );
                painter->drawArc( 8, 7, 3, 3, 135*16, 45*16 );
                painter->drawPoint( 8, 11 );
                break;
            }

            case SC_TitleBarMinButton:
            {
                painter->drawPolyline( QPolygon() <<  QPoint( 5, 7 ) << QPoint( 8, 10 ) << QPoint( 11, 7 ) );
                break;
            }

            case SC_TitleBarNormalButton:
            {
                painter->drawPolygon( QPolygon() << QPoint( 8, 5 ) << QPoint( 11, 8 ) << QPoint( 8, 11 ) << QPoint( 5, 8 ) );
                break;
            }

            case SC_TitleBarMaxButton:
            {
                painter->drawPolyline( QPolygon() << QPoint( 5, 9 ) << QPoint( 8, 6 ) << QPoint( 11, 9 ) );
                break;
            }

            case SC_TitleBarCloseButton:
            {

                painter->drawLine( QPointF( 5.5, 5.5 ), QPointF( 10.5, 10.5 ) );
                painter->drawLine( QPointF( 10.5, 5.5 ), QPointF( 5.5, 10.5 ) );
                break;

            }

            case SC_TitleBarShadeButton:
            {
                painter->drawLine( QPoint( 5, 11 ), QPoint( 11, 11 ) );
                painter->drawPolyline( QPolygon() << QPoint( 5, 5 ) << QPoint( 8, 8 ) << QPoint( 11, 5 ) );
                break;
            }

            case SC_TitleBarUnshadeButton:
            {
                painter->drawPolyline( QPolygon() << QPoint( 5, 8 ) << QPoint( 8, 5 ) << QPoint( 11, 8 ) );
                painter->drawLine( QPoint( 5, 11 ), QPoint( 11, 11 ) );
                break;
            }

            default:
            break;
        }
        painter->restore();
    }

    //__________________________________________________________________________
    void Style::renderHeaderBackground( const QRect& r, const QPalette& palette, QPainter* painter, const QWidget* widget, bool horizontal, bool reverse ) const
    {

        // use window background for the background
        if( widget ) _helper->renderWindowBackground( painter, r, widget, palette );
        else painter->fillRect( r, palette.color( QPalette::Window ) );

        if( horizontal ) renderHeaderLines( r, palette, painter, TileSet::Bottom );
        else if( reverse ) renderHeaderLines( r, palette, painter, TileSet::Left );
        else renderHeaderLines( r, palette, painter, TileSet::Right );

    }

    //__________________________________________________________________________
    void Style::renderHeaderLines( const QRect& r, const QPalette& palette, QPainter* painter, TileSet::Tiles tiles ) const
    {

        // add horizontal lines
        const QColor color( palette.color( QPalette::Window ) );
        const QColor dark( _helper->calcDarkColor( color ) );
        const QColor light( _helper->calcLightColor( color ) );

        painter->save();
        QRect rect( r );
        if( tiles & TileSet::Bottom  )
        {

            painter->setPen( dark );
            if( tiles & TileSet::Left ) painter->drawPoint( rect.bottomLeft() );
            else if( tiles& TileSet::Right ) painter->drawPoint( rect.bottomRight() );
            else painter->drawLine( rect.bottomLeft(), rect.bottomRight() );

            rect.adjust( 0,0,0,-1 );
            painter->setPen( light );
            if( tiles & TileSet::Left )
            {
                painter->drawLine( rect.bottomLeft(), rect.bottomLeft()+QPoint( 1, 0 ) );
                painter->drawLine( rect.bottomLeft()+ QPoint( 1, 0 ), rect.bottomLeft()+QPoint( 1, 1 ) );

            } else if( tiles & TileSet::Right ) {

                painter->drawLine( rect.bottomRight(), rect.bottomRight() - QPoint( 1, 0 ) );
                painter->drawLine( rect.bottomRight() - QPoint( 1, 0 ), rect.bottomRight() - QPoint( 1, -1 ) );

            } else {

                painter->drawLine( rect.bottomLeft(), rect.bottomRight() );
            }
        } else if( tiles & TileSet::Left ) {

            painter->setPen( dark );
            painter->drawLine( rect.topLeft(), rect.bottomLeft() );

            rect.adjust( 1,0,0,0 );
            painter->setPen( light );
            painter->drawLine( rect.topLeft(), rect.bottomLeft() );

        } else if( tiles & TileSet::Right ) {

            painter->setPen( dark );
            painter->drawLine( rect.topRight(), rect.bottomRight() );

            rect.adjust( 0,0,-1,0 );
            painter->setPen( light );
            painter->drawLine( rect.topRight(), rect.bottomRight() );

        }

        painter->restore();

        return;

    }

    //__________________________________________________________________________
    void Style::renderMenuItemBackground( const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
    {
        const QRect& r( option->rect );
        const QPalette& palette( option->palette );
        const QRect animatedRect( _animations->menuEngine().animatedRect( widget ) );
        if( !animatedRect.isNull() )
        {

            if( animatedRect.intersects( r ) )
            {
                const QColor color( _helper->menuBackgroundColor( _helper->calcMidColor( palette.color( QPalette::Window ) ), widget, animatedRect.center() ) );
                renderMenuItemRect( option, animatedRect, color, palette, painter );
            }

        } else if( _animations->menuEngine().isTimerActive( widget ) ) {

            const QRect previousRect( _animations->menuEngine().currentRect( widget, Previous ) );
            if( previousRect.intersects( r ) )
            {

                const QColor color( _helper->menuBackgroundColor( _helper->calcMidColor( palette.color( QPalette::Window ) ), widget, previousRect.center() ) );
                renderMenuItemRect( option, previousRect, color, palette, painter );
            }

        } else if( _animations->menuEngine().isAnimated( widget, Previous ) ) {

            QRect previousRect( _animations->menuEngine().currentRect( widget, Previous ) );
            if( previousRect.intersects( r ) )
            {
                const qreal opacity(  _animations->menuEngine().opacity( widget, Previous ) );
                const QColor color( _helper->menuBackgroundColor( _helper->calcMidColor( palette.color( QPalette::Window ) ), widget, previousRect.center() ) );
                renderMenuItemRect( option, previousRect, color, palette, painter, opacity );
            }

        }

        return;
    }

    //__________________________________________________________________________
    void Style::renderMenuItemRect( const QStyleOption* option, const QRect& rect, const QColor& base, const QPalette& palette, QPainter* painter, qreal opacity ) const
    {

        if( opacity == 0 ) return;

        // get relevant color
        // TODO: this is inconsistent with MenuBar color.
        // this should change to properly account for 'sunken' state
        QColor color( base );
        if( StyleConfigData::menuHighlightMode() == StyleConfigData::MM_STRONG )
        {

            color = palette.color( QPalette::Highlight );

        } else if( StyleConfigData::menuHighlightMode() == StyleConfigData::MM_SUBTLE ) {

            color = KColorUtils::mix( color, KColorUtils::tint( color, palette.color( QPalette::Highlight ), 0.6 ) );

        }

        // special painting for items with submenus
        const QStyleOptionMenuItem* menuItemOption = qstyleoption_cast<const QStyleOptionMenuItem*>( option );
        if( menuItemOption && menuItemOption->menuItemType == QStyleOptionMenuItem::SubMenu )
        {

            QPixmap pixmap( rect.size() );
            {
                pixmap.fill( Qt::transparent );
                QPainter painter( &pixmap );
                const QRect pixmapRect( pixmap.rect() );

                painter.setRenderHint( QPainter::Antialiasing );
                painter.setPen( Qt::NoPen );

                painter.setBrush( color );
                _helper->fillHole( painter, pixmapRect );

                _helper->holeFlat( color, 0.0 )->render( pixmapRect.adjusted( 1, 2, -2, -1 ), &painter );

                QRect maskRect( visualRect( option->direction, pixmapRect, QRect( pixmapRect.width()-40, 0, 40, pixmapRect.height() ) ) );
                QLinearGradient gradient(
                    visualPos( option->direction, maskRect, QPoint( maskRect.left(), 0 ) ),
                    visualPos( option->direction, maskRect, QPoint( maskRect.right()-4, 0 ) ) );
                gradient.setColorAt( 0.0, Qt::black );
                gradient.setColorAt( 1.0, Qt::transparent );
                painter.setBrush( gradient );
                painter.setCompositionMode( QPainter::CompositionMode_DestinationIn );
                painter.drawRect( maskRect );

                if( opacity >= 0 && opacity < 1 )
                {
                    painter.setCompositionMode( QPainter::CompositionMode_DestinationIn );
                    painter.fillRect( pixmapRect, _helper->alphaColor( Qt::black, opacity ) );
                }

                painter.end();

            }

            painter->drawPixmap( visualRect( option, rect ), pixmap );

        } else {

            if( opacity >= 0 && opacity < 1 )
            { color.setAlphaF( opacity ); }

            _helper->holeFlat( color, 0.0 )->render( rect.adjusted( 1,2,-2,-1 ), painter, TileSet::Full );

        }

    }

    //________________________________________________________________________
    void Style::renderCheckBox(
        QPainter *painter, const QRect &constRect, const QPalette &palette,
        StyleOptions options, CheckBoxState state,
        qreal opacity,
        AnimationMode mode ) const
    {

        const int size( qMin( constRect.width(), constRect.height() ) );
        const QRect rect( centerRect( constRect, size, size ) );

        if( !( options & NoFill ) )
        {
            if( options & Sunken ) _helper->holeFlat( palette.color( QPalette::Window ), 0.0, false )->render( rect.adjusted( 1, 1, -1, -1 ), painter, TileSet::Full );
            else renderSlab( painter, rect, palette.color( QPalette::Button ), options, opacity, mode, TileSet::Ring );
        }

        if( state == CheckOff ) return;

        // check mark
        qreal penThickness( 2.0 );
        const QColor color( palette.color( ( options&Sunken ) ? QPalette::WindowText:QPalette::ButtonText ) );
        const QColor background( palette.color( ( options&Sunken ) ? QPalette::Window:QPalette::Button ) );
        QPen pen( _helper->decoColor( background, color ), penThickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin );
        QPen contrastPen( _helper->calcLightColor( background ), penThickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin );
        if( state == CheckTriState )
        {

            QVector<qreal> dashes;
            dashes << 1.0 << 2.0;
            penThickness = 1.3;
            pen.setWidthF( penThickness );
            contrastPen.setWidthF( penThickness );
            pen.setDashPattern( dashes );
            contrastPen.setDashPattern( dashes );

        } else if( state == CheckSunken ) {

            pen.setColor( _helper->alphaColor( pen.color(), 0.3 ) );
            contrastPen.setColor( _helper->alphaColor( contrastPen.color(), 0.3 ) );

        }

        painter->save();
        painter->translate( QRectF( rect ).center() );

        if( !( options&Sunken ) ) painter->translate( 0, -1 );
        painter->setRenderHint( QPainter::Antialiasing );

        QPolygonF checkMark;
        checkMark << QPointF( 5, -2 ) << QPointF( -1, 5 ) << QPointF( -4, 2 );

        const qreal offset( qMin( penThickness, qreal( 1.0 ) ) );
        painter->setPen( contrastPen );
        painter->translate( 0, offset );
        painter->drawPolyline( checkMark );

        painter->setPen( pen );
        painter->translate( 0, -offset );
        painter->drawPolyline( checkMark );

        painter->restore();

        return;

    }

    //___________________________________________________________________
    void Style::renderRadioButton(
        QPainter* painter, const QRect& constRect,
        const QPalette& palette,
        StyleOptions options,
        CheckBoxState state,
        qreal opacity,
        AnimationMode mode ) const
    {

        const int size( Metrics::CheckBox_Size );
        const QRect rect( centerRect( constRect, size, size ) );

        const QColor color( palette.color( QPalette::Button ) );
        const QColor glow( slabShadowColor( options, opacity, mode ) );
        painter->drawPixmap( rect.topLeft(), _helper->roundSlab( color, glow, 0.0 ) );

        // draw the radio mark
        if( state != CheckOff )
        {
            const qreal radius( 2.6 );
            const qreal dx( 0.5*rect.width() - radius );
            const qreal dy( 0.5*rect.height() - radius );
            const QRectF symbolRect( QRectF( rect ).adjusted( dx, dy, -dx, -dy ) );

            painter->save();
            painter->setRenderHints( QPainter::Antialiasing );
            painter->setPen( Qt::NoPen );

            const QColor background( palette.color( QPalette::Button ) );
            const QColor color( palette.color( QPalette::ButtonText ) );

            // contrast
            if( state == CheckOn ) painter->setBrush( _helper->calcLightColor( background ) );
            else painter->setBrush( _helper->alphaColor( _helper->calcLightColor( background ), 0.3 ) );
            painter->translate( 0, radius/2 );
            painter->drawEllipse( symbolRect );

            // symbol
            if( state == CheckOn ) painter->setBrush( _helper->decoColor( background, color ) );
            else painter->setBrush( _helper->alphaColor( _helper->decoColor( background, color ), 0.3 ) );
            painter->translate( 0, -radius/2 );
            painter->drawEllipse( symbolRect );
            painter->restore();

        }

        return;
    }

    //______________________________________________________________________________
    void Style::renderScrollBarHole(
        QPainter *painter, const QRect &rect, const QColor &color,
        const Qt::Orientation& orientation, const TileSet::Tiles& tiles ) const
    {

        if( !rect.isValid() ) return;

        // one need to make smaller shadow
        // notably on the size when rect height is too high
        const bool smallShadow( orientation == Qt::Horizontal ? rect.height() < 10 : rect.width() < 10 );
        _helper->scrollHole( color, orientation, smallShadow )->render( rect, painter, tiles );

    }

    //______________________________________________________________________________
    void Style::renderScrollBarHandle(
        QPainter* painter, const QRect& constRect, const QPalette& palette,
        const Qt::Orientation& orientation, const bool& hover, const qreal& opacity ) const
    {

        if( !constRect.isValid() ) return;

        // define rect and check
        const bool horizontal( orientation == Qt::Horizontal );
        QRect rect( horizontal ? constRect.adjusted( 3, 3, -3, -3 ):constRect.adjusted( 3, 3, -3, -3 ) );

        painter->save();
        painter->setRenderHints( QPainter::Antialiasing );
        const QColor color( palette.color( QPalette::Button ) );

        // draw the slider
        const qreal radius = 3.5;

        // glow / shadow
        QColor glow;
        const QColor shadow( _helper->alphaColor( _helper->calcShadowColor( color ), 0.4 ) );
        const QColor hovered( _helper->viewHoverBrush().brush( QPalette::Active ).color() );

        if( opacity >= 0 ) glow = KColorUtils::mix( shadow, hovered, opacity );
        else if( hover ) glow = hovered;
        else glow = shadow;

        _helper->scrollHandle( color, glow )->
            render( constRect,
            painter, TileSet::Full );

        // contents
        const QColor mid( _helper->calcMidColor( color ) );
        QLinearGradient lg( 0, rect.top(), 0, rect.bottom() );
        lg.setColorAt(0, color );
        lg.setColorAt(1, mid );
        painter->setPen( Qt::NoPen );
        painter->setBrush( lg );
        painter->drawRoundedRect( rect.adjusted( 1, 1, -1, -1), radius - 2, radius - 2 );

        // bevel pattern
        const QColor light( _helper->calcLightColor( color ) );

        QLinearGradient patternGradient( 0, 0, horizontal ? 30:0, horizontal? 0:30 );
        patternGradient.setSpread( QGradient::ReflectSpread );
        patternGradient.setColorAt( 0.0, Qt::transparent );
        patternGradient.setColorAt( 1.0, _helper->alphaColor( light, 0.1 ) );

        QRect bevelRect( rect );
        if( horizontal ) bevelRect.adjust( 0, 3, 0, -3 );
        else bevelRect.adjust( 3, 0, -3, 0 );

        if( bevelRect.isValid() )
        {
            painter->setBrush( patternGradient );
            painter->drawRect( bevelRect );
        }

        painter->restore();
        return;

    }

    //______________________________________________________________________________
    void Style::renderScrollBarArrow(
        QPainter* painter, const QRect& rect, const QColor& color, const QColor& background,
        ArrowOrientation orientation ) const
    {

        const qreal penThickness = 1.6;
        QPolygonF arrow( genericArrow( orientation, ArrowNormal ) );

        const QColor contrast( _helper->calcLightColor( background ) );
        const QColor base( _helper->decoColor( background, color ) );

        painter->save();
        painter->translate( QRectF(rect).center() );
        painter->setRenderHint( QPainter::Antialiasing );

        const qreal offset( qMin( penThickness, qreal( 1.0 ) ) );
        painter->translate( 0,offset );
        painter->setPen( QPen( contrast, penThickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
        painter->drawPolyline( arrow );
        painter->translate( 0,-offset );

        painter->setPen( QPen( base, penThickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
        painter->drawPolyline( arrow );
        painter->restore();

        return;

    }

    //______________________________________________________________________________
    QColor Style::scrollBarArrowColor( const QStyleOptionSlider* option, const SubControl& control, const QWidget* widget ) const
    {

        // copy rect and palette
        const QRect& rect( option->rect );
        const QPalette& palette( option->palette );

        // color
        QColor color( palette.color( QPalette::WindowText ) );

        // check enabled state
        const bool enabled( option->state & State_Enabled );
        if( !enabled ) return color;

        if(
            ( control == SC_ScrollBarSubLine && option->sliderValue == option->minimum ) ||
            ( control == SC_ScrollBarAddLine && option->sliderValue == option->maximum ) )
        {

            // manually disable arrow, to indicate that scrollbar is at limit
            return palette.color( QPalette::Disabled, QPalette::WindowText );

        }

        const bool mouseOver( _animations->scrollBarEngine().isHovered( widget, control ) );
        const bool animated( _animations->scrollBarEngine().isAnimated( widget, control ) );
        const qreal opacity( _animations->scrollBarEngine().opacity( widget, control ) );

        // retrieve mouse position from engine
        QPoint position( mouseOver ? _animations->scrollBarEngine().position( widget ) : QPoint( -1, -1 ) );
        if( mouseOver && rect.contains( position ) )
        {
            // need to update the arrow controlRect on fly because there is no
            // way to get it from the styles directly, outside of repaint events
            _animations->scrollBarEngine().setSubControlRect( widget, control, rect );
        }


        if( rect.intersects(  _animations->scrollBarEngine().subControlRect( widget, control ) ) )
        {

            QColor highlight = _helper->viewHoverBrush().brush( palette ).color();
            if( animated )
            {
                color = KColorUtils::mix( color, highlight, opacity );

            } else if( mouseOver ) {

                color = highlight;

            }

        }

        return color;

    }

    //______________________________________________________________________________
    void Style::renderDebugFrame( QPainter* painter, const QRect& rect ) const
    {
        painter->save();
        painter->setRenderHints( QPainter::Antialiasing );
        painter->setBrush( Qt::NoBrush );
        painter->setPen( Qt::red );
        painter->drawRect( QRectF( rect ).adjusted( 0.5, 0.5, -0.5, -0.5 ) );
        painter->restore();
    }

    //____________________________________________________________________________________
    QColor Style::slabShadowColor( StyleOptions options, qreal opacity, AnimationMode mode ) const
    {

        QColor glow;
        if( mode == AnimationNone || opacity < 0 )
        {

            if( options & Hover ) glow = _helper->viewHoverBrush().brush( QPalette::Active ).color();
            else if( options & Focus ) glow = _helper->viewFocusBrush().brush( QPalette::Active ).color();

        } else if( mode == AnimationHover ) {

            // animated color, hover
            if( options & Focus ) glow = _helper->viewFocusBrush().brush( QPalette::Active ).color();
            if( glow.isValid() ) glow = KColorUtils::mix( glow,  _helper->viewHoverBrush().brush( QPalette::Active ).color(), opacity );
            else glow = _helper->alphaColor(  _helper->viewHoverBrush().brush( QPalette::Active ).color(), opacity );

        } else if( mode == AnimationFocus ) {

            if( options & Hover ) glow = _helper->viewHoverBrush().brush( QPalette::Active ).color();
            if( glow.isValid() ) glow = KColorUtils::mix( glow,  _helper->viewFocusBrush().brush( QPalette::Active ).color(), opacity );
            else glow = _helper->alphaColor(  _helper->viewFocusBrush().brush( QPalette::Active ).color(), opacity );

        }

        return glow;
    }

    //____________________________________________________________________________________
    QPolygonF Style::genericArrow( Style::ArrowOrientation orientation, Style::ArrowSize size ) const
    {

        QPolygonF arrow;
        switch( orientation )
        {
            case ArrowUp:
            {
                if( size == ArrowTiny ) arrow << QPointF( -2.25, 1.125 ) << QPointF( 0, -1.125 ) << QPointF( 2.25, 1.125 );
                else if( size == ArrowSmall ) arrow << QPointF( -2.5, 1.5 ) << QPointF( 0, -1.5 ) << QPointF( 2.5, 1.5 );
                else arrow << QPointF( -3.5, 2 ) << QPointF( 0, -2 ) << QPointF( 3.5, 2 );
                break;
            }

            case ArrowDown:
            {
                if( size == ArrowTiny ) arrow << QPointF( -2.25, -1.125 ) << QPointF( 0, 1.125 ) << QPointF( 2.25, -1.125 );
                else if( size == ArrowSmall ) arrow << QPointF( -2.5, -1.5 ) << QPointF( 0, 1.5 ) << QPointF( 2.5, -1.5 );
                else arrow << QPointF( -3.5, -2 ) << QPointF( 0, 2 ) << QPointF( 3.5, -2 );
                break;
            }

            case ArrowLeft:
            {
                if( size == ArrowTiny ) arrow << QPointF( 1.125, -2.25 ) << QPointF( -1.125, 0 ) << QPointF( 1.125, 2.25 );
                else if( size == ArrowSmall ) arrow << QPointF( 1.5, -2.5 ) << QPointF( -1.5, 0 ) << QPointF( 1.5, 2.5 );
                else arrow << QPointF( 2, -3.5 ) << QPointF( -2, 0 ) << QPointF( 2, 3.5 );

                break;
            }

            case ArrowRight:
            {
                if( size == ArrowTiny ) arrow << QPointF( -1.125, -2.25 ) << QPointF( 1.125, 0 ) << QPointF( -1.125, 2.25 );
                else if( size == ArrowSmall ) arrow << QPointF( -1.5, -2.5 ) << QPointF( 1.5, 0 ) << QPointF( -1.5, 2.5 );
                else arrow << QPointF( -2, -3.5 ) << QPointF( 2, 0 ) << QPointF( -2, 3.5 );
                break;
            }

            default: break;

        }

        return arrow;

    }

    //____________________________________________________________________________________
    QStyleOptionToolButton Style::separatorMenuItemOption( const QStyleOptionMenuItem* menuItemOption, const QWidget* widget ) const
    {

        // separator can have a title and an icon
        // in that case they are rendered as sunken flat toolbuttons
        QStyleOptionToolButton toolButtonOption;
        toolButtonOption.initFrom( widget );
        toolButtonOption.rect = menuItemOption->rect;
        toolButtonOption.features = QStyleOptionToolButton::None;
        toolButtonOption.state = State_On|State_Sunken|State_Enabled;
        toolButtonOption.subControls = SC_ToolButton;
        toolButtonOption.icon =  menuItemOption->icon;

        int iconWidth( pixelMetric( PM_SmallIconSize, menuItemOption, widget ) );
        toolButtonOption.iconSize = QSize( iconWidth, iconWidth );
        toolButtonOption.text = menuItemOption->text;

        toolButtonOption.toolButtonStyle = Qt::ToolButtonTextBesideIcon;

        return toolButtonOption;

    }

    //______________________________________________________________________________
    qreal Style::dialAngle( const QStyleOptionSlider* sliderOption, int value ) const
    {

        // calculate angle at which handle needs to be drawn
        qreal angle( 0 );
        if( sliderOption->maximum == sliderOption->minimum ) angle = M_PI / 2;
        else {

            qreal fraction( qreal( value - sliderOption->minimum )/qreal( sliderOption->maximum - sliderOption->minimum ) );
            if( !sliderOption->upsideDown ) fraction = 1.0 - fraction;

            if( sliderOption->dialWrapping ) angle = 1.5*M_PI - fraction*2*M_PI;
            else  angle = ( M_PI*8 - fraction*10*M_PI )/6;

        }

        return angle;

    }

}

namespace OxygenPrivate
{

    void TabBarData::drawTabBarBaseControl( const QStyleOptionTab* tabOption, QPainter* painter, const QWidget* widget )
    {


        // check parent
        if( !_style ) return;

        // make sure widget is locked
        if( !locks( widget ) ) return;

        // make sure dirty flag is set
        if( !_dirty ) return;

        // cast to TabBar and check
        const QTabBar* tabBar( qobject_cast<const QTabBar*>( widget ) );
        if( !tabBar ) return;

        // get reverseLayout flag
        const bool reverseLayout( tabOption->direction == Qt::RightToLeft );

        // get documentMode flag
        const QStyleOptionTabV3 *tabOptionV3 = qstyleoption_cast<const QStyleOptionTabV3 *>( tabOption );
        bool documentMode = tabOptionV3 ? tabOptionV3->documentMode : false;
        const QTabWidget *tabWidget = ( widget && widget->parentWidget() ) ? qobject_cast<const QTabWidget *>( widget->parentWidget() ) : NULL;
        documentMode |= ( tabWidget ? tabWidget->documentMode() : true );

        const QRect tabBarRect( _style.data()->insideMargin( tabBar->rect(), 0 ) );

        // define slab
        Oxygen::Style::SlabRect slab;

        // switch on tab shape
        switch( tabOption->shape )
        {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
            {
                Oxygen::TileSet::Tiles tiles( Oxygen::TileSet::Top );
                QRect frameRect;
                frameRect.setLeft( tabBarRect.left() - 7 + 1 );
                frameRect.setRight( tabBarRect.right() + 7 - 1 );
                frameRect.setTop( tabBarRect.bottom() - 8 );
                frameRect.setHeight( 4 );
                if( !( documentMode || reverseLayout ) ) tiles |= Oxygen::TileSet::Left;
                if( !documentMode && reverseLayout ) tiles |= Oxygen::TileSet::Right;
                slab = Oxygen::Style::SlabRect( frameRect, tiles );
                break;
            }

            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
            {
                Oxygen::TileSet::Tiles tiles( Oxygen::TileSet::Bottom );
                QRect frameRect;
                frameRect.setLeft( tabBarRect.left() - 7 + 1 );
                frameRect.setRight( tabBarRect.right() + 7 - 1 );
                frameRect.setBottom( tabBarRect.top() + 8 );
                frameRect.setTop( frameRect.bottom() - 4 );
                if( !( documentMode || reverseLayout ) ) tiles |= Oxygen::TileSet::Left;
                if( !documentMode && reverseLayout ) tiles |= Oxygen::TileSet::Right;
                slab = Oxygen::Style::SlabRect( frameRect, tiles );
                break;
            }

            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
            {
                Oxygen::TileSet::Tiles tiles( Oxygen::TileSet::Left );
                QRect frameRect;
                frameRect.setTop( tabBarRect.top() - 7 + 1 );
                frameRect.setBottom( tabBarRect.bottom() + 7 - 1 );
                frameRect.setLeft( tabBarRect.right() - 8 );
                frameRect.setWidth( 4 );
                if( !( documentMode || reverseLayout ) ) tiles |= Oxygen::TileSet::Top;
                if( !documentMode && reverseLayout ) tiles |= Oxygen::TileSet::Bottom;
                slab = Oxygen::Style::SlabRect( frameRect, tiles );
                break;
            }

            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
            {
                Oxygen::TileSet::Tiles tiles( Oxygen::TileSet::Right );
                QRect frameRect;
                frameRect.setTop( tabBarRect.top() - 7 + 1 );
                frameRect.setBottom( tabBarRect.bottom() + 7 - 1 );
                frameRect.setRight( tabBarRect.left() + 8 );
                frameRect.setLeft( frameRect.right() - 4 );
                if( !( documentMode || reverseLayout ) ) tiles |= Oxygen::TileSet::Top;
                if( !documentMode && reverseLayout ) tiles |= Oxygen::TileSet::Bottom;
                slab = Oxygen::Style::SlabRect( frameRect, tiles );
                break;
            }

            default:
            break;
        }

        const bool verticalTabs( _style.data()->isVerticalTab( tabOption ) );
        const QRect tabWidgetRect( tabWidget ?
            _style.data()->insideMargin( tabWidget->rect(), 0 ).translated( -widget->geometry().topLeft() ) :
            QRect() );

        const QPalette& palette( tabOption->palette );
        const QColor color( palette.color( QPalette::Window ) );
        _style.data()->adjustSlabRect( slab, tabWidgetRect, documentMode, verticalTabs );
        _style.data()->renderSlab( painter, slab, color, Oxygen::Style::NoFill );

        setDirty( false );
        return;

    }

}
