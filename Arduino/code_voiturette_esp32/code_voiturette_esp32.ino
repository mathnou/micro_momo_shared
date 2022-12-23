#include "BluetoothSerial.h" 
BluetoothSerial ESP_BT;  
#include <analogWrite.h>


#define pinINA1 14 // Moteur A, entrée 1 - Commande en PWM possible
#define pinINA2 15 // Moteur A, entrée 2 - Commande en PWM possible

#define pinINB1 13 // Moteur B, entrée 1 - Commande en PWM possible
#define pinINB2 12 // Moteur B, entrée 2 - Commande en PWM possible

#define pinLEDRouge 25
#define pinLEDBlanche 26

#define LDR 27 // Photoresistance 
boolean isFeuxAuto = true; //Boolean qui determine si on est en mode auto ou non pour les feux

#define pinBatteryLevel 4 //Pont diviseur de tension pour niveau de batterie
float coeffPont;


float pourcentageVitesse=0;

boolean isAvance = false;
boolean isRecule = false;




void setup() {
  Serial.begin(19200);
  ESP_BT.begin("ESP32 - MicroMobile");  
  
  // Initialize les broches de commandes 
  // du moteur A
  pinMode( pinINA1, OUTPUT );
  pinMode( pinINA2, OUTPUT );

  pinMode(pinLEDRouge, OUTPUT);
  pinMode(pinLEDBlanche, OUTPUT);

  pinMode(LDR, INPUT);

  pinMode(pinBatteryLevel, INPUT);
  coeffPont = (float)200/(200+51); //Valeurs de resistances utilisées
}

void loop() {
  // put your main code here, to run repeatedly:
  
  
  if(ESP_BT.hasClient()){
    Serial.println("Bluetooth connected");
    while(ESP_BT.hasClient()) 
    { 
      if(isFeuxAuto){
        checkStateLights();
      }
      sendLightLevel();
      sendBatteryLevel();
      int received;
      received = ESP_BT.read();        

      if((char)received=='d'){   //d comme droite
          Serial.println("roue droite");
          String chaine="";
          while((char)received!='w'){
            Serial.println((char)received);
            chaine += (char)received;
            received=ESP_BT.read();
          }
          chaine = chaine.substring(1,-1);
          char floatbuf[32]; //make this at least big enough for the whole string
          chaine.toCharArray(floatbuf, sizeof(floatbuf));
          float f = atof(floatbuf);
          pourcentageVitesse= f*100;
          roue_droite(pourcentageVitesse);
      }
      
      else if((char)received=='g'){ //comme gauche
          Serial.println("roue gauche");
          String chaine="";
          while((char)received!='w'){
            Serial.println((char)received);
            chaine += (char)received;
            received=ESP_BT.read();
          }
          chaine = chaine.substring(1,-1);
          char floatbuf[32]; //make this at least big enough for the whole string
          chaine.toCharArray(floatbuf, sizeof(floatbuf));
          float f = atof(floatbuf);
          pourcentageVitesse= f*100;
          roue_gauche(pourcentageVitesse);
      }

      
      else if((char)received=='s'){
          stopCar();

      }    

      else if((char)received=='p'){ //P comme param. Le format d'un parametre est "p-FA-O-w" avec FA pour Feux Autos et O pour ON par ex (on utilisera F pour OFF)
          Serial.println("Paramètres");
          String chaine="";
          while((char)received!='w'){
            Serial.println((char)received);
            chaine += (char)received;
            received=ESP_BT.read();
          }
          if(chaine.substring(1,3)=="FA"){ //Pour Feux Autos
            isFeuxAuto = (chaine.substring(4,5)=="0");
          }
          else if(chaine.substring(1,3)=="FM"){ //Pour feux manuels. Envoi de "p-FM-O" pour allumer les feux en mode manuel, envoi de "p-FM-F" pour les éteidre
              if(chaine.substring(4,5)=="O"){
                digitalWrite(pinLEDRouge, HIGH);
                digitalWrite(pinLEDBlanche, HIGH);
              }
              else{
                digitalWrite(pinLEDRouge, LOW);
                digitalWrite(pinLEDBlanche, LOW);
              }
          }                    

      }   
    }
  }
  else{
    Serial.println("Bluetooth NOT connected");
    stopCar();
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


void stopCar(){
  analogWrite( pinINA1, LOW);
  analogWrite(pinINB1, LOW);
  analogWrite( pinINA2, LOW);
  analogWrite(pinINB2, LOW);
  Serial.println("STOP");
  isAvance=false;
  isRecule=false;
}

void checkStateLights(){ 
  int value = analogRead(LDR);
  if(value>2000){
    digitalWrite(pinLEDRouge, HIGH);
    digitalWrite(pinLEDBlanche, HIGH);
  }
  else{
    digitalWrite(pinLEDRouge, LOW);
    digitalWrite(pinLEDBlanche, LOW);
  }
}

void sendLightLevel(){ //Le format d'envoi est "L56w" pour un niveau à 56% par ex
  int value = analogRead(LDR);
  String toSendLum = "L" + String(value*100/4096);
  toSendLum += "w"; //Pour signifier la fin du message
  ESP_BT.println(toSendLum);
}


void sendBatteryLevel(){ //Le format d'envoi est "B57w" pour un niveau à 57% par ex
  int levelBatDigit = analogRead(pinBatteryLevel);
  float tensionBat = (float)levelBatDigit*(3.3 / (float)4095) / (float)coeffPont;  //Calcul fait pour une batterie lithium
  float coeffDir = (float)100/(4.2-1.9);
  float ordonnee = (float)100-coeffDir*4.2;
  float pourcentageLevel = tensionBat*coeffDir + ordonnee ;
  Serial.println("pourcentage");
  Serial.println(pourcentageLevel);
  String toSendBattery = "B" + String(pourcentageLevel);
  toSendBattery += "w"; //Pour signifier la fin du message
  ESP_BT.println(toSendBattery);
}




