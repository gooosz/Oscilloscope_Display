//*******************************************************************
/*!
\file   config.h
\author Thomas Breuer
\date   26.09.2022
\brief  Board specific configuration
*/

//*******************************************************************
/*
Board:    Virtual

\see Virtual/board_pinout.txt
*/

//*******************************************************************
using namespace EmbSysLib::Hw;
using namespace EmbSysLib::Dev;
using namespace EmbSysLib::Ctrl;
using namespace EmbSysLib::Mod;

//-------------------------------------------------------------------
// Port
//-------------------------------------------------------------------
Port_Virtual port( "localhost:1000" );

//-------------------------------------------------------------------
// Timer
//-------------------------------------------------------------------
Timer_Mcu   timer( 1000L/*us*/ );

TaskManager taskManager( timer );

//-------------------------------------------------------------------
// ADC
//-------------------------------------------------------------------
Adc_Virtual  adc( "localhost:1000", timer );
WORD     adc_A1 = 0;
WORD     adc_A2 = 0;

//-------------------------------------------------------------------
// Display
//-------------------------------------------------------------------
//*******************************************************************
#include "../../Resource/Color/Color.h"

Font        fontFont_10x20      ( Memory_Mcu( "../../../Src/Resource/Font/font_10x20.bin"       ).getPtr() );
Font        fontFont_16x24      ( Memory_Mcu( "../../../Src/Resource/Font/font_16x24.bin"       ).getPtr() );
Font        fontFont_8x12       ( Memory_Mcu( "../../../Src/Resource/Font/font_8x12.bin"        ).getPtr() );
Font        fontFont_8x8        ( Memory_Mcu( "../../../Src/Resource/Font/font_8x8.bin"         ).getPtr() );

DisplayGraphic_Virtual  dispGraphic( 800, 480, "localhost:1000", fontFont_8x12, 1 );

ScreenGraphic screen( dispGraphic );

//-------------------------------------------------------------------
// UART
//-------------------------------------------------------------------
Uart_Stdio uart( true );

Terminal   terminal( uart, 255,255,"# +" );

//-------------------------------------------------------------------
// Touch
//-------------------------------------------------------------------
Touch_Virtual  touch( "localhost:1000", 320, 240 );

Pointer        pointer( touch );

//-------------------------------------------------------------------
// Digital
//-------------------------------------------------------------------
Digital       led_A( port,16, Digital::Out , 0 ); // LED 0
Digital       btn_A( port, 5, Digital::In  , 0 ); // Button "A"

Digital       btnLeft ( port, 0, Digital::In  , 0 ); // Button "<<"
Digital       btnCtrl ( port, 1, Digital::In  , 0 ); // Button "o"
Digital       btnRight( port, 2, Digital::In  , 0 ); // Button ">>"

//-------------------------------------------------------------------
// Control
//-------------------------------------------------------------------
DigitalIndicator indicator( led_A, taskManager );
DigitalButton    button   ( btn_A, taskManager, 40, 1000 );

DigitalEncoderJoystick    encoder( &btnLeft, &btnRight, &btnCtrl, taskManager, 150 );
