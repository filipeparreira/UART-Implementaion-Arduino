// CONFIGURAÇÕES 
#define INTERVALO_CLK 5
#define PIN_TX 40   // Qualquer pino digital
#define PIN_RX 2    // Deve ser um pino com interrupção externa 
                    // Arduino MEGA 2560 - 2, 3, 18, 19, 20, 21

// -- Definindo os timers
#define USE_TIMER_1 true
#if (defined(__AVR_ATmega644__) || defined(__AVR_ATmega644A__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__) ||                   \
     defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_NANO) || defined(ARDUINO_AVR_MINI) || defined(ARDUINO_AVR_ETHERNET) ||                        \
     defined(ARDUINO_AVR_FIO) || defined(ARDUINO_AVR_BT) || defined(ARDUINO_AVR_LILYPAD) || defined(ARDUINO_AVR_PRO) ||                            \
     defined(ARDUINO_AVR_NG) || defined(ARDUINO_AVR_UNO_WIFI_DEV_ED) || defined(ARDUINO_AVR_DUEMILANOVE) || defined(ARDUINO_AVR_FEATHER328P) ||    \
     defined(ARDUINO_AVR_METRO) || defined(ARDUINO_AVR_PROTRINKET5) || defined(ARDUINO_AVR_PROTRINKET3) || defined(ARDUINO_AVR_PROTRINKET5FTDI) || \
     defined(ARDUINO_AVR_PROTRINKET3FTDI))
#define USE_TIMER_2 true
#warning Using Timer1, Timer2
#else
#define USE_TIMER_3 true
#warning Using Timer1, Timer3
#endif

// Bibliotecas
#include "TimerInterrupt.h"
#include "Arduino.h"

// Intervalos para os timers (tempo)
#define TIMER_TX_INTERVALO_MS INTERVALO_CLK
#define TIMER_TX_FREQUENCIA (float)(1000.0f / TIMER_TX_INTERVALO_MS)

#define TIMER_RX_INTERVALO_MS INTERVALO_CLK
#define TIMER_RX_FREQUENCIA (float)(1000.0f / TIMER_RX_INTERVALO_MS)

// Pinos utilizados
unsigned int tx = PIN_TX;
unsigned int rx = PIN_RX;

// Variáveis de controle - TX
enum EstadoTX
{
  ESPERA,
  ENVIANDO,
  PARANDO
};
volatile EstadoTX estadoTX;
volatile bool temMensagem;
volatile int indexChar = 0, indexBit = 0;
String mensagem;
int bit;

// Variáveis de controle - RX
volatile bool flagBitRecebido = false, flagByteRecebido = false;
volatile bool recebendoDados = false;
volatile int indexBitRecebimento = 0;
int bitRecebido, debugBit;
volatile char debugByte = 0;
char byteRecebido;

// Função ligada a borda de descida do pino digital
void receba()
{
  // Desligar a interrupção externa, ligar a interrupção por Timer 
    // e reiniciar as variáveis de controle 
  detachInterrupt(digitalPinToInterrupt(rx));
  recebendoDados = true;
  indexBitRecebimento = 0;
  byteRecebido = 0;

#if USE_TIMER_2
  ITimer2.enableTimer();
  ITimer2.restartTimer();
#elif USE_TIMER_3
  ITimer3.enableTimer();
  ITimer3.restartTimer();
#endif
}

/*
  Classe Comunicação - Contém os métodos para implementar TX/RX 
  
  É implementado somente os métodos receber e enviar, os procedimentos de processamento dos 
  dados são implementados de forma intrínseca em tais métodos.

  receber(): Método executado a cada intervalo de interrupção do Timer2/Timer3 
    Verifica se há dados a serem recebidos, caso não há, o restante do método não é executado 
    1. Lê o bit
    2. Verifica se o bit é o StartBit (0), e o "consome"
    3. Recebe os 8 bits de dados
    4. Verifica o StopBit (1), e então reseta as variáveis, desliga a interrupção por Timer 
      e ativa a interrupção externa para receber o próximo frame

  enviar(): É implementado uma máquina de estados para o envio.
    Seguindo mesmo padrão é utilizado uma variável de controle para verificar se chegou 
      alguma mensagem via Serial, caso não tenha chegado, o restante do método não é executado
    ESPERA: Envia o StartBit e passa para o próximo estado 
    ENVIANDO: Fica neste estado até ter enviado todos os 8 bits (1 byte) que corresponde a um 
      caractere, finalizando, é passado para outro estado 
    PARANDO: Envia StopBit
    
*/  
class Comunicacao
{
public:
  static void receber()
  {
    if (!recebendoDados)
      return;

    int valor = digitalRead(rx);
    debugBit = valor;
    flagBitRecebido = true;

    if (indexBitRecebimento == 0)
    {
      indexBitRecebimento++;
    }
    else if (indexBitRecebimento >= 1 && indexBitRecebimento <= 8)
    {
      bitWrite(byteRecebido, indexBitRecebimento - 1, valor);
      indexBitRecebimento++;
    }
    else if (indexBitRecebimento == 9)
    {
      recebendoDados = false;
#if USE_TIMER_2
      ITimer2.stopTimer();
#elif USE_TIMER_3
      ITimer3.stopTimer();
#endif
      debugByte = byteRecebido;
      flagByteRecebido = true;
      attachInterrupt(digitalPinToInterrupt(rx), receba, FALLING);
    }
  }

  static void enviar()
  {
    if (!temMensagem)
      return;

    switch (estadoTX)
    {
    case ESPERA:
      digitalWrite(tx, 0);
      estadoTX = ENVIANDO;
      break;

    case ENVIANDO:
      bit = bitRead(mensagem[indexChar], indexBit);
      digitalWrite(tx, bit);
      indexBit++;

      if (indexBit >= 8)
      {
        indexBit = 0;
        estadoTX = PARANDO;
      }
      break;

    case PARANDO:
      digitalWrite(tx, HIGH);
      indexChar++;

      if (indexChar >= mensagem.length())
      {
        temMensagem = false;
      }
      estadoTX = ESPERA;
      break;
    }
  }
};

void setup()
{
  // Configuração dos pinos 
  pinMode(tx, OUTPUT);
  digitalWrite(tx, 1);
  pinMode(rx, INPUT_PULLUP);

  // Inicialização Serial
  Serial.begin(9600);

  // Inicialização dos Timers 
#if USE_TIMER_1
  ITimer1.init();
  if (ITimer1.attachInterruptInterval(TIMER_TX_INTERVALO_MS, Comunicacao::enviar))
  {
    Serial.print(F("Timer1 inicializado corretamente.\n"));
  }
  else
  {
    Serial.println(F("Não foi possível inicializar o Timer1. Inicializar com outra frequência.\n"));
  }
#endif

#if USE_TIMER_2

  ITimer2.init();

  if (ITimer2.attachInterruptInterval(TIMER_RX_INTERVALO_MS, Comunicacao::receber))
  {
    Serial.print(F("Timer2 inicializado corretamente.\n"));
    attachInterrupt(digitalPinToInterrupt(rx), receba, FALLING);
    ITimer2.stopTimer();
  }
  else
    Serial.println(F("Não foi possível inicializar o Timer3. Inicializar com outra frequência.\n"));

#elif USE_TIMER_3
  ITimer3.init();
  if (ITimer3.attachInterruptInterval(TIMER_RX_INTERVALO_MS, Comunicacao::receber))
  {
    Serial.print(F("Timer3 inicializado corretamente.\n"));
    attachInterrupt(digitalPinToInterrupt(rx), receba, FALLING);
    ITimer3.stopTimer();
  }
  else
    Serial.println(F("Não foi possível inicializar o Timer3. Inicializar com outra frequência.\n"));
#endif
}

void loop()
{
  // Verifica se foi inserido algum dado na entrada Serial
  if (Serial.available() > 0)
  {
    mensagem = Serial.readStringUntil('\n') + '\n';
    Serial.print("\nEnviando: ");
    Serial.println(mensagem);
    indexChar = 0;
    indexBit = 0;
    estadoTX = ESPERA;
    temMensagem = true;
  }
  
  if (flagBitRecebido)
  {
    flagBitRecebido = false;
  }

  // Impressão do dado recebido
  if (flagByteRecebido)
  {
    if (debugByte == -1)
      return;
    flagByteRecebido = false;
    Serial.print(debugByte);
  }
};
