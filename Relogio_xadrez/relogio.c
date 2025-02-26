// Projeto de um relógio de xadrez com Raspberry Pi Pico para a placa BitDogLab
//
// Autor: Maria Paula Ferraz Cabral de Aguiar

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"

#define BUTTON_A_PIN 5
#define BUTTON_B_PIN 6
#define BUZZER_PIN 21
#define LED_R_PIN 13
#define LED_G_PIN 11
#define LED_B_PIN 12
#define JOYSTICK_BUTTON_PIN 22


const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

#define TEMPO_TOTAL 600 // 10 minutos em segundos

// Variáveis para controle do tempo
volatile int timerA = TEMPO_TOTAL;
volatile int timerB = TEMPO_TOTAL;
volatile bool timerA_correndo = false;
volatile bool timerB_correndo = false;
volatile bool pausado = true;

void inicia_i2c(){
    // Inicialização do i2c
    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

void inicia_buttons(){
    gpio_init(BUTTON_A_PIN);
    gpio_init(BUTTON_B_PIN);
    gpio_init(JOYSTICK_BUTTON_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_set_dir(JOYSTICK_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
    gpio_pull_up(BUTTON_B_PIN);
    gpio_pull_up(JOYSTICK_BUTTON_PIN);
}

void inicia_buzzer(){ // Inicia o buzzer como saída de PWM

    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);

    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);

    // Configurar o PWM com frequência de 392 Hz (nota G4)
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (392 * 4096)); 
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(BUZZER_PIN, 0);
}

void inicia_leds(){
    gpio_init(LED_R_PIN);
    gpio_init(LED_G_PIN);
    gpio_init(LED_B_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);
    gpio_put(LED_R_PIN, 0);
    gpio_put(LED_G_PIN, 0);
    gpio_put(LED_B_PIN, 0);
}

void nome_no_display(){ // Inicia o display e mostra o nome do projeto

    ssd1306_init();

    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    // Limpa o display
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    char *texto[] = {
        "      ",
        "     ",
        "  Chess Clock   ",
        "   ",
        "   ",
        "Por Maria Paula  ",};

    int y = 0;
    for (uint i = 0; i < count_of(texto); i++)
    {
        ssd1306_draw_string(ssd, 5, y, texto[i]);
        y += 8;
    }
    render_on_display(ssd, &frame_area);
}

void dimunui_tempo(){
    if(timerA_correndo == true){
        timerA--;
    }
    if(timerB_correndo == true){
        timerB--;
    }
}

void atualiza_display(){
    ssd1306_init();

    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&frame_area);

    // Limpa o display
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    char textoA[10];
    char textoB[10];
    
    sprintf(textoA, "A: %02d:%02d", timerA / 60, timerA % 60);
    sprintf(textoB, "B: %02d:%02d", timerB / 60, timerB % 60);

    ssd1306_draw_string(ssd, 10, 10, textoA);
    ssd1306_draw_string(ssd, 10, 40, textoB);

    render_on_display(ssd, &frame_area);
}

void verifica_botoes(bool a_apertado, bool b_apertado, bool joystick_apertado){
    if(a_apertado == false){ // Se o botão A foi apertado
        sleep_ms(10); // Tempo de debounce
        if(a_apertado == false){
            pausado = false;
            timerA_correndo = false;
            timerB_correndo = true;
            gpio_put(LED_R_PIN, 0);
            gpio_put(LED_G_PIN, 0);
            gpio_put(LED_B_PIN, 1);
        }
    }
    else if(b_apertado == false){
        sleep_ms(10); // Tempo de debounce
        if(b_apertado == false){
            pausado = false;
            timerA_correndo = true;
            timerB_correndo = false;
            gpio_put(LED_R_PIN, 0);
            gpio_put(LED_G_PIN, 1);
            gpio_put(LED_B_PIN, 0);
        }
    }
    else if(joystick_apertado == false && pausado == false){
        sleep_ms(10); // Tempo de debounce
        if(joystick_apertado == false){
            pausado = true;
            timerA_correndo = false;
            timerB_correndo = false;
            gpio_put(LED_R_PIN, 0);
            gpio_put(LED_G_PIN, 0);
            gpio_put(LED_B_PIN, 0);
        }
    }
    else if(joystick_apertado == false && pausado == true){ // Se o joystick foi apertado com o tempo pausado, o tempo é reiniciado;
        sleep_ms(10); // Tempo de debounce
        if(joystick_apertado == false && pausado == true){ 
            timerA = TEMPO_TOTAL;
            timerB = TEMPO_TOTAL;
            pausado = true;
            timerA_correndo = false; 
            timerB_correndo = false;
        }
    }
}

void rotina_fim_de_tempo(){
    if(timerA == 0 || timerB == 0){
        // Ativar o buzzer para sinalizar o fim do tempo
        pwm_set_gpio_level(BUZZER_PIN, 2048);
        sleep_ms(1000); // O buzzer toca por 1 segundo
        pwm_set_gpio_level(BUZZER_PIN, 0);

        gpio_put(LED_R_PIN, 1);
        gpio_put(LED_G_PIN, 0);
        gpio_put(LED_B_PIN, 0);

        if(timerA == 0){ // Se o timer A acabou, mostra no display que o jogador A perdeu
            ssd1306_init();

            struct render_area frame_area = {
                start_column : 0,
                end_column : ssd1306_width - 1,
                start_page : 0,
                end_page : ssd1306_n_pages - 1
            };

            calculate_render_area_buffer_length(&frame_area);

            // Limpa o display
            uint8_t ssd[ssd1306_buffer_length];
            memset(ssd, 0, ssd1306_buffer_length);
            render_on_display(ssd, &frame_area);

            char *texto[] = {
                "      ",
                " ............ ",
                "    ",
                "    O TEMPO   ",
                "    ACABOU!  ",
                "      ",
                "   A perdeu",};
                

            int y = 0;
                for (uint i = 0; i < count_of(texto); i++)
                {
                    ssd1306_draw_string(ssd, 5, y, texto[i]);
                    y += 8;
                }
                render_on_display(ssd, &frame_area);

            sleep_ms(5000);
        }
        if(timerB == 0){ // Se o timer B acabou, mostra no display que o jogador B perdeu
            ssd1306_init();

            struct render_area frame_area = {
                start_column : 0,
                end_column : ssd1306_width - 1,
                start_page : 0,
                end_page : ssd1306_n_pages - 1
            };

            calculate_render_area_buffer_length(&frame_area);

            // Limpa o display
            uint8_t ssd[ssd1306_buffer_length];
            memset(ssd, 0, ssd1306_buffer_length);
            render_on_display(ssd, &frame_area);

            char *texto[] = {
                "      ",
                " ............ ",
                "    ",
                "    O TEMPO   ",
                "    ACABOU!  ",
                "      ",
                "   B perdeu",};


            int y = 0;
                for (uint i = 0; i < count_of(texto); i++)
                {
                    ssd1306_draw_string(ssd, 5, y, texto[i]);
                    y += 8;
                }
                render_on_display(ssd, &frame_area);

            sleep_ms(5000);
            
        }

        timerA = TEMPO_TOTAL;
        timerB = TEMPO_TOTAL;
        pausado = true;
        timerA_correndo = false; 
        timerB_correndo = false;
        gpio_put(LED_R_PIN, 0); // Desliga o led vermelho
    }
}

int main()
{
    stdio_init_all();
    inicia_i2c();
    inicia_buttons();
    inicia_buzzer();
    inicia_leds();

    nome_no_display(); // Aqui o display é inicializado e o nome é mostrado na tela
    gpio_put(LED_R_PIN, 1); 
    sleep_ms(3000); // Aguarda 3 segundos com o nome no display
    gpio_put(LED_R_PIN, 0);

    while (true) {

        dimunui_tempo();
        atualiza_display();

        bool a_apertado = gpio_get(BUTTON_A_PIN); // 1 solto, 0 apertado
        bool b_apertado = gpio_get(BUTTON_B_PIN); // 1 solto, 0 apertado
        bool joystick_apertado = gpio_get(JOYSTICK_BUTTON_PIN); // 1 solto, 0 apertado

        verifica_botoes(a_apertado, b_apertado, joystick_apertado);
  
        rotina_fim_de_tempo();

        sleep_ms(1000); // Para que a atualização seja feita a cada segundo
    }
}
