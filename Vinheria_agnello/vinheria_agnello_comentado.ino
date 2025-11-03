//====================== BIBLIOTECAS ======================
#include <EEPROM.h>           // Para armazenamento permanente de configurações
#include <Wire.h>             // Para comunicação I2C
#include <LiquidCrystal_I2C.h> // Para controle do display LCD via I2C
#include "DHT.h"              // Para sensor de temperatura e umidade
#include <RTClib.h>           // Para relógio em tempo real (RTC)

//====================== DEFINIÇÃO DOS BOTÕES ======================
#define BTN_CONFIRMAR 2    // D2 - Confirmar (✓)
#define BTN_UP 3           // D3 - Up (▲)
#define BTN_DOWN 4         // D4 - Down (▼)
#define BTN_CANCELAR 5     // D5 - Volta/Cancelar (✗)
#define BTN_INCREMENTA 6   // D6 - Incrementa (▶)
#define BTN_DECREMENTA 7   // D7 - Decrementa (◀)

//====================== LIMITES FIXOS PARA AS FLAGS ======================
const uint16_t LIMITE_LUMINOSIDADE = 70;    // Acima de 70% → Flag (alerta)
const int16_t LIMITE_TEMPERATURA = 30;      // Acima de 30°C → Flag (alerta)  
const uint16_t LIMITE_UMIDADE = 80;         // Acima de 80% → Flag (alerta)

const int velocidade = 20;  // Velocidade base para animações de texto
short int menuatual = 0, opcao = 0;  // Variáveis de controle de menu e navegação
 
// Definições para o sistema de setas do menu
#define SETABAIXO 1    // Seta apenas para baixo
#define SETACIMA 2     // Seta apenas para cima  
#define SETAS 0        // Ambas as setas

#define BUZZER 10      // Pino do buzzer para alertas sonoros

//====================== CONFIGURAÇÃO DO SENSOR DHT ======================
#define DHTpin 11       // Pino do sensor DHT22
#define DHTmodel DHT22  // Modelo do sensor
DHT dht(DHTpin, DHTmodel); // Instância do sensor DHT

#define LDR A1          // Pino do sensor de luz (LDR)
#define LED_VERDE 8     // Pino do LED verde (indicador de operação normal)
#define LED_VERMELHO 9  // Pino do LED vermelho (indicador de alerta/erro)
 
//====================== CONFIGURAÇÃO DO RTC ======================
RTC_DS1307 rtc;  // Instância do RTC DS1307
 
//====================== CONFIGURAÇÃO DO LCD ======================
LiquidCrystal_I2C lcd(0x27, 16, 2); // Endereço I2C 0x27, display 16x2

//====================== ANIMAÇÃO DO CAVALO ======================
// Sprites da animação do cavalo armazenados na PROGMEM para economizar RAM
const byte anim_cavalo[2][8][8] PROGMEM = {
  {
    {0x00, 0x00, 0x00, 0x00, 0x03, 0x07, 0x0E, 0x0E},
    {0x00, 0x00, 0x00, 0x00, 0x0F, 0x1F, 0x1F, 0x1F},
    {0x00, 0x00, 0x00, 0x03, 0x07, 0x1F, 0x1F, 0x1F},
    {0x00, 0x00, 0x05, 0x1F, 0x1D, 0x1F, 0x16, 0x06},
    {0x0C, 0x18, 0x10, 0x00, 0x01, 0x01, 0x01, 0x00},
    {0x1F, 0x1F, 0x1E, 0x17, 0x00, 0x00, 0x10, 0x00},
    {0x1F, 0x1F, 0x03, 0x02, 0x14, 0x04, 0x02, 0x00},
    {0x1C, 0x1C, 0x04, 0x04, 0x08, 0x00, 0x00, 0x00}
  },
  {
    {0x00, 0x00, 0x00, 0x07, 0x0F, 0x0E, 0x1C, 0x18},
    {0x00, 0x00, 0x00, 0x0F, 0x1F, 0x1F, 0x1F, 0x1F},
    {0x00, 0x00, 0x01, 0x03, 0x1F, 0x1F, 0x1F, 0x1F},
    {0x14, 0x1C, 0x1A, 0x1E, 0x1F, 0x13, 0x10, 0x10},
    {0x13, 0x13, 0x02, 0x02, 0x04, 0x00, 0x00, 0x00},
    {0x1F, 0x07, 0x0E, 0x06, 0x01, 0x00, 0x00, 0x00},
    {0x0F, 0x03, 0x03, 0x01, 0x01, 0x00, 0x00, 0x00},
    {0x10, 0x18, 0x0C, 0x02, 0x02, 0x11, 0x00, 0x00}
  }
};

// Variáveis de controle da animação
unsigned long anim_tempo_ultimo_quadro = 0;  // Último tempo de atualização
const int anim_intervalo_principal = 400;    // Intervalo entre quadros (ms)
int anim_frame_atual = 0;                    // Frame atual da animação (0 ou 1)
int anim_posicao_cavalo = -8;                // Posição horizontal do cavalo

/**
 * Executa a animação de inicialização do cavalo
 * A animação pode ser pulada pressionando o botão Confirmar
 */
void anim_executar_inicializacao() {
  lcd.clear();
  anim_posicao_cavalo = -8;  // Começa fora da tela à esquerda
  anim_tempo_ultimo_quadro = millis();
  
  bool animacao_ativa = true;
  
  while (animacao_ativa) {
    // Verifica se é hora de atualizar o frame
    if (millis() - anim_tempo_ultimo_quadro >= anim_intervalo_principal) {
      anim_tempo_ultimo_quadro = millis();
      anim_frame_atual = 1 - anim_frame_atual;  // Alterna entre 0 e 1
      
      // Carrega sprites do frame atual da PROGMEM para a RAM
      for (int i = 0; i < 8; i++) {
        uint8_t buffer[8];
        memcpy_P(buffer, anim_cavalo[anim_frame_atual][i], 8);
        lcd.createChar(i, buffer);
      }
      
      lcd.clear();
      
      // Desenha o cavalo na posição atual (4 caracteres de largura)
      for (int c = 0; c < 4; c++) {
        int xc = anim_posicao_cavalo + c;
        if ((xc >= 0) && (xc < 16)) {
          lcd.setCursor(xc, 0); lcd.write(byte(c));      // Parte superior
          lcd.setCursor(xc, 1); lcd.write(byte(c + 4));  // Parte inferior
        }
      }
      
      // Desenha texto "Os Garotos" quando o cavalo chega na posição central
      if (anim_posicao_cavalo >= 4) {
        lcd.setCursor(anim_posicao_cavalo + 4, 0);
        lcd.print("Os");
        lcd.setCursor(anim_posicao_cavalo + 4, 1);
        lcd.print("Garotos");
      }
      
      anim_posicao_cavalo++;  // Move o cavalo para a direita
      
      // Finaliza a animação quando o cavalo sai da tela
      if (anim_posicao_cavalo > 16) {
        animacao_ativa = false;
        lcd.clear();
        lcd.setCursor(4, 0);
        lcd.print("Os Garotos");
        delay(2000);
        lcd.clear();
      }
    }
    
    // Verifica se algum botão foi pressionado para pular a animação
    if (lerTecla() == 'D') {
      animacao_ativa = false;
      lcd.clear();
    }
    
    delay(10);  // Pequeno delay para não sobrecarregar o processador
  }
}

//====================== CONFIGURAÇÃO EEPROM ======================
// Endereços na EEPROM para cada configuração
#define CFG_INTERVALO_SCROLL_ADDR 0   // Velocidade do scroll (2 bytes)
#define CFG_UNIDADE_TEMP_ADDR     2   // Unidade de temperatura (2 bytes)
#define CFG_DISPLAY_ADDR          4   // Offset de fuso horário (2 bytes)  
#define CFG_INTRO_ADDR            6   // Animação de intro (2 bytes)
#define CFG_FLAGCOOLDOWN_ADDR    14   // Cooldown entre flags (2 bytes)
#define EEPROM_LUZ_MIN_ADDR      16   // Valor mínimo do LDR (2 bytes)
#define EEPROM_LUZ_MAX_ADDR      18   // Valor máximo do LDR (2 bytes)
#define ENDERECO_INICIAL_FLAGS   20   // Início do armazenamento de flags

int enderecoEEPROM;  // Ponteiro para próxima posição livre na EEPROM para flags
 
// Configurações de fábrica armazenadas na PROGMEM
const uint16_t config_fac[] PROGMEM = {
  800,  // intervaloScroll (ms)
  1,    // unidadeTemperatura (1=Celsius, 2=Fahrenheit)
  -3,   // display (offset UTC)
  1,    // intro (1=liga, 0=desliga)
  1     // flagCooldown (minutos)
};
 
const uint8_t variaveismutaveis = 5;  // Número de configurações ajustáveis
 
// Variáveis globais para configurações (carregadas da EEPROM)
uint16_t intervaloScroll;      // Velocidade do scroll em ms
uint16_t unidadeTemperatura;   // 1=Celsius, 2=Fahrenheit  
int16_t display;               // Offset UTC para fuso horário
uint16_t intro;                // 1=animacao ativa, 0=desativada
uint16_t flagCooldown;         // Cooldown entre flags em minutos

/**
 * Carrega as configurações da EEPROM para as variáveis globais
 */
void definevars(void){
  EEPROM.get(CFG_INTERVALO_SCROLL_ADDR, intervaloScroll);
  EEPROM.get(CFG_UNIDADE_TEMP_ADDR, unidadeTemperatura);
  EEPROM.get(CFG_DISPLAY_ADDR, display);
  EEPROM.get(CFG_INTRO_ADDR, intro);
  EEPROM.get(CFG_FLAGCOOLDOWN_ADDR, flagCooldown);
}
 
/**
 * Restaura todas as configurações para os valores de fábrica
 * e limpa todas as flags armazenadas
 */
void restaurarConfiguracoesDeFabrica() {
  for (int i = 0; i < variaveismutaveis; i++) {
    int16_t valor = pgm_read_word_near(config_fac + i);
    EEPROM.put(i * 2, valor);
  }
  limparEEPROMFlags();
  definevars();
  Serial.println("Configurações de fábrica restauradas!");
}
 
/**
 * Executa a configuração inicial do sistema na primeira vez que é ligado
 * Verifica se é a primeira execução e configura valores padrão
 */
void primeirosetup(void){
    if (EEPROM.read(1001) != 1) {  // Verifica flag de primeira execução
    enderecoEEPROM = ENDERECO_INICIAL_FLAGS;
    EEPROM.put(1010,enderecoEEPROM);
    restaurarConfiguracoesDeFabrica();  
    EEPROM.write(1001, 1);  // Marca que já foi configurado
  }
}

//====================== SISTEMA DE BOTÕES ======================
/**
 * Lê o estado dos botões com debounce básico
 * Retorna caractere identificando qual botão foi pressionado
 * 'A'=Up, 'B'=Down, 'C'=Cancelar, 'D'=Confirmar, 'E'=Decrementa, 'F'=Incrementa
 */
char lerTecla() {
  static unsigned long ultimaLeitura = 0;
  if (millis() - ultimaLeitura < 50) return 0;  // Debounce de 50ms
  ultimaLeitura = millis();
  
  // Verifica cada botão (LOW = pressionado devido ao PULLUP)
  if (digitalRead(BTN_CONFIRMAR) == LOW) return 'D';
  if (digitalRead(BTN_UP) == LOW) return 'A';
  if (digitalRead(BTN_DOWN) == LOW) return 'B';
  if (digitalRead(BTN_CANCELAR) == LOW) return 'C';
  if (digitalRead(BTN_INCREMENTA) == LOW) return 'F';
  if (digitalRead(BTN_DECREMENTA) == LOW) return 'E';
  
  return 0;  // Nenhum botão pressionado
}

//====================== FUNÇÕES DE AJUSTE COM BOTÕES ======================

/**
 * Ajusta a velocidade de rolagem do texto
 * Permite ajuste fino (50ms) e grosso (200ms)
 */
void ajustarVelocidadeTexto() {
  lcd.clear();
  lcd.print("Velocidade Texto");
  
  while (true) {
    lcd.setCursor(0, 1);
    lcd.print("    ");
    lcd.setCursor(0, 1);
    lcd.print(intervaloScroll);
    lcd.print(" ms");
    
    char tecla = lerTecla();
    
    if (tecla == 'C') return;  // Cancela - volta sem salvar
    if (tecla == 'D') {
      EEPROM.put(CFG_INTERVALO_SCROLL_ADDR, intervaloScroll);  // Salva na EEPROM
      tone(BUZZER, 300, 200);  // Feedback sonoro
      return;
    }
    
    // Ajustes de valor
    if (tecla == 'A') {  // Up - incrementa 50
      intervaloScroll += 50;
      if (intervaloScroll > 2000) intervaloScroll = 2000;
    }
    if (tecla == 'B') {  // Down - decrementa 50
      intervaloScroll -= 50;
      if (intervaloScroll < 100) intervaloScroll = 100;
    }
    if (tecla == 'F') {  // Incrementa - incrementa 200
      intervaloScroll += 200;
      if (intervaloScroll > 2000) intervaloScroll = 2000;
    }
    if (tecla == 'E') {  // Decrementa - decrementa 200
      intervaloScroll -= 200;
      if (intervaloScroll < 100) intervaloScroll = 100;
    }
    
    delay(150);  // Delay para não mudar muito rápido
  }
}

/**
 * Ajusta a unidade de temperatura (Celsius/Fahrenheit)
 * Interface de seleção entre duas opções
 */
void ajustarUnidadeTemp() {
  lcd.clear();
  lcd.print("Unidade Temp");
  
  bool selecionado = false;
  
  while (!selecionado) {
    lcd.setCursor(0, 1);
    lcd.print("                ");  // Limpa linha
    lcd.setCursor(0, 1);
    
    // Exibe menu de seleção com indicador >
    if (unidadeTemperatura == 1) {
      lcd.print(">Celsius  Fahrenheit");
    } else {
      lcd.print(" Celsius >Fahrenheit");
    }
    
    char tecla = lerTecla();
    
    if (tecla == 'C') return;
    if (tecla == 'D') {
      EEPROM.put(CFG_UNIDADE_TEMP_ADDR, unidadeTemperatura);
      tone(BUZZER, 300, 200);
      selecionado = true;
    }
    if (tecla == 'A' || tecla == 'F') {  // Mova para direita
      unidadeTemperatura = 2;
    }
    if (tecla == 'B' || tecla == 'E') {  // Mova para esquerda
      unidadeTemperatura = 1;
    }
    
    delay(200);
  }
}

/**
 * Ajusta o offset UTC (fuso horário)
 * Permite valores de -12 a +12
 */
void ajustarUTC() {
  lcd.clear();
  lcd.print("Ajustar UTC");
  
  while (true) {
    lcd.setCursor(0, 1);
    lcd.print("     ");  // Limpa
    lcd.setCursor(0, 1);
    
    if (display >= 0) lcd.print("+");  // Mostra sinal para positivos
    lcd.print(display);
    
    char tecla = lerTecla();
    
    if (tecla == 'C') return;
    if (tecla == 'D') {
      EEPROM.put(CFG_DISPLAY_ADDR, display);
      tone(BUZZER, 300, 200);
      return;
    }
    
    // Ajustes de valor
    if (tecla == 'A') {  // Incrementa 1
      display++;
      if (display > 12) display = 12;
    }
    if (tecla == 'B') {  // Decrementa 1
      display--;
      if (display < -12) display = -12;
    }
    if (tecla == 'F') {  // Incrementa 3
      display += 3;
      if (display > 12) display = 12;
    }
    if (tecla == 'E') {  // Decrementa 3
      display -= 3;
      if (display < -12) display = -12;
    }
    
    delay(150);
  }
}

/**
 * Ajusta o cooldown entre registros de flags (em minutos)
 * Impede que muitas flags sejam salvas rapidamente
 */
void ajustarCooldown() {
  lcd.clear();
  lcd.print("Cooldown Flags");
  
  while (true) {
    lcd.setCursor(0, 1);
    lcd.print("     ");  // Limpa
    lcd.setCursor(0, 1);
    lcd.print(flagCooldown);
    lcd.print(" min");
    
    char tecla = lerTecla();
    
    if (tecla == 'C') return;
    if (tecla == 'D') {
      EEPROM.put(CFG_FLAGCOOLDOWN_ADDR, flagCooldown);
      tone(BUZZER, 300, 200);
      return;
    }
    
    // Ajustes de valor
    if (tecla == 'A') {  // +1 minuto
      flagCooldown++;
      if (flagCooldown > 60) flagCooldown = 60;
    }
    if (tecla == 'B') {  // -1 minuto
      flagCooldown--;
      if (flagCooldown < 1) flagCooldown = 1;
    }
    if (tecla == 'F') {  // +5 minutos
      flagCooldown += 5;
      if (flagCooldown > 60) flagCooldown = 60;
    }
    if (tecla == 'E') {  // -5 minutos
      flagCooldown -= 5;
      if (flagCooldown < 1) flagCooldown = 1;
    }
    
    delay(150);
  }
}

/**
 * Ativa/desativa a animação de introdução
 */
void ajustarIntro() {
  lcd.clear();
  lcd.print("Animacao Intro");
  
  bool selecionado = false;
  
  while (!selecionado) {
    lcd.setCursor(0, 1);
    lcd.print("                ");  // Limpa linha
    lcd.setCursor(0, 1);
    
    if (intro == 1) {
      lcd.print(">Ligada  Desligada");
    } else {
      lcd.print(" Ligada >Desligada");
    }
    
    char tecla = lerTecla();
    
    if (tecla == 'C') return;
    if (tecla == 'D') {
      EEPROM.put(CFG_INTRO_ADDR, intro);
      tone(BUZZER, 300, 200);
      selecionado = true;
    }
    if (tecla == 'A' || tecla == 'F') {
      intro = 0;  // Desliga
    }
    if (tecla == 'B' || tecla == 'E') {
      intro = 1;  // Liga
    }
    
    delay(200);
  }
}

/**
 * Menu de confirmação para reset de fábrica
 * Requer confirmação explícita do usuário
 */
void confirmarReset() {
  lcd.clear();
  lcd.print("Reset de Fabrica?");
  
  int opcao = 0;  // 0=Não, 1=Sim
  
  while (true) {
    lcd.setCursor(0, 1);
    lcd.print("                ");  // Limpa linha
    
    if (opcao == 0) {
      lcd.print(">Nao   Sim");
    } else {
      lcd.print(" Nao  >Sim");
    }
    
    char tecla = lerTecla();
    
    if (tecla == 'C') return;  // Cancela
    if (tecla == 'D') {
      if (opcao == 1) {
        restaurarConfiguracoesDeFabrica();  // Executa reset
        lcd.clear();
        lcd.print("Reset Concluido!");
        delay(2000);
      }
      return;
    }
    if (tecla == 'A' || tecla == 'F') {
      opcao = 1;  // Seleciona Sim
    }
    if (tecla == 'B' || tecla == 'E') {
      opcao = 0;  // Seleciona Não
    }
    
    delay(200);
  }
}

//====================== TEXTO E DESCRIÇÕES ======================
// Todos os textos armazenados na PROGMEM para economizar RAM

const char texto0[] PROGMEM = "*-----Menu-----*";
const char texto1[] PROGMEM = "1. Display      ";
const char texto2[] PROGMEM = "2. Setup        ";
const char texto3[] PROGMEM = "3. Logs         ";
const char texto4[] PROGMEM = "*----Setup----* ";
const char texto5[] PROGMEM = "1.Veloc.Txt     ";
const char texto6[] PROGMEM = "2.Unidade Temp. ";
const char texto7[] PROGMEM = "3.UTC           ";
const char texto8[] PROGMEM = "4.Reset         ";
const char texto9[] PROGMEM = "5.Intro         ";
const char texto10[] PROGMEM = "6.Cooldown     ";
const char texto11[] PROGMEM = "7.Setup LDR    ";
const char texto12[] PROGMEM = "*----Logs----* ";
const char texto13[] PROGMEM = "1.Print Log    ";
const char texto14[] PROGMEM = "2.Limpar Flag  ";

// Array de ponteiros para todos os textos
const char* const texto[] PROGMEM = {
  texto0, texto1, texto2, 
  texto3, texto4, texto5,
  texto6, texto7, texto8, 
  texto9, texto10, texto11,
  texto12, texto13, texto14
};

// Descrições detalhadas para cada item de menu
const char descricoes0[] PROGMEM = "Navegue: Cima/Baixo |  Confirmar |  Voltar |  Ajustes   ";
const char descricoes1[] PROGMEM = "Entra no modo leitura de dados e mostra na tela   ";
const char descricoes2[] PROGMEM = "Configura os parametros do dispositivo   ";
const char descricoes3[] PROGMEM = "Entra das opcoes de log - output na Serial   ";
const char descricoes4[] PROGMEM = "Modifica as configuracoes do dispositivo aperte ✓ para entrada   ";
const char descricoes5[] PROGMEM = "Altera a velocidade da rolagem do texto   ";
const char descricoes6[] PROGMEM = "Altera a unidade de medida da temperatura - 1.Celsius - 2.Fahrenheit    ";
const char descricoes7[] PROGMEM = "Altera o fuso horario - ";
const char descricoes8[] PROGMEM = "Redefine para as configuracoes de fabrica    ";
const char descricoes9[] PROGMEM = "Liga ou desliga a intro ao ligar   ";
const char descricoes10[] PROGMEM = "Muda o intervalo de tempo(Em minutos) do registro das flags- default 1 minuto  ";
const char descricoes11[] PROGMEM = "Configura a luz minima e maxima do ambiente  ";
const char descricoes12[] PROGMEM = "Sessao dos Logs das Flags - plota no monitor serial as informacoes armazenadas  ";
const char descricoes13[] PROGMEM = "Plota no monitor serial o debug do dispositivo  ";
const char descricoes14[] PROGMEM = "Limpa as Flags da EEPROM - Cabem 140 flags no total  ";

// Array de ponteiros para todas as descrições
const char* const descricoes[] PROGMEM = {
  descricoes0, descricoes1, descricoes2,
  descricoes3, descricoes4, descricoes5,
  descricoes6, descricoes7, descricoes8,
  descricoes9, descricoes10, descricoes11,
  descricoes12, descricoes13, descricoes14
};

// Caracteres customizados para o LCD (ícones e setas)
const uint8_t customChars0[] PROGMEM = {0x00};
const uint8_t customChars1[] PROGMEM = {0x10};
const uint8_t customChars2[] PROGMEM = {0x00,0x00,0x1F,0x11,0x0A,0x04,0x00,0x00};      // Seta para baixo
const uint8_t customChars3[] PROGMEM = {0x00,0x00,0x1F,0x1F,0x0E,0x04,0x00,0x00};     // Seta para baixo pressionada
const uint8_t customChars4[] PROGMEM = {0x00,0x00,0x04,0x0A,0x11,0x1F,0x00,0x00};      // Seta para cima
const uint8_t customChars5[] PROGMEM = {0x00,0x00,0x04,0x0E,0x1F,0x1F,0x00,0x00};     // Seta para cima pressionada

// Ícones de luminosidade (sol) - 3 níveis
const uint8_t customChars6[] PROGMEM = {0x03,0x06,0x04,0x04,0x04,0x02,0x01,0x03};  // Sol fraco - esquerda
const uint8_t customChars7[] PROGMEM = {0x18,0x0C,0x04,0x04,0x04,0x08,0x10,0x18};  // Sol fraco - direita
const uint8_t customChars8[] PROGMEM = {0x03,0x06,0x04,0x07,0x07,0x03,0x01,0x03};  // Sol médio - esquerda
const uint8_t customChars9[] PROGMEM = {0x18,0x0C,0x04,0x04,0x1C,0x18,0x10,0x18};  // Sol médio - direita
const uint8_t customChars10[] PROGMEM = {0x03,0x07,0x07,0x07,0x07,0x03,0x01,0x03}; // Sol forte - esquerda
const uint8_t customChars11[] PROGMEM = {0x18,0x1C,0x1C,0x1C,0x1C,0x18,0x10,0x18}; // Sol forte - direita

// Ícones de umidade (gota) - 2 estados
const uint8_t customChars12[] PROGMEM = {0x01,0x02,0x04,0x04,0x08,0x04,0x02,0x03}; // Gota normal - esquerda
const uint8_t customChars13[] PROGMEM = {0x10,0x08,0x04,0x04,0x02,0x04,0x08,0x18}; // Gota normal - direita
const uint8_t customChars14[] PROGMEM = {0x01,0x02,0x04,0x05,0x0B,0x0F,0x07,0x03}; // Gota alerta - esquerda
const uint8_t customChars15[] PROGMEM = {0x10,0x08,0x1C,0x1C,0x1E,0x1E,0x1C,0x18}; // Gota alerta - direita

// Ícones de temperatura (termômetro) - 2 estados
const uint8_t customChars16[] PROGMEM = {0x02,0x05,0x04,0x04,0x02,0x01,0x00,0x00}; // Termômetro normal - esquerda
const uint8_t customChars17[] PROGMEM = {0x00,0x00,0x10,0x08,0x0C,0x12,0x12,0x08}; // Termômetro normal - direita
const uint8_t customChars18[] PROGMEM = {0x02,0x07,0x07,0x07,0x03,0x01,0x00,0x00}; // Termômetro alerta - esquerda
const uint8_t customChars19[] PROGMEM = {0x00,0x10,0x10,0x18,0x1C,0x1E,0x1E,0x0C}; // Termômetro alerta - direita

// Array de ponteiros para todos os caracteres customizados
const uint8_t* const customChars[] PROGMEM = {
  customChars0, customChars1, customChars2,
  customChars3, customChars4, customChars5,
  customChars6, customChars7, customChars8,
  customChars9, customChars10, customChars11,
  customChars12, customChars13, customChars14,
  customChars15, customChars16, customChars17,
  customChars18, customChars19
};

//====================== FUNÇÕES PRINCIPAIS ======================

/**
 * Inicializa todos os componentes do sistema
 * Sensores, display, botões, LEDs e RTC
 */
void begins(void){
  dht.begin();      // Inicializa sensor DHT
  lcd.init();       // Inicializa display LCD
  lcd.backlight();  // Liga backlight
  
  // Configura os botões como entradas com resistor pull-up interno
  pinMode(BTN_CONFIRMAR, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_CANCELAR, INPUT_PULLUP);
  pinMode(BTN_INCREMENTA, INPUT_PULLUP);
  pinMode(BTN_DECREMENTA, INPUT_PULLUP);
  
  // Configura LEDs e buzzer como saídas
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  
  // Inicializa RTC
  if (!rtc.begin()) {
    Serial.println("RTC não encontrado!");
    while (1);  // Para execução se RTC não funcionar
  }
  if (!rtc.isrunning()) {
    Serial.println("RTC não está funcionando!");
    // Poderia ajustar automaticamente: rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}
 
/**
 * Exibe texto na primeira linha do LCD com efeito de digitação
 * @param idxtexto Índice do texto no array 'texto'
 */
void print16(int idxtexto){
  char buffert[17];  // Buffer para texto (16 chars + null terminator)
  strcpy_P(buffert, (PGM_P)pgm_read_word(&(texto[idxtexto])));
 
  lcd.setCursor(0,0);
  for(int i = 0; i < strlen(buffert); i++){
    lcd.setCursor(i,0);
    lcd.print(buffert[i]);
    delay(velocidade);  // Efeito de digitação
  }
}
 
/**
 * Exibe setas de navegação no canto direito do LCD
 * @param modoseta Tipo de seta a exibir (SETABAIXO, SETACIMA, SETAS)
 */
void printSetas(int modoseta){
  uint8_t buffer[8];  // Buffer para dados do caractere customizado
 
  // Carrega seta para baixo
  memcpy_P(buffer, (uint8_t*)pgm_read_word(&(customChars[2])), 8);
  lcd.createChar(0, buffer);

  // Carrega seta para cima  
  memcpy_P(buffer, (uint8_t*)pgm_read_word(&(customChars[4])), 8);
  lcd.createChar(1, buffer);
 
  if(modoseta == SETABAIXO){ 
    lcd.setCursor(15,1);
    lcd.write(byte(0));  // Seta para baixo na linha inferior
  } else if(modoseta == SETACIMA){ 
    lcd.setCursor(15,0);
    lcd.write(byte(1));  // Seta para cima na linha superior
  } else{ 
    // Ambas setas
    lcd.setCursor(15,1);
    lcd.write(byte(0));  // Seta baixo
    lcd.setCursor(15,0);
    lcd.write(byte(1));  // Seta cima
  }
}
 
/**
 * Exibe descrição rolante na segunda linha do LCD
 * @param idxdescricoes Índice da descrição no array 'descricoes'
 * @param modoseta Tipo de seta a exibir
 * @return Código de ação do usuário (2=baixo, 3=cima, 4=confirmar, 5=cancelar, 1=continua)
 */
int descricoesFunc(int idxdescricoes,int modoseta){
  int cursor = 0;
  unsigned long ultimaAtualizacao = 0;
  int letras = 0;
  char textorolante[16];  // Texto atualmente visível na linha
  char buffer_desc_func[250];  // Buffer para a descrição completa
  uint8_t buffer_chars_desc_func[8];  // Buffer para caracteres customizados
  
  strcpy_P(buffer_desc_func, (PGM_P)pgm_read_word(&(descricoes[idxdescricoes])));
  printSetas(modoseta);  // Exibe setas de navegação
 
  // Preenche texto inicial
  for(int i = 0; i < 15; i++){
    textorolante[i] = buffer_desc_func[letras++];
  }
  textorolante[15] = '\0';
 
  // Loop de rolagem do texto
  while (letras < strlen(buffer_desc_func)) {
    char tecla = lerTecla();
   
    // Verifica ações do usuário
    if (tecla == 'B' && (modoseta != SETACIMA || modoseta == SETAS) ) {  // Botão Down
      memcpy_P(buffer_chars_desc_func, (uint8_t*)pgm_read_word(&(customChars[3])), 8); 
      lcd.createChar(3,buffer_chars_desc_func); 
      lcd.setCursor(15,1);
      lcd.write(byte(3));  // Seta para baixo pressionada
      return 2;  // Navega para baixo
    } else if (tecla == 'A' && (modoseta != SETABAIXO || modoseta == SETAS) ) {  // Botão Up
      memcpy_P(buffer_chars_desc_func, (uint8_t*)pgm_read_word(&(customChars[5])), 8); 
      lcd.createChar(3,buffer_chars_desc_func); 
      lcd.setCursor(15,0);
      lcd.write(byte(3));  // Seta para cima pressionada
      return 3;  // Navega para cima
    } else if (tecla == 'D'){  // Confirmar
      return 4;
    } else if (tecla == 'C') return 5;  // Cancelar
 
    // Atualiza rolagem do texto no intervalo configurado
    if (millis() - ultimaAtualizacao > intervaloScroll) {
      ultimaAtualizacao = millis();
      
      // Exibe texto atual na segunda linha
      for (int p = 0; p < 15; p++) {
        lcd.setCursor(p, 1);
        lcd.print(textorolante[p]);
      }
      
      // Desloca texto para esquerda
      for (int o = 0; o < 14; o++) {
        textorolante[o] = textorolante[o + 1];
      }
      // Adiciona novo caractere no final
      textorolante[14] = buffer_desc_func[letras++];
      textorolante[15] = '\0';
    }
    delay(10);
  }
  return 1;  // Continua sem ação
}
 
/**
 * Gerencia a exibição e navegação dos menus
 * @param i Índice do texto do menu
 * @param b Tipo de setas a exibir
 * @param filho Menu filho para navegação
 * @return Código de ação (4=confirmou, 0=sem ação)
 */
int menus(int i, int b, int filho) {
  print16(i);  // Exibe título do menu
  do {
    // Exibe descrição e aguarda ação do usuário
    opcao = descricoesFunc(i, b);

    if (opcao == 2) {  // Navega para baixo
      menuatual++;
      break;
    } else if (opcao == 3) {  // Navega para cima
      menuatual--;
      break;
    } else if (opcao == 4) {  // Confirmar
      if (filho != 0) {  // Tem menu filho definido
        lcd.noBacklight();  // Feedback visual
        delay(250);
        lcd.backlight();    
        menuatual = filho;  // Navega para menu filho
      }
      return 4; 
    } else if (opcao == 5) {  // Cancelar
      menuatual = 0;  // Volta ao menu principal
      break;
    }
  } while (opcao == 1);  // Continua enquanto não houver ação
  
  return 0;
}

/**
 * Limpa toda a área da EEPROM usada para armazenar flags
 * Reseta o ponteiro para a posição inicial
 */
void limparEEPROMFlags() {
  for (int i = 20; i <= 1000; i++) {
    EEPROM.update(i, 0xFF);  // Preenche com 0xFF (valor de reset)
  }
  EEPROM.put(1010,ENDERECO_INICIAL_FLAGS);  // Reseta ponteiro
  digitalWrite(LED_VERMELHO, LOW);  // Desliga alerta de EEPROM cheia
  Serial.println("EEPROM de 20 até 1000 limpa com sucesso.");
}
 
/**
 * Configura os valores mínimo e máximo do LDR através de calibração
 * Guia o usuário no processo de calibração da luminosidade
 */
void setupLuzMinMax() {
  int soma = 0;

  // Fase 1: Medição da luz mínima (ambiente escuro)
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Apague a luz...");
  lcd.setCursor(0, 1);
  lcd.print("Pressione ✓");

  // Aguarda confirmação do usuário
  while (lerTecla() != 'D') delay(100);

  // Realiza 10 leituras para média estável
  soma = 0;
  for (int i = 0; i < 10; i++) {
    soma += analogRead(LDR);
    delay(50);
  }
  int luzMin = soma / 10;
  EEPROM.put(EEPROM_LUZ_MIN_ADDR, luzMin);

  // Fase 2: Medição da luz máxima (ambiente iluminado)
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ligue a luz...");
  lcd.setCursor(0, 1);
  lcd.print("Pressione ✓");

  // Aguarda confirmação do usuário
  while (lerTecla() != 'D') delay(100);

  // Realiza 10 leituras para média estável
  soma = 0;
  for (int i = 0; i < 10; i++) {
    soma += analogRead(LDR);
    delay(50);
  }
  int luzMax = soma / 10;
  EEPROM.put(EEPROM_LUZ_MAX_ADDR, luzMax);

  // Confirmação da calibração
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Valores salvos");
  delay(1500);
  menuatual = 0;  // Volta ao menu principal
}

/**
 * Modo de monitoramento em tempo real dos sensores
 * Exibe dados no LCD, verifica limites e armazena flags quando necessário
 */
void monitoramentoDisplay() {
  uint16_t luzMin, luzMax;
  EEPROM.get(EEPROM_LUZ_MIN_ADDR, luzMin);
  EEPROM.get(EEPROM_LUZ_MAX_ADDR, luzMax);

  // Verifica se a calibração do LDR é válida
  if (luzMin == 0xFFFF || luzMax == 0xFFFF || luzMin == luzMax) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Config. invalida");
    lcd.setCursor(0, 1);
    lcd.print("Use o Setup");
    delay(2000);
    menuatual = 0;
    return;
  }

  // Variáveis para cálculo de média móvel da luminosidade
  int leiturasLDR[20];
  int idxLeitura = 0;
  unsigned long timerLdr = millis();
  unsigned long timerPrint = millis();
  unsigned long timerFlag = millis();
  float mediaLuz = 0;
  int8_t luzMapeada = 50;

  // Loop principal do modo monitoramento
  while (true) {
    char tecla = lerTecla();
    if (tecla == 'C') {  // Botão Cancelar - sai do modo
      noTone(BUZZER); // Para o buzzer ao sair do modo Display
      EEPROM.put(1010,enderecoEEPROM);  // Salva posição atual da EEPROM
      lcd.clear();
      menuatual = 0;
      return;
    }

    // Leitura dos sensores DHT
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    if (isnan(temp) || isnan(hum)) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Erro no sensor");
      delay(2000);
      continue;
    }

    // Atualiza leituras do LDR para média móvel (a cada 50ms)
    if (millis() - timerLdr >= 50 && idxLeitura < 20) {
      timerLdr = millis();
      leiturasLDR[idxLeitura++] = analogRead(LDR);
    }

    // Calcula média a cada 20 leituras (1 segundo)
    if (idxLeitura == 20) {
      long soma = 0;
      for (int i = 0; i < 20; i++) soma += leiturasLDR[i];
      mediaLuz = soma / 20.0;
      idxLeitura = 0;

      // Mapeia valor do LDR para porcentagem (0-100%)
      luzMapeada = map(mediaLuz, luzMin, luzMax, 0, 101);
      luzMapeada = constrain(luzMapeada, 0, 100);
    }

    // VERIFICA CONDIÇÕES PARA ATIVAÇÃO DAS FLAGS - A CADA LOOP
    bool flagLuminosidade = luzMapeada > LIMITE_LUMINOSIDADE;
    bool flagTemperatura = temp > LIMITE_TEMPERATURA;
    bool flagUmidade = hum > LIMITE_UMIDADE;
    
    bool flagAtiva = flagLuminosidade || flagTemperatura || flagUmidade;
    
    // CONTROLE DO BUZZER - FORA DO BLOCO DE SALVAR FLAG (CORREÇÃO)
    if (flagAtiva) {
      tone(BUZZER, 500); // Toca CONTINUAMENTE enquanto condições persistem
    } else {
      noTone(BUZZER); // Para quando TODAS as condições normalizam
    }

    // Atualiza display a cada 1.5 segundos
    if (millis() - timerPrint >= 1500) {
      timerPrint = millis();
      lcd.clear();
      
      // Linha superior: Valores numéricos
      lcd.setCursor(0, 0);
      lcd.print(luzMapeada);lcd.print("%   ");
      
      lcd.setCursor(6,0); lcd.print(hum, 1);
      
      float tempDisplay = unidadeTemperatura == 2 ? temp * 1.8 + 32 : temp;
      lcd.setCursor(12,0); lcd.print(tempDisplay, 1);
      
      // Linha inferior: Rótulos e ícones
      lcd.setCursor(0,1);lcd.print("L:");
      
      // Ícone de luminosidade (3 níveis)
      uint8_t spriteL[8], spriteR[8];
      if (luzMapeada <= 33) {
        memcpy_P(spriteL, (uint8_t*)pgm_read_word(&(customChars[6])), 8);
        memcpy_P(spriteR, (uint8_t*)pgm_read_word(&(customChars[7])), 8);
      } else if (luzMapeada <= LIMITE_LUMINOSIDADE) {
        memcpy_P(spriteL, (uint8_t*)pgm_read_word(&(customChars[8])), 8);
        memcpy_P(spriteR, (uint8_t*)pgm_read_word(&(customChars[9])), 8);
      } else {
        memcpy_P(spriteL, (uint8_t*)pgm_read_word(&(customChars[10])), 8);
        memcpy_P(spriteR, (uint8_t*)pgm_read_word(&(customChars[11])), 8);
      }
      lcd.createChar(4, spriteL);
      lcd.createChar(5, spriteR);
      lcd.setCursor(2, 1); lcd.write(byte(4));
      lcd.setCursor(3, 1); lcd.write(byte(5));

      // Ícone de umidade (2 estados)
      lcd.setCursor(6,1); lcd.print("H:");
      if (hum <= LIMITE_UMIDADE) {
        memcpy_P(spriteL, (uint8_t*)pgm_read_word(&(customChars[12])), 8);
        memcpy_P(spriteR, (uint8_t*)pgm_read_word(&(customChars[13])), 8);
      } else {
        memcpy_P(spriteL, (uint8_t*)pgm_read_word(&(customChars[14])), 8);
        memcpy_P(spriteR, (uint8_t*)pgm_read_word(&(customChars[15])), 8);
      }
      lcd.createChar(6, spriteL);
      lcd.createChar(7, spriteR);
      lcd.setCursor(8, 1); lcd.write(byte(6));
      lcd.setCursor(9, 1); lcd.write(byte(7));

      // Ícone de temperatura (2 estados)
      lcd.setCursor(12,1); lcd.print("T:");
      if (temp <= LIMITE_TEMPERATURA) {
        memcpy_P(spriteL, (uint8_t*)pgm_read_word(&(customChars[16])), 8);
        memcpy_P(spriteR, (uint8_t*)pgm_read_word(&(customChars[17])), 8);
      } else {
        memcpy_P(spriteL, (uint8_t*)pgm_read_word(&(customChars[18])), 8);
        memcpy_P(spriteR, (uint8_t*)pgm_read_word(&(customChars[19])), 8);
      }
      lcd.createChar(1, spriteL);
      lcd.createChar(2, spriteR);
      lcd.setCursor(14, 1); lcd.write(byte(1));
      lcd.setCursor(15, 1); lcd.write(byte(2));
    }

    // SALVAR FLAG NA EEPROM (separado do controle do buzzer)
    // Verifica condições e cooldown antes de salvar
    if (flagAtiva && (enderecoEEPROM + 7 <= 1000) && millis() - timerFlag >= (unsigned long)flagCooldown * 60000) {
      timerFlag = millis();
      DateTime now = rtc.now();
      digitalWrite(LED_VERDE, HIGH);  // LED verde indica salvamento
      
      // Salva timestamp ajustado com fuso horário (4 bytes)
      uint32_t timestamp = now.unixtime() + display * 3600;
      EEPROM.put(enderecoEEPROM, timestamp); enderecoEEPROM += 4;
      
      // Salva valores dos sensores (3 bytes)
      EEPROM.put(enderecoEEPROM, (int8_t)luzMapeada); enderecoEEPROM += 1;
      EEPROM.put(enderecoEEPROM, (int8_t)temp); enderecoEEPROM += 1;
      EEPROM.put(enderecoEEPROM, (uint8_t)hum); enderecoEEPROM += 1;

      Serial.print("Flags salvas: ");
      Serial.println((enderecoEEPROM-20)/7);  // Número total de flags salvas

      // Feedback visual no LCD
      lcd.setCursor(0, 1);
      lcd.print(((enderecoEEPROM-20)/7));
      lcd.print(":FLAG SALVO   ");
      delay(2000);
      digitalWrite(LED_VERDE, LOW);
    }
    
    // Alerta visual quando EEPROM está quase cheia
    if(enderecoEEPROM >= 980) digitalWrite(LED_VERMELHO, HIGH);
    
    delay(10);
  }
}

/**
 * Exibe informações de debug no monitor serial
 * Mostra configurações atuais, leituras de sensores e estado da EEPROM
 */
void debugEEPROM() {
  Serial.println("===== DEBUG EEPROM =====");
 
  int16_t val;
  
  // Configurações atuais
  Serial.print("Scroll: "); Serial.println(intervaloScroll);
  EEPROM.get(CFG_UNIDADE_TEMP_ADDR, val);
  Serial.print("Unidade Temp: "); Serial.println(val);
  EEPROM.get(CFG_DISPLAY_ADDR, val);
  Serial.print("Display: "); Serial.println(val);
  EEPROM.get(CFG_INTRO_ADDR, val);
  Serial.print("Intro: "); Serial.println(val);

  // Calibração do LDR
  EEPROM.get(EEPROM_LUZ_MIN_ADDR, val);
  Serial.print("Luz Mínima: "); Serial.println(val);
  EEPROM.get(EEPROM_LUZ_MAX_ADDR, val);
  Serial.print("Luz Máxima: "); Serial.println(val);

  // Estado do sistema
  EEPROM.get(CFG_FLAGCOOLDOWN_ADDR, val);
  Serial.print("Flag cooldown: "); Serial.println(val);
  EEPROM.get(1010, val);
  Serial.print("Endereco Eeprom: ");Serial.println(val);

  // ===== SEÇÃO: LEITURA ATUAL DOS SENSORES =====
  Serial.println("\n===== LEITURA ATUAL DOS SENSORES =====");
  
  // Ler valores atuais dos sensores
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  
  // Ler e calcular luminosidade
  uint16_t luzMin, luzMax;
  EEPROM.get(EEPROM_LUZ_MIN_ADDR, luzMin);
  EEPROM.get(EEPROM_LUZ_MAX_ADDR, luzMax);
  
  int leituraLDR = analogRead(LDR);
  int luzMapeada = 0;
  
  if (luzMin != 0xFFFF && luzMax != 0xFFFF && luzMin != luzMax) {
    luzMapeada = map(leituraLDR, luzMin, luzMax, 0, 100);
    luzMapeada = constrain(luzMapeada, 0, 100);
  }
  
  // Verificar se leituras do DHT são válidas
  if (isnan(temp) || isnan(hum)) {
    Serial.println("Erro na leitura do sensor DHT22!");
  } else {
    // Converter temperatura se necessário
    float tempDisplay = temp;
    char unidadeTemp[2] = "C";
    if (unidadeTemperatura == 2) {
      tempDisplay = temp * 1.8 + 32;
      strcpy(unidadeTemp, "F");
    }
    
    Serial.print("Temperatura: ");
    Serial.print(tempDisplay, 1);
    Serial.print(" ");
    Serial.println(unidadeTemp);
    
    Serial.print("Umidade: ");
    Serial.print(hum, 1);
    Serial.println(" %");
  }
  
  Serial.print("Luminosidade: ");
  Serial.print(luzMapeada);
  Serial.println(" %");
  
  Serial.print("Leitura LDR (RAW): ");
  Serial.println(leituraLDR);

  // ===== LIMITES ATUAIS =====
  Serial.println("\n===== LIMITES CONFIGURADOS =====");
  Serial.print("Limite Luminosidade: "); Serial.print(LIMITE_LUMINOSIDADE); Serial.println("%");
  Serial.print("Limite Temperatura: "); Serial.print(LIMITE_TEMPERATURA); Serial.println("°C");
  Serial.print("Limite Umidade: "); Serial.print(LIMITE_UMIDADE); Serial.println("%");
}

//====================== SETUP E LOOP PRINCIPAIS ======================

/**
 * Função setup padrão do Arduino
 * Executada uma vez na inicialização do sistema
 */
void setup() {
  definevars();           // Carrega configurações da EEPROM
  begins();               // Inicializa hardware
  if(intro) anim_executar_inicializacao();  // Executa animação se configurado
  Serial.begin(9600);     // Inicializa comunicação serial
  primeirosetup();        // Configuração inicial (primeira execução)
}
 
/**
 * Função loop principal do Arduino
 * Gerencia a máquina de estados do menu e navegação
 */
void loop() {
  // Máquina de estados para navegação nos menus
  switch(menuatual){
    // Menu principal
    case 1: menus(1, SETAS, 99); break;    // Display → monitoramento
    case 2: menus(2, SETAS, 4); break;     // Setup → menu setup
    case 3: menus(3, SETAS, 12); break;    // Logs → menu logs
    
    // Submenu Setup
    case 4: menus(4, SETAS, 0); break;     // Título setup
    case 5: menus(5, SETAS, 100); break;   // Veloc. texto → ajuste
    case 6: menus(6, SETAS, 101); break;   // Unidade temp → ajuste  
    case 7: menus(7, SETAS, 102); break;   // UTC → ajuste
    case 8: menus(8, SETAS, 103); break;   // Reset → confirmação
    case 9: menus(9, SETAS, 104); break;   // Intro → ajuste
    case 10: menus(10, SETAS, 105); break; // Cooldown → ajuste
    case 11: menus(11, SETAS, 106); break; // Setup LDR → calibração
    
    // Submenu Logs
    case 12: menus(12, SETAS, 0); break;   // Título logs
    case 13: menus(13, SETAS, 98); break;  // Print log → debug
    case 14: menus(14, SETAS, 97); break;  // Limpar flag → execução

    // Ações dos menus
    case 97: limparEEPROMFlags(); menuatual = 14; break;     // Executa limpeza
    case 98: debugEEPROM(); menuatual = 13; break;           // Executa debug
    case 99: EEPROM.get(1010,enderecoEEPROM); monitoramentoDisplay(); break;  // Modo monitoramento

    // Funções de ajuste (retornam ao menu anterior)
    case 100: ajustarVelocidadeTexto(); menuatual = 5; break;
    case 101: ajustarUnidadeTemp(); menuatual = 6; break;
    case 102: ajustarUTC(); menuatual = 7; break;
    case 103: confirmarReset(); menuatual = 8; break;
    case 104: ajustarIntro(); menuatual = 9; break;
    case 105: ajustarCooldown(); menuatual = 10; break;
    case 106: setupLuzMinMax(); menuatual = 11; break;

    // Menu principal (default)
    default:
      tone(BUZZER,261,1000);  // Feedback sonoro
      menuatual = 0;
      menus(0, SETABAIXO, 0);  // Exibe menu principal
  }
  delay(10);  // Pequeno delay para estabilidade
}