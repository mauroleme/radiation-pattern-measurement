#define HallPin A13    // Pino analógico do sensor de efeito Hall
#define M1dirpin 7     // Pino de direção do motor
#define M1steppin 6    // Pino de passo do motor
#define M1en 8         // Pino de habilitação do motor
#define rfPin A15      // Pino analógico conectado ao módulo RF

const int passosPorGrau = 9; // Número de micro-passos por grau
const int numAmostras = 10;  // Número de amostras coletadas por grau

void setup() {
  // Configuração dos pinos de entrada e saída
  pinMode(M1dirpin, OUTPUT);
  pinMode(M1steppin, OUTPUT);
  pinMode(M1en, OUTPUT);
  pinMode(HallPin, INPUT);
  pinMode(rfPin, INPUT);

  digitalWrite(M1en, LOW);     // Ativa o motor
  digitalWrite(M1dirpin, LOW); // Define a direção inicial do motor

  Serial.begin(115200);        // Inicia a comunicação serial
  Serial.println("Arduino pronto para receber comandos...");
}

void loop() {
  // Verifica se há comandos recebidos via comunicação serial
  if (Serial.available() > 0) {
    char comando = Serial.read(); // Lê o comando recebido
    if (comando == '0') {         // Comando para iniciar a medição
      buscarOrigem();             // Localiza o ponto de origem usando o sensor Hall
      realizar_medicoes_360();    // Executa medições para 360 graus
    }
  }
}

void buscarOrigem() {
  Serial.println("Iniciando busca do ponto de origem...");
  int stepsCompleted = 0;         // Contador de passos dados pelo motor
  int detectionStartStep = -1;    // Passo onde começa a detecção do sensor
  int detectionEndStep = -1;      // Passo onde termina a detecção do sensor
  bool detecting = false;         // Flag para indicar se está detectando
  const int limiar = 50;          // Limiar de leitura do sensor Hall
  const int maxSteps = 3200;      // Máximo de passos para uma rotação de 360°

  while (stepsCompleted < maxSteps) {
    int sensorValue = analogRead(HallPin); // Lê o valor do sensor Hall

    // Detecta o início e fim do sinal do sensor
    if (sensorValue < limiar && !detecting) {
      detecting = true;
      detectionStartStep = stepsCompleted;
    } else if (sensorValue >= limiar && detecting) {
      detecting = false;
      detectionEndStep = stepsCompleted;
      break;
    }

    // Executa um passo no motor
    digitalWrite(M1steppin, HIGH);
    delayMicroseconds(10);
    digitalWrite(M1steppin, LOW);
    delayMicroseconds(1000);
    stepsCompleted++;
  }

  // Calcula o ponto central da detecção e retorna para ele
  if (detectionStartStep != -1 && detectionEndStep != -1) {
    int centralStep = (detectionStartStep + detectionEndStep) / 2;
    int stepsToReturn = stepsCompleted - centralStep;
    digitalWrite(M1dirpin, HIGH); // Inverte a direção para retornar

    for (int i = 0; i < stepsToReturn; i++) {
      digitalWrite(M1steppin, HIGH);
      delayMicroseconds(10);
      digitalWrite(M1steppin, LOW);
      delayMicroseconds(1000);
    }

    Serial.println("Ponto de origem encontrado!");
  } else {
    Serial.println("Falha ao encontrar o ponto de origem.");
  }
}

void realizar_medicoes_360() {
  const int numGraus = 180; // Número de graus em cada lado
  int matriz[360][10];      // Matriz para armazenar as medições (360 ângulos x 10 amostras)

  Serial.println("Iniciando medições de 0° a 180°...");
  realizar_medicoes(0, numGraus, matriz, LOW);

  Serial.println("Retornando ao ponto de origem...");
  digitalWrite(M1dirpin, HIGH); // Retorna ao ponto de origem
  for (int passo = 0; passo < numGraus * passosPorGrau; passo++) {
    digitalWrite(M1steppin, HIGH);
    delayMicroseconds(10);
    digitalWrite(M1steppin, LOW);
    delayMicroseconds(1000);
  }

  Serial.println("Iniciando medições de 0° a -180°...");
  realizar_medicoes(numGraus, 2 * numGraus, matriz, HIGH);

  Serial.println("Medições concluídas. Enviando matriz...");
  // Envia os dados coletados via serial
  for (int angulo = 0; angulo < 360; angulo++) {
    for (int amostra = 0; amostra < 10; amostra++) {
      Serial.print(matriz[angulo][amostra]);
      if (amostra < 9) Serial.print(", ");
    }
    Serial.println();
    delay(10); // Atraso para evitar sobrecarga do buffer
  }
  Serial.println("Envio completo.");
}

void realizar_medicoes(int inicio, int fim, int matriz[][10], bool sentidoHorario) {
  digitalWrite(M1dirpin, sentidoHorario ? LOW : HIGH); // Define a direção de rotação do motor
  for (int angulo = inicio; angulo < fim; angulo++) {
    // Move o motor para o próximo ângulo
    for (int passo = 0; passo < passosPorGrau; passo++) {
      digitalWrite(M1steppin, HIGH);
      delayMicroseconds(10);
      digitalWrite(M1steppin, LOW);
      delayMicroseconds(1000);
    }

    // Coleta 10 amostras de RF para o ângulo atual
    for (int amostra = 0; amostra < numAmostras; amostra++) {
      matriz[angulo][amostra] = analogRead(rfPin);
      delay(10); // Pequeno atraso para estabilizar a leitura
    }
  }
}
