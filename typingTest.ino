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
    else{Keyboard.write(output);}
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
 76, 34, 80, 79, 89, 71, 75, 60, 81, 70, 59, 47, 92, 61, 94,123,
//`,  a, b , c , d , e , f , g , h , i , j , k , l , m , n , o ,
 96, 97,120,106,101, 46,117,105,100, 99,104,116,110,109, 98,114,
//p, q , r , s , t , u , v , w , x , y , z , { , | , } , ~ ,del,
108, 39,112,111,121,103,107, 44,113,102, 58, 63,124, 43,126, 8 ,
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
