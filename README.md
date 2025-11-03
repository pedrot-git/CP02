# Sistema de Monitoramento Ambiental - Os Garotos

Sistema embarcado avançado para monitoramento contínuo de condições ambientais, desenvolvido com Arduino UNO. O projeto monitora temperatura, umidade e luminosidade em tempo real, armazena eventos críticos e oferece interface completa com menu navegável.

---

## Circuito

<p align="center">
  <img src="Vinheria_agnello/Circuito_vinheria.png" width="600" alt="Circuito montado no WokWI">
</p>

---

## Funcionalidades Principais

### Monitoramento em Tempo Real
- **Temperatura**: Leitura precisa com sensor DHT22
- **Umidade**: Monitoramento de umidade relativa do ar  
- **Luminosidade**: Medição percentual com calibração personalizável
- **Exibição**: Display LCD com ícones customizados para cada parâmetro

### Sistema de Alertas Inteligente
- **Flags automáticas**: Registro automático quando valores excedem limites
- **Alertas visuais**: LEDs verde (normal) e vermelho (alerta)
- **Alertas sonoros**: Buzzer ativo durante condições críticas
- **Cooldown configurável**: Evita múltiplos registros consecutivos

### Armazenamento de Dados
- **EEPROM**: Armazena até 140 eventos com timestamp
- **Registro temporal**: Data e hora via RTC DS1307
- **Persistência**: Dados mantidos após desligamento

### Interface Completa
- **Menu navegável**: 6 botões para controle total
- **Configurações**: Velocidade texto, unidade temperatura, fuso horário
- **Animação**: Introdução personalizável com cavalo em movimento
- **Descrições**: Texto rolante com explicações detalhadas

---

## Componentes Utilizados

| Componente | Quantidade | Descrição |
|------------|------------|-----------|
| Arduino UNO | 1x | Microcontrolador principal |
| Sensor DHT22 | 1x | Temperatura e umidade |
| Sensor LDR | 1x | Luminosidade ambiente |
| RTC DS1307 | 1x | Relógio em tempo real |
| Display LCD 16x2 I2C | 1x | Interface visual |
| LEDs (Verde/Vermelho) | 2x | Indicadores de status |
| Buzzer | 1x | Alerta sonoro |
| Push Buttons | 6x | Controle de navegação |
| Resistores | Diversos | Circuito de proteção |

---

## Estrutura do Projeto

```
CP-02/
├── vinheria_agnello.ino          # Código principal
├── README.md                    
└── Vinheria_agnello              # Esquemático do circuito
```

---

## Como Usar

### 1. Instalação
```cpp
// Bibliotecas necessárias
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include <RTClib.h>
```

### 2. Configuração Inicial
- Conecte todos os componentes conforme diagrama
- Carregue o código no Arduino
- Na primeira execução, o sistema fará auto-configuração

### 3. Navegação no Menu

| Botão | Função | Ação |
|-------|--------|------|
| ✅ Confirmar | Selecionar | Entra em menus/confirma |
| ▲ Up | Navegação | Move para cima |
| ▼ Down | Navegação | Move para baixo |
| ✗ Cancelar | Voltar | Retorna ao menu anterior |
| ▶ Incrementa | Ajuste rápido | Incrementa valores |
| ◀ Decrementa | Ajuste rápido | Decrementa valores |

### 4. Modos de Operação

#### Modo Display
- Monitoramento contínuo dos sensores
- Exibe valores numéricos e ícones
- Salva flags automaticamente quando necessário

#### Menu Setup
- **Velocidade Texto**: 100-2000ms
- **Unidade Temperatura**: Celsius/Fahrenheit
- **Fuso Horário**: UTC -12 a +12
- **Animação Intro**: Liga/Desliga
- **Cooldown Flags**: 1-60 minutos
- **Calibração LDR**: Configura min/max luminosidade
- **Reset Fábrica**: Restaura configurações padrão

#### Menu Logs
- **Debug Serial**: Exibe status completo
- **Limpar Flags**: Remove todos os registros

---

## Configurações Padrão

| Parâmetro | Valor Padrão | Descrição |
|-----------|--------------|-----------|
| Velocidade Scroll | 800ms | Tempo entre deslocamentos |
| Temperatura | Celsius | Unidade de medida |
| Fuso Horário | UTC-3 | Horário de Brasília |
| Animação | Ativada | Introdução ao ligar |
| Cooldown | 1 minuto | Intervalo entre flags |
| Limite Luz | 70% | Aciona flag acima deste valor |
| Limite Temp | 30°C | Aciona flag acima deste valor |
| Limite Umidade | 80% | Aciona flag acima deste valor |

---

## Lógica do Sistema

### Monitoramento Contínuo
```cpp
// Leitura dos sensores a cada 1.5 segundos
temp = dht.readTemperature();
hum = dht.readHumidity();
luz = map(analogRead(LDR), luzMin, luzMax, 0, 100);
```

### Sistema de Flags
```cpp
// Condições para ativação de flags
if (luz > 70 || temp > 30 || hum > 80) {
  salvarFlagEEPROM();  // Armazena com timestamp
  ativarAlerta();      // Aciona buzzer e LED
}
```

### Interface Gráfica
- Ícones customizados para cada estado
- Animações suaves de transição
- Feedback visual imediato

---

## 📋 Requisitos Técnicos

### Hardware
- Arduino UNO R3
- Sensor DHT22 (precisão 0.5°C)
- Módulo RTC DS1307 com bateria
- Display LCD 16x2 com interface I2C
- Fonte 5V estável

### Software
- Arduino IDE 1.8+
- Bibliotecas: DHT, LiquidCrystal_I2C, RTClib, EEPROM

### Consumo
- **Operação Normal**: ~150mA
- **Com Buzzer**: ~200mA
- **Standby**: ~50mA

---

## Equipe de Desenvolvimento

| Nome | Função |
|------|--------|
| Pedro Sales Ferreira | Desenvolvimento |
| Pedro Henrique Tavares Viana | Desenvolvimento |
| David Ernesto Mogollon Gama | Desenvolvimento |


**Equipe:** Os Garotos

---

## 🔍 Informações Adicionais

### Estrutura de Armazenamento
Cada flag ocupa 7 bytes na EEPROM:
- 4 bytes: Timestamp UNIX
- 1 byte: Luminosidade (0-100%)
- 1 byte: Temperatura (°C)
- 1 byte: Umidade (0-100%)

### Capacidade
- **Flags máximas**: 140 registros
- **EEPROM utilizada**: 20-1000 (980 bytes)
- **Duração estimada**: ~23 horas com cooldown de 1 minuto

### Comunicação
- **Serial**: 9600 baud para debug
- **I2C**: LCD e RTC no barramento
- **Pinos Digitais**: 6 botões + 2 LEDs + buzzer

---

<p align="center"><b>Desenvolvido com excelência pela equipe Os Garotos</b></p>
