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
#include "http_server.h"
#include "stddef.h"
#include <esp_http_server.h>
#include "esp_log.h"
#include "wifi_controller.h"
#include "plc_controller.h"
#include "mdns.h"
/*******************************************************************************
* DEFINES E ENUMS
*******************************************************************************/

/*******************************************************************************
* TYPEDEFS
*******************************************************************************/

/*******************************************************************************
* CONSTANTES
*******************************************************************************/
static const char *TAG = "HTTP_SERVER";

/* Tabela de endpoints disponíveis via API */
static const httpd_uri_t endpoints[] =
{
    { .uri = "/wifi/info", .method = HTTP_GET, .handler = wifi_controller_get_info, },
    { .uri = "/wifi/ap", .method = HTTP_GET, .handler = wifi_controller_get_ap, },
    { .uri = "/wifi/connect", .method = HTTP_POST, .handler = wifi_controller_post_connect, },
    { .uri = "/wifi/connect", .method = HTTP_DELETE, .handler = wifi_controller_delete_connect, },
    { .uri = "/plc/topology", .method = HTTP_GET, .handler = plc_controller_get_topology, },
    { .uri = "/plc/command", .method = HTTP_POST, .handler = plc_controller_post_command, },
    { .uri = "/plc/io", .method = HTTP_POST, .handler = plc_controller_post_io, },
    { .uri = NULL }
};

/*******************************************************************************
* VARIÁVEIS
*******************************************************************************/
/* Servidor HTTP */
static httpd_handle_t server = NULL;

/*******************************************************************************
* PROTÓTIPOS DE FUNÇÕES
*******************************************************************************/
static void initialise_mdns(void);

/*******************************************************************************
* FUNÇÕES EXPORTADAS
*******************************************************************************/

/**
 * Inicializa server HTTP
 * 
 */
void http_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        /* Registra endpoints */
        ESP_LOGI(TAG, "Registering URI handlers");
        for (uint32_t idx = 0; endpoints[idx].uri != NULL; idx++)
        {
            httpd_register_uri_handler(server, &endpoints[idx]);
        }
    }

    /* Inicializa mDNS */
    initialise_mdns();
}


/*******************************************************************************
* FUNÇÕES LOCAIS
*******************************************************************************/

/**
 * Configura estrutura ESP para usar mDNS
 * 
 */
static void initialise_mdns(void)
{
    esp_err_t err = mdns_init();
    if (err) {
        ESP_LOGI(TAG, "MDNS Init failed: %d\n", err);
        return;
    }

    /* Define características mDNS */
    mdns_hostname_set("powerline");
    mdns_instance_name_set("PowerLine Server");

    mdns_txt_item_t serviceTxtData[] = {
        {"board", "esp32"},
        {"path", "/"}
    };

    ESP_ERROR_CHECK(mdns_service_add("PowerLine HTTP Server", "_http", "_tcp", 80, serviceTxtData,
                                     sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}
/*******************************************************************************
* END OF FILE
*******************************************************************************/
