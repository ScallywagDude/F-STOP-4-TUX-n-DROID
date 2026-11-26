#include "basemodui_compat.h"
#include "cbase.h"
#include "touch_settings.h"
#include "basemodpanel.h"
#include "vgui_controls/PropertyDialog.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/PropertyPage.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/Slider.h"
#include "vgui_controls/Label.h"
#include "vgui/ISurface.h"
#include "vgui/IScheme.h"
#include "tier0/memdbgon.h"

namespace BaseModUI
{
class CTouchSettingsPage : public vgui::PropertyPage
{
    DECLARE_CLASS_SIMPLE( CTouchSettingsPage, vgui::PropertyPage );
public:
    CTouchSettingsPage( vgui::Panel *parent ) : BaseClass( parent, "TouchSettings" )
    {
        const char *checkNames[] = { "Enable touch controls", "Draw touch controls", "Reverse touch look", "Filter touch input", "Touch acceleration" };
        ConVar *checkVars[] = { &touch_enable, &touch_draw, &touch_reverse, &touch_filter, &touch_enable_accel };
        for ( int i = 0; i < 5; ++i )
        {
            m_Checks[i].var = checkVars[i];
            m_Checks[i].control = new vgui::CheckButton( this, CFmtStr( "TouchCheck%d", i ), checkNames[i] );
            m_Checks[i].control->SetBounds( 18, 12 + i * 27, 510, 24 );
            m_Checks[i].control->AddActionSignalTarget( this );
        }
        const char *sliderNames[] = { "Look sensitivity", "Acceleration exponent", "Horizontal sensitivity", "Vertical sensitivity" };
        ConVar *sliderVars[] = { &touch_sensitivity, &touch_accel, &touch_yaw, &touch_pitch };
        int low[] = { 10, 100, 50, 50 }, high[] = { 600, 150, 300, 300 };
        float factors[] = { 100.f, 100.f, 1.f, 1.f };
        for ( int i = 0; i < 4; ++i )
        {
            SliderRow &row = m_Sliders[i];
            row.var = sliderVars[i]; row.factor = factors[i];
            row.caption = new vgui::Label( this, CFmtStr( "TouchCaption%d", i ), sliderNames[i] );
            row.caption->SetBounds( 18, 155 + i * 43, 220, 22 );
            row.control = new vgui::Slider( this, CFmtStr( "TouchSlider%d", i ) );
            row.control->SetBounds( 240, 153 + i * 43, 225, 30 );
            row.control->SetRange( low[i], high[i] );
            row.control->SetNumTicks( 0 );
            row.control->AddActionSignalTarget( this );
            row.value = new vgui::Label( this, CFmtStr( "TouchValue%d", i ), "" );
            row.value->SetBounds( 470, 155 + i * 43, 60, 22 );
        }
        OnResetData();
    }

    virtual void OnResetData()
    {
        for ( int i = 0; i < 5; ++i ) m_Checks[i].control->SetSelected( m_Checks[i].var->GetBool() );
        for ( int i = 0; i < 4; ++i ) m_Sliders[i].control->SetValue( (int)( m_Sliders[i].var->GetFloat() * m_Sliders[i].factor + .5f ) );
        UpdateValues();
    }
    virtual void OnApplyChanges()
    {
        for ( int i = 0; i < 5; ++i ) m_Checks[i].var->SetValue( m_Checks[i].control->IsSelected() );
        for ( int i = 0; i < 4; ++i ) m_Sliders[i].var->SetValue( m_Sliders[i].control->GetValue() / m_Sliders[i].factor );
        engine->ClientCmd_Unrestricted( "host_writeconfig\n" );
    }
private:
    MESSAGE_FUNC( OnControlChanged, "SliderMoved" ) { UpdateValues(); PostActionSignal( new KeyValues( "ApplyButtonEnable" ) ); }
    MESSAGE_FUNC( OnCheckChanged, "CheckButtonChecked" ) { PostActionSignal( new KeyValues( "ApplyButtonEnable" ) ); }
    void UpdateValues()
    {
        for ( int i = 0; i < 4; ++i )
            m_Sliders[i].value->SetText( CFmtStr( "%.2f", m_Sliders[i].control->GetValue() / m_Sliders[i].factor ) );
    }
    struct CheckRow { ConVar *var; vgui::CheckButton *control; } m_Checks[5];
    struct SliderRow { ConVar *var; float factor; vgui::Slider *control; vgui::Label *caption, *value; } m_Sliders[4];
};

static vgui::DHANDLE<vgui::PropertyDialog> dialog;

void CloseTouchOptions()
{
    if ( dialog.Get() )
    {
        dialog->Close();
        dialog = NULL;
    }
}

void OpenTouchOptions( vgui::Panel *parent )
{
    if ( !dialog.Get() )
    {
        dialog = new vgui::PropertyDialog( parent, "FStopTouchOptions" );
        // BaseModUI's scheme is for the custom console-style controls. Standard
        // VGUI property pages need SourceScheme's fonts, frame and button styles.
        dialog->SetScheme( vgui::scheme()->LoadSchemeFromFile( "resource/SourceScheme.res", "FStopDesktopSettings" ) );
        dialog->SetTitle( "Touch options", true );
        dialog->SetSize( 580, 430 );
        dialog->SetSizeable( false );
        dialog->SetDeleteSelfOnClose( true );
        dialog->SetApplyButtonVisible( true );
        dialog->AddPage( new CTouchSettingsPage( dialog ), "Touch" );
        dialog->InvalidateLayout( true, true );
        dialog->MoveToCenterOfScreen();
    }
    dialog->Activate();
}

CON_COMMAND( touch_options, "Open the touch control settings." )
{
    if ( CBaseModPanel *panel = CBaseModPanel::GetSingletonPtr() )
    {
        engine->ClientCmd_Unrestricted( "gameui_activate\nhideconsole\n" );
        OpenTouchOptions( panel );
    }
}
}
