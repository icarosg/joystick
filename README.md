# Projeto BitDogLab - Controle de LEDs, Botões, Display e Joystick

## Descrição do Projeto

Neste projeto, foram utilizados os seguintes componentes conectados à placa **BitDogLab**:

- **LED RGB** – Pinos conectados às **GPIOs 11, 12 e 13**.
- **Botão A** – Conectado à **GPIO 5**.
- **Display SSD1306** – Conectado via **I2C (GPIO 14 e GPIO 15)**.
- **Joystick** – Conectado às **GPIOs 26 e 27**.
- **Botão do Joystick** – Conectado à **GPIO 22**.

## Funcionalidades Implementadas

### 1. **Controle de LEDs RGB**

- **LED Azul:** O brilho é ajustado conforme o valor do eixo Y do joystick:
  - Quando o joystick estiver na posição central (valor 2048), o LED azul fica apagado.
  - Quando o joystick se move para cima (valores menores) ou para baixo (valores maiores), o brilho do LED aumenta gradualmente, atingindo a intensidade máxima nos extremos (0 e 4095).
  
- **LED Vermelho:** O brilho é ajustado conforme o valor do eixo X do joystick:
  - Quando o joystick estiver na posição central (valor 2048), o LED vermelho fica apagado.
  - Quando o joystick se move para a esquerda (valores menores) ou para a direita (valores maiores), o brilho do LED aumenta gradualmente, atingindo a intensidade máxima nos extremos (0 e 4095).
  
Os LEDs são controlados via **PWM**, permitindo uma variação suave na intensidade luminosa.

### 2. **Exibição no Display SSD1306**

- Um quadrado de **8x8 pixels** é exibido no display, inicialmente centralizado. O quadrado se move proporcionalmente aos valores capturados pelo joystick, criando uma interação visual dinâmica.

### 3. **Botão do Joystick**

- **Alterna o estado do LED Verde** a cada acionamento.
- **Modifica a borda do display**, alternando entre diferentes estilos de borda a cada novo acionamento do botão.

### 4. **Botão A**

- **Ativa ou desativa os LEDs PWM** a cada acionamento, permitindo controlar a intensidade dos LEDs com base na interação do usuário.

## Requisitos do Projeto

1. **Uso de Interrupções (IRQ):** Todas as funcionalidades relacionadas aos botões foram implementadas utilizando rotinas de interrupção, garantindo uma resposta rápida e precisa.
2. **Debouncing:** O tratamento do "bouncing" dos botões foi implementado via software, eliminando falhas de leitura devido a múltiplos acionamentos involuntários.
3. **Utilização do Display 128x64:** O projeto utiliza o display SSD1306 para exibir gráficos e informações de forma clara e interativa, demonstrando o entendimento do protocolo **I2C**.
4. **Organização do Código:** O código está bem estruturado, legível e comentado, facilitando o entendimento e a manutenção.

## Status do Projeto

O projeto está **finalizado** e **todas as funcionalidades estão implementadas e funcionando perfeitamente**. O sistema está operando de forma estável e confiável, atendendo a todos os requisitos estabelecidos e proporcionando uma interação intuitiva com o usuário.

## Como Clonar

1. **Clone o repositório:**
   ```bash
   git clone https://github.com/icarosg/joystick.git
   cd joystick
   
## **Demonstração**

Assista ao vídeo demonstrativo no seguinte link: [Vídeo da solução]()
