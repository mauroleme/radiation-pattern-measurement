int M1dirpin = 7;  // Motor X direction pin
int M1steppin = 6; // Motor X step pin
int M1en = 8;      // Motor X enable pin

const int halfRevolutionSteps = 1600; // Número de micro-passos para 180° com 1/16 de microstepping

void setup()
{
  pinMode(M1dirpin, OUTPUT);
  pinMode(M1steppin, OUTPUT);
  pinMode(M1en, OUTPUT);

  Serial.begin(115200);
  Serial.setTimeout(1000); // Timeout de 1 segundo
  digitalWrite(M1en, LOW); // Habilita o motor (LOW ativa o DRV8825)
}

void loop()
{
  // Verifica se o MATLAB enviou um comando
  if (Serial.available() > 0)
  {
    int command = Serial.parseInt(); // Lê o comando como número inteiro
    if (command == 1)
    {
      move_half_revolution();
      Serial.println(read_sensor()); // Retorna o valor 20
      Serial.println("OK"); 
      delay(100); // Pequeno atraso para dar tempo ao sistema de processar a comunicação
    }
  }
}

void move_half_revolution()
{
  // Gira o motor 180° no sentido horário
  digitalWrite(M1dirpin, LOW);  // Define a direção
  for (int j = 0; j < halfRevolutionSteps; j++)
  {
    digitalWrite(M1steppin, HIGH);
    delayMicroseconds(500);       // Ajuste para controlar a velocidade
    digitalWrite(M1steppin, LOW);
    delayMicroseconds(500);       // Ajuste para controlar a velocidade
  }
  
  delay(1000); // Pausa de 1 segundo

  // Gira o motor 180° no sentido anti-horário
  digitalWrite(M1dirpin, HIGH);  // Muda a direção
  for (int j = 0; j < halfRevolutionSteps; j++)
  {
    digitalWrite(M1steppin, HIGH);
    delayMicroseconds(500);       // Ajuste para controlar a velocidade
    digitalWrite(M1steppin, LOW);
    delayMicroseconds(500);       // Ajuste para controlar a velocidade
  }
}

unsigned read_sensor()
{
  return 20; // Valor fixo para teste
}


