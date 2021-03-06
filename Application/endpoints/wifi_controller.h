/*******************************************************************************
* Leonardo Mudrek de Almeida
* UTFPR - CT
*
*
* License : CC BY NC SA 4.0
*******************************************************************************/
#ifndef WIFI_CONTROLLER_H
#define WIFI_CONTROLLER_H

/*******************************************************************************
* INCLUDES
*******************************************************************************/
#include <esp_http_server.h>

/*******************************************************************************
* DEFINES E ENUMS
*******************************************************************************/

/*******************************************************************************
* TYPEDEFS
*******************************************************************************/

/*******************************************************************************
* FUNÇÕES EXPORTADAS
*******************************************************************************/
esp_err_t wifi_controller_get_info(httpd_req_t * req);
esp_err_t wifi_controller_get_ap(httpd_req_t * req);
esp_err_t wifi_controller_post_connect(httpd_req_t * req);
esp_err_t wifi_controller_delete_connect(httpd_req_t * req);
/*******************************************************************************
* END OF FILE
*******************************************************************************/
#endif
