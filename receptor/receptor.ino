// Receptor de Quadros de Caracter, Codificados em NRZ, Delimitados por Bit de Início e Fim;
// que faz Controle de Erro por Bit de Paridade e Controle de Fluxo por Handshake.

#define PINO_RX 13 // pino de recepção de dados
#define PINO_RTS 12 // pino de requisição de início e sinalização de fim de transmissão
#define PINO_CTS 11 // pino de permissão de início de transmissão e sinalização de fim de recepção
#define BAUD_RATE 223 // 223 interrupções por segundo, transmitindo até aproximadamente 20 caracteres por segundo

#include "Temporizador.h"

typedef enum PARIDADE
{
  impar,
  par
} Paridade;

bool bp; // variável global de bit de paridade de dado recebido
byte dado; // variável global de dado de caracter de mensagem
unsigned int ib; // variável global de índice de bit de quadro de caracter
Paridade p; // variável global de paridade a ser calculada de dados recebidos

void recebe_bit_de_paridade()
{
  if(digitalRead(PINO_RX) == HIGH)
  { // se ler alta tensão
    bp = true; // sinaliza que vale 1
    Serial.println("bp: 1;");
  } else
  { // senão, se ler baixa tensão
    bp = false; // sinaliza que vale 0
    Serial.println("bp: 0;");
  }
  Serial.print("dado: ");
}

void recebe_bit_de_dado(unsigned int i)
{
  if(digitalRead(PINO_RX) == HIGH)
  { // se ler alta tensão
    bitSet(dado, i); // seta 1 na posição i
    Serial.print("1");
    if(p)
    { // se paridade corresponde a par
      p = impar; // passa a ser ímpar
    } else
    { // senão, se corresponde a ímpar
      p = par; // passa a ser par
    }
  } else
  { // senão
    Serial.print("0");
  }
}

void imprime_dado()
{
  Serial.println(';');
  if(bp == !p)
  { // se bit de paridade inverso a paridade do dado
    Serial.print("caracter: '");
    Serial.print(char(dado));
    Serial.println("'.");
  } else
  { // senão,
    Serial.println("Erro!"); // imprime mensagem de erro
  }
}

// Rotina de interrupcao do timer1
// O que fazer toda vez que 1/BAUD_RATE s passou?
// Lê tensão que representa um bit de quadro, setando os bits de dado em caracter; ao receber o último bit de dado,
// verifica se paridade do dado recebido está correspondente ao bit de paridade recebido, se sim, imprime caracter.
ISR(TIMER1_COMPA_vect)
{
  if(ib == 0)
  { // se ib corrente é referente a um bit de início de quadro
    digitalRead(PINO_RX); // lê tensão do pino de recepção
    ib++; // avança referência para o próximo bit de quadro
  } else if(ib == 1)
  { // se ib corrente é referente ao bit de índice de paridade do dado
    recebe_bit_de_paridade();   
    ib++; // avança referência para o próximo bit de quadro
  } else if(ib < 10)
  { // se ib corrente é referente a um índice de bit de dado
    recebe_bit_de_dado(ib-2);
    ib++; // avança referência para o próximo bit de quadro
  } else if(ib == 10)
  { // se ib corrente é referente a um bit de fim de quadro
    digitalRead(PINO_RX); // lê tensão do pino de recepção
    imprime_dado();
    ib++; // avança referência para uma localização inválida
  } else
  { // senão,
    // não faz nada, está esperando o emissor sinalizar que terminou de transmitir
  }
}

// Executada uma vez quando o Arduino reseta
void setup()
{
  //desabilita interrupcoes
  noInterrupts();
  // Configura porta serial (Serial Monitor - Ctrl + Shift + M)
  Serial.begin(9600); // 9600 bps
  // Inicializa configurações de pinos RX, RTS e CTS
  pinMode(PINO_RX, INPUT); // entrada de recepção de dados
  pinMode(PINO_RTS, INPUT); // entrada de sinalização de estado de transmissão
  pinMode(PINO_CTS, OUTPUT); // saída de sinalização de estado de recepção
  // Configura timer
  configuraTemporizador(BAUD_RATE); // 1280 interrupções por segundo
  // habilita interrupcoes
  interrupts();
}

// O loop() eh executado continuamente (como um while(true))
void loop ( )
{
  if(digitalRead(PINO_RTS) == HIGH)
  { // se emissor fizer requisição
    dado = 0b0; // inicia dado
    ib = 0; // inicia índice de referência de bit de quadro
    p = impar; // inicia paridade de dado relativa a configuração ímpar
    digitalWrite(PINO_CTS, HIGH); // sinaliza que pode transmitir
    delayMicroseconds(100); // aguarda emissor para sincronizar os relógios
    iniciaTemporizador();
    while(digitalRead(PINO_RTS) == HIGH)
    { // espera terminar de transmitir e receber
    }
    paraTemporizador();
    digitalWrite(PINO_CTS, LOW); // sinaliza que terminou de receber
  }
}
