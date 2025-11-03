
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include <RTClib.h>


#define BTN_CONFIRMAR 2    
#define BTN_UP 3           
#define BTN_DOWN 4         
#define BTN_CANCELAR 5     
#define BTN_INCREMENTA 6   
#define BTN_DECREMENTA 7   

const uint16_t LIMITE_LUMINOSIDADE = 70;    
const int16_t LIMITE_TEMPERATURA = 30;        
const uint16_t LIMITE_UMIDADE = 80;         

const int velocidade = 20;  
short int menuatual = 0, opcao = 0;
 
#define SETABAIXO 1
#define SETACIMA 2
#define SETAS 0
#define BUZZER 10

#define DHTpin 11
#define DHTmodel DHT22
DHT dht(DHTpin, DHTmodel);

#define LDR A1
#define LED_VERDE 8
#define LED_VERMELHO 9
 
RTC_DS1307 rtc;
 
LiquidCrystal_I2C lcd(0x27, 16, 2);


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

unsigned long anim_tempo_ultimo_quadro = 0;
const int anim_intervalo_principal = 400;
int anim_frame_atual = 0;
int anim_posicao_cavalo = -8;

void anim_executar_inicializacao() {
  lcd.clear();
  anim_posicao_cavalo = -8;
  anim_tempo_ultimo_quadro = millis();
  
  bool animacao_ativa = true;
  
  while (animacao_ativa) {
    if (millis() - anim_tempo_ultimo_quadro >= anim_intervalo_principal) {
      anim_tempo_ultimo_quadro = millis();
      anim_frame_atual = 1 - anim_frame_atual;
      
      
      for (int i = 0; i < 8; i++) {
        uint8_t buffer[8];
        memcpy_P(buffer, anim_cavalo[anim_frame_atual][i], 8);
        lcd.createChar(i, buffer);
      }
      
      lcd.clear();
      
     
      for (int c = 0; c < 4; c++) {
        int xc = anim_posicao_cavalo + c;
        if ((xc >= 0) && (xc < 16)) {
          lcd.setCursor(xc, 0); lcd.write(byte(c));
          lcd.setCursor(xc, 1); lcd.write(byte(c + 4));
        }
      }
      
      
      if (anim_posicao_cavalo >= 4) {
        lcd.setCursor(anim_posicao_cavalo + 4, 0);
        lcd.print("Os");
        lcd.setCursor(anim_posicao_cavalo + 4, 1);
        lcd.print("Garotos");
      }
      
      anim_posicao_cavalo++;
      
      if (anim_posicao_cavalo > 16) {
        animacao_ativa = false;
        lcd.clear();
        lcd.setCursor(4, 0);
        lcd.print("Os Garotos");
        delay(2000);
        lcd.clear();
      }
    }
    
    
    if (lerTecla() == 'D') {
      animacao_ativa = false;
      lcd.clear();
    }
    
    delay(10);
  }
}


#define CFG_INTERVALO_SCROLL_ADDR 0  
#define CFG_UNIDADE_TEMP_ADDR     2 
#define CFG_DISPLAY_ADDR          4  
#define CFG_INTRO_ADDR            6 
#define CFG_FLAGCOOLDOWN_ADDR    14
#define EEPROM_LUZ_MIN_ADDR      16 
#define EEPROM_LUZ_MAX_ADDR      18 
#define ENDERECO_INICIAL_FLAGS   20
int enderecoEEPROM;
 
const uint16_t config_fac[] PROGMEM = {
  800,  
  1,    
  -3,   
  1,    
  1     
};
 
const uint8_t variaveismutaveis = 5; 
 
uint16_t intervaloScroll;
uint16_t unidadeTemperatura;
int16_t display;
uint16_t intro;
uint16_t flagCooldown;

void definevars(void){
  EEPROM.get(CFG_INTERVALO_SCROLL_ADDR, intervaloScroll);
  EEPROM.get(CFG_UNIDADE_TEMP_ADDR, unidadeTemperatura);
  EEPROM.get(CFG_DISPLAY_ADDR, display);
  EEPROM.get(CFG_INTRO_ADDR, intro);
  EEPROM.get(CFG_FLAGCOOLDOWN_ADDR, flagCooldown);
}
 
void restaurarConfiguracoesDeFabrica() {
  for (int i = 0; i < variaveismutaveis; i++) {
    int16_t valor = pgm_read_word_near(config_fac + i);
    EEPROM.put(i * 2, valor);
  }
  limparEEPROMFlags();
  definevars();
  Serial.println("Configurações de fábrica restauradas!");
}
 
void primeirosetup(void){
    if (EEPROM.read(1001) != 1) {
    enderecoEEPROM = ENDERECO_INICIAL_FLAGS;
    EEPROM.put(1010,enderecoEEPROM);
    restaurarConfiguracoesDeFabrica();  
    EEPROM.write(1001, 1); 
  }
}


char lerTecla() {
  static unsigned long ultimaLeitura = 0;
  if (millis() - ultimaLeitura < 50) return 0;
  ultimaLeitura = millis();
  
  if (digitalRead(BTN_CONFIRMAR) == LOW) return 'D';
  if (digitalRead(BTN_UP) == LOW) return 'A';
  if (digitalRead(BTN_DOWN) == LOW) return 'B';
  if (digitalRead(BTN_CANCELAR) == LOW) return 'C';
  if (digitalRead(BTN_INCREMENTA) == LOW) return 'F';
  if (digitalRead(BTN_DECREMENTA) == LOW) return 'E';
  
  return 0;
}


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
    
    if (tecla == 'C') return;
    if (tecla == 'D') {
      EEPROM.put(CFG_INTERVALO_SCROLL_ADDR, intervaloScroll);
      tone(BUZZER, 300, 200);
      return;
    }
    
    if (tecla == 'A') {
      intervaloScroll += 50;
      if (intervaloScroll > 2000) intervaloScroll = 2000;
    }
    if (tecla == 'B') {
      intervaloScroll -= 50;
      if (intervaloScroll < 100) intervaloScroll = 100;
    }
    if (tecla == 'F') {
      intervaloScroll += 200;
      if (intervaloScroll > 2000) intervaloScroll = 2000;
    }
    if (tecla == 'E') {
      intervaloScroll -= 200;
      if (intervaloScroll < 100) intervaloScroll = 100;
    }
    
    delay(150);
  }
}

void ajustarUnidadeTemp() {
  lcd.clear();
  lcd.print("Unidade Temp");
  
  bool selecionado = false;
  
  while (!selecionado) {
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    
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
    if (tecla == 'A' || tecla == 'F') {
      unidadeTemperatura = 2;
    }
    if (tecla == 'B' || tecla == 'E') {
      unidadeTemperatura = 1;
    }
    
    delay(200);
  }
}

void ajustarUTC() {
  lcd.clear();
  lcd.print("Ajustar UTC");
  
  while (true) {
    lcd.setCursor(0, 1);
    lcd.print("     ");
    lcd.setCursor(0, 1);
    
    if (display >= 0) lcd.print("+");
    lcd.print(display);
    
    char tecla = lerTecla();
    
    if (tecla == 'C') return;
    if (tecla == 'D') {
      EEPROM.put(CFG_DISPLAY_ADDR, display);
      tone(BUZZER, 300, 200);
      return;
    }
    
    if (tecla == 'A') {
      display++;
      if (display > 12) display = 12;
    }
    if (tecla == 'B') {
      display--;
      if (display < -12) display = -12;
    }
    if (tecla == 'F') {
      display += 3;
      if (display > 12) display = 12;
    }
    if (tecla == 'E') {
      display -= 3;
      if (display < -12) display = -12;
    }
    
    delay(150);
  }
}

void ajustarCooldown() {
  lcd.clear();
  lcd.print("Cooldown Flags");
  
  while (true) {
    lcd.setCursor(0, 1);
    lcd.print("     ");
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
    
    if (tecla == 'A') {
      flagCooldown++;
      if (flagCooldown > 60) flagCooldown = 60;
    }
    if (tecla == 'B') {
      flagCooldown--;
      if (flagCooldown < 1) flagCooldown = 1;
    }
    if (tecla == 'F') {
      flagCooldown += 5;
      if (flagCooldown > 60) flagCooldown = 60;
    }
    if (tecla == 'E') {
      flagCooldown -= 5;
      if (flagCooldown < 1) flagCooldown = 1;
    }
    
    delay(150);
  }
}

void ajustarIntro() {
  lcd.clear();
  lcd.print("Animacao Intro");
  
  bool selecionado = false;
  
  while (!selecionado) {
    lcd.setCursor(0, 1);
    lcd.print("                ");
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
      intro = 0;
    }
    if (tecla == 'B' || tecla == 'E') {
      intro = 1;
    }
    
    delay(200);
  }
}

void confirmarReset() {
  lcd.clear();
  lcd.print("Reset de Fabrica?");
  
  int opcao = 0;
  
  while (true) {
    lcd.setCursor(0, 1);
    lcd.print("                ");
    
    if (opcao == 0) {
      lcd.print(">Nao   Sim");
    } else {
      lcd.print(" Nao  >Sim");
    }
    
    char tecla = lerTecla();
    
    if (tecla == 'C') return;
    if (tecla == 'D') {
      if (opcao == 1) {
        restaurarConfiguracoesDeFabrica();
        lcd.clear();
        lcd.print("Reset Concluido!");
        delay(2000);
      }
      return;
    }
    if (tecla == 'A' || tecla == 'F') {
      opcao = 1;
    }
    if (tecla == 'B' || tecla == 'E') {
      opcao = 0;
    }
    
    delay(200);
  }
}


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

const char* const texto[] PROGMEM = {
  texto0, texto1, texto2, 
  texto3, texto4, texto5,
  texto6, texto7, texto8, 
  texto9, texto10, texto11,
  texto12, texto13, texto14
};
 
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

const char* const descricoes[] PROGMEM = {
  descricoes0, descricoes1, descricoes2,
  descricoes3, descricoes4, descricoes5,
  descricoes6, descricoes7, descricoes8,
  descricoes9, descricoes10, descricoes11,
  descricoes12, descricoes13, descricoes14
};
 
const uint8_t customChars0[] PROGMEM = {0x00};
const uint8_t customChars1[] PROGMEM = {0x10};
const uint8_t customChars2[] PROGMEM = {0x00,0x00,0x1F,0x11,0x0A,0x04,0x00,0x00};      
const uint8_t customChars3[] PROGMEM = {0x00,0x00,0x1F,0x1F,0x0E,0x04,0x00,0x00};     
const uint8_t customChars4[] PROGMEM = {0x00,0x00,0x04,0x0A,0x11,0x1F,0x00,0x00};      
const uint8_t customChars5[] PROGMEM = {0x00,0x00,0x04,0x0E,0x1F,0x1F,0x00,0x00};   

const uint8_t customChars6[] PROGMEM = {0x03,0x06,0x04,0x04,0x04,0x02,0x01,0x03};  
const uint8_t customChars7[] PROGMEM = {0x18,0x0C,0x04,0x04,0x04,0x08,0x10,0x18};  

const uint8_t customChars8[] PROGMEM = {0x03,0x06,0x04,0x07,0x07,0x03,0x01,0x03};  
const uint8_t customChars9[] PROGMEM = {0x18,0x0C,0x04,0x04,0x1C,0x18,0x10,0x18};  

const uint8_t customChars10[] PROGMEM = {0x03,0x07,0x07,0x07,0x07,0x03,0x01,0x03}; 
const uint8_t customChars11[] PROGMEM = {0x18,0x1C,0x1C,0x1C,0x1C,0x18,0x10,0x18}; 

const uint8_t customChars12[] PROGMEM = {0x01,0x02,0x04,0x04,0x08,0x04,0x02,0x03}; 
const uint8_t customChars13[] PROGMEM = {0x10,0x08,0x04,0x04,0x02,0x04,0x08,0x18}; 

const uint8_t customChars14[] PROGMEM = {0x01,0x02,0x04,0x05,0x0B,0x0F,0x07,0x03}; 
const uint8_t customChars15[] PROGMEM = {0x10,0x08,0x1C,0x1C,0x1E,0x1E,0x1C,0x18};

const uint8_t customChars16[] PROGMEM = {0x02,0x05,0x04,0x04,0x02,0x01,0x00,0x00}; 
const uint8_t customChars17[] PROGMEM = {0x00,0x00,0x10,0x08,0x0C,0x12,0x12,0x08};

const uint8_t customChars18[] PROGMEM = {0x02,0x07,0x07,0x07,0x03,0x01,0x00,0x00}; 
const uint8_t customChars19[] PROGMEM = {0x00,0x10,0x10,0x18,0x1C,0x1E,0x1E,0x0C}; 

const uint8_t* const customChars[] PROGMEM = {
  customChars0, customChars1, customChars2,
  customChars3, customChars4, customChars5,
  customChars6, customChars7, customChars8,
  customChars9, customChars10, customChars11,
  customChars12, customChars13, customChars14,
  customChars15, customChars16, customChars17,
  customChars18, customChars19
};


void begins(void){
  dht.begin();
  lcd.init();      
  lcd.backlight();
  
  
  pinMode(BTN_CONFIRMAR, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_CANCELAR, INPUT_PULLUP);
  pinMode(BTN_INCREMENTA, INPUT_PULLUP);
  pinMode(BTN_DECREMENTA, INPUT_PULLUP);
  
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  
  
  if (!rtc.begin()) {
    Serial.println("RTC não encontrado!");
    while (1);
  }
  if (!rtc.isrunning()) {
    Serial.println("RTC não está funcionando!");
    
  }
}
 
void print16(int idxtexto){
  char buffert[17];
  strcpy_P(buffert, (PGM_P)pgm_read_word(&(texto[idxtexto])));
 
  lcd.setCursor(0,0);
  for(int i = 0; i < strlen(buffert); i++){
    lcd.setCursor(i,0);
    lcd.print(buffert[i]);
    delay(velocidade); 
  }
}
 
void printSetas(int modoseta){
  uint8_t buffer[8]; 
 
  memcpy_P(buffer, (uint8_t*)pgm_read_word(&(customChars[2])), 8);
  lcd.createChar(0, buffer);

  memcpy_P(buffer, (uint8_t*)pgm_read_word(&(customChars[4])), 8);
  lcd.createChar(1, buffer);
 
  if(modoseta == SETABAIXO){ 
    lcd.setCursor(15,1);
    lcd.write(byte(0)); 
  } else if(modoseta == SETACIMA){ 
    lcd.setCursor(15,0);
    lcd.write(byte(1)); 
  } else{ 
    lcd.setCursor(15,1);
    lcd.write(byte(0));
    lcd.setCursor(15,0);
    lcd.write(byte(1));
  }
}
 
int descricoesFunc(int idxdescricoes,int modoseta){
  int cursor = 0;
  unsigned long ultimaAtualizacao = 0;
  int letras = 0;
  char textorolante[16];
  char buffer_desc_func[250]; 
  uint8_t buffer_chars_desc_func[8]; 
  strcpy_P(buffer_desc_func, (PGM_P)pgm_read_word(&(descricoes[idxdescricoes])));
  printSetas(modoseta); 
 
  for(int i = 0; i < 15; i++){
    textorolante[i] = buffer_desc_func[letras++];
  }
 
  while (letras < strlen(buffer_desc_func)) {
    char tecla = lerTecla();
   
    if (tecla == 'B' && (modoseta != SETACIMA || modoseta == SETAS) ) {
      memcpy_P(buffer_chars_desc_func, (uint8_t*)pgm_read_word(&(customChars[3])), 8); 
      lcd.createChar(3,buffer_chars_desc_func); 
      lcd.setCursor(15,1);
      lcd.write(byte(3));
      return 2;
    } else if (tecla == 'A' && (modoseta != SETABAIXO || modoseta == SETAS) ) {
      memcpy_P(buffer_chars_desc_func, (uint8_t*)pgm_read_word(&(customChars[5])), 8); 
      lcd.createChar(3,buffer_chars_desc_func); 
      lcd.setCursor(15,0);
      lcd.write(byte(3)); 
      return 3;
    } else if (tecla == 'D'){
      return 4;
    } else if (tecla == 'C')return 5;
 
    if (millis() - ultimaAtualizacao > intervaloScroll) {
      ultimaAtualizacao = millis();
      for (int p = 0; p < 15; p++) {
        lcd.setCursor(p, 1);
        lcd.print(textorolante[p]);
      }
      for (int o = 0; o < 14; o++) {
        textorolante[o] = textorolante[o + 1];
      }
      textorolante[14] = buffer_desc_func[letras++];
      textorolante[15] = '\0';
    }
    delay(10);
  }
  return 1;
}
 
int menus(int i, int b, int filho) {
  print16(i); 
  do {
    opcao = descricoesFunc(i, b);

    if (opcao == 2) { 
      menuatual++;
      break;
    } else if (opcao == 3) { 
      menuatual--;
      break;
    } else if (opcao == 4) { 
      if (filho != 0) { 
        lcd.noBacklight();  
        delay(250);
        lcd.backlight();    
        menuatual = filho; 
      }
      return 4; 
    } else if (opcao == 5) { 
      menuatual = 0; 
      break;
    }
  } while (opcao == 1);
  return 0;
}

void limparEEPROMFlags() {
  for (int i = 20; i <= 1000; i++) {
    EEPROM.update(i, 0xFF);
  }
  EEPROM.put(1010,ENDERECO_INICIAL_FLAGS);
  digitalWrite(LED_VERMELHO, LOW);
  Serial.println("EEPROM de 20 até 1000 limpa com sucesso.");
}
 
void setupLuzMinMax() {
  int soma = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Apague a luz...");
  lcd.setCursor(0, 1);
  lcd.print("Pressione ✓");

  while (lerTecla() != 'D') delay(100);

  soma = 0;
  for (int i = 0; i < 10; i++) {
    soma += analogRead(LDR);
    delay(50);
  }
  int luzMin = soma / 10;
  EEPROM.put(EEPROM_LUZ_MIN_ADDR, luzMin);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ligue a luz...");
  lcd.setCursor(0, 1);
  lcd.print("Pressione ✓");

  while (lerTecla() != 'D') delay(100);

  soma = 0;
  for (int i = 0; i < 10; i++) {
    soma += analogRead(LDR);
    delay(50);
  }
  int luzMax = soma / 10;
  EEPROM.put(EEPROM_LUZ_MAX_ADDR, luzMax);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Valores salvos");
  delay(1500);
  menuatual = 0;
}

void monitoramentoDisplay() {
  uint16_t luzMin, luzMax;
  EEPROM.get(EEPROM_LUZ_MIN_ADDR, luzMin);
  EEPROM.get(EEPROM_LUZ_MAX_ADDR, luzMax);

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

  int leiturasLDR[20];
  int idxLeitura = 0;
  unsigned long timerLdr = millis();
  unsigned long timerPrint = millis();
  unsigned long timerFlag = millis();
  float mediaLuz = 0;
  int8_t luzMapeada = 50;

  while (true) {
    char tecla = lerTecla();
    if (tecla == 'C') {
      noTone(BUZZER); 
      EEPROM.put(1010,enderecoEEPROM);
      lcd.clear();
      menuatual = 0;
      return;
    }

    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    if (isnan(temp) || isnan(hum)) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Erro no sensor");
      delay(2000);
      continue;
    }

    if (millis() - timerLdr >= 50 && idxLeitura < 20) {
      timerLdr = millis();
      leiturasLDR[idxLeitura++] = analogRead(LDR);
    }

    if (idxLeitura == 20) {
      long soma = 0;
      for (int i = 0; i < 20; i++) soma += leiturasLDR[i];
      mediaLuz = soma / 20.0;
      idxLeitura = 0;

      luzMapeada = map(mediaLuz, luzMin, luzMax, 0, 101);
      luzMapeada = constrain(luzMapeada, 0, 100);
    }

    
    bool flagLuminosidade = luzMapeada > LIMITE_LUMINOSIDADE;
    bool flagTemperatura = temp > LIMITE_TEMPERATURA;
    bool flagUmidade = hum > LIMITE_UMIDADE;
    
    bool flagAtiva = flagLuminosidade || flagTemperatura || flagUmidade;
    
    
    if (flagAtiva) {
      tone(BUZZER, 500); 
    } else {
      noTone(BUZZER); 
    }

    if (millis() - timerPrint >= 1500) {
      timerPrint = millis();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(luzMapeada);lcd.print("%   ");
      lcd.setCursor(0,1);lcd.print("L:");
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

      lcd.setCursor(6,0); lcd.print(hum, 1);
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

      float tempDisplay = unidadeTemperatura == 2 ? temp * 1.8 + 32 : temp;
      lcd.setCursor(12,0); lcd.print(tempDisplay, 1);
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

    
    if (flagAtiva && (enderecoEEPROM + 7 <= 1000) && millis() - timerFlag >= (unsigned long)flagCooldown * 60000) {
      timerFlag = millis();
      DateTime now = rtc.now();
      digitalWrite(LED_VERDE, HIGH); 
      
      uint32_t timestamp = now.unixtime() + display * 3600;
      EEPROM.put(enderecoEEPROM, timestamp); enderecoEEPROM += 4;
      EEPROM.put(enderecoEEPROM, (int8_t)luzMapeada); enderecoEEPROM += 1;
      EEPROM.put(enderecoEEPROM, (int8_t)temp); enderecoEEPROM += 1;
      EEPROM.put(enderecoEEPROM, (uint8_t)hum); enderecoEEPROM += 1;

      Serial.print("Flags salvas: ");
      Serial.println((enderecoEEPROM-20)/7);

      lcd.setCursor(0, 1);
      lcd.print(((enderecoEEPROM-20)/7));
      lcd.print(":FLAG SALVO   ");
      delay(2000);
      digitalWrite(LED_VERDE, LOW);
    }
    
    if(enderecoEEPROM >= 980) digitalWrite(LED_VERMELHO, HIGH);
    
    delay(10);
  }
}

void debugEEPROM() {
  Serial.println("===== DEBUG EEPROM =====");
 
  int16_t val;
  Serial.print("Scroll: "); Serial.println(intervaloScroll);
  EEPROM.get(CFG_UNIDADE_TEMP_ADDR, val);
  Serial.print("Unidade Temp: "); Serial.println(val);
  EEPROM.get(CFG_DISPLAY_ADDR, val);
  Serial.print("Display: "); Serial.println(val);
  EEPROM.get(CFG_INTRO_ADDR, val);
  Serial.print("Intro: "); Serial.println(val);

  EEPROM.get(EEPROM_LUZ_MIN_ADDR, val);
  Serial.print("Luz Mínima: "); Serial.println(val);
  EEPROM.get(EEPROM_LUZ_MAX_ADDR, val);
  Serial.print("Luz Máxima: "); Serial.println(val);

  EEPROM.get(CFG_FLAGCOOLDOWN_ADDR, val);
  Serial.print("Flag cooldown: "); Serial.println(val);
  EEPROM.get(1010, val);
  Serial.print("Endereco Eeprom: ");Serial.println(val);

  
  Serial.println("\n===== LEITURA ATUAL DOS SENSORES =====");
  
  
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  
  
  uint16_t luzMin, luzMax;
  EEPROM.get(EEPROM_LUZ_MIN_ADDR, luzMin);
  EEPROM.get(EEPROM_LUZ_MAX_ADDR, luzMax);
  
  int leituraLDR = analogRead(LDR);
  int luzMapeada = 0;
  
  if (luzMin != 0xFFFF && luzMax != 0xFFFF && luzMin != luzMax) {
    luzMapeada = map(leituraLDR, luzMin, luzMax, 0, 100);
    luzMapeada = constrain(luzMapeada, 0, 100);
  }
  
  
  if (isnan(temp) || isnan(hum)) {
    Serial.println("Erro na leitura do sensor DHT22!");
  } else {
    
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


  Serial.println("\n===== LIMITES CONFIGURADOS =====");
  Serial.print("Limite Luminosidade: "); Serial.print(LIMITE_LUMINOSIDADE); Serial.println("%");
  Serial.print("Limite Temperatura: "); Serial.print(LIMITE_TEMPERATURA); Serial.println("°C");
  Serial.print("Limite Umidade: "); Serial.print(LIMITE_UMIDADE); Serial.println("%");
}


void setup() {
  definevars();
  begins(); 
  if(intro) anim_executar_inicializacao();
  Serial.begin(9600); 
  primeirosetup();
}
 
void loop() {
  switch(menuatual){
    case 1: menus(1, SETAS, 99); break;
    case 2: menus(2, SETAS, 4); break;
    case 3: menus(3, SETAS, 12); break;
    case 4: menus(4, SETAS, 0); break;
    case 5: menus(5, SETAS, 100); break;
    case 6: menus(6, SETAS, 101); break;
    case 7: menus(7, SETAS, 102); break;
    case 8: menus(8, SETAS, 103); break;
    case 9: menus(9, SETAS, 104); break;
    case 10: menus(10, SETAS, 105); break;
    case 11: menus(11, SETAS, 106); break;
    case 12: menus(12, SETAS, 0); break;
    case 13: menus(13, SETAS, 98); break;
    case 14: menus(14, SETAS, 97); break;

    case 97: limparEEPROMFlags(); menuatual = 14; break;
    case 98: debugEEPROM(); menuatual = 13; break;
    case 99: EEPROM.get(1010,enderecoEEPROM); monitoramentoDisplay(); break;

    case 100: ajustarVelocidadeTexto(); menuatual = 5; break;
    case 101: ajustarUnidadeTemp(); menuatual = 6; break;
    case 102: ajustarUTC(); menuatual = 7; break;
    case 103: confirmarReset(); menuatual = 8; break;
    case 104: ajustarIntro(); menuatual = 9; break;
    case 105: ajustarCooldown(); menuatual = 10; break;
    case 106: setupLuzMinMax(); menuatual = 11; break;

    default:
      tone(BUZZER,261,1000);
      menuatual = 0;
      menus(0, SETABAIXO, 0);
  }
  delay(10);
}