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
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"
#include "wifi_config.h"
#include "wifi_info.h"
#include "wifi_config_ap.h"
#include "wifi_config_sta.h"
#include "nvs_service.h"
#include "http_server.h"
/*******************************************************************************
* DEFINES E ENUMS
*******************************************************************************/
/* Keys de gravação dos dados de configuração na NVS */
#define WIFI_CONFIG_SSID_NVS "wifi_cfg_ssid"
#define WIFI_CONFIG_PSWD_NVS "wifi_cfg_pswd"

/* Tipos de sinais utilização na task de configuração */
typedef enum wifiConfigSignal_t
{
  WIFI_CONFIG_CONNECTED_SIGNAL = BIT0,
  WIFI_CONFIG_DISCONNECTED_SIGNAL = BIT1,
  WIFI_CONFIG_END_SIGNAL = BIT2,
  WIFI_CONFIG_START_SIGNAL = BIT3,
  WIFI_CONFIG_AP_STA_CONNECTED_SIGNAL = BIT4,
  WIFI_CONFIG_NULL = 0x00,
} wifiConfigSignal_t;

/* Callback para tratamento dos sinais */
typedef void (*wifiConfigSignalCallback_t)(void);

/*******************************************************************************
* TYPEDEFS
*******************************************************************************/
/* Estrutura de controle do sinal e seu respectivo tratamento */
typedef struct wifiConfigSignalStruct_t
{
  wifiConfigSignal_t signal;
  wifiConfigSignalCallback_t handler;
} wifiConfigSignalStruct_t;

/*******************************************************************************
* PROTÓTIPOS DE FUNÇÕES
*******************************************************************************/
static void config_task(void * param);
static void init(wifi_mode_t mode);
static void init_signals(void);
static void wifi_event_handler(void *arg, esp_event_base_t eventBase,
                               int32_t eventId, void *eventDataPtr);
static void ip_event_handler(void *arg, esp_event_base_t eventBase,
                             int32_t eventId, void *eventDataPtr);
static void use_connected_callback(bool connected);
static bool read_wifi_configuration(wifi_config_t * wifiConfigPtr);
static void wifi_connected(void);
static void wifi_disconnected(void);
static void wifi_start_handler(void);
static void wifi_end_handler(void);
static void ap_sta_connected_handler(void);
static void config_erase_handler(void);
/*******************************************************************************
* CONSTANTES
*******************************************************************************/
/* Tabela de sinais a serem tratados pela task RTOS */
static const wifiConfigSignalStruct_t signals [] = {
  {
    .signal = WIFI_CONFIG_CONNECTED_SIGNAL,
    .handler = wifi_connected,
  },
  {
    .signal = WIFI_CONFIG_DISCONNECTED_SIGNAL,
    .handler = wifi_disconnected,
  },
  {
    .signal = WIFI_CONFIG_START_SIGNAL,
    .handler = wifi_start_handler,
  },
  {
    .signal = WIFI_CONFIG_END_SIGNAL,
    .handler = wifi_end_handler,
  },
  {
    .signal = WIFI_CONFIG_NULL
  }
};

/* Identificador LOG */
static const char *TAG = "WIFI_CONFIG_SERVICE";
/*******************************************************************************
* VARIÁVEIS
*******************************************************************************/
/* FreeRTOS estrutura de tratamento de sinais */
static EventGroupHandle_t wifiConfigSignal;
/* Callback para notificar conclusão na configuração WiFi */
static wifiConfigCallback_t callback = NULL;
/* Estrutura a ser manipulada para os sinais */
static EventBits_t uxBits;
/* Configuração Wi-Fi */
static wifi_config_t wifiConfig;
/* Variável dinâmica de carregamento dos sinais de configuração */
static wifiConfigSignal_t ALL_SIGNALS;
/*******************************************************************************
* FUNÇÕES EXPORTADAS
*******************************************************************************/

/**
 * Inicializa interface de configuração
 * 
 */
void wifi_config_init(void)
{
  init(WIFI_MODE_STA);

  /* Define callbacks para tratamentos dos eventos de conectividade */
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));

  xTaskCreate(config_task, "wifi_config_task", 4096, NULL, 3, NULL);

  /* Inicia estrutura Wi-Fi */
  ESP_ERROR_CHECK(esp_wifi_start());
}

/**
 * Inicializa conexão com Wi-Fi
 * 
 * @param callbackToSet - Função a ser chamada para notificar da finalização do processo
 */
void wifi_config_connect(wifiConfigCallback_t callbackToSet)
{
  callback = callbackToSet;
  
  /* Caso Wi-Fi configurado, realiza conexão. Caso contrário usa APP-EspTouch */
  read_wifi_configuration(&wifiConfig) ?  wifi_config_upload(WIFI_IF_STA) :
                                          xEventGroupSetBits(wifiConfigSignal, WIFI_CONFIG_START_SIGNAL);
}

/**
 * Escreve configuração de Wi-Fi na estrutura local
 * 
 * @param wifiConfigPtr - configuração a ser copiada e realizado set
 */
void wifi_config_set(wifi_config_t * wifiConfigPtr)
{
  memcpy(&wifiConfig, wifiConfigPtr, sizeof(wifiConfig));
  ESP_LOGI(TAG, "Config Set -> ssid (%s) pass (%s)", wifiConfig.sta.ssid, wifiConfig.sta.password);
}

/**
 * Realiza a troca de configuração na estrutura do ESP
 * 
 * @param interface - Interface a ter configuração alterada
 * @param wifiConfigPtr - Configuração a ser escrita
 * @return true - Troca com sucesso
 * @return false - Falha nos parâmetros de configuração
 */
bool wifi_config_upload(wifi_interface_t interface)
{
  ESP_ERROR_CHECK(esp_wifi_disconnect());
  ESP_ERROR_CHECK(esp_wifi_set_config(interface, &wifiConfig));
  ESP_LOGI(TAG, "WiFi New Config save -> %s", wifiConfig.sta.ssid);
  return esp_wifi_connect() == ESP_OK;
}

/**
 * Realiza gravação dos dados de configuração
 */
void wifi_config_save(void)
{
  xEventGroupSetBits(wifiConfigSignal, WIFI_CONFIG_END_SIGNAL);
}

/**
 * Recupera atual configuração WiFi
 * 
 * @param interface - modo do ESP a ser consultado
 * @param wifiConfigPtr - Ponteiro a ser escrito com a configuração
 */
void wifi_config_get(wifi_interface_t interface, wifi_config_t * wifiConfigPtr)
{
  ESP_ERROR_CHECK(esp_wifi_get_config(interface, wifiConfigPtr));
}

/**
 * Realiza nova tentativa de conexão
 * 
 * @param callbackToSet - Handler para quando processo for finalizado
 */
void wifi_config_reconnect(wifiConfigCallback_t callbackToSet)
{
  callback = callbackToSet;
  esp_wifi_connect();
}

/**
 * Apaga dados locais (serviço e DB, além de realizar o upload ao ESP
 * 
 * @return true - sucesso exclusão
 * @return false - falha exclusão
 */
bool wifi_config_delete(void)
{
  /* Zera estrutura local do serviço */
  bzero(&wifiConfig, sizeof(wifiConfig));
  /* Faz upload para estrutura do ESP */
  wifi_config_upload(WIFI_IF_STA);

  /* Executa exclusão da DB */
  return nvs_service_erase_key(WIFI_CONFIG_SSID_NVS) &&
         nvs_service_erase_key(WIFI_CONFIG_PSWD_NVS);
}

/*******************************************************************************
* FUNÇÕES LOCAIS
*******************************************************************************/
/**
 * Inicializa interface internet ESP
 * 
 * @param mode - Modo (AP ou STA) a ser utilizado na inicialização
 */
static void init(wifi_mode_t mode)
{
  init_signals();

  /* Inicializa interface de conexão e semáforo */
  ESP_ERROR_CHECK(esp_netif_init());

  wifi_config_ap_init();
  wifi_config_sta_init();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_mode(mode));
}

/**
 * Inicializa variável de sinais estática 
 * 
 */
static void init_signals(void)
{
  wifiConfigSignal = xEventGroupCreate();

  for (uint32_t idx = 0; signals[idx].handler != WIFI_CONFIG_NULL; idx++)
  {
    ALL_SIGNALS |= signals[idx].signal;
  }
}

/**
 * Realiza leitura da DB e salva na estrutura fornecida
 * 
 * @param wifiConfigPtr - Estrutura a ser escrita com os dados da DB
 * @return true - Valores lidos com sucesso
 * @return false - Falha na leitura
 */
static bool read_wifi_configuration(wifi_config_t *wifiConfigPtr)
{
  bzero(wifiConfigPtr, sizeof(wifi_config_t));

  int32_t result = nvs_service_read_string(WIFI_CONFIG_SSID_NVS,
                                           &wifiConfigPtr->sta.ssid,
                                           sizeof(wifiConfigPtr->sta.ssid));

  if (result <= 0)
  {
    /* Valor ainda não inicializado */
    return false;
  }

  result = nvs_service_read_string(WIFI_CONFIG_PSWD_NVS,
                                   &wifiConfigPtr->sta.password,
                                   sizeof(wifiConfigPtr->sta.password));
  return result == 1;
}

/**
 * Trata evento do tópico de conectividade Wi-Fi
 * 
 * @param arg 
 * @param eventBase - Tópico recebido
 * @param eventId - Tipo evento recebido
 * @param eventDataPtr - Ponteiro para os dados recebidos no evento
 */
static void wifi_event_handler(void * arg, esp_event_base_t eventBase,
                               int32_t eventId, void * eventDataPtr)
{
  switch (eventId)
  {
    case WIFI_EVENT_STA_DISCONNECTED:
      xEventGroupSetBits(wifiConfigSignal, WIFI_CONFIG_DISCONNECTED_SIGNAL);
      break;
    default: { }
  }
}

/**
 * Trata evento do tópico dos dados de IP
 * 
 * @param arg 
 * @param eventBase - Tópico recebido
 * @param eventId - Tipo evento recebido
 * @param eventDataPtr - Ponteiro para os dados recebidos no evento
 */
static void ip_event_handler(void * arg, esp_event_base_t eventBase,
                             int32_t eventId, void * eventDataPtr)
{
  switch (eventId)
  {
    case IP_EVENT_STA_GOT_IP:
      /* Estação conectada */
      xEventGroupSetBits(wifiConfigSignal, WIFI_CONFIG_CONNECTED_SIGNAL);
      break;
    default: { }
  }
}

/**
 * Inicia jornada de configuração 
 * 
 */
static void wifi_start_handler(void)
{
  /* Limpa estrutura STA */
  bzero(&wifiConfig, sizeof(wifiConfig));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiConfig));

  /* Inicializa AP+STA */
  wifi_config_ap_init_softap();
}

/**
 * Finaliza jornada de configuração, salvando dados e setando valores
 * 
 */
static void wifi_end_handler(void)
{
  bool result = false;

  wifi_config_t oldConfig;
  read_wifi_configuration(&oldConfig);

  result = ((strcmp(&wifiConfig.sta.ssid, &oldConfig.sta.ssid) != 0) &&
            (strcmp(&wifiConfig.sta.password, &oldConfig.sta.password) != 0));
  
  if (result == false)
  {
    return;
  }

  result = esp_wifi_set_config(WIFI_IF_STA, &wifiConfig) == ESP_OK;

  if (result == false)
  {
    return;
  }

  result = (nvs_service_write_string(WIFI_CONFIG_SSID_NVS, &wifiConfig.sta.ssid, sizeof(wifiConfig.sta.ssid)) &&
            nvs_service_write_string(WIFI_CONFIG_PSWD_NVS, &wifiConfig.sta.password, sizeof(wifiConfig.sta.password)));
}

/**
 * Realiza notificação de conexão 
 * 
 */
static void wifi_connected(void)
{
  /* ESP Conectado */
  ESP_LOGI(TAG, "WiFi Connected to ap");
  use_connected_callback(true);
}

/**
 * Realiza notificação de desconexão 
 * 
 */
static void wifi_disconnected(void)
{
  /* ESP Desconectado */
  ESP_LOGI(TAG, "WiFi Disconnected to ap");
  use_connected_callback(false);
}

/**
 * Task de controle da conexão 
 * 
 * @param parm 
 */
static void config_task(void *param)
{  
  while (true)
  {
    /* Aguarda set dos bits de conexão */
    uxBits = xEventGroupWaitBits(wifiConfigSignal, ALL_SIGNALS, true, false, portMAX_DELAY);

    for (uint32_t idx = 0; signals[idx].handler != WIFI_CONFIG_NULL; idx++)
    {
      if (uxBits & signals[idx].signal)
      {
        xEventGroupClearBits(wifiConfigSignal, signals[idx].signal);
        if (signals[idx].handler != NULL)
        {
          (*signals[idx].handler)();
        }
      }
    }
  }
}

/**
 * Valida se callback gravado é nulo, caso contrário realiza notificação
 * 
 * @param connected - valor de status a ser notificado
 */
static void use_connected_callback(bool connected)
{
  callback != NULL ? (*callback)(connected) : (void)0;
}

/*******************************************************************************
* END OF FILE
*******************************************************************************/
