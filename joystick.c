#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"

// macros
#define LED_COUNT 25
#define LED_PIN 7
#define DEBOUNCE_DELAY_MS 200

#define LED_R 13
#define LED_G 11
#define LED_B 12

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

#define JOY_X 27
#define JOY_Y 26
#define BTN_JOY 22
#define BTN_ACTION 5

#define DEAD_ZONE 200 // define a zona morta do joystick

// variáveis globais
ssd1306_t ssd; // inicializa a estrutura do display
char received_char = '\0';
char estadoLED[15];
volatile uint32_t last_interrupt_A_time = 0;
volatile uint32_t last_interrupt_B_time = 0;

bool border_state;    // estado da borda no display
bool green_led_state; // estado do LED verde
bool pwm_led_state;   // estado dos LEDs PWM
uint16_t center_x;    // posição central do eixo X do joystick
uint16_t center_y;    // posição central do eixo Y do joystick

// variável para controlar o debounce
absolute_time_t last_press_time;

// protótipos
void init_hardware(void);
static void gpio_irq_handler(uint gpio, uint32_t events);
void atualizarDisplay(uint16_t x, uint16_t y);

void init_hardware(void)
{
  // configura botao A na GPIO 5 com pull-up e interrupção na borda de descida
  gpio_init(5);
  gpio_set_dir(5, GPIO_IN);
  gpio_pull_up(5);

  // configura botão B na GPIO 6 com pull-up e interrupção na borda de descida
  gpio_init(6);
  gpio_set_dir(6, GPIO_IN);
  gpio_pull_up(6);

  gpio_init(LED_R);              // inicializa LED_R como saída
  gpio_set_dir(LED_R, GPIO_OUT); // configura LED_R como saída

  gpio_init(LED_G);              // inicializa LED_G como saída
  gpio_set_dir(LED_G, GPIO_OUT); // configura LED_G como saída

  gpio_init(LED_B);              // inicializa LED_B como saída
  gpio_set_dir(LED_B, GPIO_OUT); // configura LED_B como saída

  // I2C Initialisation. Using it at 400Khz.
  i2c_init(I2C_PORT, 400 * 1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // set the GPIO pin function to I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // set the GPIO pin function to I2C
  gpio_pull_up(I2C_SDA);                                        // pull up the data line
  gpio_pull_up(I2C_SCL);                                        // pull up the clock line
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // inicializa o display
  ssd1306_config(&ssd);                                         // configura o display
  ssd1306_send_data(&ssd);                                      // envia os dados para o display

  // limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  last_press_time = get_absolute_time(); // inicializa o tempo do último botão pressionado

  printf("RP2040 inicializado.\n");
}

/**
 * Função de interrupção para tratar os botões A (GPIO 5) e B (GPIO 6)
 * Implementa debouncing software usando a constante DEBOUNCE_DELAY_MS.
 */
void gpio_irq_handler(uint gpio, uint32_t events)
{
  uint32_t current_time = to_ms_since_boot(get_absolute_time());

  if (gpio == 5)
  {
    if (current_time - last_interrupt_A_time > DEBOUNCE_DELAY_MS)
    {
      last_interrupt_A_time = current_time;

      gpio_put(LED_B, 0);                // desliga o led azul
      gpio_put(LED_G, !gpio_get(LED_G)); // alterna o estado do LED verde

      printf("\nEstado do LED verde alternado\n%s \n", gpio_get(LED_G) ? "LED verde on" : "LED verde off");
    }
  }
}

// inicializa um pino GPIO como saída PWM
void pwm_init_gpio(uint pin)
{
  gpio_set_function(pin, GPIO_FUNC_PWM);   // Configura o pino para função PWM
  uint slice = pwm_gpio_to_slice_num(pin); // Obtém o slice PWM correspondente
  pwm_set_wrap(slice, 4095);               // Define o valor máximo do PWM
  pwm_set_enabled(slice, true);            // Habilita o PWM
}

// realiza a calibração do joystick
void calibrate_joystick()
{
  const int samples = 150;       // número de amostras para calibração
  uint32_t sum_x = 0, sum_y = 0; // variáveis para somar os valores lidos
  for (int i = 0; i < samples; i++)
  {
    adc_select_input(0);
    sum_x += adc_read(); // lê eixo X e soma
    adc_select_input(1);
    sum_y += adc_read(); // lê eixo Y e soma
    sleep_ms(10);
  }
  center_x = sum_x / samples; // calcula média para centro X
  center_y = sum_y / samples; // calcula média para centro Y
}

// ajusta o valor do joystick considerando a zona morta
int16_t adjust_joystick_value(int16_t raw, int16_t center)
{
  int16_t diff = raw - center;               // calcula a diferença em relação ao centro
  return (abs(diff) < DEAD_ZONE) ? 0 : diff; // se dentro da zona morta, retorna 0
}

void atualizarDisplay(uint16_t x, uint16_t y)
{
  ssd1306_fill(&ssd, false); // limpa o display

  int pos_x = ((4095 - x) * 52) / 4095; // calcula posição X do cursor
  int pos_y = (y * 113) / 4095;         // calcula posição Y do cursor

  if (border_state)
  {
    ssd1306_rect(&ssd, 0, 0, 127, 63, 1, false);
    ssd1306_rect(&ssd, 2, 2, 123, 59, 1, false);
  }

  // ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor);       // desenha um retângulo
  ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 10); // desenha uma string
  ssd1306_rect(&ssd, pos_x, pos_y, 8, 8, 1, true);
  ssd1306_send_data(&ssd); // atualiza o display

  sleep_ms(40);
}

int main()
{
  stdio_init_all();
  init_hardware();
  calibrate_joystick();

  gpio_set_irq_enabled_with_callback(5, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

  // loop principal (as ações dos botões são tratadas via IRQ)
  while (true)
  {
    adc_select_input(0);
    uint16_t x_raw = adc_read();
    adc_select_input(1);
    uint16_t y_raw = adc_read();
    printf("Joystick: X=%d, Y=%d\n", x_raw, y_raw);
    if (pwm_led_state)
    {
      int16_t x_adj = adjust_joystick_value(x_raw, center_x);
      int16_t y_adj = adjust_joystick_value(y_raw, center_y);
      pwm_set_gpio_level(LED_R, abs(y_adj) * 2);
      pwm_set_gpio_level(LED_B, abs(x_adj) * 2);
    }

    atualizarDisplay(x_raw, y_raw);
    sleep_ms(100);
  }
}