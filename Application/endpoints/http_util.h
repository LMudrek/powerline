/*******************************************************************************
* Leonardo Mudrek de Almeida
* UTFPR - CT
*
*
* License : CC BY NC SA 4.0
*******************************************************************************/
#ifndef HTTP_UTIL_H
#define HTTP_UTIL_H

/*******************************************************************************
* INCLUDES
*******************************************************************************/
#include <esp_http_server.h>
#include "cJSON.h"
#include <stdbool.h>
/*******************************************************************************
* DEFINES E ENUMS
*******************************************************************************/

/*******************************************************************************
* TYPEDEFS
*******************************************************************************/

/*******************************************************************************
* FUNÇÕES EXPORTADAS
*******************************************************************************/
esp_err_t http_util_send_response(httpd_req_t * reqPtr, const char * httpCode, const char * messagePtr);
esp_err_t http_read_body(httpd_req_t * req, char * bufferOutPtr, size_t bufferOutSize);
esp_err_t get_json_string_value(cJSON * root, const char * key, char * fieldPtr, size_t sizeField);
esp_err_t get_json_int_value(cJSON * root, const char * key, uint32_t * fieldPtr);
bool close_json(cJSON * root, char *bufferOutPtr, size_t sizeBufferOut);
/*******************************************************************************
* END OF FILE
*******************************************************************************/
#endif
