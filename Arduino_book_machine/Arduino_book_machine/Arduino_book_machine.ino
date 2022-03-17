#include <ArduinoJson.h>
#include <Ethernet.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#define zamek_VCC 53
#define zamekControl_OUT 51
#define zamekControl_IN 49

LiquidCrystal_I2C lcd(0x27,16,1);
int count = 0; 

//#define tx A8
//#define rx A9
//mp3
SoftwareSerial mp3(17,16); //6,9 >> 17,16    
static uint8_t cmdbuf[8] = {0};

// symbol biblioteki i wrzutni przykładowe ze względów bezpieczeństwa
const String symbolBibl = "1234567890";
const String symbolWrzutni = "1234567890";

String nazwa;
boolean czyWrzutnia;
boolean czyKsiazkomat;
int iloscSkrytek;
String requestLink;
String haslo;

boolean czynna;
String czynnaKomunikat;
String nieczynnaKomunikat;

//po sczytaniu kodu ISBN
String komunikat3;
//otwarcie wrzutni
String komunikat4;
//zamknięcie wrzutni
String komunikat5;
//błędny kod
String komunikat6; 

//komunikaty rezerwowe
String komunikat7;
String komunikat8;
String komunikat9;


String serverName= "tutajNazwaSerwera"; // nazwy i adresy zostały ukryte ze względów bezpieczeństwa
String bibliotekaServer; //bemowoServer << old
String initialDataLink = "tutajAdresSerwisuPrefix"+symbolBibl+"&zp="+symbolWrzutni;
String commandsDataLink;

//
unsigned long beginMicros, endMicros;
unsigned long byteCount = 0;
bool printWebData = true;  // set to false for better speed measurement
//

boolean awaria = false;

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192,168,1,29);
IPAddress myDns(192,168,1,1);
 EthernetClient client;

//do stempli czasowych
unsigned long timeStamp;

//mp3 func
void command(int8_t cmd, int16_t dat)
{
  delay(20);
 
  cmdbuf[0] = 0x7e; // bajt startu
  cmdbuf[1] = 0xFF; // wersja
  cmdbuf[2] = 0x06; // liczba bajtow polecenia
  cmdbuf[3] = cmd;  // polecenie
  cmdbuf[4] = 0x00; // 0x00 = no feedback, 0x01 = feedback
  cmdbuf[5] = (int8_t)(dat >> 8); // parametr DAT1
  cmdbuf[6] = (int8_t)(dat); //  parametr DAT2
  cmdbuf[7] = 0xef; // bajt konczacy
 
  for (uint8_t i = 0; i < 8; i++)
  {
    mp3.write(cmdbuf[i]);
  }
}

void(* resetFunc) (void) = 0;



// konfiguracja początkowa
void initConfig(){
  Serial.println(F("Connecting..."));
  lcd.print("Laczenie ...");
  // Connect to HTTP server
 
  client.setTimeout(10000);

int adressLen = serverName.length() + 1; 
char server[adressLen];
serverName.toCharArray(server, sizeof(server));   
  
  if (!client.connect(server, 80)) {
    Serial.println(F("Connection failed"));
     lcd.clear();
     lcd.print("Problem z serwerem");
     lcd.setCursor(0,1);
     lcd.print("[!]3s to restart");
     delay(3000);
     resetFunc();
  }

  Serial.println(F("Connected!"));


 //inicjowanie konfiguracji początkowej
 
  // Send HTTP request
  client.println("GET "+initialDataLink+" HTTP/1.0");
  client.println("Host: "+serverName);
  client.println(F("Connection: close"));
  if (client.println() == 0) {
    Serial.println(F("Failed to send init request"));
    lcd.clear();
     lcd.print("[!]init req error");
     lcd.setCursor(0,1);
     lcd.print("[!]3s to restart");
     delay(3000);
     resetFunc();
  }

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    return;
  }

  // Allocate the JSON document
  // Use arduinojson.org/v6/assistant to compute the capacity.
  const size_t capacity1 = JSON_OBJECT_SIZE(256);
  DynamicJsonDocument doc(capacity1);

  // Parse JSON object
  DeserializationError error = deserializeJson(doc, client);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    lcd.clear();
     lcd.print("[!]init json error");
     lcd.setCursor(0,1);
     lcd.print("[!]3s to restart");
     delay(3000);
     resetFunc();
  }

 nazwa = doc["nazwa"].as<String>();
 czyWrzutnia = doc["wrzutnia"].as<boolean>();
 czyKsiazkomat = doc["ksiazkomat"].as<boolean>();
 iloscSkrytek = doc["iloscskrytek"].as<int>();
 requestLink = doc["link"].as<String>();
 haslo = doc["haslo"].as<String>();
 
  // Disconnect
  client.stop();

  
  bibliotekaServer = requestLink;
  bibliotekaServer.remove(0,7);
  bibliotekaServer = bibliotekaServer.substring(0, bibliotekaServer.indexOf(":"));
  Serial.println("Nazwa serwera biblioteki >>>> " + bibliotekaServer);
  // ustawiam link do aciągniecia konfiguracji
  commandsDataLink = requestLink +"tutajAdres(Poufne)"+haslo+"&amp&zp="+symbolWrzutni;
}



// zaciąganie komend
void initCommands(){
  if(!client.connect("google.com",80))
  {
    return;
  }
  client.stop();
  
  
  Serial.println(F("Connecting..."));
 // lcd.print("Laczenie ...");
  // Connect to HTTP server
 
  client.setTimeout(10000);

int adressLen = bibliotekaServer.length() + 1; 
char server[adressLen];
bibliotekaServer.toCharArray(server, sizeof(server));   
  
  if (!client.connect(server, 8080)) {
    Serial.println(F("Connection failed"));
     lcd.clear();
     lcd.print("Problem z serwerem");
     lcd.setCursor(0,1);
     lcd.print("[!]3s to restart");
     delay(3000);
     resetFunc();
  }

  Serial.println(F("Connected!"));

  Serial.println("Pobieranie komend z serwera...");
  client.println("GET "+commandsDataLink+" HTTP/1.0");
  client.println("Host: "+bibliotekaServer);
  client.println(F("Connection: close"));
  if (client.println() == 0) {
    Serial.println(F("Failed to send request about commands"));
     lcd.clear();
     lcd.print("[!]Blad komunikacji");
     lcd.setCursor(0,1);
     lcd.print("[!]3s to restart");
     delay(3000);
     resetFunc();
  }

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    return;
  }

  // Allocate the JSON document
  // Use arduinojson.org/v6/assistant to compute the capacity.
  size_t capacity2 = JSON_OBJECT_SIZE(384);
 // DynamicJsonDocument doc(capacity2);
 StaticJsonDocument<400> doc; //uzywam statyczngo by nie bylo bledow z pamiecia

  // Parse JSON object
  DeserializationError error = deserializeJson(doc, client);
  if (error) {
    Serial.print(F("[getting commands error]=> deserializeJson() failed: "));
    Serial.println(error.f_str());
    lcd.clear();
     lcd.print("[!]Blad json");
     lcd.setCursor(0,1);
     lcd.print("[!]3s to restart");
     delay(3000);
     resetFunc();
  }

 

  int czyn= doc["czynna"].as<int>();
  czynna= czyn==1;

  //Serial.println(czynna ? "tak" : "nie");
  
  nieczynnaKomunikat = doc["komunikat1"].as<String>();
  czynnaKomunikat= doc["komunikat2"].as<String>();
  komunikat3 = doc["komunikat3"].as<String>();
  komunikat4 = doc["komunikat4"].as<String>();
  komunikat5 = doc["komunikat5"].as<String>();
  komunikat6 = doc["komunikat6"].as<String>();
  komunikat7 = doc["komunikat7"].as<String>();
  komunikat8 = doc["komunikat8"].as<String>();
  komunikat9 = doc["komunikat9"].as<String>();
 
  // Disconnect
  client.stop();

   Serial.println(">Zaktualizowano.");
   Serial.print("CZYNNA: ");
   Serial.println(czynna ? "tak" : "nie");
   Serial.println("CZYNNA-KOMUNIKAT: " + czynnaKomunikat);
   Serial.println("NIECZYNNA-KOMUNIKAT: " + nieczynnaKomunikat);
   Serial.println("KOMUNIKAT3: " + komunikat3);
   Serial.println("KOMUNIKAT4: " + komunikat4);
   Serial.println("KOMUNIKAT5: " + komunikat5);
   Serial.println("KOMUNIKAT6: " + komunikat6);
   Serial.println("KOMUNIKAT7: " + komunikat7);
   Serial.println("KOMUNIKAT8: " + komunikat8);
   Serial.println("KOMUNIKAT9: " + komunikat9);
}


//polskie znaki
//Ą
byte znak1[8] ={
                      B01110,
                      B10001,
                      B10001,
                      B11111,
                      B10001,
                      B10001,
                      B00010,
                      B00001
                     };
// ą
byte znak2[8] ={
                      B00000,
                      B01110,
                      B00001,
                      B01111,
                      B10001,
                      B01111,
                      B00010,
                      B00001
                     };
 //Ć
byte znak3[8] ={
                      B00001,
                      B01110,
                      B10001,
                      B10000,
                      B10000,
                      B10001,
                      B01110,
                      B00000
                     };      
// ć
byte znak4[8] ={
                      B00010,
                      B00100,
                      B01110,
                      B10000,
                      B10000,
                      B10001,
                      B01110,
                      B00000
                     };                     
// Ę
byte znak5[8] ={
                      B11111,
                      B10000,
                      B10000,
                      B11110,
                      B10000,
                      B11111,
                      B00010,
                      B00001
                     };
// ę
byte znak6[8] ={
                      B00000,
                      B01110,
                      B10001,
                      B11111,
                      B10000,
                      B01110,
                      B00010,
                      B00001
                     };                    
//Ł
byte znak7[8] ={
                      B10000,
                      B10000,
                      B10100,
                      B11000,
                      B10000,
                      B10000,
                      B11111,
                      B00000
                     };
//ł
byte znak8[8] ={
                      B01100,
                      B00100,
                      B00110,
                      B00100,
                      B01100,
                      B00100,
                      B01110,
                      B00000
                     };   
//Ń
byte znak9[8] ={
                      B00010,
                      B10101,
                      B11001,
                      B10101,
                      B10011,
                      B10001,
                      B10001,
                      B00000
                     };
// ń
byte znak10[8] ={
                      B00010,
                      B00100,
                      B10110,
                      B11001,
                      B10001,
                      B10001,
                      B10001,
                      B00000
                     };
//Ó
byte znak11[8] ={
                      B01101,
                      B10010,
                      B10101,
                      B10001,
                      B10001,
                      B10001,
                      B01110,
                      B00000
                     };                     
// ó
byte znak12[8] ={
                      B00010,
                      B00100,
                      B01110,
                      B10001,
                      B10001,
                      B10001,
                      B01110,
                      B00000
                     };  
//Ś
byte znak13[8] ={
                      B00001,
                      B01110,
                      B10000,
                      B10000,
                      B01110,
                      B00001,
                      B11110,
                      B00000
                     }; 
// ś
byte znak14[8] ={
                      B00010,
                      B00100,
                      B01110,
                      B10000,
                      B01110,
                      B00001,
                      B11110,
                      B00000
                     }; 
 //Ź
byte znak15[8] ={
                      B00010,
                      B11111,
                      B01001,
                      B00010,
                      B00100,
                      B01000,
                      B11111,
                      B00000
                     };  
//ź
byte znak16[8] ={
                      B00010,
                      B00100,
                      B11111,
                      B00010,
                      B00100,
                      B01000,
                      B11111,
                      B00000
                     }; 
//Ż
byte znak17[8] ={
                      B11111,
                      B00001,
                      B00010,
                      B11111,
                      B01000,
                      B10000,
                      B11111,
                      B00000
                     };     
// ż
byte znak18[8] ={
                      B00100,
                      B00000,
                      B11111,
                      B00010,
                      B00100,
                      B01000,
                      B11111,
                      B00000
                     };
                     
byte* znakiPL[8] = {znak2,znak4,znak6,znak8,znak10,znak12,znak14,znak18}; // tymaczasowo tylko te znaki polskie

//void choosePolishChars(){
//  String[] komunikaty = {czynnaKomunikat, nieczynnaKomunikat, komunikat3, komunikat4, komunikat5, komunikat6, komunikat7, komunikat8,komunikat9};
//  
//  for(int i =0; i<sizeof(komunikaty); i++){
//    
//  }
//}

void scrollText(String txt, boolean scanHandle){
//  lcd.clear();
//  lcd.home();
  
  int len = txt.length();
    for(int i=15; i>=0-len; i--){
      lcd.clear();
      if(scanHandle && Serial1.available())
       break;
       else 
       if(Serial1.available()){
        command(0x0F, 0x0104); // komenda że wrzutnia nieczynna
         // oczyszczam bufor Serial1 ze zczytanych w międzyczasie kodów/kart
          while(Serial1.available()){
            Serial1.read();
           }
       }
       
       
   
      if(i>=0)
       lcd.setCursor(i,0);
      if(i<0)
       txt.remove(0,1);
   
      printText(txt,true);
      //Serial.println(i);
      delay(500);
  }
}


//tablica pomocnicza do sprawdzania czy występuje poslki znak (możliwa obsługa max 8 znaków)
char pl[8] = {'ą','ć','ę','ł','ń','ó','ś','ż'};

//usuwa zepsute znaki występujące przed polskimi - do naprawy
String repair(String s){

  String res = "";
   boolean flag;
   for(int i=0; i<s.length(); i++){
     flag = false;
      for(int j=0; j<8; j++){
        if(s.charAt(i+1)==pl[j])
        flag=true;
      }
      if(!flag)
      res+=s.charAt(i);
  }
  
  return res;
}

void printText(String txt, boolean scanHandle){

 String s = repair(txt);
 boolean czyPolskiZnak;
  for(int i=0; i<s.length(); i++){

    czyPolskiZnak = false;
     for(int j=0; j<8 && !czyPolskiZnak; j++){
      if(s.charAt(i)==pl[j]){
      lcd.write((byte)j);
     // Serial.println("polski znak");
        czyPolskiZnak = true;
      }
     }
     if(!czyPolskiZnak){
    // Serial.println(s.charAt(i));
      lcd.print(s.charAt(i));
     }
  }
}


// 1 param to tekst, drugi jeśli true to czuły na skany, jeśli false to ignoruje skany
void showText(String txt, boolean scanHandle){
   lcd.clear();
   lcd.home();

  if(txt.length()>16)
    scrollText(txt, scanHandle);
  else
  {
    if(!scanHandle && Serial1.available()){
       command(0x0F, 0x0104); // komenda że wrzutnia nieczynna
    }
      
    printText(txt, scanHandle);
  }
    
}




void setup() {
 // Initialize Serial port
  Serial.begin(9600);
   Serial1.begin(9600);
  while (!Serial) continue;

  //elektrozamek
    //pin wysylajacy stan wysoki aby otworzyc
    pinMode(zamek_VCC, OUTPUT);
    digitalWrite(zamek_VCC,LOW);
 //kontrola zamknięcia
  pinMode(zamekControl_OUT, OUTPUT);
  digitalWrite(zamekControl_OUT,LOW);

  pinMode(zamekControl_IN, INPUT);
  //digitalWrite(zamekControl_IN,LOW);

  
  //LCD
  lcd.init();
  for(int i =0; i<8; i++){    // tymczasowe polskie znaki
    lcd.createChar(i,znakiPL[i]);
  }
  
  lcd.begin(16,1);
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Init ...");
  delay(200);

  lcd.clear();
  lcd.print("audio config ...");
  //mp3 settings
  mp3.begin(9600);
  delay(500); // Czekamy 500ms na inicjalizacje  
  command(0x09, 0x0002); // Wybieramy karte SD jako zrodlo
  delay(200); // Czekamu 200ms na inicjalizacje
  command(0x06, 0x0064); // Ustaw glosnosc na 30 >>>   1E

  
  lcd.clear();
  lcd.print("LAN config ...");
  // Initialize Ethernet library
  byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
  if (!Ethernet.begin(mac)) {
    Serial.println(F("Failed to configure Ethernet"));
     lcd.clear();
     lcd.print("[!]ethernet error");
     delay(3000);
     lcd.clear();
     lcd.home();
     lcd.print("[!]restart...");
     delay(2000);
     resetFunc();
  }
  delay(1000);

   initConfig();
   initCommands();
  
  lcd.clear();
  lcd.print("Polaczono: "+nazwa);

   Serial.println("=======");
   Serial.println("===========");
   Serial.println("==========ARDUINO KSIAZKOMAT==========");
   Serial.println("NAZWA: " + nazwa);
   String w = czyWrzutnia ? "tak" : "nie";
   String k = czyKsiazkomat ? "tak" : "nie";
   Serial.println("WRZUTNIA: " + w);
   Serial.println("KSIAZKOMAT: " + k);
   Serial.print("LICZBA SKRYTEK: ");
   Serial.println(iloscSkrytek);
   Serial.println("LINK: " + requestLink);
   Serial.println("HASLO: " + haslo);
   Serial.println();
   Serial.print("CZYNNA: ");
   Serial.println(czynna ? "tak" : "nie");
   Serial.println("CZYNNA-KOMUNIKAT: " + czynnaKomunikat);
   Serial.println("NIECZYNNA-KOMUNIKAT: " + nieczynnaKomunikat);
   Serial.println("KOMUNIKAT3: " + komunikat3);
   Serial.println("KOMUNIKAT4: " + komunikat4);
   Serial.println("KOMUNIKAT5: " + komunikat5);
   Serial.println("KOMUNIKAT6: " + komunikat6);
   Serial.println("KOMUNIKAT7: " + komunikat7);
   Serial.println("KOMUNIKAT8: " + komunikat8);
   Serial.println("KOMUNIKAT9: " + komunikat9);
   Serial.println("=======================================");
   Serial.println("===========");
   Serial.println("=======");

   delay(5000);

 //showText(czynnaKomunikat,true);
}



// oczekiwanie na podanie kodu/karty
void loop() {
  
    
if(czynna){
  
  if(client.connect("google.com",80))
  {
    client.stop();
    
    if(awaria){
      awaria = false;
    }
    
    showText(czynnaKomunikat,true);

    if(Serial1.available()) // sprawdzam czy przychodzi coś z czytnika
    {
      doScan();
    }
  }
  else
  {
    if(!awaria){
      awaria = true;
      lcd.clear();
      lcd.print("[!] awaria [!]");
    }
    delay(2000);

    // oczyszczam bufor Serial1 ze zczytanych w międzyczasie kodów/kart
    while(Serial1.available()){
      Serial1.read();
    }
  }
  
}
else{
   showText(nieczynnaKomunikat,false);
   delay(2000);
    // oczyszczam bufor Serial1 ze zczytanych w międzyczasie kodów/kart
    while(Serial1.available()){
      Serial1.read();
    }
}
  
//aktualizacja konfiguracji
initCommands();
}



// w przypadku odczytu kodu
void doScan(){
// sprawdzam czy nie utracono połączenia w międzyczasie
if(!client.connect("google.com",80))
  {
    return;
  }

  client.stop();
  

 count = 0; // Reset count to zero
 
     lcd.clear();
     lcd.setCursor(0,0);
     String res = "";
    while(Serial1.available()) // Keep reading Byte by Byte from the Buffer till the Buffer is empty
    {
     char input = Serial1.read(); // Read 1 Byte of data and store it in a character variable
        res+=input;

  
      count++; // Increment the Byte count after every Byte Read
      delay(5); // A small delay 
    }
    Serial.println("Zczytano:");
    Serial.println(res);
    res.replace("\n", "");

   String num = "";
      
   //kod kreskowy 9 cyfr (wysyłamy jak dostaliśmy)
   //kod qr obcinamy przed 'O'
   //miraf 10 znaków hex <<< obróbka
   //kod kreskowy z książki 12 cyfr (wysyłamy jak dostaliśmy)


    //jeśli ISBN
   if(res.length()==13) // w przypadku kodu ISBN oczekujemy kolejnego kodu
  {
    Serial.println("Odczytano potencjalny kod ISBN - prośba o kolejny kod bez wysyłania requesta..");
    // żądanie kolejnego kodu
    
    timeStamp = millis();
    while(millis() - timeStamp < 5000){
      showText(komunikat3,true);
      if(Serial1.available()){  // sprawdzam czy przychodzi coś z czytnika
        doScan();
        break;
      }
    }  
    return;
  }
  else  // mifare
    if(res.length() == 10){
      Serial.println("Przetwarzam kod z karty mifare...");
             
             String s1 = res.substring(0,res.length()-6);
             String s2 = res.substring(4,res.length());
             Serial.print(s1);
             Serial.print(", ");
             Serial.println(s2);
            
             int liczba1 = strtol(&s1[0], NULL, 16);
             long liczba2 = strtol(&s2[0], NULL, 16);

             String buff_1 = "";
             buff_1 += liczba1;
             String buff_2 = "";
             buff_2 += liczba2;
              Serial.print("1 czesc: ");
              Serial.print(buff_1);
              Serial.print(" , 2 czesc: ");
              Serial.println(buff_2);

              
             //format do xxx-xxxxxxxx
             //format 1 części do "xxx"
            
              for(int a=0; a<3-buff_1.length(); a++){
                num+="0";
              }
               //dodaje resztę
              num+=liczba1;
              
              num+="-";

              //format drugiej części do "xxxxxxxx"
               for(int a=0; a<8-buff_2.length(); a++){
                num+="0";
              }

              //dodaje resztę
              num+=liczba2;

              Serial.print("Po przetworzeniu: ");
              Serial.println(num);
    }
    else //kreskowy lub z ksiazki
    if(res.length()== 9 || res.length()== 12){
      num = res;
    }
    else{
      // qr
        for(int i=0; i<res.length()&& res.charAt(i)!='O'; i++)
          {
             Serial.print(res.charAt(i));
             num+=res.charAt(i);
          }
    }

   lcd.print(num);
   //aktualizacja konfiguracji
    initCommands();
    if(!czynna)
      return;
      
    doRequest(num);
    showText(czynnaKomunikat,true);
}


void sendReq(String s){
  String req = requestLink +"s8vb3ldpa0dp3dc.php?h="+ haslo +"&s="+ symbolWrzutni +"&k="+ s;
  Serial.print("Wsyłam żądanie: ");
  Serial.println(req);
  // Initialize Ethernet library
//  byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
//  if (!Ethernet.begin(mac)) {
//    Serial.println(F("Failed to configure Ethernet"));
//    return;
//  }
//  delay(1000);

  //Serial.println(F("Connecting..."));

  // Connect to HTTP server
  client.setTimeout(10000);
 int addLen = bibliotekaServer.length() + 1; 
 char libServ[addLen];
 bibliotekaServer.toCharArray(libServ, sizeof(libServ));   
  
  if (!client.connect(libServ, 8080)) {
    Serial.println(F("info req - Connection failed"));
    return;
  }

  Serial.println(F("Connected!"));

  client.println("GET "+req+" HTTP/1.0");
  client.println("Host: "+bibliotekaServer);
  client.println(F("Connection: close"));
  if (client.println() == 0) {
    Serial.println(F("Failed to send request"));
    return;
  }

   beginMicros = micros();
   boolean uzyskanoDane = false;

    // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    return;
  }

  Serial.println("Otrzymano:");

 String response = "";
  while(!uzyskanoDane)
  {
      int len = client.available();
      if (len > 0) {
        byte buffer[80];
        if (len > 80) len = 80;
        client.read(buffer, len);
        
        if (printWebData) {
          Serial.write(buffer, len); // show in the serial monitor (slows some boards)
          response+= (char*)buffer;
        }
        byteCount = byteCount + len;
     }



    // if the server's disconnected, stop the client:
    if (!client.connected()) {
      endMicros = micros();
      Serial.println();
      Serial.println("disconnecting.");
      client.stop();
      Serial.print("Received ");
      Serial.print(byteCount);
      Serial.print(" bytes in ");
      float seconds = (float)(endMicros - beginMicros) / 1000000.0;
      Serial.print(seconds, 4);
      float rate = (float)byteCount / seconds / 1000.0;
      Serial.print(", rate = ");
      Serial.print(rate);
      Serial.print(" kbytes/second");
      Serial.println();
  
     uzyskanoDane = true;
    
    }
  }
}


void doRequest(String number){
  Serial.println("numer: ");
  Serial.println(number);
  String reqLinq = requestLink +"tutajCzescAdresu(poufne)"+ haslo +"&s="+ symbolWrzutni +"&k="+ number;

  Serial.print("Wsyłam żądanie: ");
  Serial.println(reqLinq);
  
  String response = "";

  // Initialize Ethernet library
  byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
  if (!Ethernet.begin(mac)) {
    Serial.println(F("Failed to configure Ethernet"));
    return;
  }
  delay(1000);

  Serial.println(F("Connecting..."));

  // Connect to HTTP server
  client.setTimeout(10000);
 int addLen = bibliotekaServer.length() + 1; 
 char libServ[addLen];
 bibliotekaServer.toCharArray(libServ, sizeof(libServ));   
  
  if (!client.connect(libServ, 8080)) {
    Serial.println(F("Connection failed"));
    return;
  }

  Serial.println(F("Connected!"));

  client.println("GET "+reqLinq+" HTTP/1.0");
  client.println("Host: "+bibliotekaServer);
  client.println(F("Connection: close"));
  if (client.println() == 0) {
    Serial.println(F("Failed to send request"));
    return;
  }

   beginMicros = micros();
   boolean uzyskanoDane = false;

    // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    return;
  }

  Serial.println("Otrzymano:");
 
  while(!uzyskanoDane)
  {
      int len = client.available();
      if (len > 0) {
        byte buffer[80];
        if (len > 80) len = 80;
        client.read(buffer, len);
        
        if (printWebData) {
          Serial.write(buffer, len); // show in the serial monitor (slows some boards)
          response+= (char*)buffer;
        }
        byteCount = byteCount + len;
     }



    // if the server's disconnected, stop the client:
    if (!client.connected()) {
      endMicros = micros();
      Serial.println();
      Serial.println("disconnecting.");
      client.stop();
      Serial.print("Received ");
      Serial.print(byteCount);
      Serial.print(" bytes in ");
      float seconds = (float)(endMicros - beginMicros) / 1000000.0;
      Serial.print(seconds, 4);
      float rate = (float)byteCount / seconds / 1000.0;
      Serial.print(", rate = ");
      Serial.print(rate);
      Serial.print(" kbytes/second");
      Serial.println();
  
     uzyskanoDane = true;
    
    }
  }


  Serial.println("Odczytano: " + response);

  if(response.indexOf("-----")>=0)
  {
    lcd. clear();
    command(0x0F, 0x0103);
    showText(komunikat6,true);
    timeStamp = millis();
    while(millis() - timeStamp < 5000){
      if(Serial1.available())
      {
        command(0x16, 0x0000); 
        doScan();
        break;
      }
   }
   return;
  }
  else
  if(response.indexOf("czytelnik")>=0)
  {
    Serial.println(">>>Otwieram");
    
     // sprawdzam czy wrzutnia jest zamknięta zamekControl_OUT ustawiam na HIGH
      digitalWrite(zamekControl_OUT,HIGH);
      
     showText(komunikat4,true);

     
     // wysyłam stan wysoki na elektrozamek (jeśli wrzutnia jest zamknięta -> zamekControl_IN==HIGH << moja wersja)
     if(digitalRead(zamekControl_IN)==HIGH)
     {
      digitalWrite(zamek_VCC,HIGH);
      sendReq("wrzutnia_otwarta");
     }
        
        
      //komunikat audio
      command(0x0F, 0x0101);

       timeStamp = millis();
       while(millis() - timeStamp < 3000){
        //w przypadku nowego skanu
      if(Serial1.available()){
        command(0x16, 0x0000); 
        //przerywam stan wysoki na zamek
        digitalWrite(zamek_VCC,LOW);
        //przerywam kontrolę zamknięcia
        digitalWrite(zamekControl_OUT,LOW);
        doScan();
        return;
      }
   }
   //koniec stanu wysokiego na zamek
   digitalWrite(zamek_VCC,LOW);
   delay(100);
  
    //dopóki jest otwarta
    while(digitalRead(zamekControl_IN)==LOW)
   {
    if(Serial1.available()){
      //przerywam kontrolę zamknięcia
        digitalWrite(zamekControl_OUT,LOW);
        doScan();
        return;
      }
   }
   //przerywam kontrolę zamknięcia
        digitalWrite(zamekControl_OUT,LOW);
        sendReq("wrzutnia_zamknieta");
    Serial.println(">>>Zamykam");
      //komunikat audio
      command(0x16, 0x0000); 
      command(0x0F, 0x0102);
      showText(komunikat5,true);
     timeStamp = millis();
    while(millis() - timeStamp < 3000){
      if(Serial1.available())
      {
        command(0x16, 0x0000); 
        doScan();
        break;
      }
   }
  }
}
