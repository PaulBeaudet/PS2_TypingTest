//typingTest.ino
//type performance testing using the E1115B ps2 convertion module
// HARDWARE - Micro: RX to module TX / 5v to 5 / GND to 0V

#include <avr/pgmspace.h>//explicitly stated read only memory

void setup()
{
  Serial1.begin(115200); //E1115B baud rate when "DR" pin is open
                        //Rate is 56k when connected to ground
  Serial.begin(115200);
  Keyboard.begin();
}

void loop()
{
  inOut(); 
}

void inOut()
{
  if(Serial1.available())
  {
    byte input = Serial1.read();
    byte output = convertion(input);
    if(output == 0xff){Serial.println(input, DEC);}
    else
    {
      transferTime(output);
      Keyboard.write(output);
    }
  }
}

const byte dvorak[] PROGMEM =
{ //127 byte ASCII convertion table
//S, ! , " , # , $ , % , & , ' , ( , ) , * , + , , , - , . , / ,
 32, 33, 95, 35, 36, 37, 38, 45, 40, 41, 42,125,119, 91,118,122,
//0, 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 , : , ; , < , = , > , ? ,
 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 83,115, 87,125, 86, 90,
//@, A , B , C , D , E , F , G , H , I , J , K , L , M , N , O ,
 64, 65, 88, 74, 69, 62, 85, 73, 68, 67, 72, 84, 78, 77, 66, 82,
//P, Q , R , S , T , U , V , W , X , Y , Z , [ , \ , ] , ^ , _ ,
 76, 34, 80, 79, 89, 71, 75, 60, 81, 70, 58, 47, 92, 61, 94,123,
//`,  a, b , c , d , e , f , g , h , i , j , k , l , m , n , o ,
 96, 97,120,106,101, 46,117,105,100, 99,104,116,110,109, 98,114,
//p, q , r , s , t , u , v , w , x , y , z , { , | , } , ~ ,del,
108, 39,112,111,121,103,107, 44,113,102, 59, 63,124, 43,126, 8 ,
};

#define SPECIALCASES 11
const byte special[2][SPECIALCASES] PROGMEM =
{//B,T, CR,LFT,RGT, UP,DWN,HME,PGU,PGD,END 
  {8,9, 13,219,221,218,220,193,194,197,196,},
  //conversion from keyboard(top) to virtual keyboard(bottom)
  {9,8,176,216,215,218,217,210,211,214,213,}
 //0,1, 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 , 10,
};

byte convertion(byte letter)
{
  static boolean qwerty = true;
  if(letter == 192) //toggle layout via insert key
  {
    qwerty = !qwerty;
    return 0xff; //ignore this case
  }
  
  if(letter < 32 || letter > 127)
  {
    for(byte i = 0; i < SPECIALCASES; i++)
    {
      if(pgm_read_byte(&special[0][i])==letter)
      {
        return pgm_read_byte(&special[1][i]);
      }
    }
    return 0xff; //ignore other cases given nothing is found in these ranges
  }
  
  if(qwerty){return letter;}
  else{return pgm_read_byte(&dvorak[letter-32]);}
}

// Hold for caps

byte heldASCII(byte letter)
{
  static unsigned long holdTimes = 0;
  static unsigned long lastTime = 0;
  static byte lastLetter = 0;
  
  if(letter == lastLetter)
  {
    if(millis() - lastTime < 250)
    {
      Keyboard.write(8);
      
    }
  }
  else
  {
    holdTimes = 0;
    lastLetter = letter;
    lastTime = millis();
  }

  if(letter == 32){return 0;}//spacebar
  if(letter == 9){return 0;}//TAB_KEY
  
  if(holdTimes == 18 && letter > 95){return 8;}
  if(holdTimes == 19)// first hold
  {//letters covered by main layout
    if(letter > 95){return letter-32;} //shift cases
    return letter; //outside cases are repeating
  }
  if(holdTimes > 43)
  {//outside main layout letters repeat
    if(letter < 95){return letter;}
  }
  return 0; //cases not covered
}
//--------- Performance testing functions ---------------

void transferTime(byte trigger)
{
  static unsigned long durration = 0;
  static byte letter = 0;
  
  Serial.write(letter);
  Serial.print(F(" -> "));
  Serial.write(trigger);
  Serial.print(F(" = "));
  unsigned long transTime = millis() - durration;
  Serial.println(transTime);
  speedClock(transTime);
  durration = millis();
  letter = trigger;
}

void speedClock(unsigned long currentTranfer)
{
  static unsigned int transferTotal = 0;
  static unsigned int writeProgression = 0;
  static unsigned int lastCadence = 0;
  
  if(currentTranfer > 5000) //idle case
  {
    transferTotal = 0;//reset recording
    writeProgression = 0;
    return;
  }
  
  transferTotal += currentTranfer;
  writeProgression++;
  if(transferTotal > 6000 && writeProgression > 4)
  {
    unsigned int multiplier = 60000 / transferTotal;
    unsigned int cadence = writeProgression / 5;
    if(cadence != lastCadence)
    {
      Serial.print(cadence * multiplier);
      Serial.println(F("rawWPM"));
      lastCadence = cadence;
      transferTotal = 0;//reset recording
      writeProgression = 0;
    }
  }
    
}
