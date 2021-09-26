#include <WiFi.h>
#include "esp_camera.h"
#include "esp_http_server.h"
#include "static.hpp"
//

//macros

#define PART_BOUNDARY "123456789000000000000987654321"
#define STREAM_CONTENT_TYPE "multipart/x-mixed-replace;boundary=" PART_BOUNDARY
#define STREAM_BOUNDARY "\r\n--" PART_BOUNDARY "\r\n"
#define STREAM_PART "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n"

static esp_err_t streamHandler(httpd_req_t *req)
{

    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    size_t jpg_buf_len = 0;
    uint8_t *jpg_buf = NULL;
    char *part_buf[64];

    res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
    if (res != ESP_OK)
    {
        return res;
    }

    while (true && res == ESP_OK)
    {
        fb = esp_camera_fb_get();
        if (!fb)
        {
            Serial.println("Camera capture failed");
            res = ESP_FAIL;
        }
        else
        {

            jpg_buf_len = fb->len;
            jpg_buf = fb->buf;
        }
        size_t hlen = snprintf((char *)part_buf, 64, STREAM_PART, jpg_buf_len);
        const char *responses[3] = {
            (char *)part_buf, (char *)jpg_buf, STREAM_BOUNDARY};
        const size_t sizes_response[3] = {
            hlen, jpg_buf_len, strlen(STREAM_BOUNDARY)};

        for (int i = 0; i < 3 && res == ESP_OK; i++)
        {
            res = httpd_resp_send_chunk(req, responses[i], sizes_response[i]);
        }

        if (fb)
        {
            esp_camera_fb_return(fb);
            fb = NULL;
            jpg_buf = NULL;
        }
        else if (jpg_buf)
        {
            free(jpg_buf);
            jpg_buf = NULL;
        }

        Serial.printf("MJPG: %uB\n", (uint32_t)(jpg_buf_len));
    }
    return res;
}
static esp_err_t html_resp(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");

    return httpd_resp_send(req, (const char *)homepage_html, homepage_html_len);
}
static esp_err_t moveRobot(httpd_req_t *req)
{
    Serial.println(req->uri);
    return httpd_resp_send(req, "cum chalice", strlen("cum chalice"));
}

void startServer()

{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t stream_img_httpd = NULL;
    httpd_handle_t robot_control = NULL;

    config.server_port = 80;
    httpd_uri_t static_page = {
        .uri = "/",
        .method = HTTP_GET, //HTTP_GET
        .handler = html_resp,
        .user_ctx = NULL};
    httpd_uri_t move_robot = {
        .uri = "/move",
        .method = HTTP_GET, //HTTP_GET
        .handler = moveRobot,
        .user_ctx = NULL};
    if (httpd_start(&robot_control, &config) == ESP_OK)
    {
        httpd_register_uri_handler(robot_control, &static_page);
        httpd_register_uri_handler(robot_control, &move_robot);
    }

    config.server_port++;
    config.ctrl_port++;
    httpd_uri_t img = {
        .uri = "/",
        .method = HTTP_GET, //HTTP_GET
        .handler = streamHandler,
        .user_ctx = NULL};

    //Serial.printf("Starting web server on port: '%d'\n", config.server_port);
    if (httpd_start(&stream_img_httpd, &config) == ESP_OK)
    {

        httpd_register_uri_handler(stream_img_httpd, &img);
    }

} //a value of type "WebRequestMethod" cannot be used to initialize an entity of type "httpd_method_t"
