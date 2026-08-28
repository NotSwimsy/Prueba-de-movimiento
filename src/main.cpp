#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <AS5600.h>


const char* ssid = "CESJT";
const char* password = "itisjtsmg";

WiFiServer server(5000);
//hola--


//TB6600 - AZIMUT
// ___________________

#define STEP_PIN_AZ 23
#define DIR_PIN_AZ  22
#define ENA_PIN_AZ  26


//TB6600 - ELEVACION
// _______________________

#define STEP_PIN_EL 19
#define DIR_PIN_EL  18



// I2C - AS5600 AZIMUT


#define SDA_AZ 16
#define SCL_AZ 17


// I2C - AS5600 ELEVACION


#define SDA_EL 25
#define SCL_EL 27


// ==========================
// BUSES I2C
// ========================

TwoWire I2C_AZ = TwoWire(0);
TwoWire I2C_EL = TwoWire(1);



// AS5600
AS5600 encoderAZ(&I2C_AZ);
AS5600 encoderEL(&I2C_EL);



// CONFIGURACION DE MOTORES
const int pasosPorVuelta = 200;
const int microstep = 8;

const int pasosTotales = pasosPorVuelta * microstep;


// ======================================================
// POSICIONES
float azimuth = 0.0;
float elevation = 0.0;

float azimuthReal = 0.0; // significa que estamos suponiendo una posición física inicial concreta que todavía no conocemos
float elevationReal = 0.0; //tenemos que poner físicamente el rotor en una posición de referencia y medir qué valor entrega cada AS5600. 
//Después hacemos la conversión correcta.


// POSICIONES INICIALES DE LOS ENCODERS


float azimuthInicial = 0.0;
float elevationInicial = 0.0;

bool encodersInicializados = false;

// ====================================================
// LIMITES FISICOS DE ELEVACION
// ====================================================
//
// CAMBIAR ESTOS VALORES CUANDO MIDAMOS EL RANGO REAL
// DEL ROTOR.
//
// ej:
// 0° = antena completamente abajo
// 175° = antena completamente arriba
//

const float ELEVACION_MIN = 0.0;
const float ELEVACION_MAX = 175.0;


// ==============================
// INICIALIZAR ENCODERS

void iniciarEncoders()
{
  // ----------------------------------------------------
  // I2C AZIMUT
  // ----------------------------------------------------

  I2C_AZ.begin(
    SDA_AZ,
    SCL_AZ,
    400000
  );

  delay(100);

  Serial.println("Inicializando AS5600 AZIMUT...");

  if (encoderAZ.begin())
  {
    Serial.println("AS5600 AZIMUT: OK");
  }
  else
  {
    Serial.println("AS5600 AZIMUT: ERROR");
  }


  // ----------------------------------------------------
  //I2C ELEVACION
  // ----------------------------------------------------

  I2C_EL.begin(
    SDA_EL,
    SCL_EL,
    400000
  );

  delay(100);

  Serial.println("Inicializando AS5600 ELEVACION...");

  if (encoderEL.begin())
  {
    Serial.println("AS5600 ELEVACION: OK");
  }
  else
  {
    Serial.println("AS5600 ELEVACION: ERROR");
  }
}



//LEER ANGULO RAW AZIMUT


float leerAzimutRaw()
{
  uint16_t raw = encoderAZ.rawAngle();

  float grados =
    (raw * 360.0) / 4096.0;

  return grados;
}



///LEER ANGULO RAW ELEVACION

float leerElevacionRaw()
{
  uint16_t raw = encoderEL.rawAngle();

  float grados =
    (raw * 360.0) / 4096.0;

  return grados;
}



// NORMALIZAR ANGULO 0 - 360
float normalizar360(float angulo)
{
  while (angulo >= 360.0)
    angulo -= 360.0;

  while (angulo < 0.0)
    angulo += 360.0;

  return angulo;
}


// ======================
///CALCULAR AZIMUT REAL
// El AZIMUT es circular
// si el sensor pasa de 359° a 0°,
// el programa entiende que solamente avanzó 1°
//

float calcularAzimut()
{
  float actual = leerAzimutRaw();

  float diferencia =
    actual - azimuthInicial;


  if (diferencia > 180.0)
    diferencia -= 360.0;


  if (diferencia < -180.0)
    diferencia += 360.0;


  return normalizar360(
    diferencia + 180.0
  );
}


// =============================
// CALCULAR ELEVACION REAL
//la elevación NO es circular
// tiene un límite inferior y superior


float calcularElevacion()
{
  float actual = leerElevacionRaw();

  float diferencia =
    actual - elevationInicial;


  //mantener dentro del rango físico
  diferencia = constrain(
    diferencia,
    ELEVACION_MIN,
    ELEVACION_MAX
  );


  return diferencia;
}



// ACTUALIZAR POSICIONES REALES

void actualizarEncoders()
{
  azimuthReal = calcularAzimut();

  elevationReal = calcularElevacion();
}



// ESTABLECER POSICION INICIAL

// ===========================================

void inicializarPosicion()
{
  delay(500);


  // Leer sensores
  azimuthInicial = leerAzimutRaw();

  elevationInicial = leerElevacionRaw();


  // Posiciones iniciales
  azimuthReal = 180.0;

  elevationReal = 0.0;


  encodersInicializados = true;


  Serial.println();
  Serial.println("--------------------------------------");
  Serial.println(" POSICION INICIAL DE LOS ENCODERS");
  Serial.println("--------------------------------------");


  Serial.print("AS5600 AZ inicial: ");
  Serial.print(azimuthInicial, 2);
  Serial.println("°");


  Serial.print("AS5600 EL inicial: ");
  Serial.print(elevationInicial, 2);
  Serial.println("°");


  Serial.println();
}

// SETUP
// =======================

void setup()
{
  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("_______________________________________");
  Serial.println("       ATLAS - ROTOR SATELITAL");
  Serial.println("_______________________________________");


  // ==================
  // INICIAR ENCODERS
  // ===============================

  iniciarEncoders();

  delay(200);


  // ============================
  // ESTABLECER POSICION INICIAL
  // =============================

  inicializarPosicion();



  // TB6600 AZIMUT


  pinMode(STEP_PIN_AZ, OUTPUT);
  pinMode(DIR_PIN_AZ, OUTPUT);
  pinMode(ENA_PIN_AZ, OUTPUT);

  digitalWrite(STEP_PIN_AZ, LOW);
  digitalWrite(DIR_PIN_AZ, LOW);

  // Habilitar driver
  digitalWrite(ENA_PIN_AZ, LOW);


  // TB6600 ELEVACION


  pinMode(STEP_PIN_EL, OUTPUT);
  pinMode(DIR_PIN_EL, OUTPUT);

  digitalWrite(STEP_PIN_EL, LOW);
  digitalWrite(DIR_PIN_EL, LOW);


  // =======
  // WIFI
  // =========

  Serial.println();
  Serial.println("Conectando a WiFi...");

  WiFi.begin(
    ssid,
    password
  );


  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);

    Serial.print(".");
  }


  Serial.println();
  Serial.println("WiFi conectado!");



  Serial.print("IP del ESP32: ");
  Serial.println(
    WiFi.localIP()
  );

  Serial.print("Puerto: ");
  Serial.println(5000);


  // ==================
  // INICIAR SERVIDOR
  // ==================
  server.begin();


  // ====================================================
  // MENSAJE FINAL
  // ====================================================

  Serial.println();
  Serial.println("______________________________________");
  Serial.println("           ATLAS LISTO");
  Serial.println("______________________________________");

  Serial.println();
  Serial.println("Esperando conexion de Gpredict...");
}

// ---------------------------
// VARIABLES DE SEGUIMIENTO
// ---------------------------

float objetivoAzimut = 0.0;
float objetivoElevacion = 0.0;

bool hayObjetivo = false;


// --------------------------
// CONFIGURACION DE MOVIMIENTO
// ---------------------------

// Tiempo minimo entre pasos
const unsigned long INTERVALO_PASO = 3;

// Tiempo minimo entre lecturas de posicion
const unsigned long INTERVALO_ENCODER = 20;


// =============
// TOLERANCIA
// ==============

const float TOLERANCIA_AZ = 0.5;
const float TOLERANCIA_EL = 0.5;



// TIEMPOS


unsigned long ultimoPasoAZ = 0;
unsigned long ultimoPasoEL = 0;

unsigned long ultimaLecturaEncoder = 0;


// ==========================
// DIFERENCIA ANGULAR AZIMUT
// ====================
//
// busca siempre el camino mas corto
//
// ejemplo:
// 350° -> 10° = +20°
// 10° -> 350° = -20°


float diferenciaAzimut(
  float actual,
  float objetivo
)
{
  float diferencia =
    objetivo - actual;


  if (diferencia > 180.0)
    diferencia -= 360.0;


  if (diferencia < -180.0)
    diferencia += 360.0;


  return diferencia;
}


// ========================
// HACER UN PASO AZIMUT
// ====================

void pasoAzimut(float error)
{
  if (error > 0)
  {
    digitalWrite(
      DIR_PIN_AZ,
      HIGH
    );
  }
  else
  {
    digitalWrite(
      DIR_PIN_AZ,
      LOW
    );
  }


  digitalWrite(
    STEP_PIN_AZ,
    HIGH
  );

  delayMicroseconds(400);

  digitalWrite(
    STEP_PIN_AZ,
    LOW
  );

  delayMicroseconds(400);
}


// ======================
// HACER UN PASO ELEVACION
// =============

void pasoElevacion(float error)
{
  if (error > 0)
  {
    digitalWrite(
      DIR_PIN_EL,
      HIGH
    );
  }
  else
  {
    digitalWrite(
      DIR_PIN_EL,
      LOW
    );
  }


  digitalWrite(
    STEP_PIN_EL,
    HIGH
  );

  delayMicroseconds(400);

  digitalWrite(
    STEP_PIN_EL,
    LOW
  );

  delayMicroseconds(400);
}

//===================================
// CONTROL AZIMUT
// ===================================

void controlarAzimut()
{
  float error =
    diferenciaAzimut(
      azimuthReal,
      objetivoAzimut
    );


  // Ya esta suficientemente cerca
  if (abs(error) <= TOLERANCIA_AZ)
    return;


  //evitar pasos demasiado rapidos
  if (
    millis() - ultimoPasoAZ
    < INTERVALO_PASO
  )
  {
    return;
  }


  ultimoPasoAZ = millis();


  //un solo paso
  pasoAzimut(error);
}


// ==================
// CONTROL ELEVACION
// ===========
void controlarElevacion()
{
  float error =
    objetivoElevacion -
    elevationReal;


  //ya esta suficientemente cerca
  if (abs(error) <= TOLERANCIA_EL)
    return;


  //limite inferior
  if (
    elevationReal <= ELEVACION_MIN &&
    error < 0
  )
  {
    return;
  }


  // superior
  if (
    elevationReal >= ELEVACION_MAX &&
    error > 0
  )
  {
    return;
  }


  // evitar pasos demasiado rapidos
  if (
    millis() - ultimoPasoEL
    < INTERVALO_PASO
  )
  {
    return;
  }


  ultimoPasoEL = millis();


  //un solo paso
  pasoElevacion(error);
}


// ACTUALIZAR ENCODERS
// ======================================================

void actualizarPosicion()
{
  if (
    millis() - ultimaLecturaEncoder
    < INTERVALO_ENCODER
  )
  {
    return;
  }


  ultimaLecturaEncoder = millis();


  azimuthReal =
    calcularAzimut();


  elevationReal =
    calcularElevacion();
}


// PROCESAR COMANDOS DE GPREDICT


void procesarGpredict()
{
  WiFiClient client = server.available();

  if (!client)
    return;


  Serial.println();
  Serial.println("GPREDICT CONECTADO");


  String cmd = "";

  unsigned long ultimoEstado = millis();


  // ===============================
  // MIENTRAS GPREDICT ESTE CONECTADO
  // ====================================

  while (client.connected())
  {

    // ===========================
    // RECIBIR DATOS
    // =====================

    while (client.available())
    {
      char c = client.read();

      cmd += c;


      // -----------------------------------------------
      //COMANDO COMPLETO
      // -----------------------------------------------

      if (c == '\n')
      {
        cmd.trim();

        cmd.replace(',', '.');


        Serial.print("GPREDICT: ");
        Serial.println(cmd);


        // =============================================
        //CONSULTA DE POSICION
        // =============================================

        if (cmd == "p")
        {
          actualizarPosicion();


          client.println(
            azimuthReal,
            2
          );

          client.println(
            elevationReal,
            2
          );
        }


        // =================================
        // NUEVA POSICION
        // ================================

        else
        {
          float az;
          float el;


          if (
            sscanf(
              cmd.c_str(),
              "P %f %f",
              &az,
              &el
            ) == 2
          )
          {

            // -----------------------------------------
            // GUARDAR AZIMUT
            // -----------------------------------------

            objetivoAzimut =
              normalizar360(az);


            // -----------------------------------------
          // LIMITAR ELEVACION
            // -----------------------------------------

            objetivoElevacion =
              constrain(
                el,
                ELEVACION_MIN,
                ELEVACION_MAX
              );


            hayObjetivo = true;


            Serial.println();
            Serial.println("===== NUEVO OBJETIVO =====");


            Serial.print("AZ objetivo: ");
            Serial.print(
              objetivoAzimut,
              2
            );

            Serial.println("°");


            Serial.print("EL objetivo: ");
            Serial.print(
              objetivoElevacion,
              2
            );

            Serial.println("°");


            client.println("RPRT 0");
          }


          // ============================
          // STOP
          // ===================

          else if (cmd == "S")
          {
            Serial.println("STOP");

            hayObjetivo = false;

            client.println("RPRT 0");
          }


          // ====================
          // QUIT
          // =======================

          else if (cmd == "q")
          {
            Serial.println(
              "GPREDICT solicito desconexion"
            );

            hayObjetivo = false;

            break;
          }
        }


        //limpiar comando
        cmd = "";
      }
    }


    // =======================
    //ACTUALIZAR POSICION REAL
    // =====================

    actualizarPosicion();



    //sEGUIMIENTO CONTINUO
 

    if (hayObjetivo)
    {
      controlarAzimut();

      controlarElevacion();
    }


 
    // MOSTRAR ESTADO


    if (
      millis() - ultimoEstado >= 500
    )
    {
      ultimoEstado = millis();


      Serial.print("AZ REAL: ");
      Serial.print(
        azimuthReal,
        2
      );

      Serial.print("° | AZ OBJ: ");
      Serial.print(
        objetivoAzimut,
        2
      );


      Serial.print("° | EL REAL: ");
      Serial.print(
        elevationReal,
        2
      );


      Serial.print("° | EL OBJ: ");
      Serial.print(
        objetivoElevacion,
        2
      );


      Serial.println("°");
    }


    delay(1);
  }



  //DESCONECTAR

  client.stop();

  hayObjetivo = false;


  Serial.println();
  Serial.println(
    "GPREDICT DESCONECTADO"
  );
}



// LOOP PRINCIPAL


void loop()
{
  procesarGpredict();
}