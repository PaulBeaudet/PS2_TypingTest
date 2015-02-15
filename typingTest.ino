//typingTest.ino Copyright 2015 Paul Beaudet - See license.txt for details
//type performance testing using the E1115B ps2 convertion module
// HARDWARE - Micro: RX to module TX / 5v to 5 / GND to 0V

#include <avr/pgmspace.h>//explicitly stated read only memory

void setup()
{
  Serial1.begin(115200); //E1115B baud rate when "DR" pin is open
                         //Rate is 56k when connected to ground
  Serial.begin(115200);  //Heads up display
  Keyboard.begin();
}

void loop()
{
  inOut(); 
}

void inOut()
{
  static byte lastOutput = 0;
  
  if(Serial1.available())
  {
    byte input = Serial1.read();
    byte output = convertion(input);
    if(output)
    {
      if(output != 9 && lastOutput == 9){Keyboard.releaseAll();}
      Keyboard.write(output);
      if(output != 9 && lastOutput != 9){Keyboard.releaseAll();}
      transferTime(output);
      lastOutput = output;
    }
    else
    {
      controlChars(input);
      transferTime(input);
    }
  }
}

const byte dvorak[] PROGMEM =
{ //127 byte ASCII convertion table
//S, ! , " , # , $ , % , & , ' , ( , ) , * , + , , , - , . , / ,
 32, 33, 95, 35, 36, 37, 38, 45, 40, 41, 42,125,119, 91,118,122,
//0, 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 , : , ; , < , = , > , ? ,
 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 83,115, 87, 93, 86, 90,
//@, A , B , C , D , E , F , G , H , I , J , K , L , M , N , O ,
 64, 65, 88, 74, 69, 62, 85, 73, 68, 67, 72, 84, 78, 77, 66, 82,
//P, Q , R , S , T , U , V , W , X , Y , Z , [ , \ , ] , ^ , _ ,
 76, 34, 80, 79, 89, 71, 75, 60, 81, 70, 58, 47, 92, 61, 94,123,
//`,  a, b , c , d , e , f , g , h , i , j , k , l , m , n , o ,
 96, 97,120,106,101, 46,117,105,100, 99,104,116,110,109, 98,114,
//p, q , r , s , t , u , v , w , x , y , z , { , | , } , ~ ,del,
108, 39,112,111,121,103,107, 44,113,102, 59, 63,124, 43,126, 8 ,
};

#define SPECIALCASES 12
const byte special[2][SPECIALCASES] PROGMEM =
{//B,T, CR,LFT,RGT, UP,DWN,HME,PGU,PGD,END,DEL 
  {8,9, 13,219,221,218,220,193,194,197,196,195},
  //conversion from keyboard(top) to virtual keyboard(bottom)
  {8,8,176,216,215,218,217,210,211,214,213,  9}
 //0,1, 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 , 10,
};

byte convertion(byte letter)
{
  static boolean qwerty = false;
  if(letter == 192) //toggle layout via insert key
  {
    qwerty = !qwerty;
    return 0;
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
    return 0;//given nothing found, return raw value
  } 
  if(qwerty){return letter;}
  else{return pgm_read_byte(&dvorak[letter-32]);}
}

void controlChars(byte input)
{
  if(input == 129 || input == 136)//control cases
  {
    Keyboard.press(128);//KB_LEFT_CTRL
  }
  if(input == 128 || input == 135)
  {
    Keyboard.press(130);//KB_LEFT_ALT
  }
  if(input == 208)//press zero for upload
  {
    Keyboard.press(128);
    Keyboard.press(117);//u
    Keyboard.releaseAll();
  }
}

//--------- Performance testing functions ---------------

void transferTime(byte letter)//real time output
{
  static unsigned long durration = 0;
  
  unsigned long transTime = millis() - durration;
  dataOutput(transTime, letter); //componded output
  if(letter < 32 || letter > 127){Serial.print(letter, DEC);}
  else {Serial.write(letter);}
  Serial.print(F("-"));
  Serial.println(transTime);
  durration = millis();
}

//detailed reporting
#define RECORDEDPOSITIONS 9
#define LASTPOS RECORDEDPOSITIONS-1
#define RECLIMIT RECORDEDPOSITIONS-2
#define IDLETIME 3000
#define HISTORY 15
#define AMINUTE 60000

void dataOutput(unsigned long durration, byte letter)
{
  SPW(durration, letter);
  speedo(durration, letter);
  errorTime(durration, letter);
  //wordTime(durration, letter);
}

void SPW(unsigned long durration, byte letter)//speed per word
{
  static unsigned int history[HISTORY];
  static byte writePlace = 0;
  static boolean newWord = false;

  if(letter == 8)
  {
    if(newWord){newWord=false;}
    writePlace--;
    history[writePlace]=0;
  }
  else
  {
    if(letter == 32){newWord=true;}
    else if(newWord)
    {
      unsigned int think = 0; unsigned int totalMs = 0;
      for(byte i = 0;i<HISTORY;i++)
      {
        totalMs += history[i];
        if(history[i] > think){think = history[i];}
        history[i]=0;
      }
      Serial.print(F("word @")); Serial.println(totalMs);
      unsigned int spw = AMINUTE/((totalMs - think) / (writePlace-1)) / 5;
      Serial.print(spw); Serial.println(F(" SPW"));
      averageSPW(spw);
      writePlace = 0;
      newWord = false;
    }
    history[writePlace]=durration;
    writePlace++;
  }
}

void averageSPW(byte spw)
{
  static unsigned int lastSpeeds[HISTORY];
  static byte count = 0;
  
  if(count == HISTORY)
  {//do the average and reset the history and count
    unsigned int totalMs = 0;
    for(byte i = 0; i < HISTORY; i++)
    {
      totalMs += lastSpeeds[i];
      lastSpeeds[i]=0;
    }
    Serial.print(totalMs/HISTORY);
    Serial.println(F("spw running"));
    count=0;
  }
  lastSpeeds[count] = spw;
  count++;
}

void speedo(unsigned long currentTranfer, byte letter)
{
  static unsigned long totalTime = 0;
  static byte wordPosition = 0;
  static unsigned int totalWords = 0;
  static unsigned long positionTotal[RECORDEDPOSITIONS];
  static unsigned int strokes = 0;

  if(currentTranfer > IDLETIME)
  {//after idle
    if(totalTime && strokes > 4)
    { //given that tranfer acumalated and at least 5 strkes made
      unsigned long cpm = 60000 / (totalTime / strokes);
      Serial.print(cpm);
      Serial.print(F("CPM "));
      Serial.print(cpm / 5);
      Serial.println(F("raw"));
      for(byte i = 0; i < LASTPOS; i++)
      {
        Serial.print(F("P"));
        Serial.print(i);
        Serial.print(F(":"));
        Serial.print(positionTotal[i]);
        Serial.print(F("ms "));
      }
      Serial.print(F("LAST:"));
      Serial.println(positionTotal[LASTPOS]);
      Serial.print(strokes);
      Serial.print(F("strokes to "));
      Serial.print(totalWords);
      Serial.print(F("words @"));
      Serial.print(totalTime);
      Serial.println(F("ms"));
    }
    //reset all counters regardless of print condition
    for(byte i = 0; i < RECORDEDPOSITIONS; i++){positionTotal[i]=0;}
    totalTime = 0;
    strokes = 0;
    totalWords = 0;
  }
  else //collect information
  {
    totalTime += currentTranfer;
    strokes++;
    if(letter == 32)//display and communicate info about words
    {
      wordPosition = 0;
      positionTotal[LASTPOS] += currentTranfer;
      totalWords++;
    }
    else
    {
      positionTotal[wordPosition] += currentTranfer;
      if(wordPosition < RECLIMIT){wordPosition++;}
    }
  }
}

void wordTime(unsigned long durration, byte letter)
{
  static unsigned long wordDurration = 0;
  
  wordDurration += durration;
  if(letter == 32)
  {
    Serial.print(F("word @"));
    Serial.println(wordDurration);
    wordDurration = 0;
  }
}

void errorTime(unsigned long durration, byte letter)
{
  static unsigned int corrections = 0;
  static unsigned int errorTime = 0;
  static unsigned int history[HISTORY];
  static byte writePlace = 0;
  static byte numRemoved = 0;
  static boolean newWord = false;
  
  if(durration > IDLETIME)
  {//after idle  
    Serial.print(corrections);Serial.print(F("BS @"));
    Serial.print(errorTime);  Serial.println(F("ms"));
    corrections = 0;
    errorTime = 0;
  }
  else
  {
    if(letter == 8)
    {
      corrections++;
      if(newWord){newWord=false;}
      errorTime += durration;
      if(writePlace)
      {
        writePlace--;
        errorTime += history[writePlace];
      }
      history[writePlace]=0;
    }
    else 
    {
      if(letter == 32){newWord=true;}
      else if(newWord)
      {
        for(byte i = 0;i<HISTORY;i++){history[i]=0;}
        writePlace = 0;
      }
      history[writePlace]=durration;
      writePlace++;
    }
  }
}
