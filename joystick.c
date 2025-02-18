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
volatile uint32_t last_interrupt_A_time = 0;

bool border_state;         // estado da borda no display
bool green_led_state;      // estado do LED verde
bool pwm_led_state = true; // estado dos LEDs PWM
uint16_t center_x;         // posição central do eixo X do joystick
uint16_t center_y;         // posição central do eixo Y do joystick

// variável para controlar o debounce
absolute_time_t last_press_time;

// protótipos
void init_hardware(void);
static void gpio_irq_handler(uint gpio, uint32_t events);
void atualizarDisplay(uint16_t x, uint16_t y);
void pwm_init_gpio(uint pin);
void calibrate_joystick();
int16_t adjust_joystick_value(int16_t raw, int16_t center);

void init_hardware(void)
{
  adc_init();           // Inicializa o ADC
  adc_gpio_init(JOY_X); // Configura ADC no pino X do joystick
  adc_gpio_init(JOY_Y); // Configura ADC no pino Y do joystick

  gpio_init(BTN_JOY);             // Inicializa botão do joystick
  gpio_set_dir(BTN_JOY, GPIO_IN); // Define como entrada
  gpio_pull_up(BTN_JOY);          // Habilita pull-up interno

  gpio_init(BTN_ACTION);             // Inicializa botão de ação
  gpio_set_dir(BTN_ACTION, GPIO_IN); // Define como entrada
  gpio_pull_up(BTN_ACTION);          // Habilita pull-up interno

  // Inicialização dos LEDs
  pwm_init_gpio(LED_R);
  pwm_init_gpio(LED_B);
  gpio_init(LED_G);
  gpio_set_dir(LED_G, GPIO_OUT);
  gpio_put(LED_G, false);

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

  if (gpio == BTN_JOY)
  {
    if (current_time - last_interrupt_A_time > DEBOUNCE_DELAY_MS)
    {
      last_interrupt_A_time = current_time;
      green_led_state = !green_led_state; // Alterna o estado do LED verde
      gpio_put(LED_G, green_led_state);   // Atualiza o LED verde
      printf("\nEstado do LED verde alternado\n%s \n", green_led_state ? "LED verde on" : "LED verde off");

      // Alternar a borda do display
      border_state = !border_state;
    }
  }
  else if (gpio == BTN_ACTION)
  {
    if (current_time - last_interrupt_A_time > DEBOUNCE_DELAY_MS)
    {
      last_interrupt_A_time = current_time;
      pwm_led_state = !pwm_led_state; // Alterna o estado do controle PWM dos LEDs
      printf("\nEstado do PWM dos LEDs %s\n", pwm_led_state ? "Ativado" : "Desativado");
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

  // Atualiza borda do display se necessário
  if (border_state)
  {
    ssd1306_rect(&ssd, 0, 0, 127, 63, 1, false);
    ssd1306_rect(&ssd, 2, 2, 123, 59, 1, false);
  }

  // ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor);       // desenha um retângulo
  // ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 10); // desenha uma string

  ssd1306_rect(&ssd, pos_x, pos_y, 8, 8, 1, true);
  ssd1306_send_data(&ssd); // atualiza o display

  sleep_ms(40);
}

int main()
{
  stdio_init_all();
  init_hardware();
  calibrate_joystick();

  gpio_set_irq_enabled_with_callback(BTN_JOY, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
  gpio_set_irq_enabled(BTN_ACTION, GPIO_IRQ_EDGE_FALL, true);

  // loop principal (as ações dos botões são tratadas via IRQ)
  while (true)
  {
    adc_select_input(0);
    uint16_t x_raw = adc_read();
    adc_select_input(1);
    uint16_t y_raw = adc_read();
    printf("Joystick: X=%d, Y=%d\n", x_raw, y_raw);

    // Controle dos LEDs com PWM
    if (pwm_led_state)
    {
      int16_t x_adj = adjust_joystick_value(x_raw, center_x);
      int16_t y_adj = adjust_joystick_value(y_raw, center_y);
      pwm_set_gpio_level(LED_R, abs(y_adj) * 2); // LED vermelho
      pwm_set_gpio_level(LED_B, abs(x_adj) * 2); // LED azul
    }

    atualizarDisplay(x_raw, y_raw);
    sleep_ms(100);
  }
}
