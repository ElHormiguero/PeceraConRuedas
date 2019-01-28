/* Sistema de control de motores de una pecera basado en la posición del pez obtenida con una cámara pixy.
   Autor: Javier Vargas. El Hormiguero.
   https://creativecommons.org/licenses/by/4.0/
*/

//PINES
#define PinMotorDerPWM 9
#define PinMotorDerINA 2
#define PinMotorDerINB 4
#define PinMotorIzqPWM 6
#define PinMotorIzqINA 7
#define PinMotorIzqINB 8
#define PinLed A0
#define PinBoton A1

//CONFIGURACION
//#define debug //Imprimir posicion, velocidad y giro.
#define muestreo 20 //ms de muestreo
#define centroX 180 //Posicion x de velocidad 0
#define centroY 116 //Posicion y de velocidad 0
#define margen 2 //Margen entorno al centro de velocidad 0
#define ymax 170 //Posicion y maxima //160
#define ymin 70 //Posicion y minima
#define xmax 270 //Posicion x maxima //280 
#define xmin 70 //Posicion x minima
#define velmax 100 //Salida PWM maxima para el control de motores
#define velmin 25 //Salida PWM minima para el control de motores
#define K 0.05f //Filtro de la velocidad

#include <Pixy2.h>
Pixy2 pixy;

#include "BlinkLed.h"
BlinkLed led(PinLed, 500); //Periodo de 500ms

int Velocidad = 0;
int Giro = 0;
boolean on = 0;
unsigned long m = 0;

void setup() {
  Serial.begin(115200);

  //PinMode
  pinMode(PinMotorDerPWM, OUTPUT);
  pinMode(PinMotorDerINA, OUTPUT);
  pinMode(PinMotorDerINB, OUTPUT);
  pinMode(PinMotorIzqPWM, OUTPUT);
  pinMode(PinMotorIzqINA, OUTPUT);
  pinMode(PinMotorIzqINB, OUTPUT);
  pinMode(PinBoton, INPUT_PULLUP);

  //Led
  led.begin();
  led.Blink();
  led.On();

  //Inicio de la camara pixy
  pixy.init();
  //pixy.setLamp(255, 255);
}

void loop() {
  if (m != millis() / muestreo) {
    m = millis() / muestreo;

    //Lectura del boton
    if (LecturaBoton()) {
      on = !on;
      //Apagado
      if (!on) {
        led.Blink();
        led.On();
        Velocidad = 0;
        Giro = 0;
      }
      //Encendido
      if (on) led.NoBlink();
    }

    //Estado encendido
    if (on) {

      //Activa el reconocimiento
      pixy.ccc.getBlocks();

      //Si hay objetos detectados
      if (pixy.ccc.numBlocks) {

        //Enciende el led
        led.On();

        //Obtenemos la velocidad y giro
        Velocidad = -ObtenerVelocidad(pixy.ccc.blocks[0].m_x);
        Giro = -ObtenerGiro(pixy.ccc.blocks[0].m_y);

#ifdef debug
        Serial.print(pixy.ccc.blocks[0].m_x);
        Serial.print(" -> ");
        Serial.print(Velocidad);
        Serial.print("   /   ");
        Serial.print(pixy.ccc.blocks[0].m_y);
        Serial.print(" -> ");
        Serial.println(Giro);
#endif
      }

      //No hay objeto detectado
      else {
        led.Off();
        Velocidad = 0;
        Giro = 0;
      }


    }

    //Movemos los motores
    
    ControlMotores(Velocidad, Giro);
    
    //Estado del led
    led.Update();
  }
}


int ObtenerVelocidad(int x) {

  //Velocidad positiva
  if (x > centroX + margen) {
    int vel = map(x, centroX + margen, xmax, 0, 100);
    vel = constrain(vel, 0, 100);
    return vel;
  }

  //Velocidad negativa
  if (x < centroX - margen) {
    int vel = map(x, centroX - margen, xmin, 0, -100);
    vel = constrain(vel, -100, 0);
    return vel;
  }

  return 0;
}

int ObtenerGiro(int y) {

  //Giro positivo
  if (y > centroY + margen) {
    int gir = map(y, centroY + margen, ymax, 0, 100);
    gir = constrain(gir, 0, 100);
    return gir;
  }

  //Giro negativo
  if (y < centroY - margen) {
    int gir = map(y, centroY - margen, ymin, 0, -100);
    gir = constrain(gir, -100, 0);
    return gir;
  }

  return 0;
}

void ControlMotores(float v, float g) {
  static float VelDer = 0;
  static float VelIzq = 0;
  float VelDer1;
  float VelIzq1;

  //Velocidad de cada rueda
  if (v >= 0) {
    VelDer1 = constrain(v - g, -100, 100);
    VelIzq1 = constrain(v + g, -100, 100);
  }
  else if (v < 0) {
    VelDer1 = constrain(v + g, -100, 100);
    VelIzq1 = constrain(v - g, -100, 100);
  }

  //Filtro de la velocidad de cada rueda
  VelDer = (float)VelDer1 * K + (float)(1 - K) * VelDer;
  VelIzq = (float)VelIzq1 * K + (float)(1 - K) * VelIzq;

  //  Serial.print(VelDer);
  //  Serial.print("/");
  //  Serial.println(VelIzq);

  //Motor derecho
  if ((int)VelDer < 0) {
    digitalWrite(PinMotorDerINB, LOW);
    digitalWrite(PinMotorDerINA, HIGH);
  }
  else if ((int)VelDer > 0) {
    digitalWrite(PinMotorDerINA, LOW);
    digitalWrite(PinMotorDerINB, HIGH);
  }
  else if ((int)VelDer == 0) {
    digitalWrite(PinMotorDerINA, LOW);
    digitalWrite(PinMotorDerINB, LOW);
  }
  analogWrite(PinMotorDerPWM, map(abs(VelDer), 0, 100, velmin, velmax));

  //Motor Izquierdo
  if ((int)VelIzq < 0) {
    digitalWrite(PinMotorIzqINB, LOW);
    digitalWrite(PinMotorIzqINA, HIGH);
  }
  else if ((int)VelIzq > 0) {
    digitalWrite(PinMotorIzqINA, LOW);
    digitalWrite(PinMotorIzqINB, HIGH);
  }
  else if ((int)VelIzq == 0) {
    digitalWrite(PinMotorIzqINA, LOW);
    digitalWrite(PinMotorIzqINB, LOW);
  }
  analogWrite(PinMotorIzqPWM, map(abs(VelIzq), 0, 100, velmin, velmax));

}

boolean LecturaBoton() {
  if (!digitalRead(PinBoton)) {
    delay(300);
    return 1;
  }
  else return 0;
}
