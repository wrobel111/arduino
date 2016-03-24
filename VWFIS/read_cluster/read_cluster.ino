#define FIS_READ_intCLK 1 //interupt on FIS_READ_CLK line
#define FIS_READ_CLK 3 //clk pin 3 - int1
#define FIS_READ_DATA 12 //data pin 12
#define FIS_READ_ENA 2 //enable pin 2 int0
#define FIS_READ_intENA 0 //interupt on FIS_READ_ENA line

#include <LiquidCrystal.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

volatile uint8_t FIS_READ_adr=0;
volatile uint64_t FIS_READ_msg1=0;
volatile uint64_t FIS_READ_msg2=0;
volatile uint8_t FIS_READ_cksum=0;
volatile uint8_t FIS_READ_msgbit=0;
volatile uint8_t FIS_READ_newmsg1=0;
volatile uint8_t FIS_READ_newmsg2=0;
volatile uint8_t FIS_READ_adrok=0;
volatile uint8_t FIS_READ_cksumok=0;
volatile uint8_t FIS_READ_tmp_cksum=0;

volatile uint64_t prev_update = 0;

void FIS_READ_read_data_line(){
  if (!digitalRead(FIS_READ_CLK)){
    if(!FIS_READ_adrok){
      FIS_READ_read_adr();
    }
    else if (!FIS_READ_newmsg1){
      FIS_READ_read_msg1();
    }
    else if (!FIS_READ_newmsg2){
      FIS_READ_read_msg2();
    }
    else if (!FIS_READ_cksumok){
      FIS_READ_read_cksum();
    }
  }
}

void FIS_READ_read_cksum(){
  if(digitalRead(FIS_READ_DATA)){
    FIS_READ_cksum = (FIS_READ_cksum<<1) | 0x00000001;
    FIS_READ_msgbit++;
  }
  else
  {
    FIS_READ_cksum = (FIS_READ_cksum<<1);
    FIS_READ_msgbit++;
  }
  if (FIS_READ_msgbit==8)
  {
    FIS_READ_newmsg1=0;
    FIS_READ_newmsg2=0;
    FIS_READ_adrok=0;
    FIS_READ_tmp_cksum=(0xFF^FIS_READ_adr);
    for (int i=56;i>=0;i=i-8){
      FIS_READ_tmp_cksum=FIS_READ_tmp_cksum+(0xFF^((FIS_READ_msg1>>i) & 0xFF))
        +(0xFF^((FIS_READ_msg2>>i) & 0xFF));
    }
    if((FIS_READ_tmp_cksum%256)==FIS_READ_cksum){
    FIS_READ_cksumok=1;
    } else {
      FIS_READ_msg1 = 0x00000000;
      FIS_READ_msg2 = 0x00000000;
      FIS_READ_cksumok=0;
    }
    FIS_READ_msgbit=0;
  }

}

void FIS_READ_read_msg1(){
  if(digitalRead(FIS_READ_DATA)){
    FIS_READ_msg1 = (FIS_READ_msg1<<1) | 0x00000001;
    FIS_READ_msgbit++;
  }
  else
  {
    FIS_READ_msg1 = (FIS_READ_msg1<<1);
    FIS_READ_msgbit++;
  }
  if (FIS_READ_msgbit==64)
  {
    FIS_READ_newmsg1=1;
    FIS_READ_msgbit=0;
  }
}

void FIS_READ_read_msg2(){
  if(digitalRead(FIS_READ_DATA)){
    FIS_READ_msg2 = (FIS_READ_msg2<<1) | 0x00000001;
    FIS_READ_msgbit++;
  }
  else
  {
    FIS_READ_msg2 = (FIS_READ_msg2<<1);
    FIS_READ_msgbit++;
  }
  if (FIS_READ_msgbit==64)
  {
    FIS_READ_newmsg2=1;
    FIS_READ_msgbit=0;
  }
}

void FIS_READ_read_adr(){
  if(digitalRead(FIS_READ_DATA)){
    FIS_READ_adr = (FIS_READ_adr<<1) | 0x00000001;
    FIS_READ_msgbit++;
  }
  else
  {
    FIS_READ_adr = (FIS_READ_adr<<1);
    FIS_READ_msgbit++;
  }
  if (FIS_READ_msgbit==8)// && FIS_READ_adr==0xF)
  {
    FIS_READ_adrok=1;
    FIS_READ_msgbit=0;
  }
}

void FIS_READ_detect_ena_line(){
  if (digitalRead(FIS_READ_ENA)){ //Enable line changed to HIGH -> data on data line are valid
    //init all again
    FIS_READ_msgbit=0;
    FIS_READ_newmsg1=0;
    FIS_READ_newmsg2=0;
    FIS_READ_adrok=0;
    FIS_READ_cksumok=0;
    FIS_READ_tmp_cksum=0;
    attachInterrupt(FIS_READ_intCLK,FIS_READ_read_data_line,CHANGE); //can be changed to CHANGE->uncoment "if (!digitalRead(FIS_READ_CLK)){" in FIS_READ_read_data_line function
  } else {
  detachInterrupt(FIS_READ_intCLK);
  }
}

void setup() { 
  attachInterrupt(FIS_READ_intCLK,FIS_READ_read_data_line,CHANGE);
  pinMode(FIS_READ_CLK,INPUT_PULLUP);
  lcd.begin(16,2);
  lcd.home();
  lcd.clear();
  pinMode(FIS_READ_ENA,INPUT);//no pull up! this is inactive state low, active is high
  pinMode(FIS_READ_DATA,INPUT_PULLUP);
  if (!digitalRead(FIS_READ_ENA)){
  digitalWrite(FIS_READ_ENA,HIGH);
  delay(1);
  digitalWrite(FIS_READ_ENA,LOW);
  }
  attachInterrupt(FIS_READ_intENA,FIS_READ_detect_ena_line,CHANGE);
}

void loop() {
  if(FIS_READ_cksumok){ //whole packet received and checksum is ok    
   // lcd.clear();
   lcd.home();
    for(int i=56;i>=0;i=i-8){
      int c = (0xFF^((FIS_READ_msg1>>i) & 0xFF));
      if (c == 102 ) c=95;
      lcd.write(c);
    }
    lcd.setCursor(0,1);
    for(int i=56;i>=0;i=i-8){
      int c = (0xFF^((FIS_READ_msg2>>i) & 0xFF));
      if (c == 102 ) c=95;
      lcd.write(0xFF^((FIS_READ_msg2>>i) & 0xFF));   
    }
    FIS_READ_cksumok=0;
    prev_update=millis();
    if (!digitalRead(FIS_READ_ENA)){ 
      digitalWrite(FIS_READ_ENA,HIGH);
      delay(1);
      digitalWrite(FIS_READ_ENA,LOW);
    }
  } else {
    if (millis() - prev_update > 1000 ) {//leave text for 1000ms
      lcd.clear();
      prev_update=millis();
       if (!digitalRead(FIS_READ_ENA)){ 
        digitalWrite(FIS_READ_ENA,HIGH);
        delay(1);
        digitalWrite(FIS_READ_ENA,LOW);
      }
    }
  }
}



