/*******************************************************************************
* Leonardo Mudrek de Almeida
* UTFPR - CT
*
*
* License : CC BY NC SA 4.0
*******************************************************************************/
#ifndef PLC_CONTROLLER_H
#define PLC_CONTROLLER_H

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
esp_err_t plc_controller_get_topology(httpd_req_t * req);
esp_err_t plc_controller_post_command(httpd_req_t * req);
esp_err_t plc_controller_post_io(httpd_req_t * req);
/*******************************************************************************
* END OF FILE
*******************************************************************************/
#endif