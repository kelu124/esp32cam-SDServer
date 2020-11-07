# esp32cam-partageoir

## Main idea

* Have an ESP32cam act as a partegeoir / sharing platforms.


## Changelog

* Main / Nov02c: basic AP creation, file management.
  * need to toggle admin rights to delete files
  * Datetime provided by browser
  * takes pictures
  * enables a dead-drop chat in ./log.txt

## Experiments 

* Experiments - esp32server
  * Nov01a: Prise de photos et manips de fichiers.
  * Nov01b: lists all, recursively. Longue page si plein de fichiers ( Topkhung ).
  * Nov01c: bugged, loads, sees images, but won't list files 
  * Nov01d: adding capture of image, control of LED, read/write of IOs. 
  * Nov02c: First workable version
  * Nov02c: Hall sensor test

## Ideas

Playing with ESP32 ideas
* Listing files?
  * https://electroniqueamateur.blogspot.com/2020/07/esp32-cam-gestion-distance-de-la-carte.html
* IOs & stuff
  * https://github.com/Ponkabonk/ESP32-CAM-Demo
  * http://www.iotsharing.com/2019/07/how-to-turn-esp-with-sdcard-or-spiffs-a-web-file-server.html
* Interesting
  * https://github.com/boblemaire/ESPAsyncSDWebServer/blob/master/ESPAsyncSDWebServer.ino
* Other stuff
  * https://github.com/Ponkabonk/ESP32-CAM-Demo
  * https://github.com/topkhung/ESP32_Web_ClickToDownload_File_SD_Card/blob/master/ESP32_Web_file_Download.ino


https://github.com/G6EJD/ESP32-8266-File-Upload/blob/master/ESP_File_Download_Upload.ino



DateTime : 
* https://stackoverflow.com/questions/39182400/current-date-and-current-time-in-hidden-fields
* https://stackoverflow.com/questions/15103890/submit-todays-date-as-a-hidden-form-input-value
