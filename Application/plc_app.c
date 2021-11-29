/*******************************************************************************
* Leonardo Mudrek de Almeida
* UTFPR - CT
*
*
* License : CC BY NC SA 4.0
*******************************************************************************/

/*******************************************************************************
* INCLUDES
*******************************************************************************/
#include "plc_app.h"
#include "plc_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_log.h"
/*******************************************************************************
* DEFINES E ENUMS
*******************************************************************************/

/*******************************************************************************
* TYPEDEFS
*******************************************************************************/
/* Callback para tratamento sinal aplicação */
typedef void (*plcAppSignalCallback_t)(void);

/* Estrutura de tratamento de sinais */
typedef struct plcAppSignalStruct_t
{
  plcAppSignal_t signal;
  plcAppSignalCallback_t handler;
} plcAppSignalStruct_t;

/*******************************************************************************
* PROTÓTIPOS DE FUNÇÕES
*******************************************************************************/
static void app_task(void * param);
static void init_signals(void);
static void plc_error_init(void);
/*******************************************************************************
* CONSTANTES
*******************************************************************************/
/* Tabela tratamento de sinais */
static const plcAppSignalStruct_t signals[] = {
    {
        .signal = PLC_APP_ERROR_INIT,
        .handler = plc_error_init,      
    },
    {.signal = PLC_APP_NULL}
};
/*******************************************************************************
* VARIÁVEIS
*******************************************************************************/
/* FreeRTOS estrutura de tratamento de sinais */
static EventGroupHandle_t plcAppSignal;
/* Estrutura a ser manipulada para os sinais */
static EventBits_t uxBits;
/* Variáveis de leitura de todos os sinais da aplicação */
static plcAppSignal_t ALL_SIGNALS;
/*******************************************************************************
* FUNÇÕES EXPORTADAS
*******************************************************************************/

/**
 * Inicializa aplicação da estrutura PLC
 * 
 */
void plc_app_init(void)
{
  /* Cria sinais e taks de controle dos eventos */
  init_signals();
  xTaskCreate(app_task, "plc_app_task", 4096, NULL, 3, NULL);

  /* Configura estrutura ESP para lidar com módulo PLC */
  plc_config_init();

  /* Configura módulo para modo desejado */
  if (plc_configure_module() == false)
  {
    plc_app_set(PLC_APP_ERROR_INIT);
  }
}

/**
 * Seta sinal para tratamento da aplicação
 * 
 * @param signal sinal a ser setado
 */
void plc_app_set(plcAppSignal_t signal)
{
  xEventGroupSetBits(plcAppSignal, signal);
}
/*******************************************************************************
* FUNÇÕES LOCAIS
*******************************************************************************/
/**
 * Task de controle da comunicação 
 * 
 * @param parm 
 */
static void app_task(void * param)
{
  while (true)
  {
    /* Aguarda set dos bits de sinais */
    uxBits = xEventGroupWaitBits(plcAppSignal, ALL_SIGNALS, true, false, portMAX_DELAY);

    for (uint32_t idx = 0; signals[idx].handler != PLC_APP_NULL; idx++)
    {
      if (uxBits & signals[idx].signal)
      {
        /* Sinal encontrado na tabela*/

        /* Limpa sinal*/
        xEventGroupClearBits(plcAppSignal, signals[idx].signal);

        if (signals[idx].handler != NULL)
        {
          /* Existe callback cadastrado, executa handler */
          (*signals[idx].handler)();
        }
      }
    }
  }
}

/**
 * Inicializa estrutura de sinais aplicação 
 * 
 */
static void init_signals(void)
{
  plcAppSignal = xEventGroupCreate();

  for (uint32_t idx = 0; signals[idx].handler != PLC_APP_NULL; idx++)
  {
    ALL_SIGNALS |= signals[idx].signal;
  }
}

/**
 * Handler para tratamento de falha na inicialização 
 * 
 */
static void plc_error_init(void)
{
  plc_configure_module();
}
/*******************************************************************************
* END OF FILE
*******************************************************************************/
