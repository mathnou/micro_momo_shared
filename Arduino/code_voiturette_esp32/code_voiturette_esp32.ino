#include "BluetoothSerial.h" 
BluetoothSerial ESP_BT;  
#include <analogWrite.h>


#define pinINA1 14 // Moteur A, entrée 1 - Commande en PWM possible
#define pinINA2 15 // Moteur A, entrée 2 - Commande en PWM possible

#define pinINB1 13 // Moteur B, entrée 1 - Commande en PWM possible
#define pinINB2 12 // Moteur B, entrée 2 - Commande en PWM possible

#define pinLEDRouge 25    //c'est toujours moi
#define pinLEDBlanche 26  //coucou fini les conneries 
// Et non en fait, c'est repartiiiii

#define LDR 27 // Photoresistance 

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
}

void loop() {
  // put your main code here, to run repeatedly:
  
  if(ESP_BT.hasClient()){
    Serial.println("Bluetooth connected");
    while(ESP_BT.hasClient()) 
    { 
      int received;
      received = ESP_BT.read();
        
      if((char)received=='a'){
            // --- Avancer les deux moteurs-------------------
            // Marche avant
            Serial.println("Avancer : " + (String)pourcentageVitesse);
            avancer(pourcentageVitesse);
     }

        else if((char)received=='r'){
            // --- Reculer  les deux moteurs-------------------
            //Marche arrière
            Serial.println("Reculer : " + (String)pourcentageVitesse);
            reculer(pourcentageVitesse);
            
            
        }

        else if((char)received=='d'){
            // --- Tourner à droite-------------------
            int duty = int(pourcentageVitesse*2.55);
            Serial.println("Tourner à droite");
            analogWrite( pinINA2, LOW);
            analogWrite( pinINB1, LOW);
            analogWrite( pinINA1, duty );
            analogWrite(pinINB2, duty);
            delay(100);
            avancer(pourcentageVitesse);
        }
        else if((char)received=='g'){
            // --- Tourner à gauche-------------------
            int duty = int(pourcentageVitesse*2.55);
            Serial.println("Tourner à gauche");
            analogWrite( pinINA1, LOW);
            analogWrite( pinINB2, LOW);
            analogWrite( pinINA2, duty );
            analogWrite(pinINB1, duty);
            delay(100);
            avancer(pourcentageVitesse);
        }
        else if((char)received=='s'){
            // --- STOP -------------------
            stopCar();

        }
        else if((char)received=='v'){
            // --- STOP -------------------
            Serial.println("Changement de vitesse");
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
            if(isAvance){
              avancer(pourcentageVitesse); 
            }
            else if(isRecule){
              reculer(pourcentageVitesse);
            }
        }     
    }
  }
  else{
    Serial.println("Bluetooth NOT connected");
  }
  checkStateLights();
  
}

void avancer(float pourcentageVitesse){
  analogWrite( pinINA2, LOW );
  analogWrite(pinINB2, LOW);
  int duty = int(pourcentageVitesse*2.55);
  analogWrite( pinINA1, duty );
  analogWrite(pinINB1, duty);
  isAvance=true;
  isRecule=false;
}

void reculer(float pourcentagevitesse){
  analogWrite( pinINA1, LOW );
  analogWrite(pinINB1, LOW);
  int duty = int(pourcentageVitesse*2.55);
  analogWrite( pinINA2, duty );
  analogWrite(pinINB2, duty);
  isAvance=false;
  isRecule=true;
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

void stopCar(){
  analogWrite( pinINA1, LOW);
  analogWrite(pinINB1, LOW);
  analogWrite( pinINA2, LOW);
  analogWrite(pinINB2, LOW);
  Serial.println("STOP");
  isAvance=false;
  isRecule=false;
}
