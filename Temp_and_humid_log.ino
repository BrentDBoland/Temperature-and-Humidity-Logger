#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MAX31856.h>
#include <DHT.h>

#define DHTTYPE DHT22
#define BUTTON_A 9
#define BUTTON_B 6
#define BUTTON_C 5
#define LED      13
#define cardSelect 4
#define buzz 10
#define oneshot false


//file vars
File logfile;
File lastFile;
char filename[15];

//temperature and humidity vars
float currentT[10];
float currentCJTorH[10];

//internal controls
long currentmillis;
long previousmillis;
int measureNUM;
bool SDfound;
bool continuouslog=false;

//measurement settings
long interval=2000; //measurement interval 
bool buzzer=false; // J9 will be unusable while true
bool bpressed=false;
max31856_thermocoupletype_t tctype = MAX31856_TCTYPE_K;

//Accessory vars
Adafruit_SSD1306 display = Adafruit_SSD1306();
Adafruit_MAX31856 TCRs[10];
DHT HS[10]={DHT(1, DHTTYPE) , DHT(1, DHTTYPE) , DHT(1, DHTTYPE) , DHT(1, DHTTYPE) , DHT(1, DHTTYPE) , DHT(1, DHTTYPE) , DHT(1, DHTTYPE) , DHT(1, DHTTYPE) , DHT(1, DHTTYPE) , DHT(1, DHTTYPE)};
int8_t pinlist[10]={14,15,13,16,12,17,11,18,10,19}; //[J1,J2,J3,J4,J5,J6,J7,J8,J9,J10]
int8_t currentslot;
bool TCRfound[10];
bool HSfound[10];

 





#if (SSD1306_LCDHEIGHT != 32)
 #error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif


 
void setup() { 

  if(buzzer){
    pinMode(buzz,OUTPUT);
    digitalWrite(buzz,LOW);
  }
  Serial.begin(9600);
 
  Serial.println("OLED FeatherWing test");
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  // init done
  Serial.println("OLED begun");
  

  display.clearDisplay();
  display.display();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  Serial.println("IO test");
 
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  //thermocouple setup
  for(int i=0; i<10; i++){
    if(!(buzzer && i==8)){
      TCRs[i]=Adafruit_MAX31856(pinlist[i]);
      TCRs[i].begin();
      TCRs[i].setThermocoupleType(tctype);
      TCRfound[i]=TCRs[i].getThermocoupleType()==tctype;
    }else{
      TCRfound[i]=false;
    }
  }

  for(int i=0; i<10; i++){
    if(!((buzzer && i==8)||TCRfound[i])){
//      display.setCursor(0,0);
//      display.clearDisplay();
//      display.print("pin ");display.print(pinlist[i]);display.println(" chip ck");
//      display.display();
      HS[i]=DHT(pinlist[i], DHTTYPE);
      HS[i].begin();
      HSfound[i]=!isnan(HS[i].readHumidity());
//      display.print("found ");display.println(HSfound[i]);
//      display.display();
//      delay(500);
    }else{
      HSfound[i]=false;
    }
  }
  
  for(int i=0; i<10&&(!TCRfound[currentslot])&&(!HSfound[currentslot]);i++){
    currentslot=i;
  }
  

  measureNUM=0;
  
  for(int i=0; i<10;i++) {
    if(TCRfound[i]){
      display.setCursor(0,0);
      display.clearDisplay();
      display.print("Slot ");display.print(i);display.println(" TC type: ");
      switch ( TCRs[i].getThermocoupleType() ) {
        case MAX31856_TCTYPE_B: display.println("B Type"); break;
        case MAX31856_TCTYPE_E: display.println("E Type"); break;
        case MAX31856_TCTYPE_J: display.println("J Type"); break;
        case MAX31856_TCTYPE_K: display.println("K Type"); break;
        case MAX31856_TCTYPE_N: display.println("N Type"); break;
        case MAX31856_TCTYPE_R: display.println("R Type"); break;
        case MAX31856_TCTYPE_S: display.println("S Type"); break;
        case MAX31856_TCTYPE_T: display.println("T Type"); break;
        case MAX31856_VMODE_G8: display.println("Voltage x8 Gain mode"); break;
        case MAX31856_VMODE_G32: display.println("Voltage x8 Gain mode"); break;
        default: display.println("Unknown"); break;
      }
      display.display();
      delay(500);
    }
  }
  display.setCursor(0,0);
  display.clearDisplay();
  
  if (!SD.begin(cardSelect)) {
    //blinking message that card could not be initialized
    for(int i=0; i<5; i++){
      display.setCursor(0,0);
      display.clearDisplay();
      if(i%2==0){
        display.println("Card init. failed!");
      }
      display.display();
      delay(200);
    }
    SDfound=false;
  }else{SDfound=true;}

  if (SD.exists("lastFile.txt")){
    lastFile=SD.open("lastFile.txt",FILE_READ);
    lastFile.read(filename,15);
    filenamePLUS(filename);
    lastFile.close();
  } else{
    strcpy(filename, "Log_BA00.txt");
  }
  
  while(digitalRead(BUTTON_C) && SDfound){
    display.setCursor(0,0);
    display.clearDisplay();
    display.print("File: ");
    display.print(filename);
    if (!SD.exists(filename)){
      display.print("\n");
      display.print("<New File>");
    } else{
      display.print("\n");
      display.print("<Open>");
    }
    if(! digitalRead(BUTTON_A)){
      filenamePLUS(filename);
    } else if(! digitalRead(BUTTON_B)){
      filenameMINUS(filename);
    }
    display.display();
    delay(100);
    
  }

  //make new file if none exists. If one exists gives option to overwrite
  if(SD.exists(filename)){
    display.setCursor(0,0);
    display.clearDisplay();
    display.println("Y  Overwrite?");
    display.println("N");
    display.display();
    while(digitalRead(BUTTON_A)&&digitalRead(BUTTON_B)){
    }
    if(!digitalRead(BUTTON_A)){
      SD.remove(filename);
      delay(100);
      logfile=SD.open(filename,FILE_WRITE);
      printheader(logfile);
    }else{
      logfile=SD.open(filename,FILE_WRITE);
    }
  }else{
    logfile=SD.open(filename,FILE_WRITE);
    printheader(logfile);
  }
  
  display.setCursor(0,0);
  display.clearDisplay();
  
  if( !SDfound ){
    logfile.close();
  }else if( !logfile ) {
    //blinking message that file could not be created
    for(int i=0; i<5; i++){
      display.setCursor(0,0);
      display.clearDisplay();
      if(i%2==0){
        display.print("Couldnt create \n");
        display.println(filename);
      }
      display.display();
      delay(500);
    }    
    logfile.close();
    
  } else {
    
    logfile.close();
    SD.remove("lastFile.txt");
    lastFile=SD.open("lastFile.txt",FILE_WRITE);
    if( ! lastFile ) {
      display.print("Couldnt create\n"); 
      display.println("lastFile.txt");
      display.display();
      delay(1000);
    } else {
      lastFile.print(filename);
    }
    lastFile.close();
  }

  display.setCursor(0,0);
  display.clearDisplay();
  display.display();
  if(buzzer){
    digitalWrite(buzz,HIGH);
    delay(200);
    digitalWrite(buzz,LOW);
  }else{delay(200);}
  previousmillis=millis();
  currentmillis=millis();
  
}
 

///////////////Main Body///////////////

void loop() {
  //switch the thermocouple temp displayed
  if (! digitalRead(BUTTON_A)){
    switchDisplay();
  }

  //Log singgle value. Uses and average of singlesampnum points after a delay of singledelay milliseconds if oneshot is off. If oneshot is on it just takes a single point (don't turn on oneshot)
  if (!digitalRead(BUTTON_B) && SDfound && !continuouslog){
    previousmillis=millis();
    measureNUM++;
    
    
    logfile = SD.open(filename, FILE_WRITE);
    if(checkSD()){
  
      logtemps(logfile);
      logfile.close();
  
      display.print("M");display.print(measureNUM);display.println(" Logged");
      display.display();
      beep(200);
      while(currentmillis-previousmillis<interval){
        currentmillis=millis();
      }
      previousmillis+=interval;
    }
    
  }

  //toggle continuous temperature logging
  if (! digitalRead(BUTTON_C) && SDfound){
    if(!bpressed){
      if(!continuouslog){
        beginCmode();
      } else{
        endCmode();
      }
    }
    bpressed=true;  
  }
  if (digitalRead(BUTTON_C)){
    bpressed=false;
  }


  // wait loop
  currentmillis=millis();
  while(digitalRead(BUTTON_A)&&(digitalRead(BUTTON_B)||continuouslog)&&digitalRead(BUTTON_C)&&(currentmillis-previousmillis<interval)){
    currentmillis=millis();
  }

   //Read the temps and set next read time
  if(currentmillis-previousmillis>=interval){
    previousmillis+=interval;
    
    measure();
    printmeasurement();
    if(continuouslog){
      display.print("Continuous Logging M");display.print(measureNUM);
      logfile = SD.open(filename, FILE_WRITE);
      logtemps(logfile);
      logfile.close();
    }
    display.display();
  }
}


/////////////Functions/////////////////

//increments the file name up one
void filenamePLUS(char* filename) {
  filename[5]=filename[5]+((filename[7]-'0'+1)/10+(filename[6]-'0'))/10;
  if(filename[5]>'Z'){
    filename[5]='Z';
  }
  filename[6]='0'+((filename[7]-'0'+1)/10+(filename[6]-'0'))%10;
  filename[7]='0'+(filename[7]-'0'+1)%10;
}

//increments the file name down one
void filenameMINUS(char* filename) {
  filename[5]=filename[5]+((filename[7]-'0'+9)/10+(filename[6]-'0')+9)/10-1;
  if(filename[5]<'A'){
    filename[5]='A';
  }
  filename[6]='0'+((filename[6]-'0')+(filename[7]-'0'+9)/10+9)%10;
  filename[7]='0'+(filename[7]-'0'+9)%10;
}

//Prints lables to the file to identify subsiquent data
void printheader(File lfile){
  lfile.print("meas_num\t");
    lfile.print("time(ms)\t");
    for(int i=0;i<9;i++){
      lfile.print("J");lfile.print(i+1);lfile.print("_sensor\t");
      lfile.print("J");lfile.print(i+1);lfile.print("_TC\t");
      lfile.print("J");lfile.print(i+1);lfile.print("_CJ\t");
    }
    lfile.print("\r\n");
}

//Saves the most recent temperature measurement from the chip to the temperature vars if global var oneshot is false. Otherwise it takes the measurements when called which could take a up to a few seconds.
void measure(void) {
  for (int i=0; i<9;i++){         
    if(TCRfound[i]){
      currentT[i]=TCRs[i].readThermocoupleTemperature(oneshot);
      currentCJTorH[i]=TCRs[i].readCJTemperature(oneshot);
      faultCheck(i);
    }else if(HSfound[i]){
      currentT[i]=HS[i].readTemperature();
      currentCJTorH[i]=HS[i].readHumidity();
    }
  }
  currentmillis=millis();
}

// Logs temps to file passed
void logtemps(File lfile){
  lfile.print(measureNUM);lfile.print("\t");
  lfile.print(currentmillis);lfile.print("\t");
  for(int i=0;i<9;i++){
    if(TCRfound[i]){
      lfile.print("TCR");lfile.print("\t");
      lfile.print(currentT[i]);lfile.print("\t");
      lfile.print(currentCJTorH[i]);lfile.print("\t");
    } else if (HSfound[i]){
      lfile.print("HR");lfile.print("\t");
      lfile.print(currentT[i]);lfile.print("\t");
      lfile.print(currentCJTorH[i]);lfile.print("\t");
    }else{
      lfile.print("NA\tNA\tNA\t");
    }
  }
  lfile.print("\r\n");
}

//clears then prints temps to screen. Must be followed by display command to make visible
void printmeasurement(void){
  if(TCRfound[currentslot]){
    printtemps();
  }else if(HSfound[currentslot]){
    printhumid();
  }else{
    display.setCursor(0,0);
    display.clearDisplay();
    display.println("No sensors found");
    display.println(" ");
    if(!SDfound){
      display.print("No SDcard");
    }
  }
}
void printtemps(void){
  display.setCursor(0,0);
  display.clearDisplay();
  display.print("CJ");display.print(currentslot+1);display.print(" Temp: "); display.print(currentCJTorH[currentslot]);display.println("C");
  display.print("TC");display.print(currentslot+1);display.print(" Temp: "); display.print(currentT[currentslot]);display.println("C");
  if(!SDfound){
    display.print("No SDcard");
  }
}

//clears then prints humidity to screen. Must be followed by display command to make visible
void printhumid(void){
  display.setCursor(0,0);
  display.clearDisplay();
  display.print("T");display.print(currentslot+1);display.print(" Temp: "); display.print(currentT[currentslot]);display.println("C");
  display.print("H");display.print(currentslot+1);display.print(" Humid: "); display.print(currentCJTorH[currentslot]);display.println("%");
  if(!SDfound){
    display.print("No SDcard");
  }
}

//checks for SD card. Only use in loop after logfile has been opened
bool checkSD(void){
  logfile.seek(logfile.position()-1);
  SDfound=!(logfile.read()==-1);
  return SDfound;
}

void beep(long t){
  if(buzzer){
    digitalWrite(buzz,HIGH);
    delay(t);
    digitalWrite(buzz,LOW);
  }
}

void beginCmode(void){
  measureNUM++;
  printmeasurement();
  logfile = SD.open(filename, FILE_WRITE);
  if(checkSD()){
    continuouslog=true;
    display.print("Continuous Logging M");display.print(measureNUM);
  }
  display.display();
  beep(200);
  logfile.close();
  previousmillis=millis();
}

void endCmode(void){
  printmeasurement();
  display.display();
  beep(200);
  continuouslog=false;
}

void switchDisplay(void){
  currentslot=(currentslot+1)%10;
  for (int i=0; i<9&&!TCRfound[currentslot]&&!HSfound[currentslot];i++){
    currentslot=(currentslot+1)%10;
  }
  printmeasurement();
  if(continuouslog){
    display.print("Continuous Logging M");display.print(measureNUM);
  }
  display.display();
  delay(100);
}

void faultCheck(int i){
  uint8_t fault =TCRs[i].readFault();
  if(fault){
    if(buzzer){
      digitalWrite(buzz,HIGH);
    }
            
    display.setCursor(0,0);
    display.clearDisplay();
    display.print("CJ");display.print(i+1);

    if (fault & MAX31856_FAULT_CJRANGE) display.println("Cold Junction Range Fault");
    if (fault & MAX31856_FAULT_TCRANGE) display.println("Thermocouple Range Fault");
    if (fault & MAX31856_FAULT_CJHIGH)  display.println("Cold Junction High Fault");
    if (fault & MAX31856_FAULT_CJLOW)   display.println("Cold Junction Low Fault");
    if (fault & MAX31856_FAULT_TCHIGH)  display.println("Thermocouple High Fault");
    if (fault & MAX31856_FAULT_TCLOW)   display.println("Thermocouple Low Fault");
    if (fault & MAX31856_FAULT_OVUV)    display.println("Over/Under Voltage Fault");
    if (fault & MAX31856_FAULT_OPEN)    display.println("Thermocouple Open Fault");
    display.display();
  }
  while(fault){
    fault =TCRs[i].readFault();
  }
  if(buzzer){
    digitalWrite(buzz,LOW);
  }
}
