#include <Servo.h>

const int NUM_SERVOS = 5; //Cantidad de servos del brazo

//Pines donde estan conectados los servos:
//Placa base, Hombro, Codo, Muñeca, Pinzas
const int PINES[NUM_SERVOS] = {9, 6, 5, 3, 11};

Servo servos[NUM_SERVOS]; //0: Placa base, 1: Hombro, 2: Codo, 3: Muñeca, 4: Pinzas

int actual[NUM_SERVOS] = {90, 90, 90, 90, 90}; //Posiciones actuales de los servos

void setup() { //Configuracion de los pines

  Serial.begin(9600);      //Iniciamos la comunicacion serie (igual que el Monitor Serie)
  Serial.setTimeout(100);  //Tiempo maximo de espera al leer numeros (evita pausas de 1 segundo)

  for(int i = 0; i < NUM_SERVOS; i++) {
    servos[i].attach(PINES[i]); //Conectamos cada servo a su pin
    servos[i].write(actual[i]); //Posicion inicial de todos los servos
  }

  delay(1500); //Esperamos a que los servos lleguen a su posicion

  Serial.println("Listo. Escribe: servo(1-5) grados(0-180). Ejemplo: 1 45");
}

void moverBrazo(const int destino[], int duracion) {

  int inicio[NUM_SERVOS]; //Guardamos las posiciones iniciales
  int meta[NUM_SERVOS];   //Destinos ya limitados al rango valido

  for(int i = 0; i < NUM_SERVOS; i++) {
    inicio[i] = actual[i];
    //Guardamos donde esta cada servo para saber desde donde empezar

    meta[i] = constrain(destino[i], 0, 180);
    //Limitamos el destino entre 0 y 180 grados para proteger el servo
  }

  int updates = duracion / 15; //Calculamos cuantas actualizaciones tendremos

  if(updates < 1) {
    updates = 1; //Evitamos que el numero de actualizaciones sea cero
  }

  //Recorremos todas las actualizaciones del movimiento
  for(int update = 1; update <= updates; update++) {

    //Recorremos los cinco servos
    for(int i = 0; i < NUM_SERVOS; i++) {

      //Calculamos la posicion intermedia de cada servo
      //(long evita el desbordamiento de int en Arduino UNO)
      int posicion = inicio[i] +
      (long)(meta[i] - inicio[i]) * update / updates;

      //Enviamos los grados calculados al servo correspondiente
      servos[i].write(posicion);
    }

    delay(15); //Esperamos 15 milisegundos antes de la siguiente actualizacion
  }

  //Actualizamos las posiciones actuales de los cinco servos
  for(int i = 0; i < NUM_SERVOS; i++) {
    actual[i] = meta[i];
  }
}

void loop() {

  //Solo actuamos si llego algo por el Monitor Serie
  if(Serial.available() > 0) {

    int numServo = Serial.parseInt(); //Primer numero: cual servo (1 a 5)
    int grados = Serial.parseInt();   //Segundo numero: a cuantos grados

    //Limpiamos lo que haya sobrado en el buffer (por ejemplo el salto de linea)
    while(Serial.available() > 0) {
      Serial.read();
    }

    //Validamos que el servo exista
    if(numServo < 1 || numServo > NUM_SERVOS) {
      Serial.println("Servo invalido. Usa un numero del 1 al 5.");
      return;
    }

    //Copiamos las posiciones actuales para mover solo el servo elegido
    int destino[NUM_SERVOS];
    for(int i = 0; i < NUM_SERVOS; i++) {
      destino[i] = actual[i];
    }

    destino[numServo - 1] = grados; //Cambiamos solo el servo pedido (indice = numero - 1)

    moverBrazo(destino, 1000); //Movemos el brazo en 1000 milisegundos

    //Confirmamos en el Monitor Serie donde quedo el servo
    Serial.print("Servo ");
    Serial.print(numServo);
    Serial.print(" -> ");
    Serial.print(actual[numServo - 1]);
    Serial.println(" grados");
  }
}