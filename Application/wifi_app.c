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
#include "wifi_app.h"
#include "wifi_config.h"
#include "esp_wifi.h"
#include "nvs_service.h"
#include "esp_log.h"
#include "wifi_info.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "http_server.h"
/*******************************************************************************
* DEFINES E ENUMS
*******************************************************************************/

/*******************************************************************************
* TYPEDEFS
*******************************************************************************/
/* Callback para tratamento sinal aplicação */
typedef void (*wifiAppSignalCallback_t)(void);

/* Estrutura de tratamento de sinais */
typedef struct wifiAppSignalStruct_t
{
  wifiAppSignal_t signal;
  wifiAppSignalCallback_t handler;
} wifiAppSignalStruct_t;

/*******************************************************************************
* PROTÓTIPOS DE FUNÇÕES
*******************************************************************************/
static void app_task(void *param);
static void connection_callback(bool connected);
static void wifi_connected(void);
static void wifi_connect(void);
static void wifi_disconnected(void);
static void wifi_reconnect(void);
static void wifi_restart_config(void);
static void init_signals(void);
static void wifi_sta_erase(void);
/*******************************************************************************
* CONSTANTES
*******************************************************************************/
/* Tabela tratamento de sinais */
static const wifiAppSignalStruct_t signals[] = {
    {
        .signal = WIFI_APP_CONNECTED_SIGNAL,
        .handler = wifi_connected,
    },
    {
        .signal = WIFI_APP_DISCONNECTED_SIGNAL,
        .handler = wifi_disconnected,
    },
    {
        .signal = WIFI_APP_CONNECT_SIGNAL,
        .handler = wifi_connect,
    },
    {
      .signal = WIFI_APP_RECONNECT_SIGNAL,
      .handler = wifi_reconnect,
    },
    {
      .signal = WIFI_APP_RESTART_CONFIG_SIGNAL,
      .handler = wifi_restart_config,
    },
    {
      .signal = WIFI_APP_STA_DEFAULT_SIGNAL,
      .handler = wifi_sta_erase,
    },
    {.signal = WIFI_APP_NULL}};

/*******************************************************************************
* VARIÁVEIS
*******************************************************************************/
/* FreeRTOS estrutura de tratamento de sinais */
static EventGroupHandle_t wifiAppSignal;
/* Estrutura a ser manipulada para os sinais */
static EventBits_t uxBits;
/* Task Retry */
static TaskHandle_t xTaskRetryHandle = NULL;
/* Variáveis de leitura de todos os sinais da aplicação */
static wifiAppSignal_t ALL_SIGNALS;
/* Contador retentativas conexão */
static uint32_t reconnectCounter = 0;
/* Identificador LOG */
static const char *TAG = "WIFI_APP";
/*******************************************************************************
* FUNÇÕES EXPORTADAS
*******************************************************************************/
/**
 * Inicializa aplicação da estrutura Wi-Fi
 * 
 */
void wifi_app_connect(void)
{
  /* Cria sinais e taks de controle dos eventos */
  init_signals();
  wifi_config_init();
  xTaskCreate(app_task, "wifi_app_task", 4096, NULL, 3, NULL);
  wifi_app_set(WIFI_APP_CONNECT_SIGNAL);
  http_server_start();
}

/**
 * Seta sinal para tratamento da aplicação
 * 
 * @param signal sinal a ser setado
 */
void wifi_app_set(wifiAppSignal_t signal)
{
  xEventGroupSetBits(wifiAppSignal, signal);
}

/*******************************************************************************
* FUNÇÕES LOCAIS
*******************************************************************************/
/**
 * Task de controle da conexão 
 * 
 * @param parm 
 */
static void app_task(void * param)
{
  while (true)
  {
    /* Aguarda set dos bits de conexão */
    uxBits = xEventGroupWaitBits(wifiAppSignal, ALL_SIGNALS, true, false, portMAX_DELAY);

    for (uint32_t idx = 0; signals[idx].handler != WIFI_APP_NULL; idx++)
    {
      if (uxBits & signals[idx].signal)
      {
        xEventGroupClearBits(wifiAppSignal, signals[idx].signal);
        if (signals[idx].handler != NULL)
        {
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
  wifiAppSignal = xEventGroupCreate();

  for (uint32_t idx = 0; signals[idx].handler != WIFI_APP_NULL; idx++)
  {
    ALL_SIGNALS |= signals[idx].signal;
  }
}

/**
 * Handler para tratamento de eventos de conectividade
 * 
 * @param connectedValue  valor da conexão, true = conectado
 */
static void connection_callback(bool connectedValue)
{
  xEventGroupSetBits(
      wifiAppSignal,
      connectedValue ? WIFI_APP_CONNECTED_SIGNAL : WIFI_APP_DISCONNECTED_SIGNAL);
}

/**
 * Trata evento de conexão
 * 
 */
static void wifi_connected(void)
{
  /* ESP Conectado */
  ESP_LOGI(TAG, "Start HTTPs Server");
  reconnectCounter = 0;
  wifi_config_save();
}

/**
 * Trata evento de desconexão
 * 
 */
static void wifi_disconnected(void)
{
  /* ESP Desconectado */
  ESP_LOGI(TAG, "Stop HTTPs Server");
  xEventGroupSetBits(wifiAppSignal, WIFI_APP_RECONNECT_SIGNAL);
}

/**
 * Trata evento para realizar conexão
 * 
 */
static void wifi_connect(void)
{
  ESP_LOGI(TAG, "Start Wi-Fi Config");
  /* Start configuração conexão Wi-Fi */
  wifi_config_connect(connection_callback);
}

/**
 * Trata evento para realizar reconexão com a rede
 * 
 */
static void wifi_reconnect(void)
{
  reconnectCounter ++ < 5 ? wifi_config_reconnect(connection_callback) :
                            wifi_app_set(WIFI_APP_CONNECT_SIGNAL);
}

/**
 * Trata evento para realizar reinicialização da estrutura Wi-Fi
 * 
 */
static void wifi_restart_config(void)
{
  vTaskDelay(3000 / portTICK_PERIOD_MS);

  if (wifi_config_upload(WIFI_IF_STA) == false)
  {
    /* Falha conexão, restart fluxo */
    wifi_app_set(WIFI_APP_CONNECT_SIGNAL);
    return;
  };
}

/**
 * Trata evento para apagar dados de conexão Wi-Fi
 * 
 */
static void wifi_sta_erase(void)
{
  vTaskDelay(3000 / portTICK_PERIOD_MS);

  if (wifi_config_delete() == true)
  {
    /* Exclusão com sucesso, restart fluxo */
    wifi_app_set(WIFI_APP_CONNECT_SIGNAL);
  }
}

/*******************************************************************************
* END OF FILE
*******************************************************************************/
