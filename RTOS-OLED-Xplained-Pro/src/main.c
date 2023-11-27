#include "conf_board.h"
#include <asf.h>
#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

/* Botao da placa */
#define BUT_PIO PIOA
#define BUT_PIO_ID ID_PIOA
#define BUT_PIO_PIN 11
#define BUT_PIO_PIN_MASK (1 << BUT_PIO_PIN)

/************************************************************************/
/* defines                                                              */
/************************************************************************/
// LED in PC08
#define LED_PIO           PIOC
#define LED_PIO_ID        ID_PIOC
#define LED_PIO_IDX       8
#define LED_PIO_IDX_MASK  (1 << LED_PIO_IDX)

// Configurations for the LED 1 (PA0)
#define LED1_PIO           PIOA                 // peripheral that controls the LED1
#define LED1_PIO_ID        ID_PIOA              // ID from peripheral PIOA (controls LED1)
#define LED1_PIO_IDX       0                    // ID from LED1 in PIO
#define LED1_PIO_IDX_MASK  (1 << LED1_PIO_IDX)   // Mask to CONTROL the LED1

// Configurations for the LED 2 (PC30)
#define LED2_PIO           PIOC                 // peripheral that controls the LED2
#define LED2_PIO_ID        ID_PIOC              // ID from peripheral PIOC (controls LED2)
#define LED2_PIO_IDX       30                    // ID from LED2 in PIO
#define LED2_PIO_IDX_MASK  (1 << LED2_PIO_IDX)   // Mask to CONTROL the LED2

// Configurations for the LED 3 (PB2)
#define LED3_PIO           PIOB                 // peripheral that controls the LED3
#define LED3_PIO_ID        ID_PIOB              // ID from peripheral PIOB (controls LED3)
#define LED3_PIO_IDX       2                    // ID from LED3 in PIO
#define LED3_PIO_IDX_MASK  (1 << LED3_PIO_IDX)   // Mask to CONTROL the LED3

// Configurations for the button 1 (PD28)
#define BUT1_PIO           PIOD               // peripheral that controls the BUT1
#define BUT1_PIO_ID        ID_PIOD            // ID from peripheral PIOD (controls BUT1)
#define BUT1_PIO_IDX       28                 // ID from BUT1 in PIO
#define BUT1_PIO_IDX_MASK (1u << BUT1_PIO_IDX) // Already done

// Configurations for the button 2 (PC31)
#define BUT2_PIO           PIOC               // peripheral that controls the BUT2
#define BUT2_PIO_ID        ID_PIOC            // ID from peripheral PIOC (controls BUT2)
#define BUT2_PIO_IDX       31                 // ID from BUT2 in PIO
#define BUT2_PIO_IDX_MASK (1u << BUT2_PIO_IDX) // Already done

// Configurations for the button 3 (PA19)
#define BUT3_PIO           PIOA               // peripheral that controls the BUT3
#define BUT3_PIO_ID        ID_PIOA            // ID from peripheral PIOA (controls BUT3)
#define BUT3_PIO_IDX       19                 // ID from BUT3 in PIO
#define BUT3_PIO_IDX_MASK (1u << BUT3_PIO_IDX) // Already done


/** RTOS  */
#define TASK_OLED_STACK_SIZE (1024 * 6 / sizeof(portSTACK_TYPE))
#define TASK_OLED_STACK_PRIORITY (tskIDLE_PRIORITY)

#define TASK_PISCA_LED_STACK_SIZE                (1024*6/sizeof(portSTACK_TYPE))
#define TASK_PISCA_LED_STACK_PRIORITY            (tskIDLE_PRIORITY)

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,
                                          signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);

/** prototypes */
void but_callback(void);
static void BUT_init(void);
void but1_callback(void);
void but2_callback(void);
void but3_callback(void);
static void init(void);
void configure_rtt_chronometer(int timer_time);
void pin_toggle(Pio *pio, uint32_t mask);
void pisca_led(int n, int t);

typedef struct  {
	uint32_t year;
	uint32_t month;
	uint32_t day;
	uint32_t week;
	uint32_t hour;
	uint32_t minute;
	uint32_t second;
} calendar;

void LED_init(int estado);
void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type);

/************************************************************************/
/* VAR globais                                                          */
/************************************************************************/
volatile char flag_rtc_alarm = 0;

/************************************************************************/
/* RTOS application funcs                                               */
/************************************************************************/

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,
                                          signed char *pcTaskName) {
  printf("stack overflow \n");
  for (;;) {
  }
}

extern void vApplicationIdleHook(void) {}

extern void vApplicationTickHook(void) {}

extern void vApplicationMallocFailedHook(void) {
  configASSERT((volatile void *)NULL);
}

/************************************************************************/
/* handlers / callbacks                                                 */
/************************************************************************/

void but_callback(void) {}

/************************************************************************/
/* TASKS                                                                */
/************************************************************************/

static void task_oled(void *pvParameters) {
  gfx_mono_ssd1306_init();
  gfx_mono_draw_string("Exemplo RTOS", 0, 0, &sysfont);
  gfx_mono_draw_string("oii", 0, 20, &sysfont);

  for (;;) {
  }
}

/************************************************************************/
/* funcoes                                                              */
/************************************************************************/

static void configure_console(void) {
  const usart_serial_options_t uart_serial_options = {
      .baudrate = CONF_UART_BAUDRATE,
      .charlength = CONF_UART_CHAR_LENGTH,
      .paritytype = CONF_UART_PARITY,
      .stopbits = CONF_UART_STOP_BITS,
  };

  /* Configure console UART. */
  stdio_serial_init(CONF_UART, &uart_serial_options);

  /* Specify that stdout should not be buffered. */
  setbuf(stdout, NULL);
}

static void BUT_init(void) {
  /* configura prioridae */
  NVIC_EnableIRQ(BUT_PIO_ID);
  NVIC_SetPriority(BUT_PIO_ID, 4);

  /* conf botão como entrada */
  pio_configure(BUT_PIO, PIO_INPUT, BUT_PIO_PIN_MASK,
                PIO_PULLUP | PIO_DEBOUNCE);
  pio_set_debounce_filter(BUT_PIO, BUT_PIO_PIN_MASK, 60);
  pio_enable_interrupt(BUT_PIO, BUT_PIO_PIN_MASK);
  pio_handler_set(BUT_PIO, BUT_PIO_ID, BUT_PIO_PIN_MASK, PIO_IT_FALL_EDGE,
                  but_callback);
}

/**
* Configura o RTC para funcionar com interrupcao de alarme
*/
void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type) {
	/* Configura o PMC */
	pmc_enable_periph_clk(ID_RTC);

	/* Default RTC configuration, 24-hour mode */
	rtc_set_hour_mode(rtc, 0);

	/* Configura data e hora manualmente */
	rtc_set_date(rtc, t.year, t.month, t.day, t.week);
	rtc_set_time(rtc, t.hour, t.minute, t.second);

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(id_rtc);
	NVIC_ClearPendingIRQ(id_rtc);
	NVIC_SetPriority(id_rtc, 4);
	NVIC_EnableIRQ(id_rtc);

	/* Ativa interrupcao via alarme */
	rtc_enable_interrupt(rtc,  irq_type);
}

/************************************************************************/
/* main                                                                 */
/************************************************************************/

int main(void) {
  /* Initialize the SAM system */
  sysclk_init();
  board_init();

  /* Initialize the console uart */
  configure_console();

  /* Create task to control oled */
  if (xTaskCreate(task_oled, "oled", TASK_OLED_STACK_SIZE, NULL,
                  TASK_OLED_STACK_PRIORITY, NULL) != pdPASS) {
    printf("Failed to create oled task\r\n");
  }

  /* Start the scheduler. */
  vTaskStartScheduler();

  /* RTOS não deve chegar aqui !! */
  while (1) {
  }

  /* Will only get here if there was insufficient memory to create the idle
   * task. */
  return 0;
}
