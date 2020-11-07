/******************************************************************
  Fork of :
  L'ESP32-CAM présente une page web qui permet de prendre des photos
  et de les enregistrer sur une carte SD, de visionner les photos
  déjà présentes sur la carte, et de supprimer les photos non-désirées.
  Pour plus d'informations:
  http://electroniqueamateur.blogspot.com/2020/07/esp32-cam-gestion-distance-de-la-carte.html
******************************************************************/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include "FS.h"       // manipulation de fichiers
#include "SD_MMC.h"   // carte SD
#include "esp_camera.h" // caméra!

// WIFI SSID
const char* ssid = "partageoir";

WebServer server(80);

#define LEDrouge 33 // LED rouge: GPIO 33
#define LEDblanche 4 // LED blanche: GPIO 4
#define canalPWM 7 // un canal PWM disponible

static bool cartePresente = false;

int numero_fichier = 0; // numéro de la photo (nom du fichier)

// prise de la photo et création du fichier jpeg

void SendHTML_Header() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  String   webpage = "";
  server.sendContent(webpage);

}

void enregistrer_photo (void)
{
  char adresse[40] = ""; // chemin d'accès du fichier .jpeg
  camera_fb_t * fb = NULL; // frame buffer

  String message = "Pic ?\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  String  myString ;
  myString = server.arg(1);
  int myStringLength = myString.length() + 1;

  char FName[myStringLength];
  myString.toCharArray(FName, myStringLength);

  int result = atoi(FName);

  //  message += "\n" + FName + "\n";
  // prise de la photo

  for (uint8_t i = 0; i < server.args(); i++) {
    Serial.println("\nArgs\nNAME: " + server.argName(i) + "\nVALUE:  " + server.arg(i) + "\n");
  }


  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Echec de la prise de photo.");
    return;
  }
  Serial.println("startdate");
  numero_fichier = numero_fichier + 1;


  // enregitrement du fichier sur la carte SD
  sprintf (adresse, "/%s_%d.jpg", FName, numero_fichier);
  //Serial.println("File to be saved as: "+adresse);
  fs::FS &fs = SD_MMC;
  File file = fs.open(adresse, FILE_WRITE);

  if (!file) {
    Serial.println("Echec lors de la creation du fichier: ");
    Serial.println(adresse);
  }
  else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.printf("Fichier enregistre: %s\n", adresse);
  }
  file.close();
  esp_camera_fb_return(fb);

  // on affiche un message de confirmation
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);

  server.send(200,  "text/html", "");

  //WiFiClient client = server.client();
  server.sendContent(message);
  server.sendContent("<h1>Une nouvelle photo a &eacute;t&eacute; prise.</h1>");
  server.sendContent("<p><a href = / > Retour &agrave; la liste des fichiers </a></p>");
  SendHTML_Stop();

}


void returnOK() {
  server.send(200, "text/plain", "");
}

void returnFail(String msg) {
  server.send(500, "text/plain", msg + "\r\n");
}

// Affichage d'un fichier présent sur la carte
bool loadFromSdCard(String path) {
  String dataType = "text/plain";

  if (path.endsWith("/")) {
    if (path == "/") {
      printDirectory(path);
    } else {
      int lastIndex = path.length() - 1;
      path.remove(lastIndex);

      printDirectory(path);
      Serial.println("loadFromSdCard - Opening: ");
      Serial.println(path);
    }
  }
  else {

    if (path.endsWith(".src")) {
      path = path.substring(0, path.lastIndexOf("."));
    } else if (path.endsWith(".htm")) {
      dataType = "text/html";
    } else if (path.endsWith(".md")) {
      dataType = "text/html";
    } else if (path.endsWith(".css")) {
      dataType = "text/css";
    } else if (path.endsWith(".js")) {
      dataType = "application/javascript";
    } else if (path.endsWith(".png")) {
      dataType = "image/png";
    } else if (path.endsWith(".gif")) {
      dataType = "image/gif";
    } else if (path.endsWith(".jpg")) {
      dataType = "image/jpeg";
    } else if (path.endsWith(".ico")) {
      dataType = "image/x-icon";
    } else if (path.endsWith(".xml")) {
      dataType = "text/xml";
    } else if (path.endsWith(".pdf")) {
      dataType = "application/pdf";
    } else if (path.endsWith(".zip")) {
      dataType = "application/zip";
    }

    fs::FS &fs = SD_MMC;

    File dataFile = fs.open(path.c_str());

    if (!dataFile) {
      return false;
    }

    if (server.hasArg("download")) {
      dataType = "application/octet-stream";
    }

    if (server.streamFile(dataFile, dataType) != dataFile.size()) {
      Serial.println("Sent less data than expected!");
    }

    dataFile.close();

  }
  return true;
}


// utilisé lors de la suppression d'un fichier
void deleteRecursive(String path) {

  fs::FS &fs = SD_MMC;
  File file = fs.open((char *)path.c_str());
  if (!file.isDirectory()) {
    file.close();
    fs.remove((char *)path.c_str());
    return;
  }

  file.rewindDirectory();
  while (true) {
    File entry = file.openNextFile();
    if (!entry) {
      break;
    }
    String entryPath = path + "/" + entry.name();
    if (entry.isDirectory()) {
      entry.close();
      deleteRecursive(entryPath);
    } else {
      entry.close();
      fs.remove((char *)entryPath.c_str());
    }
    yield();
  }

  fs.rmdir((char *)path.c_str());
  file.close();
}

// suppression d'un fichier

void handleDelete() {

  fs::FS &fs = SD_MMC;

  if (server.args() == 0) {
    return returnFail("Mauvais arguments?");
  }
  String path = server.arg(0);
  if (path == "/" || !fs.exists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }
  deleteRecursive(path);

  // on affiche un message de confirmation
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200,  "text/html", "");
  server.sendContent("<h1>Le fichier a &eacute;t&eacute; supprim&eacute;</h1>");
  server.sendContent("<p><a href = / > Retour &agrave; la liste des fichiers </a></p>");
  SendHTML_Stop();

}


void addLog() {

  fs::FS &fs = SD_MMC;
  File fileToAppend = fs.open("/log.txt", FILE_APPEND);
  String mystate = server.arg("log");
  String todate = server.arg("date");

  if (!fileToAppend) {
    String myString = "Welcome!\n";
    int myStringLength = myString.length() + 1;
    char myChar[myStringLength];
    myString.toCharArray(myChar, myStringLength);

    Serial.println("There was an error opening the file for appending");
    File newFile = fs.open("/log.txt", FILE_WRITE);
    newFile.write(*myChar);
    newFile.close();
    return;
  } else {
    if (mystate) {
      if (todate) {
        int myStringLength = mystate.length() + 1;
        char myChar[myStringLength];
        mystate.toCharArray(myChar, myStringLength);
        fileToAppend.print(todate);
        fileToAppend.print(": ");
        if (fileToAppend.println(mystate)) {
          Serial.println("File content was appended");
        } else {
          Serial.println("File append failed");
        }
        fileToAppend.close();
      }
    }
  }
    server.send(200,  "text/html", "");

  server.sendContent("<p><h1>Messages in the log</h1><p>");

  File fileToRead = fs.open("/log.txt");

  if (!fileToRead) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.println("File Content:");

  while (fileToRead.available()) {

    //Serial.write(fileToRead.read());
    server.sendContent(fileToRead.readString());
    server.sendContent("<p>");
  }

  fileToRead.close();
  SendHTML_Stop();
}



// Affichage du contenu de la carte
void printDirectory(String path) {

  fs::FS &fs = SD_MMC;
  //String path = "/";
  Serial.println("Opening v2: ");
  Serial.println(path);


  File dir = fs.open((char *)path.c_str());
  //path = String();
  if (!dir.isDirectory()) {
    dir.close();
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200,  "text/html", "");
    server.sendContent("<h1>Prise de photo</h1><p><b>");
    server.sendContent(path);
    server.sendContent("<p>Pas un repertoire</b>");
    Serial.println("Pas un repertoire");
    return returnFail("PAS UN REPERTOIRE");

  }

  dir.rewindDirectory();
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  //SendHTML_Header();
  server.send(200, "text/html", "");
  server.sendContent("<h1>Prise de photo</h1>");
  server.sendContent("<br> <form action='/clic' method='GET'> ");
  server.sendContent("<INPUT type='hidden' type='text' name='datetime' id='todayDateTime' value='test1'>"); //
  server.sendContent("<INPUT type='hidden' type='text' name='today' id='todayDate' value='test2'>"); //
  server.sendContent("<INPUT type='submit' value='Prendre une photo'></form><br> ");

  server.sendContent("<br> <form action='/log' method='GET'> ");
  server.sendContent("<INPUT type='text' type='text' name='log' id='log' value='Un message interessant..'>"); //
  server.sendContent("<INPUT type='hidden' type='text' name='date' id='todayDate2' value='Un message interessant..'>"); //
  server.sendContent("<INPUT type='submit' value='Laisser un mot'></form><br> ");

  server.sendContent("<script type='text/javascript'>");
  server.sendContent("var date = new Date();");
  server.sendContent("var currentDate = date.toISOString().slice(0,10).replace('-','').replace('-','');");
  server.sendContent("var currentTime = date.getHours() + '' + date.getMinutes();");
  server.sendContent("document.getElementById('todayDateTime').value = currentDate+'-'+currentTime;");
  server.sendContent("document.getElementById('todayDate').value = currentDate;");
  server.sendContent("document.getElementById('todayDate2').value = currentDate;");
  server.sendContent("</script>");

  server.sendContent("<h1>Contenu du partageoir</h1><p>");
  server.sendContent("<table style='width:100%'><thead><tr><th>File</th>");

  int val = 0;      // variable to store the read value
  val = digitalRead(LEDrouge);


  if ( val == LOW) {
    server.sendContent("<th>Action</th>");

  }
  server.sendContent("</thead><tbody>");


  for (int cnt = 0; true; ++cnt) {
    File entry = dir.openNextFile();
    if (!entry) {
      break;
    }

    String output;

    output += "<tr><td><a href = ";
    output += entry.name();
    if (entry.isDirectory()) {
      output += "/";
    }
    output += "> ";
    output += entry.name();


    output += "</a>  ";

    if ( val == LOW) {
      output += "</td><td><a href = '/delete?url=";
      output += entry.name();
      output += "'> [Supprimer] </a>";
    }

    output += "</td></tr>";

    server.sendContent(output);
    entry.close();
  }
  dir.close();
  server.sendContent("</table></tbody>");
  SendHTML_Stop();
}

void SendHTML_Stop() {
  server.sendContent("<p>&nbsp<p><center><a href='/'>Back</a></center>");
  server.client().stop(); // Stop is needed because no content length was sent
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void handle_write() {

  Serial.println("Changing GPIO 33 state.");
  String mystate = server.arg("admin");
  String reply = "Bad argument";

  if (mystate == "1")
  {
    pinMode(LEDrouge, OUTPUT); // GPIO 33 réglée en sortie
    digitalWrite(LEDrouge, LOW);

    reply = "OK";

  } else if (mystate == "0")
  {
    pinMode(LEDrouge, OUTPUT); // GPIO 33 réglée en sortie
    digitalWrite(LEDrouge, HIGH);
    reply = "OK";
  }

  server.send(200, "text/html", reply);
  SendHTML_Stop();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// on tente d'afficher le fichier demandé. Sinon, message d'erreur
void handleNotFound() {
  if (cartePresente && loadFromSdCard(server.uri())) {
    return;
  }
  String message = "Carte SD non detectee ou action imprevue\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " NAME:" + server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  Serial.print(message);
  SendHTML_Stop();
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void setup(void) {

  pinMode(LEDrouge, OUTPUT); // GPIO 33 réglée en sortie
  digitalWrite(LEDrouge, HIGH); // desactivating LED

  // définition des broches de la caméra pour le modèle AI Thinker - ESP32-CAM
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;  //YUV422|GRAYSCALE|RGB565|JPEG
  config.frame_size = FRAMESIZE_VGA;  // QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
  config.jpeg_quality = 10;  // 0-63 ; plus bas = meilleure qualité
  config.fb_count = 2; // nombre de frame buffers

  // initialisation de la caméra
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Echec de l'initialisation de la camera, erreur 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();

  // connexion au WIFI

  Serial.begin(115200);


  WiFi.softAP(ssid);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // initialisation du web server
  server.on("/delete", HTTP_GET, handleDelete);
  server.on("/log", HTTP_GET, addLog);
  server.on("/clic", HTTP_GET, enregistrer_photo);
  server.on("/write", HTTP_GET, handle_write);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Serveur HTTP en fonction.");

  // initialisation de la carte micro SD
  if (SD_MMC.begin()) {
    uint8_t cardType = SD_MMC.cardType();
    if (cardType != CARD_NONE) {
      Serial.println("Carte SD Initialisee.");
      cartePresente = true;
    }
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void loop(void) {
  server.handleClient();
}
