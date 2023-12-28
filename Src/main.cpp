//*******************************************************************
/*!
\file   ...
\author Thomas Breuer, Len-Marvin Adler
\date   11.12.2023
\brief  Display for Oscilloscope and ADC measuring
*/

//*******************************************************************
#include <stdio.h>
#include "math.h"

//-------------------------------------------------------------------
#include "EmbSysLib.h"
#include "ReportHandler.h"
#include "config.h"

#include "ringbuf_t.hpp"


//-------------------------------------------------------------------
const int xmax = screen.getWidth()  - 1;
const int ymax = screen.getHeight() - 1;

//-------------------------------------------------------------------
// y-axis will be labeled from -15 to +15
// that makes 30 different labels to display
const int voltmin = -15;
const int voltmax = 15;
const int numOfVoltLabels = abs(voltmax) + abs(voltmin);
const int pixelPerVolt = ymax / numOfVoltLabels;
const int onlyLabelEvery = voltmax / 7.5;	// only label every onlyLabelEvery times

// voltages at gpio pin
const int gpio_vmin = 0;
const float gpio_vmax = 3.3;


//-------------------------------------------------------------------
// voltage measurements are stored in voltage array
#define SAMPLESIZE (799 - 2*4)
// 799px on display, 4px margin left and margin right
// you can only display one value per pixel
volatile float volts[SAMPLESIZE] = {0};
volatile bool voltsVoll = false;
volatile int voltCount = 0;



//-------------------------------------------------------------------
// how many seconds to fit on whole display
const int secondsToDisplay = 10;	// display 10 time values, evenly spaced on x-axis
const float pixelPerSecond = xmax / secondsToDisplay;

// how many x-pixels until the next sample value to draw
// 1000 values per 100µs => 100µs * 1000 = 100*10⁻⁶*1000 = 0.1s für 1000 Werte
// => 1s für 10*1000 = 10000 Werte
//#define SAMPLESPERSECOND (SAMPLESIZE * 0.0001 * 100 * 10)
//const float nextSampleTime = (pixelPerSecond / SAMPLESPERSECOND);
const int firstLabel = xmax/2 - (secondsToDisplay/2) * pixelPerSecond;	// x coord of first label of time axis
const int lastLabel = firstLabel + secondsToDisplay * pixelPerSecond;	// x coord of last label of time axis


/// get Voltage at GPIO Pin
///
/// @returns voltage measured at GPIO/ADC A1
/// between 0V and 3V
float getADCVolt(void)
{
	// 16-Bit register
	//return (adc.get(adc_A1) * (float)gpio_vmax) / (unsigned)0xFFF;
	return (adc.get(adc_A1) * gpio_vmax) / (unsigned)0xFFFF;
}

//-------------------------------------------------------------------
/// get y coordinate from measured voltage
///
/// @param volt The measured voltage
/// @param pixelPerVolt The scaling, how many pixel will be needed to draw 0V to 1V, etc.
/// @returns Corresponding Y-Coordinate to draw on screen
int yCoordFromVolt(double volt, int pixelPerVolt)
{
	// volt außerhalb der erlaubten [voltmin, voltmax]
	if (volt > voltmax || volt < voltmin) {
		return ymax/2 + pixelPerVolt;	// -1
	}
	// pixel on screen is volt * pixelPerVolt
	// because pixelPerVolt is amount of pixel needed for 1V
	if (volt >= 0) {
		// 0V to 15V is ymax/2 to 0px
		return (int) (ymax/2 - volt * pixelPerVolt);
	} else {
		// -15V to 0V is ymax to ymax/2
		return (int) (ymax/2 + abs(volt) * pixelPerVolt);
	}
}


/// Timer needed for using RTC
///
/// @see
class MyTimer : TaskManager::Task
{
public:
	/// time
	int time=0;
	MyTimer(TaskManager &tm)
	{
		tm.add(this);
	}

	/// get new ADC measurement
	///
	/// will be called every 100µs
	/// and pulls a new measurement by ADC from it's 12 Bit register
	/// zieht Werte mit Frequenz 10kHz
	void update()
	{
		// measure every 100µs
		// 100µs defined in config.h l.130
		// 100µs for 1 values => 10 values per ms are measured
		// => 10_000 values per second
		// 1/(10000 Werte/s) = 1s/(10000Werte) = 0.0001s pro Wert
		// => Frequenz f = 1/(0.0001s) = 10k 1/s = 10kHz
		time++;
		if (!voltsVoll) {
			volts[voltCount++] = getADCVolt();
			if (voltCount == SAMPLESIZE) {
				voltsVoll = true;
			}
		}
		//......
	}
};


#define DASHBREITE 10
/// draws the coordinate system on screen
///
/// Draws the time (horizontal) axis
/// and voltage (vertical) axis on screen
/// with scaling (labels/values)
/// @param pixelPerVolt The distance from 1 label on Y-Axis to another, so how many pixel are inbetween the labels of Volt
/// @param pixelPerSecond The distance from 1 label on X-Axis to another, so how many pixel are inbetween the labels of Time
void drawCoordinateSystem(int pixelPerVolt, int pixelPerSecond)
{
	  // draw y-achse und Beschriftungen U fuer Voltage
	  screen.drawLine( xmax/2,      0, xmax/2, ymax  , 1, Color::Red ); // vertikal
	  // then the rest
	  // 10 pixel dash

	  // ---vertical axis---
	  // ueber der 0 muessen 15mal beschriftet werden
	  // distance from current label to next on
	  // dash ist '-' auf der Achse an dem Beschriftung liegt
	  //int dashBreite = 10;
	  int label = 0;
	  for (int y=ymax/2; y>=0; y-=pixelPerVolt) {
		  int dashStart = xmax/2 - DASHBREITE/2;
		  int dashEnd = xmax/2 + DASHBREITE/2;
		  // draw beschriftung at x=0-haelfte der breite, y
		  screen.drawLine(dashStart, y, dashEnd, y, 1, Color::Red);
		  // only draw every 2 labels
		  if (label % onlyLabelEvery == 0)
			  // -30 so that the number is left from the dash
			  // -10 so that the number is vertically center aligned with dash
			  screen.drawText(dashStart - 30, y-10, "%3d", label);
		  label += 1;
	  }
	  label = 0;
	  // unter der 0 muessen 15mal beschriftet werden
	  for (int y=ymax/2; y<=ymax; y+=pixelPerVolt) {
	  		  int dashStart = xmax/2 - DASHBREITE/2;
	  		  int dashEnd = xmax/2 + DASHBREITE/2;
	  		  // draw beschriftung at x=0-haelfte der breite, y
	  		  screen.drawLine(dashStart, y, dashEnd, y, 1, Color::Red);
	  		  // only draw every 2 labels and from -15 to +15
	  		  if (label % onlyLabelEvery == 0 && label <= voltmax && label >= voltmin)
	  			  // -30 so that the number is left from the dash
	  			  // -10 so that the number is vertically center aligned with dash
	  			  screen.drawText(dashStart - 30, y-10, "%3d", label);
	  		  label -= 1;
	  }
	  // 22700ms per display
	  // so 22.7s, with 1000 every 0.1s
	  // so 10000 every 1s


	  // ---horizontal axis---
	  // draw x-achse und Beschriftungen t fuer Zeit
	  screen.drawLine(0, ymax/2, xmax  , ymax/2, 1, Color::Red ); // horizontal

	  label = 0;	// go from middle to right first
	  // TODO: draw scaling of time axis
	  //int timeLabel = 0;
	  // don't draw 0 again, because the y-axis always did
	  for (int x=firstLabel; x<=xmax; x+=pixelPerSecond) {
		  	  int dashStart = ymax/2 - DASHBREITE/2;
		  	  int dashEnd = ymax/2 + DASHBREITE/2;
			  // draw beschriftung at y=0-hae
			  screen.drawLine(x, dashStart, x, dashEnd, 1, Color::Red);
			  if (label % 2 == 0 && label > 0 && label < secondsToDisplay)
				  // only draw even numbers
				  // x-25, dashStart+15
				  // print only 1 decimal, so 0.1, 0.2, etc.
				  // can't print decimals, so by knowing 10 labels, each 1/10 of 0.1s, so 0.01
				  // we can do
			  	  screen.drawText(x-25, dashStart+15, "0.0%d", label);
			  label += 1;
	  }

}


/// returns the X-Coordinate of a given time/second
///
/// @param timer Current time of RTC, e.g. MyTimer.time in 100th µs
/// @param pixelPerSecond Defines the scaling, how many pixels are needed to display all values 0s to 1s
int xCoordFromTime(int time, int pixelPerSecond)
{
	float seconds = (float)(time) / 10000;	// 10^5s / 10^4 = s
	// screen seconds to display:
	// [0,10], then [10,20], then [20,30] ,etc.
	int x = seconds * pixelPerSecond;
	/*if (x > xmax) {
		// add pixelPerSecond - (xmax - labelmax + xmin - labelmin)
		// to account for the lost displayed seconds between display edges
		//int lastLabelToEdge = xmax - xmin - (secondsToDisplay * pixelPerSecond); // pixel from last label to xmax
		int lastLabelToEdge = xmin;
		int firstLabelToEdge = xmin;	// pixel from first label to xmin
		// multiply lost seconds by how many times x is over the edge (if modulo)
		int offset = pixelPerSecond - (seconds/secondsToDisplay) * (lastLabelToEdge + firstLabelToEdge);
		x -= offset;
	}*/
	int res = ((int)x % (secondsToDisplay*pixelPerSecond)) + firstLabel;	// add xmin for offset
	return res;	// wrap at xmax
}

/// print info in top left corner of screen
///
/// displays 6 decimal places of volt
/// @param volt measured voltage
void printInfoText(float volt)
{
	/* convert float to array of chars to print */
	char voltarr[10];	// max 2 digits (integer part) + 6 digits (decimal part), round up to 10 digis just in case
	snprintf(voltarr, sizeof(voltarr), "%.6f", volt);
	screen.printf(2, 1, "U = %s", voltarr);
	// 10^accuracy is how to get accuracy many decimal places
}

/// used for representing a float value using two integers
///
/// screen.drawText() can't print floats so using two integers instead representing
/// the integer part and the decimal part and print those two together is a workaround
///
struct float2int {
	/// integer part of a float
	int integer;	// vor Komma
	/// decimal part of a float
	int decimal;	// nach Komma
};


/// convert a float to a float2int to use it in screen.drawText
///
/// @param value the float to convert
/// @param accuracy the amount of decimal places to use (no rounding)
struct float2int float2int(float value, int accuracy)
{
	struct float2int fi;
	fi.integer = (int) value;
	fi.decimal = (value - fi.integer) * pow(10, accuracy);	// 10^accuracy is how to get accuracy many decimal places
	return fi;
}

//*******************************************************************
int main( void )
{
  MyTimer timer(taskManager);

  adc.enable(adc_A1);
  adc.enable(adc_A2);

  // Setup
  screen.setFont     ( fontFont_10x20 );
  screen.setTextColor( Color::White   );
  screen.setBackColor( Color::Black   );
  screen.clear();

  // Frame
  drawCoordinateSystem(pixelPerVolt, pixelPerSecond);
  screen.drawText(10, 10, "time range = %d.%ds", 0, 1);


  int x = firstLabel; // x is time-axis
  int y = 0;

  // clicked button flags
  // only needed to know if a button has been pressed ONCE
  bool clicked_start	= false;
  bool clicked_stop		= false;
  bool clicked_next		= false;	// start next measure

  // timer.time = 0;	// set time to 0 at start, this will only go for 1st screen, after that theres an offset
  int t0 = timer.time;	// start time of measuring

  // drawn once is used to indicate if volts array has been already drawn
  // as it only needs to be drawn once until next measurement starts (btnRight -> clicked_next)
  bool drawnOnce = false;


  while( 1 )
  {
	  if (btnRight.getEvent() == Digital::Event::ACTIVATED) {
		  //clicked_next = true;
		  // start new adc measurement
		  voltCount = 0;
		  voltsVoll = false;
		  timer.time = 0;	// reset time
		  screen.clear();	// clear old sample from screen
		  drawCoordinateSystem(pixelPerVolt, pixelPerSecond);
		  drawnOnce = false;
	  }
	/*
	 * Button left for starting measurement
	 * Button center for stopping measurement and keep displaying current screen
	 * Button right for resetting screen to only coordinateSystem and reset time, etc.
	*/

	// Button left clicked; Start measuring
	// only allow start after stop or reset
	/*if (btnLeft.get() || (clicked_start && (clicked_stop || clicked_reset))) {
		// set start; reset all other button flags
		clicked_start	= true;
		clicked_stop	= false;
		clicked_reset	= false;
	}

	// Button right clicked; Reset display, time, old measurements
	// only allow reset after stopping measurement
	if (btnRight.get() || (clicked_reset && clicked_stop)) {
		// set reset; reset start button flag
		clicked_start	= false;
		clicked_stop	= true;
		clicked_reset 	= true;
		// reset display, time, volts array
		x = 1;
		drawDefaultScreen(0, 0);	// dummy values for time, voltage
		continue;
	}

	// Button centered clicked; Stop measuring and keep current display
	// must be checked after other buttons, because continue would otherwise stop program forever
	if (btnCtrl.get() || clicked_stop) {
		// set stop; reset all other button flags
		clicked_start	= false;
		clicked_stop	= true;
		clicked_reset	= false;
		// keep current display until reset button pressed
		continue;
	}*/


    // Textausgabe (dauert lange!)
	// can't print floats, ask Prof



    /*
     * Pixel ausgeben
     * Draw all content of volts array
     * corresponds to measurement of 1 measurement of volt per 100µs
     * 1000 * 0.0001 = 0.1s measurement
    */
    if (voltsVoll && !drawnOnce) {
    	/*int dashStart = ymax/2 - DASHBREITE/2;
    	int dashEnd = ymax/2 + DASHBREITE/2;
    	// draw beschriftung at y=0-hae
    	screen.drawLine(x, dashStart, x, dashEnd, 1, Color::Red);*/
    	// draw the 0.1s of measurement
    	// 10^-4 * 1000 = alle 0.1s ist voltsVoll = true
    	for (int i=0; i<SAMPLESIZE; i++) {
    		if (i == 700) {
    			printf("test");
    		}
    		// every 100µs 1 value is measured
    		// so the time difference between 2 values is 100µs
    		x = i + firstLabel; // offset, start drawing from firstLabel on	//xCoordFromTime(i, pixelPerSecond);
    		y = yCoordFromVolt(volts[i], pixelPerVolt);
    		screen.drawPixel(x, y, Color::Yellow); // draw corresponding pixel of measured values
    		printInfoText(volts[i]);	// print info about current measured values
    		// for some reason this refresh has to stay
    		// or else sometimes the screen doesn't update after certain time values
    		screen.refresh();

    		// TODO: check for end of screen and clear
    	}
    	// only draw the volt array once for perfomance reasons
    	// the values will stay on screen automatically until new sample is started
    	drawnOnce = true;
    }

    /*x = xCoordFromTime(timer, pixelPerSecond);
    y = yCoordFromVolt(volt, pixelPerVolt);
    screen.drawPixel( x , y, Color::Yellow );*/


    // seconds
    //endOfScreenHandle(timer.time);


    // Bildschirm aktualisieren
    screen.refresh();

    // Ggf. etwas warten, Zeitangabe nicht zuverlaessig
    //System::delayMilliSec( 1 );


  }
}
