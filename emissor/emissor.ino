// Emissor de Quadros de Caracter, Codificados em NRZ, Delimitados por Bit de Início e Fim;
// que faz Controle de Erro por Bit de Paridade e Controle de Fluxo por Handshake.

#define PINO_TX 13 // pino de transmissão de dados
#define PINO_RTS 12 // pino de requisição de início e sinalização de fim de transmissão
#define PINO_CTS 11 // pino de permissão de início de transmissão e sinalização de fim de recepção
#define BAUD_RATE 223 // 223 interrupções por segundo, transmitindo até aproximadamente 20 caracteres por segundo

#include "Temporizador.h"

typedef enum PARIDADE
{
  impar,
  par
} Paridade;

// Sinaliza se a paridade da quantidade de bits 1's do dado é
// correspondente a paridade pré-estabelicida para transmissão.
Paridade paridade(char dado, Paridade p)
{
  Paridade x = p; // inicia bandeira
  // se x é iniciado como true, configura sinalização para paridade par,
  // se é iniciado como false, configura sinalização para paridade ímpar
  for (unsigned int i = 0; i < 8; i++)
  { // percorre dado ascendentemente
    if ((dado >> i) & 1)
    { // se bit na posição i for 1
      if (x)
      { // se bandeira atual é par
        x = impar; // passa a ser ímpar
      } else
      { // senão, se ímpar
        x = par; // passa a ser par
      }
    }
  }
  return x; // retorna bandeira
}

void emite_bit_de_paridade(char dado, Paridade p)
{
  if (paridade(dado, p))
  { // se dado tiver paridade p
    digitalWrite(PINO_TX, LOW); // seta baixa tensão como bit de paridade 0
    Serial.println("bp: 0;");
  } else
  { // senão
    digitalWrite(PINO_TX, HIGH); // seta alta tensão como bit de paridade 1
    Serial.println("bp: 1;");
  }
  Serial.print("dado: ");
}

void emite_bit_de_dado(char dado, unsigned int i)
{
  if ((dado >> i) & 1)
  { // se bit do dado referente a posição i for 1
    digitalWrite(PINO_TX, HIGH); // seta alta tensão
    Serial.print("1");
  } else
  { // senão
    digitalWrite(PINO_TX, LOW); // seta baixa tensão
    Serial.print("0");
  }
}

String mensagem; // vetor global de dados a serem emitidos nas interrupções
unsigned int iq; // variável global de índice de quadro de caracter de mensagem
unsigned int ib; // variável global de índice de bit de quadro de caracter de mensagem

// Rotina de interrupcao do timer1
// O que fazer toda vez que 1/BAUD_RATE s passou?
// Emite um bit de quadro setando a tensão referente ao valor do bit.
// Ao terminar de enviar um quadro, sinaliza para receptor pelo RTS.
ISR(TIMER1_COMPA_vect)
{
  if (ib == 0)
  { // se ib corrente é referente a um bit de início de quadro
    digitalWrite(PINO_TX, LOW); // seta baixa tensão como bit 0
    ib++; // avança referência para o próximo bit de quadro
  } else if (ib == 1)
  { // se ib corrente é referente ao bit de índice de paridade do dado
    emite_bit_de_paridade(mensagem.charAt(iq), impar); // seta tensão para paridade
    ib++; // avança referência para o próximo bit de quadro
  } else if (ib < 10)
  { // se ib corrente é referente a um índice de bit de dado
    emite_bit_de_dado(mensagem.charAt(iq), ib - 2); // seta tensão para bit de dado
    ib++; // avança referência para o próximo bit de quadro
  } else if (ib == 10)
  { // se ib corrente é referente a um bit de fim de quadro
    digitalWrite(PINO_TX, LOW); // emite baixa tensão como bit 0
    Serial.println('.');
    // Serial.println();
    ib++; // avança referência para uma localização inválida
  } else if (ib == 11)
  { // se já terminou de transmitir quadro
    digitalWrite(PINO_RTS, LOW); // sinaliza fim de transmissão para receptor
    ib++; // incrementa para não setar RTS novamente
  } else
  { // senão,
    // não faz nada, está esperando o receptor sinalizar que terminou de receber
  }
}

// Executada uma vez quando o Arduino reseta
void setup()
{
  //desabilita interrupcoes
  noInterrupts();
  // Configura porta serial (Serial Monitor - Ctrl + Shift + M)
  Serial.begin(9600); // 9600 bps
  // Inicializa configurações de pinos TX, RTS e CTS
  pinMode(PINO_TX, OUTPUT); // saída de transmissão de dados
  pinMode(PINO_RTS, OUTPUT); // saída de sinalização de estado de transmissão
  pinMode(PINO_CTS, INPUT); // entrada de sinalização de estado de recepção
  // Configura timer
  configuraTemporizador(BAUD_RATE); // 223 interrupções por segundo
  // habilita interrupcoes
  interrupts();
}

void transmite_quadro()
{
  digitalWrite(PINO_RTS, HIGH); // seta RTS
  bool RTS = true; // sinaliza que fez requisição
  while (RTS)
  { // enquanto passar corrente pela saída RTS
    if (digitalRead(PINO_CTS) == HIGH)
    { // se receptor setar CTS em alta tensão
      iniciaTemporizador();
      while (digitalRead(PINO_CTS) == HIGH)
      { // espera receptor terminar de receber
      }
      paraTemporizador();
      RTS = false; // sinaliza fim de transmissão
    }
  }
}

void transmite_mensagem()
{
  for (iq = 0; iq < mensagem.length(); iq++)
  { // para cada caracter
    Serial.print(iq + 1);
    Serial.print("º quadro, caracter '");
    Serial.print(mensagem.charAt(iq));
    Serial.println("':");
    ib = 0; // inicia índice de referência de bit de quadro
    transmite_quadro(); // transmite quadro
  }
}

// O loop() eh executado continuamente (como um while(true))
void loop ( )
{
  if (Serial.available() > 0)
  { // se há dados disponíveis no buffer
    mensagem = Serial.readString(); // lê string do buffer
    // mensagem.trim(); // remove espaços em branco de início e fim
    transmite_mensagem(); // transmite mensagem
    Serial.flush(); // limpa o buffer
  }
}
