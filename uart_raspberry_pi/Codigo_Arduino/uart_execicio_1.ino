/*
  UnB - Universidade de Brasilia
  FGA
  Exercício 1 - UART - Sistemas Embarcados
  Prof. Renato Sampaio
  
  Data: 08/09/2020

  Codigo do Microcontrolador Arduino para estabelecer comunicação serial com 
  o Raspberry Pi.
  
*/

/* --------------------------------------------------
 *  Configuração das Portas Seriais
 *  
 *  Serial = Comunicação Arduino - PC
 *  Serial1 = Comuinicação Arduino - Raspberry Pi
 ---------------------------------------------------*/
// #define DEBUG
 
void setup() {
  // Configura Pino do LED
  pinMode(13, OUTPUT);
  // Configura Serial - PC
#ifdef DEBUG  
  Serial.begin(9600);
  while (!Serial) {
    ; // Aguarda inicialização da Serial
  }
#endif  
  // Configura Serial - Raspberry Pi
  Serial1.begin(9600);
}

/*------------------------------------------------------
 *  Configuração de Variáveis Globais
 ----------------------------------------------------- */
#define CMD_SOLICITA_INT    0xA1
#define CMD_SOLICITA_FLOAT  0xA2
#define CMD_SOLICITA_STRING 0xA3

#define CMD_ENVIA_INT    0xB1
#define CMD_ENVIA_FLOAT  0xB2
#define CMD_ENVIA_STRING 0xB3

#define CMD_ERROR 0xE1
#define CMD_STRING_OK 0xC1

long int dado_inteiro = 41987;
float dado_real = 3.141516;
char dado_string[] = "Mensagem de Teste pela UART";

char dado_string_retorno[] = "String Recebida: ";

long int dado_inteiro_recebido = 0;
float dado_float_recebido = 0.0;
char dado_string_recebido[] = "";

unsigned char dado_recebe;
unsigned char string_ok = 0xC1;

char dados_recebidos[256];
char string_recebida[256];

typedef union {
 float valor_float;
 byte bytes[4];
} bytesFloat;

typedef union {
 long int valor_int;
 byte bytes[4];
} bytesInt;

/*------------------------------------------------------
 *  Funções Auxiliares
 ----------------------------------------------------- */
void le_uart(int num_bytes) {
  int i = 0;
  while(i < num_bytes) {
    if(Serial1.available()) {
      dados_recebidos[i] = Serial1.read();
      Serial.print(dados_recebidos[i]);
      i++;
    }
  }
}

void le_matricula() {

  int matricula = 0;
  le_uart(4);
#ifdef DEBUG  
  Serial.print("Matricula: ");
  for(int i = 0; i < 4; i++) {
    matricula = matricula * 10;
    matricula = matricula + (int) dados_recebidos[i];
  }
  Serial.print(matricula);
  Serial.println(" ");
#endif  
}

long le_inteiro() {
  long num_inteiro = 0;
  le_uart(4);
  memcpy(&num_inteiro, dados_recebidos, 4);
#ifdef DEBUG
  Serial.print("Inteiro recebido: ");
  Serial.println(num_inteiro);
#endif  
  //return valor.valor_int;
  return num_inteiro;
}

float le_float() {
  float num_float;
  le_uart(4);
  memcpy(&num_float, dados_recebidos, 4);
#ifdef DEBUG  
  Serial.print("Float recebido: ");
  Serial.println(num_float);
#endif  
  return num_float;  
}

void le_string() {
  int tamanho;
  le_uart(1);
  tamanho = (int) dados_recebidos[0];
#ifdef DEBUG  
  Serial.print("String recebida [");
  Serial.print(tamanho);
  Serial.print("]: ");
#endif  
  le_uart(tamanho);
  for (int i = 0; i < tamanho; ++i)
  {
    string_recebida[i] = dados_recebidos[i];
#ifdef DEBUG  
    Serial.print(dados_recebidos[i]);
#endif  
  }
  string_recebida[tamanho] = '\0';
  Serial.println("");

}

void envia_int(long int dado) {
  bytesInt envia_dado;
  envia_dado.valor_int = dado;
  Serial1.write(envia_dado.bytes,4);
}

void envia_float(float dado) {
  bytesFloat envia_dado;
  envia_dado.valor_float = dado;
  Serial1.write(envia_dado.bytes,4);
}

void envia_tamanho_da_string(int tamanho) {
  Serial1.write((char) tamanho);
}

void envia_string(char * mensagem, int tamanho) {
  envia_tamanho_da_string(tamanho);
  Serial1.write(mensagem, tamanho);   
}

/*------------------------------------------------------
 *  Loop Principal
 ----------------------------------------------------- */
void loop() {

  if(Serial1.available()) {
    dado_recebe = Serial1.read();
#ifdef DEBUG    
    Serial.print("Comando Recebido: ");
    Serial.println(dado_recebe, HEX);
#endif
    switch (dado_recebe) {
      case CMD_SOLICITA_INT:
        le_matricula();
#ifdef DEBUG        
        Serial.println("Solicitou um INTEIRO");
#endif        
        envia_int(dado_inteiro);
        break;
      case CMD_SOLICITA_FLOAT:
        le_matricula();
#ifdef DEBUG        
        Serial.println("Solicitou um FLOAT");
#endif        
        envia_float(dado_real);
        break;
      case CMD_SOLICITA_STRING:
        le_matricula();
#ifdef DEBUG        
        Serial.print("Solicitou uma STRING: ");
        Serial.println(strlen(dado_string)+1);
#endif        
        envia_string(dado_string, strlen(dado_string)+1);
        break;
      case CMD_ENVIA_INT:
        dado_inteiro_recebido = 0;
        dado_inteiro_recebido = le_inteiro();
        le_matricula();
        envia_int(dado_inteiro_recebido*2);
        break;
      case CMD_ENVIA_FLOAT:
        dado_float_recebido = 0.0;
        dado_float_recebido = le_float();
        le_matricula();
        envia_float(dado_float_recebido*2);
        break;
      case CMD_ENVIA_STRING:
        le_string();
        le_matricula();
        Serial.print(strlen(string_recebida)+strlen(dado_string_retorno)+1);
        envia_tamanho_da_string(strlen(string_recebida)+strlen(dado_string_retorno)+1);
        envia_string(dado_string_retorno, strlen(dado_string_retorno));
        envia_string(string_recebida, strlen(string_recebida)+1);
        break;
      default:
        Serial1.write(CMD_ERROR);
#ifdef DEBUG      
        Serial.print("Dado invalido!!!");
#endif        
        break;
      
    } // End Switch
    
  } // End IF INICIAL
  delay(10);
}
