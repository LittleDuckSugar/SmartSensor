#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#include <ArduinoJson.h>
#define Tread_h

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

// InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
#define INFLUXDB_URL "http://192.168.4.4:8086"
// InfluxDB v2 server ou cloud API authentication token (Use: InfluxDB UI -> Data -> Tokens -> <select token>)
#define INFLUXDB_TOKEN "TRHyr04TTlGZVeyJBjsbUq9H9ScNYc27c4cJ1bAC3psRhThLJbzFevqTZGKgL9vXZT5dOv5cehrhCM-Bz-pXyA=="
// InfluxDB v2 organization id (Use: InfluxDB UI -> User -> About -> Common Ids )
#define INFLUXDB_ORG "Ynov"
// InfluxDB v2 bucket name (Use: InfluxDB UI ->  Data -> Buckets)
#define INFLUXDB_BUCKET "salon"

#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Point d'entré
Point sensorH("DHT22");
Point wifiSensor("wifi");

#include "DHT.h"
#define DHTPIN 5

DHT dht(DHTPIN, DHT22);

const char* ssid = "IoTSmartSensor";
const char* password = "iotroot9";

int pause = 60000;
bool is_temp;
bool is_hum;
bool is_allowed;

String influxdb_url;
String influxdb_token;
String influxdb_org;
String influxdb_bucket;


ESP8266WebServer server(80);
//POST
void sensor() {
  String postBody = server.arg("plain");
  Serial.println(postBody);

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, postBody);
  if (error) {
    Serial.print(F("Error parsing JSON "));
    Serial.println(error.c_str());

    String msg = error.c_str();

    server.send(400, F("text/html"),
                "Error in parsin json body! <br>" + msg);

  } else {
    JsonObject postObj = doc.as<JsonObject>();

    Serial.print(F("HTTP Method: "));
    Serial.println(server.method());

    if (server.method() == HTTP_POST) {
      if (postObj.containsKey("delay") && postObj.containsKey("is_hum") && postObj.containsKey("is_temp") && postObj.containsKey("is_allowed")) {

        pause = doc["delay"];
        is_hum = doc["is_hum"];
        is_temp = doc["is_temp"];
        is_allowed = doc["is_allowed"];
        Serial.println(F("done."));

        DynamicJsonDocument doc(512);
        doc["status"] = "OK";

        Serial.print(F("Stream..."));
        String buf;
        serializeJson(doc, buf);

        server.send(201, F("application/json"), buf);
        Serial.print(F("done."));

      } else {
        DynamicJsonDocument doc(512);
        doc["status"] = "KO";
        doc["message"] = F("No data found, or incorrect!");

        Serial.print(F("Stream..."));
        String buf;
        serializeJson(doc, buf);

        server.send(400, F("application/json"), buf);
        Serial.print(F("done."));
      }
    }
  }
}

//POST
void influxdb() {
  String postBody = server.arg("plain");
  Serial.println(postBody);

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, postBody);
  if (error) {
    Serial.print(F("Error parsing JSON "));
    Serial.println(error.c_str());

    String msg = error.c_str();

    server.send(400, F("text/html"),
                "Error in parsin json body! <br>" + msg);

  } else {
    JsonObject postObj = doc.as<JsonObject>();

    Serial.print(F("HTTP Method: "));
    Serial.println(server.method());

    if (server.method() == HTTP_POST) {
      if (postObj.containsKey("influxdb_url") && postObj.containsKey("influxdb_token") && postObj.containsKey("influxdb_org") && postObj.containsKey("influxdb_bucket")) {


        influxdb_url = doc["influxdb_url"].as<String>();
        influxdb_token = doc["influxdb_token"].as<String>();
        influxdb_org = doc["influxdb_org"].as<String>();
        influxdb_bucket = doc["influxdb_bucket"].as<String>();

        Serial.println(F("done."));

        DynamicJsonDocument doc(512);
        doc["status"] = "OK";

        Serial.print(F("Stream..."));
        String buf;
        serializeJson(doc, buf);

        server.send(201, F("application/json"), buf);
        Serial.print(F("done."));

      } else {
        DynamicJsonDocument doc(512);
        doc["status"] = "KO";
        doc["message"] = F("No data found, or incorrect!");

        Serial.print(F("Stream..."));
        String buf;
        serializeJson(doc, buf);

        server.send(400, F("application/json"), buf);
        Serial.print(F("done."));
      }
    }
  }
}

//Page principal (/) en GET
void systemPage() {
  DynamicJsonDocument doc(512);
  doc["system"] = "http://192.168.4.2/sytem";
  doc["sensor"] = "http://192.168.4.2/sensor";
  doc["network"] = "http://192.168.4.2/network";


  String buf;
  serializeJson(doc, buf);
  server.send(200, F("application/json"), buf);
}

//Page /system  en GET
void timePage() {
  DynamicJsonDocument doc(512);
  doc["current_timezone"] = "CET-1CEST?m3.5.0/33";
  doc["timezones"][0] = "PST8PDT";
  doc["timezones"][1] = "EST5EDT";
  doc["timezones"][2] = "JST-9";
  doc["timezones"][3] = "CEST-1CEST,M3.5.0M10.5.0/3";
  doc["username"] = "admin";
  doc["statut"] = "Running";

  String buf;
  serializeJson(doc, buf);
  server.send(200, F("application/json"), buf);
}
//Lecture Humidité
float cHum() {
  float h = dht.readHumidity();
  Serial.print("humidite ");
  Serial.println(h);
  return h;
}

//Lecture température
float cTemp() {
  float t = dht.readTemperature();
  Serial.print("Temperature ");
  Serial.println(t);
  return t;
}

//Page /sensor en GET
void sensorPage() {

  DynamicJsonDocument doc(512);
  doc ["sensor"] = "DHT22";
  doc ["pin"] = "5";
  doc ["delay"] = pause;
  doc ["is_hum"] = is_hum;
  doc ["is_temp"] = is_temp;
  doc ["is_allowed"] = is_allowed;
  doc ["current_hum"] = cHum();
  doc ["current_temp"] = cTemp();

  sender("Humidity", cHum());

  String buf;
  serializeJson(doc, buf);
  server.send(200, F("application/json"), buf);
}

//Page /network en GET
void networkPage() {
  DynamicJsonDocument doc(512);
  doc ["ssid"] = "IoT-SmartSensor";
  doc ["ip"] = "192.168.43.143";
  doc ["api_port"] = "5543";
  doc ["gateway"] = "192.168.43.1";
  doc ["subnet_mask"] = "255.255.255.0";
  doc ["signal_strengh"] = "-22";


  String buf;
  serializeJson(doc, buf);
  server.send(200, F("application/json"), buf);
}

//Page /influxdb en GET
void influxdbPage() {
  DynamicJsonDocument doc(512);
  doc["influxdb_url"] = influxdb_url;
  doc["influxdb_token"] = "configured";
  doc["influxdb_org"] = influxdb_org;
  doc["influxdb_bucket"] = influxdb_bucket;
  doc["isConnected"] = "false";
  doc["status"] = "setup done but connection failed";

  String buf;
  serializeJson(doc, buf);
  server.send(200, F("application/json"), buf);
}
//Coucou pense a lock ton pc la prochaine fois

// Define routing
void restServerRouting() {
  //    server.on(" / ", HTTP_GET, []() {
  //        server.send(200, F("text / html"),
  //            F("Welcome to the REST Web Server"));
  //    });

  server.on(F("/"), HTTP_GET, systemPage);
  server.on(F("/system"), HTTP_GET, timePage);
  server.on(F("/sensor"), HTTP_GET, sensorPage);
  server.on(F("/network"), HTTP_GET, networkPage );
  server.on(F("/influxdb"), HTTP_GET, influxdbPage );
  server.on(F("/sensor"), HTTP_POST, sensor);
  server.on(F("/influxdb"), HTTP_POST, influxdb);

}

// Manage not found URL
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text / plain", message);
}

void setup(void) {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Set server routing
  restServerRouting();
  // Set not found response
  server.onNotFound(handleNotFound);
  // Start server
  server.begin();
  Serial.println("HTTP server started");

  // Vérification de la connexion au serveur InfluxDB
  if (client.validateConnection()) {
    Serial.print("Connexion a InfluxDB valide: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("Connexion a InfluxDB echouer: ");
    Serial.println(client.getLastErrorMessage());
  }

  dht.begin();
}

// Fonction sender permet d'envoyer des informations a la base de données
void sender(String cHum, float h) {

  // Vide les champs précendant en gardant le point utilisé. Les tags restes présent
  sensorH.clearFields();

  // Enregistre la valeur dans le point
  sensorH.addField(cHum, h);

  // Affiche sur le port série le contenu de ce qui va être envoyé
  Serial.print("Ecriture: ");
  Serial.println(sensorH.toLineProtocol());

  // Si il n'y a pas de WiFi, on essai de se reconnecter
  if ((WiFi.RSSI() == 0) && (wifiMulti.run() != WL_CONNECTED)) {
    Serial.println("Connexion WiFi perdu");
  }

  // On écrit (on envoie) le point dans la base de donnée
  if (!client.writePoint(sensorH)) {
    Serial.print("Ecriture echouer InfluxDB: ");
    Serial.println(client.getLastErrorMessage());
  }
}

void loop() {
  server.handleClient();
}
