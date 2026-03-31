
// ****************************************************************
// Sketch Esp8266 Filesystem Manager spezifisch sortiert Modular(Tab)
// created: Jens Fleischer, 2020-06-08
// last mod: Jens Fleischer, 2020-12-19
// For more information visit: https://fipsok.de
// ****************************************************************
// Hardware: Esp8266
// Software: Esp8266 Arduino Core 2.7.0 - 3.0.2
// Geprüft: von 1MB bis 2MB Flash
// Getestet auf: Nodemcu
/******************************************************************
  Copyright (c) 2020 Jens Fleischer. All rights reserved.

  This file is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This file is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
*******************************************************************/
// Diese Version von LittleFS sollte als Tab eingebunden werden.
// #include <LittleFS.h> #include <ESP8266WebServer.h> müssen im Haupttab aufgerufen werden
// Die Funktionalität des ESP8266 Webservers ist erforderlich.
// "server.onNotFound()" darf nicht im Setup des ESP8266 Webserver stehen.
// Die Funktion "setupFS();" muss im Setup aufgerufen werden.
/**************************************************************************************/

#include <list>
#include <tuple>
#include <FS.h>
#include <LittleFS.h>

extern WebServer server;

const char WARNING[] PROGMEM = R"(<h2>Der Sketch wurde mit "FS:none" kompilliert!)";
const char HELPER[] PROGMEM = R"(<form method="POST" action="/upload" enctype="multipart/form-data">
<input type="file" name="[]" multiple><button>Upload</button></form>Lade die fs.html hoch.)";
bool uploadRejected = false;

void sendResponse() {
  server.sendHeader("Location", "fs.html");
  server.send(303, "message/http");
}

void sendUploadResponse() {
  if (uploadRejected) {
    uploadRejected = false;
    server.send(400, "text/plain", "Invalid upload path");
    return;
  }
  sendResponse();
}

const String formatBytes(size_t const& bytes) {                                        // lesbare Anzeige der Speichergrößen
  return bytes < 1024 ? static_cast<String>(bytes) + " Byte" : bytes < 1048576 ? static_cast<String>(bytes / 1024.0) + " KB" : static_cast<String>(bytes / 1048576.0) + " MB";
}

// Validate LittleFS paths before file access so requests stay inside the mounted filesystem.
bool isSafeLittleFSPath(const String &path) {
  return path.length() > 0 &&
         path.startsWith("/") &&
         path.indexOf("..") == -1 &&
         path.indexOf('\\') == -1 &&
         path.indexOf("//") == -1;
}

// Normalize directory arguments to a rooted LittleFS path without trailing slashes.
String normalizeLittleFSDirectory(String path) {
  if (path.length() == 0) {
    return "/";
  }
  if (!path.startsWith("/")) {
    path = "/" + path;
  }
  while (path.length() > 1 && path.endsWith("/")) {
    path.remove(path.length() - 1);
  }
  return path;
}

// Accept upload names only when they cannot inject path separators or traversal sequences.
bool isSafeUploadName(const String &filename) {
  return filename.length() > 0 &&
         filename.indexOf('/') == -1 &&
         filename.indexOf('\\') == -1 &&
         filename.indexOf("..") == -1;
}

// Replace folder name characters that are disallowed by the filesystem UI with underscores.
String sanitizeFolderName(String folderName) {
  for (auto& ch : folderName) {
    if (ch == '"' || ch == '%' || ch == '&' || ch == '/' || ch == ':' || ch == ';' || ch == '\\') {
      ch = '_';
    }
  }
  return folderName;
}

bool handleList() {                                                                    // Senden aller Daten an den Client
  // Dir dir = LittleFS.openDir("/");
  // using namespace std;
  // using records = tuple<String, String, int>;
  // list<records> dirList;
  // while (dir.next()) {                                                                 // Ordner und Dateien zur Liste hinzufügen
  //   if (dir.isDirectory()) {
  //     uint8_t ran {0};
  //     Dir fold = LittleFS.openDir(dir.fileName());
  //     while (fold.next())  {
  //       ran++;
  //       dirList.emplace_back(dir.fileName(), fold.fileName(), fold.fileSize());
  //     }
  //     if (!ran) dirList.emplace_back(dir.fileName(), "", 0);
  //   }
  //   else {
  //     dirList.emplace_back("", dir.fileName(), dir.fileSize());
  //   }
  // }
  // dirList.sort([](const records & f, const records & l) {                              // Dateien sortieren
  //   if (server.arg(0) == "1") {
  //     return get<2>(f) > get<2>(l);
  //   } else {
  //     for (uint8_t i = 0; i < 31; i++) {
  //       if (tolower(get<1>(f)[i]) < tolower(get<1>(l)[i])) return true;
  //       else if (tolower(get<1>(f)[i]) > tolower(get<1>(l)[i])) return false;
  //     }
  //     return false;
  //   }
  // });
  // dirList.sort([](const records & f, const records & l) {                              // Ordner sortieren
  //   if (get<0>(f)[0] != 0x00 || get<0>(l)[0] != 0x00) {
  //     for (uint8_t i = 0; i < 31; i++) {
  //         if (tolower(get<0>(f)[i]) < tolower(get<0>(l)[i])) return true;
  //         else if (tolower(get<0>(f)[i]) > tolower(get<0>(l)[i])) return false;
  //     }
  //   }
  //   return false;
  // });
  // String temp = "[";
  // for (auto& t : dirList) {
  //   if (temp != "[") temp += ',';
  //   temp += "{\"folder\":\"" + get<0>(t) + "\",\"name\":\"" + get<1>(t) + "\",\"size\":\"" + formatBytes(get<2>(t)) + "\"}";
  // }
  // temp += ",{\"usedBytes\":\"" + formatBytes(LittleFS.usedBytes()) +                      // Berechnet den verwendeten Speicherplatz
  //         "\",\"totalBytes\":\"" + formatBytes(LittleFS.totalBytes()) +                   // Zeigt die Größe des Speichers
  //         "\",\"freeBytes\":\"" + (LittleFS.totalBytes() - LittleFS.usedBytes()) + "\"}]";   // Berechnet den freien Speicherplatz
  // server.send(200, "application/json", temp);
  return true;
}

void deleteRecursive(const String &path) {
  // if (LittleFS.remove(path)) {
  //   LittleFS.open(path.substring(0, path.lastIndexOf('/')) + "/", "w");
  //   return;
  // }
  // Dir dir = LittleFS.openDir(path);
  // while (dir.next()) {
  //   deleteRecursive(path + '/' + dir.fileName());
  // }
  // LittleFS.rmdir(path);
}

String getContentType(String filename){
  if(filename.endsWith(F(".htm")))          return F("text/html");
  else if(filename.endsWith(F(".html")))    return F("text/html");
  else if(filename.endsWith(F(".css")))     return F("text/css");
  else if(filename.endsWith(F(".js")))      return F("application/javascript");
  else if(filename.endsWith(F(".json")))    return F("application/json");
  else if(filename.endsWith(F(".png")))     return F("image/png");
  else if(filename.endsWith(F(".svg")))     return F("image/svg+xml ");
  else if(filename.endsWith(F(".gif")))     return F("image/gif");
  else if(filename.endsWith(F(".jpg")))     return F("image/jpeg");
  else if(filename.endsWith(F(".jpeg")))    return F("image/jpeg");
  else if(filename.endsWith(F(".ico")))     return F("image/x-icon");
  else if(filename.endsWith(F(".xml")))     return F("text/xml");
  else if(filename.endsWith(F(".pdf")))     return F("application/x-pdf");
  else if(filename.endsWith(F(".zip")))     return F("application/x-zip");
  else if(filename.endsWith(F(".gz")))      return F("application/x-gzip");
  return F("text/plain");
}

bool handleFile(String &&path) {
  if (server.hasArg("new")) {
    String folderName {sanitizeFolderName(server.arg("new"))};
    String folderPath = "/" + folderName;
    if (isSafeUploadName(folderName) && isSafeLittleFSPath(folderPath)) {
      LittleFS.mkdir(folderPath);
    }
  }
  if (server.hasArg("sort")) return handleList();
  if (server.hasArg("delete")) {
    String deletePath {server.arg("delete")};
    if (!isSafeLittleFSPath(deletePath)) {
      server.send(400, "text/plain", "Invalid path");
      return true;
    }
    deleteRecursive(deletePath);
    sendResponse();
    return true;
  }

  // String temp = "";
  // File rootDir = LittleFS.open("/");
  // if(!rootDir.isDirectory()) {
  //   Serial.println("rootDir is not a directory");
  // }

  // File f = rootDir.openNextFile();
  // while(f) {
  //   temp += String(f.name()) + "\n";
  //   Serial.println(f.name());

  //   f = rootDir.openNextFile();
  // }
  // server.send(200, "text/plain", temp);
  // return true;

  if (!LittleFS.exists("/fs.html")) server.send(200, "text/html", HELPER);     // ermöglicht das hochladen der fs.html
  if (!isSafeLittleFSPath(path)) {
    server.send(400, "text/plain", "Invalid path");
    return true;
  }
  if (path.endsWith("/")) path += "index.html";
  if (path == "/spiffs.html") sendResponse(); // Vorrübergehend für den Admin Tab
  return LittleFS.exists(path) ? ({File f = LittleFS.open(path, "r"); server.streamFile(f, getContentType(path)); f.close(); true;}) : false;
}

void handleUpload() {                                                                  // Dateien ins Filesystem schreiben
  static File fsUploadFile;
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    uploadRejected = false;
    String uploadDir = normalizeLittleFSDirectory(server.arg("f"));  // fs.html posts uploads to /upload?f=<directory>
    String decodedFilename = server.urlDecode(upload.filename);
    if (decodedFilename.length() > 31) {  // Dateinamen kürzen
      decodedFilename = decodedFilename.substring(decodedFilename.length() - 31, decodedFilename.length());
    }
    if (!isSafeLittleFSPath(uploadDir) || !isSafeUploadName(decodedFilename)) {
      uploadRejected = true;
      Serial.printf("Rejected upload path: %s/%s\n", uploadDir.c_str(), decodedFilename.c_str());
      return;
    }
    printf(PSTR("handleFileUpload Name: /%s\n"), decodedFilename.c_str());
    String uploadPath = uploadDir == "/" ? "/" + decodedFilename : uploadDir + "/" + decodedFilename;
    fsUploadFile = LittleFS.open(uploadPath, "w");
    if (!fsUploadFile) {
      uploadRejected = true;
      return;
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadRejected) {
      return;
    }
    printf(PSTR("handleFileUpload Data: %u\n"), upload.currentSize);
    fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadRejected) {
      return;
    }
    printf(PSTR("handleFileUpload Size: %u\n"), upload.totalSize);
    fsUploadFile.close();
  }
}

void formatFS() {                                                                      // Formatiert das Filesystem
  LittleFS.format();
  sendResponse();
}

void setupFS() {                                                                       // Funktionsaufruf "setupFS();" muss im Setup eingebunden werden
  if (!LittleFS.begin(true)) {  // true parameter formats the filesystem if mounting fails
    Serial.println("LittleFS Mount Failed");
    return;
  }
  Serial.println("LittleFS Mount Successful");
  server.on("/format", formatFS);
  server.on("/upload", HTTP_POST, sendUploadResponse, handleUpload);
  server.onNotFound([]() {
    if (!handleFile(server.urlDecode(server.uri())))
      server.send(404, "text/plain", "FileNotFound");
  });
}
