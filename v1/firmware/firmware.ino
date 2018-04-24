/*
*   .__a__.        .__a__.
 *   |     |        |     |
 *   f     b        f     b
 *   |     |        |     |
 *   .__g__.        .__g__.
 *   |     |        |     |
 *   e     c        e     c
 *   |     |        |     |
 *   .__d__. *        .__d__. * 
 * 
 * |__________|      |____________|
 *   digit 2            digit 1



     g f G a b    g f G a b    
   ._|_|_|_|_|____|_|_|_|_|_.
   |                        |
   |     8            8     |
   |        *            *  |
   ._|_|_|_|_|____|_|_|_|_|_.
     e d G c *    e d G c * 


left:
==============
PC6 ----    reset
PD0 7segment: e
PD1 7segment: d 
PD2 7segment: c
PD3 7segment: g
PD4 7segment: f
.
.
PB6 7segment: a
PB7 7segment: b
PD5 7segment: digit 1 enable
PD6   
PD7
PB0 beeper



right:
=============
PC5
PC4
PC3
PC2
PC1
PC0
.
.
.
PB5 ---     sck
PB4 ---     miso
PB3 ---     mosi
PB2
PB1 7segment: digit 2 enable
 */


// ############################################################################

#define BEEPER_ENABLE 0 /*B*/
volatile short __beepingCounter = 0;
volatile bool beep = false;

void setupBeeper() {
  //pin
  DDRB |= (1<<BEEPER_ENABLE);
}

void shortBeep() {
  beep = true;
  delay(300);
  beep = false;
}
// ############################################################################
#define SEV_SEGMENT_DIG2EN 1 /*B*/
#define SEV_SEGMENT_DIG1EN 5 /*D*/

#define SEV_SEGMENT_A 6 /*B*/
#define SEV_SEGMENT_B 7 /*B*/
#define SEV_SEGMENT_C 2 /*D*/ 
#define SEV_SEGMENT_D 1 /*D*/  
#define SEV_SEGMENT_E 0 /*D*/
#define SEV_SEGMENT_F 4 /*D*/
#define SEV_SEGMENT_G 3 /*D*/ 

// number to seven segment lookup table   
//0bXgfedcba
#define SEV_SEGMENT_SYMBINDX_DASH 10
#define SEV_SEGMENT_SYMBINDX_C 11
#define SEV_SEGMENT_SYMBINDX_P 12
#define SEV_SEGMENT_SYMBINDX_E 13
volatile const char SEVSEG_DIGIT_TO_SEGMENTS[] = {
    0b00111111, //0
    0b00000110, //1
    0b01011011, //2
    0b01001111, //3
    0b01100110, //4
    0b01101101, //5
    0b01111100, //6
    0b00000111, //7
    0b01111111, //8
    0b01100111, //9
    0b01000000, // dash ( - )
    0b00111001, // C
    0b01110011, // P
    0b01111001 // E
};

volatile char segments[] = {0,0};

void writeNumberToDisplay(char num) {
  if (num < 0 || num > 99) {
    segments[0] = SEVSEG_DIGIT_TO_SEGMENTS[SEV_SEGMENT_SYMBINDX_DASH]; 
    segments[1] = SEVSEG_DIGIT_TO_SEGMENTS[SEV_SEGMENT_SYMBINDX_DASH]; 
  }
  
  segments[0] = SEVSEG_DIGIT_TO_SEGMENTS[num%10]; 
  segments[1] = SEVSEG_DIGIT_TO_SEGMENTS[num/10];

  if (num < 10) {
    //don't show leading 0 for small numbers
    segments[1] = 0;
  }
}


//a=0, b=1, c=2, d=3, e=4, f=5, g=6
void activateSegment(char segNum) {
  switch(segNum) {
    case 0:
      PORTB |= (1<<SEV_SEGMENT_A);
      break;
    case 1:
      PORTB |= (1<<SEV_SEGMENT_B);
      break;
    case 2:
      PORTD |= (1<<SEV_SEGMENT_C);
      break;
    case 3:
      PORTD |= (1<<SEV_SEGMENT_D);
      break;
    case 4:
      PORTD |= (1<<SEV_SEGMENT_E);
      break;
    case 5:
      PORTD |= (1<<SEV_SEGMENT_F);
      break;
    case 6:
      PORTD |= (1<<SEV_SEGMENT_G);
      break;
  }
}

void clearSegments() {
  PORTB &= ~( (1<<SEV_SEGMENT_A) | (1<<SEV_SEGMENT_B));
  PORTD &= ~( (1<<SEV_SEGMENT_C) | (1<<SEV_SEGMENT_D) | (1<<SEV_SEGMENT_E) | (1<<SEV_SEGMENT_F) | (1<<SEV_SEGMENT_G));
}

void setup7Segment() {
  //7 segment setup
  DDRB |= (1<<SEV_SEGMENT_DIG2EN) | (1<<SEV_SEGMENT_A) | (1<<SEV_SEGMENT_B);
  DDRD |= (1<<SEV_SEGMENT_DIG1EN) | (1<<SEV_SEGMENT_C) | \ 
         (1<<SEV_SEGMENT_D) |  (1<<SEV_SEGMENT_E) | (1<<SEV_SEGMENT_F) | (1<<SEV_SEGMENT_G);
 
  //only activate one digit so we can strobe inputs with xor
  PORTB |= (1<<SEV_SEGMENT_DIG2EN);

  //timer driven interrupt that strobes 7 segment display
  TCCR1A |= (1<<WGM12); //normal operation, pins disconnected 
  TCCR1B |= (1<<CS10); // no prescaler

  // compare match value
  OCR1AL = 255;
  OCR1AH = 0;
  
  TIMSK1 |= (1<<OCIE1A); //enable compare interrupt
}

// ############################################################################

ISR(TIMER1_COMPA_vect) {
  // 7 Segment strobe
  clearSegments();
  PORTB ^= (1<<SEV_SEGMENT_DIG2EN);
  PORTD ^= (1<<SEV_SEGMENT_DIG1EN);
  

  //7 Segment display number
  //digit to display is the currently active one
  char seg = (PORTD & (1<<SEV_SEGMENT_DIG1EN) ? segments[0] : segments[1]);
 
  //no need to check the last bit, not used anyways
   for(char i=0;i<7;++i) {
    if (seg & (1<<i)) {
       activateSegment(i);
     }
   }
  

  //beeper if enabled
  if (beep) {
    __beepingCounter++;
    if (__beepingCounter > 50) { //beep interval is set here
      PORTB ^= (1<<BEEPER_ENABLE);
      __beepingCounter = 0;
    }
  } else {
    PORTB &= ~(1<<BEEPER_ENABLE);
    __beepingCounter = 0;
  }
}

// ############################################################################


void setupPowerSaver() {
  //disable two wire, timer 2, spi, uart
  PRR |= (1<<7) | (1<<6) | (1<< 2) | (1<<1);
}

// ############################################################################

void setupADC() {
  //input
  DDRD &= ~( (1<<6) | (1<<7));
  DDRC &= ~( (1<<0) | (1<<1) | (1<<2) | (1<<3) | (1<<4) | (1<<5));
  PORTD &= ~( (1<<6) | (1<<7));
  PORTC &= ~( (1<<0) | (1<<1) | (1<<2) | (1<<3) | (1<<4) | (1<<5));

  ACSR |= (1<<ACD);
  
  //1.1V internal reference
  ADMUX |= (1<<REFS1) | (1<<REFS0);

  //enable and 64 prescaler to get 125khz clock to adc
  ADCSRA |= (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1);
}

#define ADC_AVERAGES_CNT 20
char getADCReading(char index) {
  if (index < 0 || index > 5) {
    return -100; //invalid index
  }
  
  //set adc mux to selected one
  ADMUX &= 0b11110000; //clear
  ADMUX |= index & 0b00000111; //only using 3 bits for 0-7
  delay(5); //settle

  short sum = 0;
  for(char i=0;i<ADC_AVERAGES_CNT;++i) {
    ADCSRA |= (1<<ADSC);
    while(ADCSRA & (1<<ADSC)) {}
    sum += short(ADCL) | ( short(ADCH)<<8);
    delayMicroseconds(100); //low pass filter around 10khz 
  }
  sum /= ADC_AVERAGES_CNT;

  //lm35  has 10mv per °C starting at 0°C;   negative temperatures have negative voltage
  //we can't handle those because I didn't bias the sensor inputs
  //int temperature =  ((sum * 1024) / 1100);  for some reason, the sensors read perfectly without this math, 
  //just divide by ten for real temp
 
  return  sum / 10;
}

// on start we check where probes are connected,  
// connected probe is indicated by a value greater than 1 (this implies that when the device is
// started in sub 0 temperatures it won't work, see getADCReading
char temperatures[] = {0, 0, 0, 0, 0, 0};

// ############################################################################

void setup() { 
  cli();
  setup7Segment();
  setupBeeper();
  setupPowerSaver();  
  setupADC();
  sei();

  // test beeper
  beep = true;
  delay(500);
  beep = false;
}

//assuming critical temp is above warn temp
#define CRITICAL_TEMPERATURE 40   /*°C*/
#define WARN_TEMPERATURE 35  /*°C*/

enum MENUS {
  SHOW_CONNECTED_PROBES,
  TEMP_CYCLE, 
  TEMP_WARNING,
  TEMP_CRITICAL
};

MENUS currentMenu = SHOW_CONNECTED_PROBES;

bool firstTempReading = true;

char tempCycleMenu_Index = 0;
char tempWarningAndCriticalMenu_Index = 0;

bool tempWarningMenu_Blinking = false; //internal use
int tempWarningMenu_ContinuousBeepCounter = 0;


void loop() {
   //update adc readings 
   //in theory, all unconnected probes should read 0,  
   for(char i = 0;i<sizeof(temperatures);++i) {
      char currReading = getADCReading(i);
      if (!firstTempReading) {
        //if a probe that wasn't connected at start ignore it
        //todo: better solution so user doesn't have to count probes to make sure everything is fine
        if (temperatures[i] == 0) {
          continue;
        }
        
        //error in aquision or probe disconnected or cabeling error 
        if (abs(currReading - temperatures[i]) > 5) {
          currentMenu = TEMP_CRITICAL;
          tempWarningAndCriticalMenu_Index = i; 
          break;
        } 
        // all clear, update temperature 
        temperatures[i] = currReading;
      }else{
        //initial readout to determine if senspors are present
        //no sensor connected when reading below threshold 
        //although design features pulldown resistors, strong inducted currents still drive
        //value up
        if (currReading > 20) {
          temperatures[i] = currReading;
        }
      }
   }
   firstTempReading = false;

  if (currentMenu != TEMP_CRITICAL) {
    //analyze temperatures
    char highestIndex = 0;
    for(char i = 1;i<sizeof(temperatures);++i) {
      if (temperatures[i] > temperatures[highestIndex]) {
        highestIndex = i;
      }
    }
  
    //reset warning back to temp cycle
    if (currentMenu == TEMP_WARNING && temperatures[highestIndex] < WARN_TEMPERATURE) {
       currentMenu = TEMP_CYCLE;
    }
  
    //set warnings/error if neccessary 
    if (temperatures[highestIndex] >= WARN_TEMPERATURE) {
      if (temperatures[highestIndex] >= CRITICAL_TEMPERATURE) {
        currentMenu = TEMP_CRITICAL;
      } else {
        if (currentMenu != TEMP_WARNING) {
          //don't continue beeping from here after we set the status the loop before
           shortBeep();
        }
        currentMenu = TEMP_WARNING;
      }
      tempWarningAndCriticalMenu_Index = highestIndex;
    }
  }
  
  char connectedProbes = 0;
   switch (currentMenu) {
    case SHOW_CONNECTED_PROBES:
        for(char i;i<sizeof(temperatures);++i) {
          if (temperatures[i] > 0) {
            ++connectedProbes;
          }
        }
        // unneccessary now because array size is hardoded above todo: fix
        if (connectedProbes > 9) {
          connectedProbes = 9;
        }
        segments[0] = SEVSEG_DIGIT_TO_SEGMENTS[connectedProbes];
        segments[1] = SEVSEG_DIGIT_TO_SEGMENTS[SEV_SEGMENT_SYMBINDX_C];
        delay(2000);
        currentMenu = TEMP_CYCLE;
        tempCycleMenu_Index = 0;
        break;
    case TEMP_CYCLE:
      //check if probe is connected, if not go to next one
      do {
        //endless loop when no probe is connected
        for (;tempCycleMenu_Index<sizeof(temperatures);++tempCycleMenu_Index) {
          if (temperatures[tempCycleMenu_Index] > 0) {
            break;
          }
        }
        if (tempCycleMenu_Index > sizeof(temperatures)-1) {
          //reset index 
          tempCycleMenu_Index = 0;
        }
      } while(temperatures[tempCycleMenu_Index] == 0);
    
      //show probe number
      segments[0] = SEVSEG_DIGIT_TO_SEGMENTS[tempCycleMenu_Index];
      segments[1] = SEVSEG_DIGIT_TO_SEGMENTS[SEV_SEGMENT_SYMBINDX_P];
      delay(500);
      //show temp
      writeNumberToDisplay(temperatures[tempCycleMenu_Index]);
      delay(500);
      ++tempCycleMenu_Index;
     break;
     case TEMP_WARNING:
       if (tempWarningMenu_Blinking) {
        //show probe number
        segments[0] = SEVSEG_DIGIT_TO_SEGMENTS[tempWarningAndCriticalMenu_Index];
        segments[1] = SEVSEG_DIGIT_TO_SEGMENTS[SEV_SEGMENT_SYMBINDX_P];
       } else {
        segments[0] = 0;
        segments[1] = 0;
       }
       tempWarningMenu_Blinking = !tempWarningMenu_Blinking;
       tempWarningMenu_ContinuousBeepCounter++;
       if (tempWarningMenu_ContinuousBeepCounter > 40) {
        tempWarningMenu_ContinuousBeepCounter = 0;
        shortBeep();
       }
       delay(250);
    break;
    case TEMP_CRITICAL:
      segments[0] = SEVSEG_DIGIT_TO_SEGMENTS[tempWarningAndCriticalMenu_Index];
      segments[1] = SEVSEG_DIGIT_TO_SEGMENTS[SEV_SEGMENT_SYMBINDX_P];
      beep = true;
      while(true){}
    break;
    default:
      segments[0] = SEVSEG_DIGIT_TO_SEGMENTS[SEV_SEGMENT_SYMBINDX_DASH];
      segments[1] = SEVSEG_DIGIT_TO_SEGMENTS[SEV_SEGMENT_SYMBINDX_DASH];
      beep = true;
    break;
  }
   

}
