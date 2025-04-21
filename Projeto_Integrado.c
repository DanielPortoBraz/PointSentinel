// ------Bibliotecas------
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/i2c.h"

#include "ws2818b.pio.h" 
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "animacoes.h"


// Macro para calcular o módulo de um valor. 
// Recebe um valor e retorna ele mesmo caso seja maior ou igual a 0, senão, retorna o oposto do valor
#define mod(x) ((x) >= 0 ? (x): -(x))

// -----Pinos e valores padõres------

// Valores de coordenadas
static uint8_t last_x = 0;
static uint8_t last_y = 0;
static uint8_t point_x = 0; // Eixo x do Ponto Restrito
static uint8_t point_y = 0; // Eixo y do Ponto Restrito
static uint8_t proximity;

// Buzzer
// Buzzer 1
const uint8_t BUZZER_1_PIN = 21;
const uint16_t PERIOD = 17750; // WRAP
const float DIVCLK = 16.0; // Divisor inteiro
static uint slice_21;
const uint16_t dc_values[] = {PERIOD * 0.3, 0}; // Duty Cycle de 30% e 0%

// Buzzer 2
const uint8_t BUZZER_2_PIN = 10;
static uint slice_10;


// Joystick
const uint8_t VRx = 27;
const uint8_t VRy = 26;
const uint8_t ADC_CHAN_0 = 0;
const uint8_t ADC_CHAN_1 = 1; 
static uint16_t vrx_value;
static uint16_t vry_value;
static uint16_t half_adc = 2048;

// Matriz de LEDs 5x5
#define LED_COUNT 25
#define LED_PIN 7
struct pixel_t { // Estrutura de cada LED GRB (Ordem do ws2818b)
    uint8_t G, R, B;
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; 
npLED_t leds[LED_COUNT]; // Buffer de pixels da matriz
PIO np_pio; 
uint sm;

// Display OLED 1306
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
ssd1306_t ssd; // Inicializa a estrutura do display

// Botões
#define BUTTON_A_PIN 5 // Envia o alerta de proximidade
#define BUTTON_B_PIN 6 // Seleciona o Ponto Restrito
static volatile bool press_mark = false; // Permite a marcação do Ponto Restrito
static volatile bool lim_button = false; // Garante um limite de vezes que o botão deve ser lido
static volatile bool enable_alert = 1; // Ativa o alerta do Buzzer 10

static volatile uint32_t last_time = 0; 

// LED RGB
const uint8_t leds_pins[] = {13, 11, 12};



// ----------- FUNÇÕES DE CONFIGURAÇÂO -------------

// ------ Funções do PWM ------

void setup_pwm(){

    // PWM do BUZZER 1
    // Configura para soar 440 Hz
    gpio_set_function(BUZZER_1_PIN, GPIO_FUNC_PWM);
    slice_21 = pwm_gpio_to_slice_num(BUZZER_1_PIN);
    pwm_set_clkdiv(slice_21, DIVCLK);
    pwm_set_wrap(slice_21, PERIOD);
    pwm_set_gpio_level(BUZZER_1_PIN, 0);
    pwm_set_enabled(slice_21, true);

    // PWM do BUZZER 2
    // Configura para soar 440 Hz também
    gpio_set_function(BUZZER_2_PIN, GPIO_FUNC_PWM);
    slice_10 = pwm_gpio_to_slice_num(BUZZER_2_PIN);
    pwm_set_clkdiv(slice_10, DIVCLK);
    pwm_set_wrap(slice_10, PERIOD);
    pwm_set_gpio_level(BUZZER_2_PIN, 0);
    pwm_set_enabled(slice_10, true);
}


// ------ Funções do Joystick -------

// Inicializa o joystick e os conversores ADC
void init_joystick(){
    adc_init();
    adc_gpio_init(VRx);
    adc_gpio_init(VRy);
}

// Obtém o valor atual das coordenadas do joystick com base na leitura ADC 
void read_joystick(uint16_t *vrx_value, uint16_t *vry_value){
    adc_select_input(ADC_CHAN_1);
    sleep_us(2);
    *vrx_value = adc_read();

    adc_select_input(ADC_CHAN_0);
    sleep_us(2);
    *vry_value = adc_read();
}


// ------Funções da matriz de LEDs ws2818b------

/*
 * Inicializa a máquina PIO para controle da matriz de LEDs.
 */
void npInit(uint pin) {

    // Cria programa PIO.
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;

    // Seleciona uma das máquinas de estado livres
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0) {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true); 
    }

    // Inicia programa na máquina PIO obtida.
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

    // Limpa buffer de pixels.
    for (uint i = 0; i < LED_COUNT; ++i) {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

// Função para converter a posição da matriz para uma posição do vetor.
int getIndex(int x, int y) {
    // Se a linha for par (0, 2, 4), percorremos da esquerda para a direita.
    // Se a linha for ímpar (1, 3), percorremos da direita para a esquerda.
    if (y % 2 == 0) {
        return 24-(y * 5 + x); // Linha par (esquerda para direita).
    } else {
        return 24-(y * 5 + (4 - x)); // Linha ímpar (direita para esquerda).
    }
}


/**
 * Atribui uma cor RGB a um LED.
 */
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    leds[index].R = r; 
    leds[index].G = g; 
    leds[index].B = b; 
}

/**
 * Limpa o buffer de pixels.
 */
void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i)
        npSetLED(i, 0, 0, 0);
}

/**
 * Escreve os dados do buffer nos LEDs.
 */
void npWrite() {
    // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100); // Espera 100us, sinal de RESET do datasheet.
}
  

// ------Funções do display OLED ssd1306------

void initialize_i2c(){
    i2c_init(I2C_PORT, 400 * 1000); // Frequência de transmissão de 400 khz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // GPIO para função de I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // GPIO para função de I2C
    gpio_pull_up(I2C_SDA); 
    gpio_pull_up(I2C_SCL); 
}


// ------Funções dos botões------

void init_buttons(){
    gpio_init(BUTTON_A_PIN);
    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
    gpio_pull_up(BUTTON_B_PIN);
}

// Função callback de interrupção
void gpio_irq_handler(uint gpio, uint32_t events){
    
    // Guarda o tempo em us desde o boot do sistema
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    if(current_time - last_time > 200) { // Efeito de debounce gerado pelo atraso de 200 ms na leitura do botão
        last_time = current_time;

        // Ao apertar o botão A, envia um alerta sonoro para o objeto
        if (!gpio_get(BUTTON_A_PIN)){
            enable_alert = !enable_alert;
            pwm_set_gpio_level(BUZZER_2_PIN, dc_values[enable_alert]);
        }

        else if (!gpio_get(BUTTON_B_PIN)){
            press_mark = true;
        }
    }
}   


// ------Funções do LED RGB------
void init_leds(){
    for (int i = 0; i < 3; i++){
        gpio_init(leds_pins[i]);
        gpio_set_dir(leds_pins[i], GPIO_OUT);
        gpio_put(leds_pins[i], false);
    }
}


// >>>>> FUNÇÔES DE LÓGICA DO PROJETO <<<<<<

// Recebe a coordenada do joystick e converte para a equivalente do display OLED 
void move_square(uint16_t vrx_value, uint16_t vry_value, uint8_t *x, uint8_t *y){
    *x = ((vrx_value * (128 - 8)) / 4095); // Posição x do quadrado no display
    *y = 56 - ((vry_value * (64 - 8)) / 4095); // Posição y do quadrado no display
} 

// Retorna o nível de proximidade do quadrado com relação ao Ponto Restrito
int calculate_proximity(uint16_t vrx_value, uint16_t vry_value, uint8_t point_x, uint8_t point_y){

    // 100%
    if (mod(vrx_value - point_x) <= 16 && mod(vry_value - point_y) <= 8){
        return 5;
    }

    // 80%
    else if(mod(vrx_value - point_x) <= 32 && mod(vry_value - point_y) <= 32){
        return 4;
    }

    // 60%
    else if(mod(vrx_value - point_x) <= 48 && mod(vry_value - point_y) <= 48){
        return 3;
    }

    // 40%
    else if(mod(vrx_value - point_x) <= 64 && mod(vry_value - point_y) <= 64){
        return 2;
    }

    // 20%
    else if (mod(vrx_value - point_x) <= 72 && mod(vry_value - point_y) <= 72){
        return 1;
    }

    // 0%
    else {
        return 0;
    }
}

// Exibe o nível de proximidade na matriz de LEDs
void show_proximity(int level){

    for (int v = 0; v < level; v++){

        for(int tam = 0; tam < LED_COUNT; tam++){
            npSetLED(tam, level_proximity[v][24 - tam][0], level_proximity[v][24 - tam][1], level_proximity[v][24 - tam][2]);
            npWrite();
        }
        sleep_ms(10);
    }
}


// ======== Programa principal =========
int main()
{
    stdio_init_all(); // Inicialização da entrada/saída de dados

    // Inicializações
    initialize_i2c();
    npInit(LED_PIN);
    setup_pwm();
    init_joystick();
    init_buttons();
    init_leds();

    // Configuração do Display OLED 1306
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Limpa a matriz de LEDs
    npClear();
    npWrite();

    // Interrupção do Botão A
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    // Interrupção do Botão B
    gpio_set_irq_enabled_with_callback(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    uint16_t vrx_value, vry_value; // Valores ADC dos eixos do joystick
    uint8_t x = 0, y = 0; // Coordenadas do quadrado no display

    while (true) { 
        // Limpa o display
        ssd1306_fill(&ssd, false);
        ssd1306_send_data(&ssd);
        
        // Escreve a mensagem inicial no display
        ssd1306_draw_string(&ssd,"APERTE B", 48, 20);
        ssd1306_draw_string(&ssd,"PARA SELECIONAR O PONTO", 10, 32);
        ssd1306_send_data(&ssd);
        sleep_ms(3000);

        ssd1306_fill(&ssd, false);
        ssd1306_send_data(&ssd);

        // Rotina principal
        while(true){
            ssd1306_fill(&ssd, false);
            ssd1306_send_data(&ssd);
            
            // Guarda a posição anterior do quadrado
            last_x = x;
            last_y = y;
            
            // Faz a leitura dos eixos do joystick para as variáveis x e y
            read_joystick(&vrx_value, &vry_value);

            // Atualiza a posição atual do quadrado no display
            move_square(vrx_value, vry_value, &x, &y);
            ssd1306_square(&ssd, 8, x, y);
            ssd1306_send_data(&ssd);

            // Define os eixos do Ponto Restrito, se o botão B foi apertado pela primeira vez
            if (press_mark && !lim_button){
                point_x = x;
                point_y = y;
                lim_button = true;
            }
            
            // Exibe o Ponto Restrito no display
            if (point_x != 0 && point_y != 0){
                ssd1306_point(&ssd, point_x, point_y); // Marca o Ponto Restrito
                ssd1306_send_data(&ssd);

                if (last_x != x || last_y != y) // Se o quadrado mudou de posição
                    // Atualiza o nível de proximidade na matriz de leds
                    proximity = calculate_proximity(x, y, point_x, point_y);
                    show_proximity(proximity);

                    // Se o quadrado estiver muito próximo, acionará o alerta
                    if (proximity == 5){
                        pwm_set_gpio_level(BUZZER_1_PIN, dc_values[0]);
                        gpio_put(leds_pins[0], true);
                    }

                    else {
                        pwm_set_gpio_level(BUZZER_1_PIN, dc_values[1]);
                        gpio_put(leds_pins[0], false); 
                    }
            }

            // Informações para monitoramento serial
            printf("Coordenadas: Objeto\nx: %d\ny: %d\n\n", x, y);
            printf("Coordenadas: Ponto\nx: %d\ny: %d\n\n", x, y);
            printf("Nível de proximidade: %d %%\n", proximity * 100 / 5);
            printf("Botão: %d\n", press_mark);
            sleep_us(10);
        }

        // Reseta a quantidade de vezes que o botão B foi pressionado
        press_mark = 0;
        npClear();
        npWrite();

        sleep_us(10);
    }
}