#include "BluetoothSerial.h" 
BluetoothSerial ESP_BT;  
#include <analogWrite.h>
#include <Arduino.h>


#define pinINA1 14 // Moteur A, entrée 1 - Commande en PWM possible
#define pinINA2 15 // Moteur A, entrée 2 - Commande en PWM possible

#define pinINB1 13 // Moteur B, entrée 1 - Commande en PWM possible
#define pinINB2 12 // Moteur B, entrée 2 - Commande en PWM possible

#define pinLEDRouge 25
#define pinLEDBlanche 26

#define LDR 27 // Photoresistance 

#define pinBatteryLevel 4 //Pont diviseur de tension pour niveau de batterie
float coeffPont;

float pourcentageVitesse=0;
int eclairage_minimal = 2048;
long int t1 = 0;
long int t2 = 0;
boolean feux_allume = false;
boolean isAllume = false;
boolean feux_auto = true;
boolean isAvance = false;
boolean isRecule = false;
float correctionRoueDroite = 1;
float correctionRoueGauche = 1;
String toSendBattery = "";
String toSendLight = "";
String toSendLuminosity = "";

#define DISABLE_CODE_FOR_RECEIVER // Saves 450 bytes program memory and 269 bytes RAM if receiving functions are not used.
#include "PinDefinitionsAndMore.h" // Define macros for input and output pin etc.
#include <IRremote.hpp>
int led2Pin = 2;

uint8_t sRepeats = 0;
uint8_t address[6]  = {0x94, 0xB9, 0x7E, 0xD9, 0xBA, 0x7A}; //L'adresse de l'ESP de la base
bool connected;

void setup() {
  Serial.begin(19200);
  ESP_BT.begin("57-JACQUELINE-FR");  
  
  // Initialize les broches de commandes 
  // du moteur A
  pinMode( pinINA1, OUTPUT );
  pinMode( pinINA2, OUTPUT );

  pinMode(pinLEDRouge, OUTPUT);
  pinMode(pinLEDBlanche, OUTPUT);

  pinMode(LDR, INPUT);

  pinMode(pinBatteryLevel, INPUT);
  coeffPont = (float)12/(12+3); //Valeurs de resistances utilisées (12kOhms et 3kOhms)
  
  pinMode(led2Pin, OUTPUT);

  Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRREMOTE));
  /*
   * The IR library setup. That's all!
   */
  IrSender.begin(); // Start with IR_SEND_PIN as send pin and if NO_LED_FEEDBACK_CODE is NOT defined, enable feedback LED at default feedback LED pin
  //IrSender.begin(DISABLE_LED_FEEDBACK); // Start with IR_SEND_PIN as send pin and disable feedback LED at default feedback LED pin
  Serial.print(F("Send IR signals at pin "));
  Serial.println(IR_SEND_PIN);
}

void loop() {
  // put your main code here, to run repeatedly:
  
  
  if(ESP_BT.hasClient()){
    Serial.println("Bluetooth connected");
    while(ESP_BT.hasClient()) 
    {
      int received;
      received = ESP_BT.read();        

      if((char)received=='d'){   //d comme droite
          String chaine="";
          while((char)received!='w'){
            chaine += (char)received;
            received=ESP_BT.read();
          }
          chaine = chaine.substring(1,-1);
          char floatbuf[32]; //make this at least big enough for the whole string
          chaine.toCharArray(floatbuf, sizeof(floatbuf));
          float f = atof(floatbuf);
          pourcentageVitesse= f*100;
          roue_droite(pourcentageVitesse*correctionRoueDroite);
      }
      
      else if((char)received=='g'){ //g comme gauche
          String chaine="";
          while((char)received!='w'){
            chaine += (char)received;
            received=ESP_BT.read();
          }
          chaine = chaine.substring(1,-1);
          char floatbuf[32]; //make this at least big enough for the whole string
          chaine.toCharArray(floatbuf, sizeof(floatbuf));
          float f = atof(floatbuf);
          pourcentageVitesse= f*100;
          roue_gauche(pourcentageVitesse*correctionRoueGauche);
      }

      else if((char)received=='m'){ //comme manuel, pour l'eclairage
        feux_auto = false;              
      }
      else if((char)received=='a'){ //comme automatique, pour l'eclairage
        feux_auto = true;              
      }
      else if((char)received=='f'){ //comme feux, pour allumer/eteindre l'eclairage
        feux_allume = 1-feux_allume;             
      }
      else if((char)received=='t'){//comme Tiger, dans eye of the tiger YEEEE man !
        danse();
        Serial.println("danse");
      }
      else if((char)received=='r'){ //comme retour, pour le comeBack Home
        comeBack();              
      }
      else if((char)received=='l'){//comme light level
        String chaine="";
          while((char)received!='w'){
            Serial.println((char)received);
            chaine += (char)received;
            received=ESP_BT.read();
          }
          chaine = chaine.substring(1,-1);
          char floatbuf[32]; //make this at least big enough for the whole string
          chaine.toCharArray(floatbuf, sizeof(floatbuf));
          float lum = atof(floatbuf);
          eclairage_minimal = (int)(lum/100*4096);
      }
      else if((char)received=='s'){//comme stop
          stopCar();
      }
      else if((char)received=='c'){//comme correction
        String chaine="";
          while((char)received!='w'){
            Serial.println((char)received);
            chaine += (char)received;
            received=ESP_BT.read();
          }
          chaine = chaine.substring(1,-1);
          char floatbuf[32]; //make this at least big enough for the whole string
          chaine.toCharArray(floatbuf, sizeof(floatbuf));
          float correctionRoue = atof(floatbuf);
          if(correctionRoue >= 1){
            correctionRoueDroite = 2 - correctionRoue;
            correctionRoueGauche = 1;
          }
          else{
            correctionRoueGauche = correctionRoue;
            correctionRoueDroite = 1;
          }
      }
      sendParameter();    
    }
  }
  else{
    Serial.println("Bluetooth NOT connected");
    stopCar();
    digitalWrite(pinLEDRouge, HIGH);
    digitalWrite(pinLEDBlanche, HIGH);
    delay(200);
    digitalWrite(pinLEDRouge, LOW);
    digitalWrite(pinLEDBlanche, LOW);
    delay(200);
  }
  
}

//                    //
//FONCTIONS AUXILIAIRES//
//                    //

void roue_gauche(float pourcentageVitesse){
  if(pourcentageVitesse > 0){
    analogWrite( pinINA2, LOW );
    int duty = int(pourcentageVitesse*2.55);
    analogWrite( pinINA1, duty );
  }
  else{
    pourcentageVitesse *= -1;
    analogWrite( pinINA1, LOW );
    int duty = int(pourcentageVitesse*2.55);
    analogWrite( pinINA2, duty );
  }
}

void roue_droite(float pourcentageVitesse){
  if(pourcentageVitesse > 0){
    analogWrite( pinINB2, LOW );
    int duty = int(pourcentageVitesse*2.55);
    analogWrite( pinINB1, duty );
  }
  else{
    pourcentageVitesse *= -1;
    analogWrite( pinINB1, LOW );
    int duty = int(pourcentageVitesse*2.55);
    analogWrite( pinINB2, duty );
  }
}

void checkStateLights(){
  int value = analogRead(LDR);//plus value est grand, plus il fait sombre
  
  float luminosity = 100 - (100*(float)value)/4096;
  toSendLuminosity = "L" + String((int)luminosity) + "l";
  if(feux_auto){  //si les feux sont en mode automatiques
    if(value<eclairage_minimal){//suffisament clair -> on eteint
      digitalWrite(pinLEDRouge, LOW);
      digitalWrite(pinLEDBlanche, LOW);
      isAllume = false;
    }
    else{//trop sombre -> on allume
      digitalWrite(pinLEDRouge, HIGH);
      digitalWrite(pinLEDBlanche, HIGH);
      isAllume = true;
    }
  }
  else{ //si les feux sont en mode manuels
    if(feux_allume){//on les allume
      digitalWrite(pinLEDRouge, HIGH);
      digitalWrite(pinLEDBlanche, HIGH);
      isAllume = true;
    }
    else{//on les eteints
      digitalWrite(pinLEDRouge, LOW);
      digitalWrite(pinLEDBlanche, LOW);
      isAllume = false;
    }
  }
  if(isAllume){
    toSendLight = "FOf";//feux "ouvert"
  }
  else{
    toSendLight = "FFf";//feux "fermé"
  }
  
}

void stopCar(){
  analogWrite( pinINA1, LOW);
  analogWrite(pinINB1, LOW);
  analogWrite( pinINA2, LOW);
  analogWrite(pinINB2, LOW);
  Serial.println("STOP");
  isAvance=false;
  isRecule=false;
}

void BatteryLevel(){ //Le format d'envoi est "B57b" pour un niveau à 57% par ex
  int levelBatDigit = analogRead(pinBatteryLevel);
  float tensionBat = (float)levelBatDigit*(3.3 / (float)4095) / (float)coeffPont;  //Calcul fait pour une batterie lithium
  float coeffDir = (float)100/(4.2-1.9);
  float ordonnee = (float)100-coeffDir*4.2;
  float pourcentageLevel = tensionBat*coeffDir + ordonnee ;
  toSendBattery = "B" + String(pourcentageLevel);
  toSendBattery += "b"; //Pour signifier la fin du message
}

void sendParameter(){//recupere toutes les infos et les met dans un chaine qu'on en voie à l'esp
  BatteryLevel();//on genere la chaine de caractere de la batterie
  checkStateLights();//idem avec les feux
  
  t2 = micros();
  if(t2-t1 > 300000){//on envoie toutes les 0.3 secondes
    ESP_BT.print(toSendBattery + toSendLight + toSendLuminosity);
    Serial.println("message : " + toSendBattery + toSendLight + toSendLuminosity);
    t1 = micros();
  }
}

///////////////////////////////////////tout ca pour faire danser la micro momo///////////////////////////////////////////////////////////////////////////
void danse(){
  delay(10380);
  Serial.println("Choree demarre");
  avancer(100.0);
  delay(800);
  stopCar();
  delay(200);

  reculer(100.0);
  delay(400);
  //Serial.println("Avance");
  avancer(100.0);
  delay(400);
  reculer(100.0);
  delay(400);
  stopCar();

  delay(1000);

  avancer(100.0);
  delay(400);
  //Serial.println("Avance");
  reculer(100.0);
  delay(400);
  avancer(100.0);
  delay(400);
  //Serial.println("Stop");
  stopCar();

  delay(1000);

  reculer(100.0);
  delay(400);
  //Serial.println("Avance");
  avancer(100.0);
  delay(400);
  reculer(100.0);
  delay(400);

  tourADroite(100.0, 1500);
  stopCar();

  delay(800);

  avancer(100.0);
  delay(800);
  stopCar();
  delay(200);

  reculer(100.0);
  delay(400);
  //Serial.println("Avance");
  avancer(100.0);
  delay(400);
  reculer(100.0);
  delay(400);
  stopCar();

  delay(1000);

  avancer(100.0);
  delay(400);
  //Serial.println("Avance");
  reculer(100.0);
  delay(400);
  avancer(100.0);
  delay(400);
  //Serial.println("Stop");
  stopCar();

  delay(1000);

  reculer(100.0);
  delay(400);
  //Serial.println("Avance");
  avancer(100.0);
  delay(400);
  stopCar();
  tourADroite(100.0, 1500);
  delay(400);
  stopCar();
}


void avancer(float pourcentageVitesse){
  roue_gauche(pourcentageVitesse);
  roue_droite(pourcentageVitesse);
  isAvance=true;
  isRecule=false;
}

void reculer(float pourcentageVitesse){
  analogWrite( pinINA1, LOW );
  analogWrite(pinINB1, LOW);
  Serial.println(pourcentageVitesse);
  int duty = int(pourcentageVitesse*2.55);
  analogWrite( pinINA2, duty );
  analogWrite(pinINB2, duty);
  isAvance=false;
  isRecule=true;
}

void tourADroite(float pourcentageVitesse, int delayValue){
  roue_gauche(pourcentageVitesse);
  delay(delayValue);
}

void tourAGauche(float pourcentageVitesse, int delayValue){
  roue_droite(pourcentageVitesse);
  delay(delayValue);
}

void comeBack(){
  Serial.println("dans le come back");
  ESP_BT.disconnect();
  ESP_BT.end();
  ESP_BT.begin("Test infra", true);  

  connected = ESP_BT.connect(address);

  if(connected) {
    Serial.println("Connected Succesfully!");
  } else {
    while(!ESP_BT.connected(10000)) {
      Serial.println("Failed to connect. Make sure remote device is available and in range, then restart app."); 
    }
  }
  
  if(connected) {
    Serial.println("Connected Succesfully!");
    bool hasToTurnLeft=0;
    
    while(ESP_BT.hasClient()){
      IrSender.sendNEC(0x5583, 0x85, sRepeats);
      delay(200); //delay(200);
      int received;
      received = ESP_BT.read();
      //Serial.println("Yo man");        
  
      if((char)received=='d'){   //d comme detecte
          Serial.println("Signal reçu");
          //avancer(100);
          roue_gauche(100);
          roue_droite(100);
          delay(300);
          stopCar();
          //tourAGauche(100, 50);
          //stopCar();
          hasToTurnLeft=1;
      }
      else{
        if(hasToTurnLeft==1){
          tourAGauche(100,110);
          stopCar();
        }
        Serial.println("Signal perdu");
        tourADroite(100,10);
        stopCar();
        hasToTurnLeft=0;
        //delay(200);
      }
      //delay(100);
    }

    ESP_BT.disconnect();
    ESP_BT.end();
    ESP_BT.begin("57-JACQUELINE-FR");  
    stopCar();
    
  } 
  
}
