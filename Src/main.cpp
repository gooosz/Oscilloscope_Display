//*******************************************************************
/*!
\file   ...
\author Thomas Breuer, Len-Marvin Adler
\date   12.01.2024
\brief  Display for Oscilloscope and ADC measuring
*/

//*******************************************************************
#include <stdio.h>
#include "math.h"

//-------------------------------------------------------------------
#include "EmbSysLib.h"
#include "ReportHandler.h"
#include "config.h"


//-------------------------------------------------------------------
const int xmax = screen.getWidth()  - 1;
const int ymax = screen.getHeight() - 1;

//-------------------------------------------------------------------
// y-axis will be labeled from -15 to +15
// that makes 30 different labels to display
const int voltmin = -5;
const int voltmax = 5;
const int numOfVoltLabels = abs(voltmax) + abs(voltmin);
const int pixelPerVolt = ymax / numOfVoltLabels;
const int onlyLabelEvery = 2; //voltmax / 7.5;	// only label every onlyLabelEvery times, (every 2 times)

// voltages at gpio pin
const int gpio_vmin = 0;
const float gpio_vmax = 3.3;


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


//-------------------------------------------------------------------
// voltage measurements are stored in voltage array
// amount of pixels from firstLabel to lastLabel will be the sample size
// just use screen size which is max possible sample size
// but only store values until (lastLabel - firstLabel)
// this way VLA can be avoided
#define SAMPLESIZEMAX 800 //(799 - 2*4);
const int sampleSize = lastLabel - firstLabel;
// 799px on display, 4px margin left and margin right
// you can only display one value per pixel
volatile float volts[SAMPLESIZEMAX] = {0};
volatile bool voltsVoll = false;
volatile int voltCount = 0;

/// get Voltage at GPIO Pin
///
/// @returns voltage measured at GPIO/ADC A1
/// between 0V and 3.3V
float getADCVolt(void)
{
	// 16-Bit IDR register
	return (adc.get(adc_A1) * gpio_vmax) / (unsigned)0xFFFF;
}

/// get input voltage from voltage measured at gpio pin
///
/// does what is described in our paper in section 4.3
/// @param gpio_volt voltage measured by ADC1 GPIO Pin
/// @param u_offset voltage that got added to input voltage via opamp
float getInputVoltage(float gpio_volt, float u_offset)
{
	float r1 = 230;	// ohm
	float r2 = 100;	// ohm
	float u_vorst = gpio_volt * (r1 + r2) / (float)r2;
	return u_vorst - u_offset;
}

// how many 100th µs have elapsed since starting a measure
// if this is >= 50000, so 5s then stop restarting measurement and just print the measured sample

/// restarts measuring a sample
///
/// so take a new sample
void restartMeasurement(int &time)
{
	voltCount = 0;
	voltsVoll = false;
	time = 0;
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
	if (volt > voltmax) {
		volt = voltmax;
	}
	if (volt < voltmin) {
		volt = voltmin;
	}
	// pixel on screen is volt * pixelPerVolt
	// because pixelPerVolt is amount of pixel needed for 1V
	// 0V to 15V is ymax/2 to 0px
	return (int) (ymax/2 - volt * pixelPerVolt);
}


/// Timer needed for using RTC, Timer Interrupt
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
		time++;
		if (!voltsVoll) {
			// U_offset is 5V
			volts[voltCount++] = getInputVoltage(getADCVolt(), 5);
			if (voltCount == sampleSize) {
				voltsVoll = true;
			}
		}
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

	  // ---horizontal axis---
	  // draw x-achse und Beschriftungen t fuer Zeit
	  screen.drawLine(0, ymax/2, xmax  , ymax/2, 1, Color::Red ); // horizontal

	  label = 0;
	  // don't draw 0 again, because the y-axis always did
	  for (int x=firstLabel; x<=xmax; x+=pixelPerSecond) {
		  	  int dashStart = ymax/2 - DASHBREITE/2;
		  	  int dashEnd = ymax/2 + DASHBREITE/2;
			  // draw beschriftung at y=0-hae
			  screen.drawLine(x, dashStart, x, dashEnd, 1, Color::Red);
			  if (label % 8 == 0 && label > 0) {
				  screen.drawText(x-5, dashStart+15, "%d", label);
			  }
			  label += 8;
	  }

}

/// prints value of time range to top left of screen
void printTimeRange(void)
{
	// print time range = 0.1s on top left of screen
	screen.drawText(10, 10, "time range = %dms", 80);
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


  int x = firstLabel; // x is time-axis
  int y = 0;

  // drawn once is used to indicate if volts array has been already drawn
  // as it only needs to be drawn once until next measurement starts (btnRight -> clicked_next)
  bool drawnOnce = false;


  while( 1 )
  {
	  if (btnRight.getEvent() == Digital::Event::ACTIVATED) {
		  // start new adc measurement, reset time
		  restartMeasurement(timer.time);
		  screen.clear();	// clear old sample from screen
		  drawCoordinateSystem(pixelPerVolt, pixelPerSecond);
		  drawnOnce = false;	// new sample hasn't been drawn yet
	  }

	  /*
	   * Pixel ausgeben
	   * Draw all content of volts array
	   * corresponds to measurement of 1 measurement of volt per 100µs
	   * 800 * 0.0001 = 0.08s measurement
	  */
	  if (voltsVoll && !drawnOnce) {
    	// draw time range = 0.08s as info
    	printTimeRange();
    	// 10^-4 * 1000 = alle 0.08s ist voltsVoll = true
    	for (int i=0; i<sampleSize; i++) {
    		// every 100µs 1 value is measured
    		// so the time difference between 2 values is 100µs
    		x = i + firstLabel; // offset, start drawing from firstLabel on
    		y = yCoordFromVolt(volts[i], pixelPerVolt);
    		screen.drawPixel(x, y, Color::Yellow); // draw corresponding pixel of measured values
    	}
    	// only draw the volt array once for performance reasons
    	// the values will stay on screen automatically until new sample is started
    	drawnOnce = true;
	  }

	  // Bildschirm aktualisieren
	  // for some reason both refreshes has to stay
	  // or else sometimes the screen doesn't update after certain time values
	  screen.refresh();
	  screen.refresh();
  }
}
