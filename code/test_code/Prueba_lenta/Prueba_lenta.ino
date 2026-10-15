
#include <Servo.h>

Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;
Servo servo5;

int actual[] = {90, 90, 90, 90, 90};

int pines[] = {9, 6, 5, 3, 11};

void moverBrazo(int destino[], int duracion) {
  int inicio[5];

  for (int i = 0; i < 5; i++) {
    inicio[i] = actual[i];
  }

  int pasos = duracion / 15;

  if (pasos < 1) pasos = 1;

  for (int paso = 1; paso <= pasos; paso++) {
    for (int i = 0; i < 5; i++) {
      int posicion = inicio[i] +
        (destino[i] - inicio[i]) * paso / pasos;

      switch (i) {
        case 0: servo1.write(posicion); break;
        case 1: servo2.write(posicion); break;
        case 2: servo3.write(posicion); break;
        case 3: servo4.write(posicion); break;
        case 4: servo5.write(posicion); break;
      }
    }

    delay(15);
  }

  for (int i = 0; i < 5; i++) {
    actual[i] = destino[i];
  }
}

void setup() {
  servo1.attach(9);
  servo2.attach(6);
  servo3.attach(5);
  servo4.attach(3);
  servo5.attach(11);

  servo1.write(90);
  servo2.write(90);
  servo3.write(90);
  servo4.write(90);
  servo5.write(90);

  delay(1500);
}

void loop() {
  // 1. Abrir la garra y preparar el brazo
  int posicion1[] = {100, 75, 110, 90, 90};
  moverBrazo(posicion1, 900);
  delay(400);

  // 2. Acercarse al objeto
  int posicion2[] = {115, 105, 65, 115, 90};
  moverBrazo(posicion2, 1100);
  delay(300);

  // 3. Cerrar la garra
  int posicion3[] = {115, 105, 65, 115, 45};
  moverBrazo(posicion3, 500);
  delay(700);

  // 4. Levantar el objeto
  int posicion4[] = {100, 75, 100, 90, 45};
  moverBrazo(posicion4, 1000);
  delay(400);

  // 5. Girar y trasladar el objeto
  int posicion5[] = {65, 85, 90, 110, 45};
  moverBrazo(posicion5, 1200);
  delay(500);

  // 6. Soltar el objeto
  int posicion6[] = {65, 85, 90, 110, 90};
  moverBrazo(posicion6, 500);
  delay(700);

  // 7. Regresar a la posición inicial
  int inicio[] = {90, 90, 90, 90, 90};
  moverBrazo(inicio, 1300);
  delay(2000);
}
