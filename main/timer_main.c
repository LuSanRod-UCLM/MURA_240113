
/***********************************************************************************************************
 * Ejemplo del uso de una máquina de estados
 * Manejo de una luz temporizada mediante un pulsador
 ***********************************************************************************************************/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "esp_log.h"

#include "fsm.h"

/* Etiqueta para depuración */
const char* TAG = "app_main";

/* Pines GPIO asignados a la entrada (PULSADOR) y a la salida (LED) */


#define GPIO_PIN_LED    21 // Pin asignado al LED
#define GPIO_PIN_BUTTON 22 // Pin asignado al pulsador

/* Estado de activación del pulsador (activo a nivel bajo) */
#define BUTTON_LEVEL_ACTIVE 0

/* Periódo de revisión del estado de la máquina de estados */
#define FSM_CYCLE_PERIOD_MS 200
// #define FSM_CYCLE_PERIOD_MS 500

/* Tiempo que está activa la salida tras la pulsación del pulsador */
#define TIMER_CYCLE_S 4

/* Cuenta del tiempo que lleva activa la salida */
int timer = 0;

/* Comprueba si ha vencido el tiempo que debe estar activa */
int  timer_expired(void* params)
{
    return (timer == 0);
}

/* Comienza el tiempo de salida activa */
void timer_start(void)
{
    timer = (TIMER_CYCLE_S * 1000) / FSM_CYCLE_PERIOD_MS;
}

/* Decrementa el tiempo que la salida debe permanecer activa */
void timer_next(void)
{
    if (timer) --timer;
    ESP_LOGD(TAG, "Tiempo restante: %d", timer);

}

/* Comprueba si el pulsador está pulsado */
int button_pressed(void *params) 
{ 
    bool button_state = (BUTTON_LEVEL_ACTIVE == gpio_get_level(GPIO_PIN_BUTTON));
    ESP_LOGD(TAG, "Estado de botón: %d", gpio_get_level(GPIO_PIN_BUTTON));
    if (button_state) ESP_LOGI(TAG, "Botón pulsado");

    return button_state;
}

/* Activa el LED de salida y comienza su temporización */
void lampara_on (void* params)
{
    ESP_LOGD(TAG, "Activa LED y comienza temporización");
    gpio_set_level(GPIO_PIN_LED, 1);
    timer_start();
}

/* Desactiva el LED de salida */
void lampara_off (void* fsm)
{
    gpio_set_level(GPIO_PIN_LED, 0);
}

/* Prepara la máquina de estados que controla la luz temporizada */
fsm_t* lampara_new (void)
{
    static fsm_trans_t lampara_tt[] = {
        {  0, button_pressed, 1, lampara_on },
        {  1, button_pressed, 1, lampara_on },
        {  1, timer_expired, 0, lampara_off },
        { -1, NULL, -1, NULL },
    };
    return fsm_new (lampara_tt);
}

/* Tarea de gestión de la máquina de estados */
void app_main() 
{
    /* Prepara la entrada y la salida */
    ESP_LOGI(TAG, "Preparando entrada y salida");
    gpio_reset_pin(GPIO_PIN_LED);
    gpio_reset_pin(GPIO_PIN_BUTTON);
    gpio_set_direction(GPIO_PIN_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_PIN_BUTTON, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_PIN_BUTTON, GPIO_PULLUP_ONLY);

    /* Configura la máquina de estados */
    ESP_LOGI(TAG, "Configura el comportamiento de la luz");
    fsm_t* fsm_lampara = lampara_new();

    /* Ciclo de gestión de la máquina */
    ESP_LOGI(TAG, "Pone a funcionar la luz temporizada");
    TickType_t last = xTaskGetTickCount();
    while (true) {
        fsm_update (fsm_lampara);
        timer_next();
        vTaskDelayUntil(&last, pdMS_TO_TICKS(FSM_CYCLE_PERIOD_MS));
    }
}
