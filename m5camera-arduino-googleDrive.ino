/*
  M5CameraでGoogle DriveにJPEGファイルをアップロードするテスト
  とりあえずやっつけなのであとで整理したいなと思いまつ
*/

#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>
//#define M5CAM_MODEL_A
#define M5CAM_MODEL_B

#if defined(M5CAM_MODEL_A)
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    15
  #define XCLK_GPIO_NUM     27
  #define SIOD_GPIO_NUM     25
  #define SIOC_GPIO_NUM     23
  
  #define Y9_GPIO_NUM       19
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM        5
  #define Y4_GPIO_NUM       34
  #define Y3_GPIO_NUM       35
  #define Y2_GPIO_NUM       32
  #define VSYNC_GPIO_NUM    22
  #define HREF_GPIO_NUM     26
  #define PCLK_GPIO_NUM     21
#elif defined(M5CAM_MODEL_B)
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    15
  #define XCLK_GPIO_NUM     27
  #define SIOD_GPIO_NUM     22
  #define SIOC_GPIO_NUM     23
  
  #define Y9_GPIO_NUM       19
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM        5
  #define Y4_GPIO_NUM       34
  #define Y3_GPIO_NUM       35
  #define Y2_GPIO_NUM       32
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     26
  #define PCLK_GPIO_NUM     21
#else
  #error "Not supported"
#endif

WiFiClientSecure client;
#define WIFI_CONN_TIMEOUT 30000 //Wi-Fi接続のタイムアウト
String ssid = "SSID";
String pass = "PASSWORD";

/*  事前にgoogle.comのCAを取得しておく */
const char* root_ca= \
     "-----BEGIN CERTIFICATE-----\n" \
     "MIIEXDCCA0SgAwIBAgINAeOpMBz8cgY4P5pTHTANBgkqhkiG9w0BAQsFADBMMSAw\n" \
     "HgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMjETMBEGA1UEChMKR2xvYmFs\n" \
     "U2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjAeFw0xNzA2MTUwMDAwNDJaFw0yMTEy\n" \
     "MTUwMDAwNDJaMFQxCzAJBgNVBAYTAlVTMR4wHAYDVQQKExVHb29nbGUgVHJ1c3Qg\n" \
     "U2VydmljZXMxJTAjBgNVBAMTHEdvb2dsZSBJbnRlcm5ldCBBdXRob3JpdHkgRzMw\n" \
     "ggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDKUkvqHv/OJGuo2nIYaNVW\n" \
     "XQ5IWi01CXZaz6TIHLGp/lOJ+600/4hbn7vn6AAB3DVzdQOts7G5pH0rJnnOFUAK\n" \
     "71G4nzKMfHCGUksW/mona+Y2emJQ2N+aicwJKetPKRSIgAuPOB6Aahh8Hb2XO3h9\n" \
     "RUk2T0HNouB2VzxoMXlkyW7XUR5mw6JkLHnA52XDVoRTWkNty5oCINLvGmnRsJ1z\n" \
     "ouAqYGVQMc/7sy+/EYhALrVJEA8KbtyX+r8snwU5C1hUrwaW6MWOARa8qBpNQcWT\n" \
     "kaIeoYvy/sGIJEmjR0vFEwHdp1cSaWIr6/4g72n7OqXwfinu7ZYW97EfoOSQJeAz\n" \
     "AgMBAAGjggEzMIIBLzAOBgNVHQ8BAf8EBAMCAYYwHQYDVR0lBBYwFAYIKwYBBQUH\n" \
     "AwEGCCsGAQUFBwMCMBIGA1UdEwEB/wQIMAYBAf8CAQAwHQYDVR0OBBYEFHfCuFCa\n" \
     "Z3Z2sS3ChtCDoH6mfrpLMB8GA1UdIwQYMBaAFJviB1dnHB7AagbeWbSaLd/cGYYu\n" \
     "MDUGCCsGAQUFBwEBBCkwJzAlBggrBgEFBQcwAYYZaHR0cDovL29jc3AucGtpLmdv\n" \
     "b2cvZ3NyMjAyBgNVHR8EKzApMCegJaAjhiFodHRwOi8vY3JsLnBraS5nb29nL2dz\n" \
     "cjIvZ3NyMi5jcmwwPwYDVR0gBDgwNjA0BgZngQwBAgIwKjAoBggrBgEFBQcCARYc\n" \
     "aHR0cHM6Ly9wa2kuZ29vZy9yZXBvc2l0b3J5LzANBgkqhkiG9w0BAQsFAAOCAQEA\n" \
     "HLeJluRT7bvs26gyAZ8so81trUISd7O45skDUmAge1cnxhG1P2cNmSxbWsoiCt2e\n" \
     "ux9LSD+PAj2LIYRFHW31/6xoic1k4tbWXkDCjir37xTTNqRAMPUyFRWSdvt+nlPq\n" \
     "wnb8Oa2I/maSJukcxDjNSfpDh/Bd1lZNgdd/8cLdsE3+wypufJ9uXO1iQpnh9zbu\n" \
     "FIwsIONGl1p3A8CgxkqI/UAih3JaGOqcpcdaCIzkBaR9uYQ1X4k2Vg5APRLouzVy\n" \
     "7a8IVk6wuy6pm+T7HT4LY8ibS5FEZlfAFLSW8NwsVz9SBK2Vqn1N0PIMn5xA6NZV\n" \
     "c7o835DLAFshEWfC7TIe3g==\n" \
     "-----END CERTIFICATE-----\n";

/*  HTTPSでPOSTする  */
String httpsPost(String url, String data, int port)
{
  String host = url.substring(8, url.indexOf("/",8));
  const char* charHost = host.c_str();
  String response = "";
  char rcvBuf[2048];
  int rcvCount = 0;
  while(!client.connect(charHost, port))
  {
    delay(10);
  }
  if (client.connected())
  {
    client.print(data);
    long timeout = millis();
    while (rcvCount == 0)
    {
      while (client.available())
      {
        rcvBuf[rcvCount]=client.read();
        rcvCount ++;
      }
    }
  }
  response = rcvBuf;
  client.stop();
  return (response);
}


/*  画像をPOSTする */
String postPic(String url, String token, uint8_t* addr, int picLen, int port)
{ 
  String host = url.substring(8, url.indexOf("/",8));
  const char* charHost = host.c_str();
  String response = "";
  char rcvBuf[2048];
  int rcvCount = 0;
  Serial.print("Connecting ");
  Serial.println(charHost);

  String boundary = "------WebKitFormBoundary6n1BXCrYS8DFaBeb\r\n";
  String data2="\r\n";
  data2 += boundary;
  data2 += "Content-Disposition: form-data; name=\"metadata\"; filename=\"blob\"\r\n";
  data2 += "Content-Type: application/json\r\n";
  data2 += "\r\n";
  data2 += "{ \"title\": \"esp32.jpg\", \"mimeType\": \"image/jpeg\", \"description\": \"Uploaded From ESP32\" }\r\n";
  data2 += boundary;
  data2 += "Content-Disposition: form-data; name=\"file\"; filename=\"upload.jpg\"\r\n";
  data2 += "Content-Type: image/jpeg\r\n";
  data2 += "\r\n";

  String data =
  "POST /upload/drive/v2/files?uploadType=multipart HTTP/1.1\r\n"
  "HOST: www.googleapis.com\r\n"
  "authorization: Bearer ";
  data += token;
  data += "\r\n";
  data += "content-type: multipart/form-data; boundary=----WebKitFormBoundary6n1BXCrYS8DFaBeb\r\n";
  data += "content-length: ";
  int len = data2.length();
  len += picLen;
  len += boundary.length();
  len += ((String)"\r\n").length();
  data += (String)len;
  data += "\r\n";
  data += data2;
    
  while(!client.connect(charHost, port))
  {
    delay(10);
    Serial.print(".");
  }
  if (client.connected())
  {
    client.print(data);

    int sizeDivT = ((int)(picLen / 1000));
    int remSize = ((int)picLen - (int)(picLen / 1000)*1000);

    /*  1000bytesに区切ってPOST  */
    for(int i = 0; i < sizeDivT ;i++)
    {
        client.write((addr + i * 1000), 1000);
    }
    if(remSize > 0)
    {
        client.write((addr + (sizeDivT * 1000)), remSize);
    }

    client.print("\r\n------WebKitFormBoundary6n1BXCrYS8DFaBeb--\r\n");
    long timeout = millis();
    while (rcvCount == 0)
    {
      while (client.available())
      {
        rcvBuf[rcvCount]=client.read();
        Serial.write(rcvBuf[rcvCount]);
        rcvCount ++;
        if(rcvCount>2047)break;
      }
    }
  }
  else Serial.println("Connection failed");
  response = rcvBuf;
  client.stop();
  return (response);
}


/*  コードからリフレッシュトークンを取得  */
String getRefreshToken(String code, String client_id, String client_secret)
{
  String body =  
  "------WebKitFormBoundarytjGcuAGS9sMRWKcr\r\n"
  "Content-Disposition: form-data; name=\"code\"\r\n"
  "\r\n"
  + code +
  "\r\n"
  "------WebKitFormBoundarytjGcuAGS9sMRWKcr\r\n"
  "Content-Disposition: form-data; name=\"client_id\"\r\n"
  "\r\n"
  + client_id +
  "\r\n"
  "------WebKitFormBoundarytjGcuAGS9sMRWKcr\r\n"
  "Content-Disposition: form-data; name=\"client_secret\"\r\n"
  "\r\n"
  + client_secret +
  "\r\n"
  "------WebKitFormBoundarytjGcuAGS9sMRWKcr\r\n"
  "Content-Disposition: form-data; name=\"redirect_uri\"\r\n"
  "\r\n"
  "urn:ietf:wg:oauth:2.0:oob\r\n"
  "------WebKitFormBoundarytjGcuAGS9sMRWKcr\r\n"
  "Content-Disposition: form-data; name=\"grant_type\"\r\n"
  "\r\n"
  "authorization_code\r\n"
  "------WebKitFormBoundarytjGcuAGS9sMRWKcr--\r\n";

  String header=
  "POST /o/oauth2/token HTTP/1.1\r\n"
  "HOST: accounts.google.com\r\n"
  "content-type: multipart/form-data; boundary=----WebKitFormBoundarytjGcuAGS9sMRWKcr\r\n"
  "content-length: "
  + (String)body.length() +
  "\r\n"
  "\r\n";

  Serial.println(header);
  Serial.println(body);
  String ret = httpsPost("https://accounts.google.com/o/oauth2/token", header + body, 443);

  Serial.println("RAW response:");
  Serial.println(ret);
  Serial.println();
  
  if(ret.indexOf("200 OK") > -1) Serial.println("Got the refresh token!");
  else Serial.println("NG");

  int refresh_tokenStartPos = ret.indexOf("refresh_token");
  refresh_tokenStartPos = ret.indexOf("\"",refresh_tokenStartPos) + 1;
  refresh_tokenStartPos = ret.indexOf("\"",refresh_tokenStartPos) + 1;
  int refresh_tokenEndPos   = ret.indexOf("\"",refresh_tokenStartPos);
  String refresh_token = ret.substring(refresh_tokenStartPos,refresh_tokenEndPos); 
  Serial.println("refresh_token:"+refresh_token);
  return refresh_token;
}


/*  リフレッシュトークンからアクセストークンを取得 */
String getAccessToken(String refresh_token, String client_id, String client_secret)
{
  String body =
  "------WebKitFormBoundarytjGcuAGS9sMRWKcr\r\n"
  "Content-Disposition: form-data; name=\"refresh_token\"\r\n"
  "\r\n"
  + refresh_token +
  "\r\n"
  "------WebKitFormBoundarytjGcuAGS9sMRWKcr\r\n"
  "Content-Disposition: form-data; name=\"client_id\"\r\n"
  "\r\n"
  + client_id +
  "\r\n"
  "------WebKitFormBoundarytjGcuAGS9sMRWKcr\r\n"
  "Content-Disposition: form-data; name=\"client_secret\"\r\n"
  "\r\n"
  + client_secret +
  "\r\n"
  "------WebKitFormBoundarytjGcuAGS9sMRWKcr\r\n"
  "Content-Disposition: form-data; name=\"grant_type\"\r\n"
  "\r\n"
  "refresh_token\r\n"
  "------WebKitFormBoundarytjGcuAGS9sMRWKcr--\r\n";
  
  String header=
  "POST /o/oauth2/token HTTP/1.1\r\n"
  "HOST: accounts.google.com\r\n"
  "content-type: multipart/form-data; boundary=----WebKitFormBoundarytjGcuAGS9sMRWKcr\r\n"
  "content-length: "
  + (String)body.length() +
  "\r\n"
  "\r\n";

  Serial.println(header);
  Serial.println(body);
  String ret = httpsPost("https://accounts.google.com/o/oauth2/token", header + body, 443);

  Serial.println("RAW response:");
  Serial.println(ret);
  Serial.println();
  
  if(ret.indexOf("200 OK") > -1) Serial.println("Got the token!");
  else Serial.println("NG");

  int access_tokenStartPos = ret.indexOf("access_token");
  access_tokenStartPos = ret.indexOf("\"",access_tokenStartPos) + 1;
  access_tokenStartPos = ret.indexOf("\"",access_tokenStartPos) + 1;
  int access_tokenEndPos   = ret.indexOf("\"",access_tokenStartPos);
  String access_token = ret.substring(access_tokenStartPos,access_tokenEndPos); 
  Serial.println("Access_token:"+access_token);
  return access_token;
}


/*  画像取得  */
camera_fb_t * getJPEG()
{
  camera_fb_t * fb = NULL;
  /*  数回撮影してAE(自動露出)をあわせる */
  esp_camera_fb_get();
  esp_camera_fb_get();
  esp_camera_fb_get();
  esp_camera_fb_get();
  fb = esp_camera_fb_get();  //JPEG取得
  /*
  fb->buf JPEGバッファ
  fb->len JPEGサイズ
  */
  
  if (!fb) 
  {
    Serial.printf("Camera capture failed");
  }
  esp_camera_fb_return(fb); //後始末
  Serial.printf("JPG: %uB ", (uint32_t)(fb->len));
  return fb;  
}


/*  APに接続 */
bool initSTA() 
{
  WiFi.begin(ssid.c_str(), pass.c_str());
  int n;
  long timeout = millis();
  while ((n = WiFi.status()) != WL_CONNECTED && (millis()-timeout) < WIFI_CONN_TIMEOUT)
  {
    Serial.print(".");
    delay(500);
    if (n == WL_NO_SSID_AVAIL || n == WL_CONNECT_FAILED)
    {
      delay(1000);
      WiFi.reconnect();
    }
  }
  if (n == WL_CONNECTED)
  {
    return true;
  }
  else
  {
    return false;
  }
}



void setup() 
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  initSTA();  //APに接続
  /*
    SPIフラッシュがフォーマットされてなかったらフォーマット
    設定データがなければリフレッシュトークンを取得して保存する。
    設定データは行ごとに分けて
    ・client_id
    ・client_secret
    ・refresh_token
    と保存
  */
  if(!SPIFFS.begin())
  {
    Serial.println("SPIFFS formatting");
    SPIFFS.format();
  }
  String code = "";
  String client_id = "";
  String client_secret = "";
  File fd = SPIFFS.open("/access_token.txt", "r");
  fd.readStringUntil('\n');
  fd.readStringUntil('\n');
  String line = fd.readStringUntil('\n');
  fd.close();
  if(line == "")
  {
    /*  APIのクライアントIDを入力  */
    Serial.println("Client ID?");
    while(!Serial.available()){}
    client_id = Serial.readStringUntil('\n');
    client_id.trim();
    fd = SPIFFS.open("/access_token.txt", "w");
    if (!fd) 
    {
      Serial.println("Config open error");
    }
    fd.println(client_id);

    /*  コードを取得してくれと表示 */
    Serial.println("Please access to");
    Serial.println("https://accounts.google.com/o/oauth2/auth?client_id=" + client_id +"&response_type=code&redirect_uri=urn:ietf:wg:oauth:2.0:oob&scope=https://www.googleapis.com/auth/drive");

    /*  コードを入力  */
    Serial.println("Code?");
    while(!Serial.available()){}
    code = Serial.readStringUntil('\n');
    code.trim();

    /*  APIのクライアントシークレットを入力  */
    Serial.println("Client secret?");
    while(!Serial.available()){}
    client_secret = Serial.readStringUntil('\n');
    client_secret.trim();
    fd.println(client_secret);
    Serial.println("OK");
    delay(1000);
    Serial.println("Refresh token:");
    fd.println(getRefreshToken(code, client_id, client_secret));
    fd.close();
  }
  

  /*  設定読み込み  */  
  fd = SPIFFS.open("/access_token.txt", "r");
  client_id = fd.readStringUntil('\r');
  Serial.print("Client ID");
  client_id.trim();
  Serial.print(client_id);
  Serial.println();
  client_secret = fd.readStringUntil('\r');
  client_secret.trim();
  Serial.print("Client Secret");
  Serial.print(client_secret);
  Serial.println();
  String refresh_token = fd.readStringUntil('\r');
  refresh_token.trim();  
  Serial.print("Refresh token");
  Serial.print(refresh_token);
  Serial.println();
  fd.close();

  /*  カメラ初期化設定*/
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if(psramFound())
  {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  else 
  {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  //カメラ初期化
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) 
  {
    Serial.printf("Camera init failed with error 0x%x", err);
  }
  else
  {
    client.setCACert(root_ca);  
    String access_token = getAccessToken(refresh_token, client_id, client_secret);
    Serial.println("Access token: "+access_token);
    camera_fb_t * fb = getJPEG();
    String ret = postPic("https://www.googleapis.com/upload/drive/v2/files?uploadType=multipart", access_token, fb->buf, fb->len, 443); 
    Serial.println(ret);
  }
  /*
  あとでカメラ設定変更するときは
  sensor_t * s = esp_camera_sensor_get(); //カメラプロパティのポインタ取得
  s->set_framesize(s, FRAMESIZE_UXGA);  //解像度設定
  とかやればおｋ
  */
}

/*  特に何もしない */
void loop() 
{
  delay(10000);
}
