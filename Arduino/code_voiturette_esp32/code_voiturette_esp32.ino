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

float pourcentageVitesse=0;
int eclairage_minimal = 1000;
long int t1 = 0;
long int t2 = 0;
boolean feux_allume = false;
boolean isAllume = false;
boolean feux_auto = true;
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
}

void loop() {
  // put your main code here, to run repeatedly:
  
  
  if(ESP_BT.hasClient()){
    Serial.println("Bluetooth connected");
    while(ESP_BT.hasClient()) 
    { 
      checkStateLights();
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

      else if((char)received=='m'){ //comme manuel, pour l'eclairage
        feux_auto = false;              
      }
      else if((char)received=='a'){ //comme automatique, pour l'eclairage
        feux_auto = true;              
      }
      else if((char)received=='f'){ //comme feux, pour allumer l'eclairage
        feux_allume = 1-feux_allume; 
        isAllume = feux_allume;             
      }
      else if((char)received=='s'){
          stopCar();

      }    
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
  checkStateLights();
  
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
  if(feux_auto){  //si les feux sont en mode automatiques
    int value = analogRead(LDR);
    if(value>eclairage_minimal){//suffisament clair -> on eteint
      digitalWrite(pinLEDRouge, HIGH);
      digitalWrite(pinLEDBlanche, HIGH);
      isAllume = false;
    }
    else{//trop sombre -> on allume
      digitalWrite(pinLEDRouge, LOW);
      digitalWrite(pinLEDBlanche, LOW);
      isAllume = true;
    }
  }
  else{ //si les feux sont en mode manuels
    if(feux_allume){//on les allume
      digitalWrite(pinLEDRouge, LOW);
      digitalWrite(pinLEDBlanche, LOW);
    }
    else{//on les eteints
      digitalWrite(pinLEDRouge, HIGH);
      digitalWrite(pinLEDBlanche, HIGH);
    }
  }
  
  t2 = micros();
  if(t2-t1 > 300000){
    Serial.println("coucou");
    if(isAllume){
      ESP_BT.println('a');
    }
    else{
      ESP_BT.println('e');
    }
    t1 = micros();
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
