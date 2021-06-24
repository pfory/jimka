/*
**************************
*     MODUL  HC-SR04     *
*      www.santy.cz      *
*     © Aleš  Müller     *
*********************************************
*  upraveno z jiného zdoje                  *
*  program volně šiřitelný, prosím sdílejte *
*********************************************/
 
int vcc  = 2; /* PŘIPOJÍME PIN D2 NA Vcc */
int trig = D6; /* PŘIPOJÍME PIN D3 NA Trig */
int echo = D7; /* PŘIPOJÍME PIN D4 NA Echo */
int gnd  = 5; /* PŘIPOJÍME PIN D5 NA GND */
/* deklarace proměnných */
long zpozdeni, palce, cm;
 
void setup() {
/* nastavíme piny pro výstup / napájení */
pinMode (vcc,OUTPUT);
pinMode (gnd,OUTPUT);
/* pin Echo používáme na čtení odraženého signálu od překážky (jeho zpoždění)  */
pinMode(echo,INPUT);
/* komunikace po seriové lince */
Serial.begin(115200);
}
 
void loop()
{
 digitalWrite(vcc, HIGH);
/* signál (PING) se pouští jako HIGH na 2 mikrosekundy nebo více */
/* ještě před signálem dáme krátký puls LOW pro čistý následující HIGH */
// The PING))) is triggered by a HIGH pulse of 2 or more microseconds.
// Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
pinMode(trig, OUTPUT);
digitalWrite(trig, LOW);
delayMicroseconds(2);
digitalWrite(trig, HIGH);
delayMicroseconds(5);
digitalWrite(trig, LOW);
 
zpozdeni = pulseIn(echo, HIGH);
 
/* pomocí funkce si překonvertujeme zpoždění na délkové jednotky */
palce = MikrosekundyNaPalce(zpozdeni);
cm = MikrosekundyNaCentimetry(zpozdeni);
 
Serial.print(palce);
Serial.print(" palcu, ");
Serial.print(cm);
Serial.print(" centimetru.");
Serial.println();
 
/* pauza, abychom to stihli přečíst  :-) */
delay(250);
}
 
long MikrosekundyNaPalce(long microseconds)
{
  /* rychlost zvuku je cca 73.746 mikrosekund na palec (1130 stop za sekundu) */
  /* nezapomeňte, že signál musí urazit cestu k překážce a zpět, tedy ještě vydělit dvěma! */
return microseconds / 74 / 2;
}
 
long MikrosekundyNaCentimetry(long microseconds)
{
  /* rychlost zvuku je cca 340 m/s nebo 29 mikrosekund na centimetr */
  /* nezapomeňte, že signál musí urazit cestu k překážce a zpět, tedy ještě vydělit dvěma! */
return microseconds / 29 / 2;

    }

