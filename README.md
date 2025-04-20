
# PointSentinel

## 📌 Descrição

**PointSentinel** é um sistema embarcado desenvolvido com o microcontrolador **RP2040** e a placa **BitDogLab**, cujo objetivo é monitorar a posição de um objeto em movimento próximo a uma região de perigo ou acesso restrito. Através de um joystick, é possível movimentar um ponto (objeto) em um espaço simulado. Caso este objeto se aproxime demais da área restrita previamente definida, alertas visuais e sonoros são emitidos. Este projeto tem fins de revisão, portanto não consiste numa aplicação real, sendo apenas uma simulação.

## Funcionalidades

- **Movimentação do objeto via joystick analógico**
- **Seleção do ponto restrito via botão físico**
- **Cálculo de nível de proximidade entre objeto e ponto de risco**
- **Exibição gráfica da posição no display OLED (SSD1306)**
- **Feedback visual por matriz de LEDs WS2818b 5x5**
- **Alertas sonoros com dois buzzers (proximidade e botão de aviso)**
- **Controle RGB de estado crítico**
- **Sistema de alerta independente via interrupção**

## Componentes utilizados

| Componente         | Função                                                   |
|--------------------|-----------------------------------------------------------|
| **Raspberry Pi Pico** | Microcontrolador principal                               |
| **BitDogLab Board** | Interface de hardware expandida (joystick, botões, etc)  |
| **Display OLED SSD1306** | Exibição da posição do objeto e do ponto restrito       |
| **Joystick Analógico** | Controle do movimento do ponto (objeto monitorado)     |
| **Buzzer 1 (pino 21)** | Alerta automático de proximidade                       |
| **Buzzer 2 (pino 10)** | Alerta acionado manualmente pelo monitor               |
| **Botões A e B** | A: Alerta sonoro / B: Seleção do ponto de risco              |
| **Matriz de LEDs WS2818b (5x5)** | Indicação visual de nível de risco           |
| **LED RGB (pinos 11, 12, 13)** | Indicação luminosa do estado do sistema       |

## Como funciona

1. O sistema inicializa o display, matriz de LEDs, PWM dos buzzers, ADCs do joystick e configura as interrupções dos botões.
2. O **usuário move o objeto** com o joystick, que é representado por um quadrado no display.
3. Ao pressionar o **botão B**, é **marcado o ponto restrito** no display.
4. A **proximidade entre objeto e ponto restrito** é calculada e mapeada para **5 níveis de risco**.
5. A matriz de LEDs exibe o nível de proximidade com cores personalizadas.
6. Ao atingir o nível mais crítico (100% de risco), **um alerta sonoro** é acionado via `BUZZER 1` e o LED vermelho acende.
7. O **botão A** ativa/desativa manualmente um **segundo alerta sonoro** via `BUZZER 2`, **de forma independente** ao loop principal.

## Organização dos arquivos

```
PointSentinel/
├── Projeto_Integrado.c       # Código principal do projeto
├── inc/
│   ├── ssd1306.h             # Biblioteca para controle do display OLED
│   ├── font.h                # Fonte 5x5 utilizada no display
├── ssd1306.c                 # Implementação das funções do display OLED
├── animacoes.h               # Cores animadas para cada nível de proximidade
└── ws2818b.pio               # Programa PIO para controlar a matriz de LEDs
```

## Como compilar

1. Configure o SDK do Raspberry Pi Pico.
2. Crie um projeto CMake com os arquivos `.c` e `.h` organizados como no exemplo acima.
3. Compile com `cmake` e `make`:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```
4. Grave o `.uf2` gerado na memória do Pico via USB.

## Autor

Projeto desenvolvido por Daniel Porto Braz, como parte de um estudo aplicado em **Sistemas Embarcados** com a plataforma Raspberry Pi Pico W.
