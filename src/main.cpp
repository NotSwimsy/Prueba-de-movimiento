#include <Arduino.h>
#include <Wire.h>
#include <AS5600.h>


//PINES TB6600 azimut (modificar)


#define STEP_PIN_AZ 23
#define DIR_PIN_AZ  22
#define ENA_PIN_AZ  26



// PINES TB6600 elevacion (modificar si estan mal)
#define STEP_PIN_EL 19
#define DIR_PIN_EL  18



//I2C AS5600 AZ
#define SDA_AZ 16
#define SCL_AZ 17



//I2C AS5600 EL
#define SDA_EL 25
#define SCL_EL 27



//BUSES I2C

TwoWire I2C_AZ = TwoWire(0);
TwoWire I2C_EL = TwoWire(1);


//encoders AS5600
AS5600 encoderAZ(&I2C_AZ);
AS5600 encoderEL(&I2C_EL);



const int PASOS_POR_VUELTA = 200;
const int MICROSTEP = 8;

const int PASOS_360 = PASOS_POR_VUELTA * MICROSTEP;


//limites
const float LIMITE_AZ = 360.0;
const float LIMITE_EL = 180.0;


// POSICION DE LOS ENCODERS
//la posición inicial se considera 0° 
// AZ 0° a 360°
// EL 0° a 180°
//

float posicionAZ = 0.0;
float posicionEL = 0.0;


//lectura de los encoders
float rawAZAnterior = 0.0;
float rawELAnterior = 0.0;


const int TIEMPO_PASO = 500;   //microsegundos de velocidad del motor



//lee AS5600 AZ

float leerAZ()
{
  uint16_t raw = encoderAZ.rawAngle();

  return (raw * 360.0) / 4096.0;
}

// AS5600 EL
float leerEL()
{
  uint16_t raw = encoderEL.rawAngle();
  return (raw * 360.0) / 4096.0;
}



// ACTUALIZAR POSICION AZ
//
//detecta:
//
// 359->0 = avance
// 0->359 = retroceso


void actualizarAZ()
{
  float actual = leerAZ();

  float diferencia = actual - rawAZAnterior;


//cruce 359 -> 0

  if (diferencia > 180.0)
  {
    diferencia -= 360.0;
  }


//cruce 0 -> 359

  if (diferencia < -180.0)
  {
    diferencia += 360.0;
  }


  posicionAZ += diferencia;

  rawAZAnterior = actual;
}



// ACTUALIZAR POSICION EL
void actualizarEL()
{
  float actual = leerEL();

  float diferencia = actual - rawELAnterior;


  if (diferencia > 180.0)
  {
    diferencia -= 360.0;
  }


  if (diferencia < -180.0)
  {
    diferencia += 360.0;
  }


  posicionEL += diferencia;

  rawELAnterior = actual;
}



void pasoAZ(bool adelante)
{
//primero actualizar encoder

  actualizarAZ();

  if (adelante && posicionAZ >= LIMITE_AZ)
  {
    Serial.println("AZ: LIMITE +360°");
    return;
  }


  if (!adelante && posicionAZ <= 0.0)
  {
    Serial.println("AZ: LIMITE 0°");
    return;
  }


  //dirección

  if (adelante)
  {
    digitalWrite(DIR_PIN_AZ, HIGH);
  }
  else
  {
    digitalWrite(DIR_PIN_AZ, LOW);
  }


  //paso

  digitalWrite(STEP_PIN_AZ, HIGH);

  delayMicroseconds(TIEMPO_PASO);

  digitalWrite(STEP_PIN_AZ, LOW);

  delayMicroseconds(TIEMPO_PASO);


  // Actualizar encoder después del paso

  actualizarAZ();
}


//un paso el
void pasoEL(bool adelante)
{
//actualizar encoder

  actualizarEL();



  if (adelante && posicionEL >= LIMITE_EL)
  {
    Serial.println("EL: LIMITE +180°");
    return;
  }


  if (!adelante && posicionEL <= 0.0)
  {
    Serial.println("EL: LIMITE 0°");
    return;
  }


  //dirección

  if (adelante)
  {
    digitalWrite(DIR_PIN_EL, HIGH);
  }
  else
  {
    digitalWrite(DIR_PIN_EL, LOW);
  }


  //paso

  digitalWrite(STEP_PIN_EL, HIGH);

  delayMicroseconds(TIEMPO_PASO);

  digitalWrite(STEP_PIN_EL, LOW);

  delayMicroseconds(TIEMPO_PASO);


  //actualizar encoder

  actualizarEL();
}



//MOSTRAR POSICIONES
void mostrarPosiciones()
{
  actualizarAZ();
  actualizarEL();


  Serial.println();
  Serial.println("==============================");

  Serial.print("AZ encoder: ");
  Serial.print(leerAZ(), 2);
  Serial.println("°");

  Serial.print("AZ posicion: ");
  Serial.print(posicionAZ, 2);
  Serial.println("°");

  Serial.println("------------------------------");

  Serial.print("EL encoder: ");
  Serial.print(leerEL(), 2);
  Serial.println("°");

  Serial.print("EL posicion: ");
  Serial.print(posicionEL, 2);
  Serial.println("°");

  Serial.println("==============================");
}




void setup()
{
  Serial.begin(115200);

  delay(1000);


  Serial.println();
  Serial.println("======================================");
  Serial.println("     PRUEBA ROTOR ATLAS");
  Serial.println("======================================");



  //I2C AZ
  I2C_AZ.begin(
    SDA_AZ,
    SCL_AZ,
    400000
  );

  delay(100);


  if (encoderAZ.begin())
  {
    Serial.println("AS5600 AZ: OK");
  }
  else
  {
    Serial.println("AS5600 AZ: ERROR");
  }


 
  // I2C EL
 

  I2C_EL.begin(
    SDA_EL,
    SCL_EL,
    400000
  );

  delay(100);


  if (encoderEL.begin())
  {
    Serial.println("AS5600 EL: OK");
  }
  else
  {
    Serial.println("AS5600 EL: ERROR");
  }



// PINES AZ


  pinMode(STEP_PIN_AZ, OUTPUT);
  pinMode(DIR_PIN_AZ, OUTPUT);
  pinMode(ENA_PIN_AZ, OUTPUT);


  digitalWrite(STEP_PIN_AZ, LOW);
  digitalWrite(DIR_PIN_AZ, LOW);


  //habilitar TB6600
  digitalWrite(ENA_PIN_AZ, LOW);


 
  //PINES EL
  pinMode(STEP_PIN_EL, OUTPUT);
  pinMode(DIR_PIN_EL, OUTPUT);


  digitalWrite(STEP_PIN_EL, LOW);
  digitalWrite(DIR_PIN_EL, LOW);


  // POSICION INICIAL
  rawAZAnterior = leerAZ();
  rawELAnterior = leerEL();


  posicionAZ = 0.0;
  posicionEL = 0.0;


  Serial.println();
  Serial.println("======================================");
  Serial.println("POSICION INICIAL");
  Serial.println("======================================");

  Serial.print("AZ AS5600: ");
  Serial.print(rawAZAnterior, 2);
  Serial.println("°");

  Serial.print("EL AS5600: ");
  Serial.print(rawELAnterior, 2);
  Serial.println("°");


 
                        // COMANDOS
 

  Serial.println();
  Serial.println("COMANDOS:");
  Serial.println("------------------------------");
  Serial.println("a = AZ +");
  Serial.println("z = AZ -");
  Serial.println("e = EL +");
  Serial.println("d = EL -");
  Serial.println("p = mostrar posiciones");
  Serial.println("s = detener");
  Serial.println("------------------------------");
  Serial.println();

  Serial.println("ROTOR LISTO");
}



void loop()
{
  if (Serial.available())
  {
    char comando = Serial.read();



    //AZIMUT +


    if (comando == 'a')
    {
      Serial.println("AZ +");

      pasoAZ(true);
    }


    //AZIMUT -
 

    else if (comando == 'z')
    {
      Serial.println("AZ -");

      pasoAZ(false);
    }



    //EL +


    else if (comando == 'e')
    {
      Serial.println("EL +");

      pasoEL(true);
    }


    //EL-

    else if (comando == 'd')
    {
      Serial.println("EL -");

      pasoEL(false);
    }


    // POSICIONES
    else if (comando == 'p')
    {
      mostrarPosiciones();
    }


    // STOP
    else if (comando == 's')
    {
      digitalWrite(STEP_PIN_AZ, LOW);
      digitalWrite(STEP_PIN_EL, LOW);

      Serial.println("STOP");
    }
  }
}
