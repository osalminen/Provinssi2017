/*
 * Arduino program for the Provinssi 2017 liike-energiateos project, tested on Arduino Uno, 
 * might work with other Arduinos that share the same processor
 * 
 * This file contains everything related to the board receiving and sending various informations through the serial connection
 */


 //Happens after every loop, executes if there is something in the Serial buffer
void serialEvent(void) { 
  if (Serial.available()){
    //Prepare to read input from the buffer
    String input = "";
    char i_buffer[40]; //Buffer for strtok, 40 chars should be plenty for incoming commands. 
    char *p = i_buffer;
    char *str;

    //Move contents of the buffer to local variable
    input = Serial.readStringUntil('\r\n'); //timeout after 1000msec if ending chars missing
    input.toCharArray(i_buffer, sizeof(i_buffer)); //Convert the String object to buffer into char array
    
    count = 0; //Reset counter for the stats splashout
    
    //While the pointer is pointing to next token
    while ((str = strtok_r(p, ":", &p)) != NULL){ //delimiter is the semicolon
      byte cmd = atoi(str);
      switch (cmd){
        
        default:
          //Print out the menu  
          Serial.println(P("\nProvinssiprojektin käyttöohje:"));
          Serial.println(P("Kaikki komennot ovat muotoa komento[:käsky]"));
          Serial.println(P("Komentoja voi ketjuttaa, komentojen ja mahdollisten käskyjen välissä kaksoispiste (:)"));
          Serial.println(P("Paina 0 nähdäksesi tämän ohjeen"));
          Serial.println(P("Paina 1 muuttaaksesi RPM triggeriä, esim. 1:200"));
          Serial.println(P("Paina 2 vaihtaaksesi pyörien lukumäärää, esim 2:5"));
          Serial.println(P("Paina 3 vaihtaaksesi aikaa sekunteissa jolta RPM data kerätään, esim 3:5"));
          Serial.println(P("Paina 4 vaihtaaksesi hystereesin suuruutta hertseissä, 1 Hz = 60RPM, esim 4:1"));
          Serial.println(P("Paina 5 vaihtaaksesi korjauskerrointa, esim 5:3"));
          Serial.println(P("Paina 6 nähdäksesi statistiikkaa minuutin välein"));
          Serial.println(P("Paina 7 tallentaaksesi parametrien arvot muistiin"));
          Serial.println(P("Paina 8 ladataksesi parametrien arvot muistista"));
          Serial.println(P("Paina 9 palauttaaksesi vakioarvot muistiin"));
          break;
          
        case 1:{
          //Set trigger rpm
          str = strtok_r(p, ":", &p);
          //Trigger rpm can't be lower than 0
          if (atoi(str) < 0){str =(char *) "0";}; //Cast the string constant to char *, otherwise you'll get bunch of warnings
          setRpm = atoi(str);
          Serial.print(P("Trigger rpm successfully changed to: "));
          Serial.println(setRpm);
          break;
        }
        
        case 2:{
          //Number of bikes
          str = strtok_r(p, ":", &p);
          //Number of bikes can't be lower than 1
          if (atoi(str) < 1){str =(char *) "1";};
          NumberOfBikes = atoi(str);
          Serial.print(P("Number of bikes successfully changed to: "));
          Serial.println(NumberOfBikes);
          break;
        }
        
        case 3:{
          //sampleDur
          str = strtok_r(p, ":", &p);
          //Sample duration must be at least 1 second
          if (atoi(str) < 1){str =(char *) "1";}
          else if (atoi(str) > 20){str =(char *) "20";}
          sampleDur = atoi(str);
          Serial.print(P("Sample duration successfully changed to: "));
          Serial.println(sampleDur);
          break;
        }
        
        case 4:{
          //Hysteresis
          str = strtok_r(p, ":", &p);
          //Hysteresis can't be negative
          if (atoi(str) < 0){str =(char *) "0";}
          hysteresis = atoi(str);
          Serial.print(P("Hysteresis RPM successfully changed to: "));
          Serial.println(hysteresis*60);
          break;
        }
        
        case 5:{
          //Hattuvakio
          str = strtok_r(p, ":", &p);
          //Adjustment factor for pedals is at least 1
          if (atoi(str) <1){str =(char *) "1";}
          hattuvakio = atoi(str);
          Serial.print(P("Pedal adjustment factor successfully changed to: "));
          Serial.println(hattuvakio);
          break;
        }
        
        case 6:
          //Statistics
          Serial.print(P("\nLast reset was "));
          Serial.print((millis()/1000)/60);
          Serial.println(P(" minutes ago."));
          delay(100);
          Serial.print(P("Current total RPM is "));
          Serial.print(totRpm);
          Serial.print(P(" while for "));
          Serial.print(NumberOfBikes);
          Serial.print(P(" bikes it should be at least "));
          Serial.println(setRpm*NumberOfBikes);
          delay(100);
          stats(); //Saving space, this is used also in the automated status loop
          delay(100);
          Serial.print(P("The relay is on: "));
          Serial.println(relayState);
          delay(100);
          Serial.print(P("SampleDuration: "));
          Serial.println(sampleDur);
          Serial.print(P("Hysteresis RPM: "));
          Serial.println(hysteresis*60);
          delay(100);
          Serial.print(P("Adjustment factor for the pedals: "));
          Serial.println(hattuvakio);
          break; 
        
        case 7:{
          //Save values
          eeWrite(1, setRpm);
          eeWrite(2, NumberOfBikes);
          eeWrite(3, sampleDur);
          eeWrite(4, hysteresis);
          eeWrite(5, hattuvakio);
          Serial.println(P("Saving values to memory completed"));
          break;
          }
          
        case 8:{
          //Load values
          variableSetup();
          break;
          }
          
        case 9:{
          //Default values
          eeDefault();
          Serial.println(P("Default values saved to memory"));
          variableSetup();
          break;
          }
      }
    }
  }
}

void stats(void){
  Serial.print(P("RPM/bike: "));
  Serial.print(totRpm/NumberOfBikes);
  Serial.print(P(", trigger RPM/bike: "));
  Serial.println(setRpm); 
}
