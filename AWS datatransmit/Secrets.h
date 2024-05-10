
/*********
Header file

Instructions for AWS - https://how2electronics.com/connecting-esp32-to-amazon-aws-iot-core-using-mqtt/

*********/

#include <pgmspace.h>
 
#define SECRET
#define THINGNAME "ESP_WROOM_32"                      //TO DO: type your thing name between the quotation marks
 
const char WIFI_SSID[] = "UW MPSK";              //TO DO: type the WiFi network name (eg. "UW MPSK")
const char WIFI_PASSWORD[] = "AfN3>S]NS]";          //TO DO: type the WiFi password in the quotes
const char AWS_IOT_ENDPOINT[] = "a4dhn494kwrz-ats.iot.us-west-1.amazonaws.com";       //TO DO: type your AWS IoT endpoint, located on the Settings page of the AWS IoT Console
 
// Amazon Root CA 1                           //TO DO: copy the Amazon Root CA 1 Certificate Key below
static const char AWS_CERT_CA[] PROGMEM = R"EOF(-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----)EOF";
 
// Device Certificate                         //TO DO: copy and paste the Device Certificate Key below
static const char AWS_CERT_CRT[] PROGMEM = R"KEY(-----BEGIN CERTIFICATE-----
MIIDWjCCAkKgAwIBAgIVAKVgVuMGHOYlp4W/ucfQnh/7MFDRMA0GCSqGSIb3DQEB
CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t
IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0yNDA1MDkxODM1
MjJaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh
dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDYHaJ4OHA9aQleGgR2
YBKf6Lj9+0WGSB9+HHow3Dlnwb4LYuQ5NEzVq9BY9rPG/p8X4g0iXgvXxgM0151i
3d5hKPrbm+q4DK3H4mZfvXg/lRQG/NDJDl2xMsFHmEnG83RldfLcv6w4vqFb+6iW
8XFUQ4Pvu1PblFjgO3+kHhFNEJeCDzactR08AfdIZPMw2uI3q2JD2LxsQyboHyoT
FrRYMz87h+WjHB0tsN0Oaigz3sQ07CkPFxFqYXDT8xNWc8HhNV1m8db7Efm7vdqz
rgtH7OxI67XlomDC8ikQYbwkxDFaDLxqHXbR27cW/9KBBnrHU2SLjauI3DLAtPV1
tdQBAgMBAAGjYDBeMB8GA1UdIwQYMBaAFD65xEO1Rmx7i8qCTMpf3paQo5FfMB0G
A1UdDgQWBBQRpJqphN/yyCe+QlXDHL5VB36bODAMBgNVHRMBAf8EAjAAMA4GA1Ud
DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEAn0tWvGjg80R37pfIQq+bdErp
31e0IVpGd0jZIAUK8Q/7W3jeKZUGWvP5zEVgSJqawJHg+7XCNICFOvzEyHwmbnng
pt96BGIQw2LL9utXEtSTEXRBL4lKQSrQ8EteJjMEbLwpgj6AtswjLVTDA07oNLU2
UerirBCxVbLz2ZtYx1+Ccl7tl4vc2ADvaSvQXuqi9elJ0GudIMuI3OJR+PKaO4xL
DIgY/qTtxZISu8+L3bDEDolMJ7SoY30awIpR4cJEDURiTi0QpoRxvS2cCJzGuD5C
Srb0c6B+pE2ftEdpRYaIIf5m7K1cnpFHdMtG+iuTvevi4dA6yCXVwTi+1LAiZg==
-----END CERTIFICATE-----
)KEY";
 
// Device Private Key                         //TO DO: copy and paste the Device Private Key below
static const char AWS_CERT_PRIVATE[] PROGMEM = R"KEY(-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEA2B2ieDhwPWkJXhoEdmASn+i4/ftFhkgffhx6MNw5Z8G+C2Lk
OTRM1avQWPazxv6fF+INIl4L18YDNNedYt3eYSj625vquAytx+JmX714P5UUBvzQ
yQ5dsTLBR5hJxvN0ZXXy3L+sOL6hW/uolvFxVEOD77tT25RY4Dt/pB4RTRCXgg82
nLUdPAH3SGTzMNriN6tiQ9i8bEMm6B8qExa0WDM/O4floxwdLbDdDmooM97ENOwp
DxcRamFw0/MTVnPB4TVdZvHW+xH5u73as64LR+zsSOu15aJgwvIpEGG8JMQxWgy8
ah120du3Fv/SgQZ6x1Nki42riNwywLT1dbXUAQIDAQABAoIBABCQXOZ39OpQBZ7H
wbXM8UB+VsdondCH8Jn61sat6lxwdf9rafCUeJsb2xEmf2ruEwWClt0edivoilBf
gjmUpbmuqWNxlNDcb+lVCeGmpPPCmlFaySnPxxCQwk2Zd3YfLLmmC6ItsqZ6veVN
6YMAKF57+j3Cjs/Sk+lDximvAEZ2rgN+n+mh5kRvhr+pdsd5uAXSOwqWsEn68r4b
lsED8XE0QXUm02lkLP6BCyeqeyK7xE8ePbIgDqHuhxi+oQU/DZX74bEyq+7al6sF
PA7GDrBgoFZHpk+tOpZUcM31JVHJhgJ7nfk3O6u2S5tCOo3CQc+yTHzz17HvUtun
IW74OQECgYEA8nmqSjhBCkn9tBlNLqofvNUyjLdVBdy4tEebsA/5qT961QgQZ+zo
HcyF7ckOIIwweL/vKLPvPpU9Fjl4Eisau+D0jarVwFHgMcnLhOvzdDCTeXen9lo7
gXIwWGlAdRSIPBWgNIUJEr0hQQcBGeTLLqnU3PSI47KkDb4CRhSaxtECgYEA5CuU
Mt1acYU3N/ZBYlGZe75mpX9hEBK2zUq4vh9QFmdGELxYhri4sTb3X/h4lKpwHVbJ
5HWRfM2kU7WOo2Yog0W/EdVEpoKJKQ3hdiSVDlQVk7Hr54kuy/iV0y5LDiNnv3KE
Db0pwFTYQoIQd3oYJyrkgJhGpWU6S7HQ9EeW5jECgYA1fBDL/kyacGe96jbULEKD
7KMR1v6yJFW5rnHxDBUN+CqoAvdbdS5eJFcGdGrhUwr+F5gxaj9Dlre6x0Js/4UO
HDjVZvFvUqAZW2HBGCrDXlOpX3N1K8Ikc4OmfNDzAmxqlHqaGx49O+qx5VGg3I7U
5DFH68imMSluvpLR0omxgQKBgHWKYZjmYO40AyMoD7y3borJphVBjl36VgNhp9QB
S3+PChAbBru4MyTPvRXKaUuklYb3q5+uVRlm0m1xDR7txJcDeg+Uvv3MJx5oESqM
Y+Dyvf7M3Er1Z48Z6wzUP0P0R0DfnFl7hiOXUaTJXfQ+iGy88uOwDkyr3iEaZBtD
OeAxAoGBAM78XMU54vSkHH3S8Tc8G8/UiLBrdZ91gi4q8yCUh9X9q6pkqqaEIoDF
4DmuIOd3x+rWxTaNeS4+GHSwkP1hjoAxhSSGi2odQtwR0Cz5YDpIKkh8z0hKKFwI
8NaXiy3PDTZIgcagwh0NybjijPvi/iFRvFi3QOI7m4pDk9jCEbBW
-----END RSA PRIVATE KEY-----)KEY";