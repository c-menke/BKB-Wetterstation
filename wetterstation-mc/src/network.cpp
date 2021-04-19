#include <Arduino.h>
#include <WiFi101.h>
#include "config.h"
#include "utils.cpp"

#ifndef __NETWORK_H_INC__
#define __NETWORK_H_INC__

/**
 * 
 * Struktur für das Sammeln der Sensor Daten.
 * 'sensorId' ist dabei die von der openSenseMap
 * zugeteilte ID des jeweiligen sensors.
 * 
 **/
typedef struct osmMeasurement
{
    const char *sensorId;
    float value;
} osmMeasurement;

class Network
{
private:
    // Ein Array welches Sensor Daten sammelt.
    // Da die Sensor Daten bei jedem Messzyklus gleich bleiben wird
    // das Array lediglich überschrieben und die variable 'num_measurements' hochgezählt
    // oder auf Null zurückgesetzt.
    osmMeasurement measurements[NUM_SENSORS];

    // Der Index der aktuellen Messung, dient als Position in der Array 'measurements'
    // und als Summe der bisherig enthaltenen Messungen.
    uint8_t num_measurements = 0;

    // Der Puffer für das Webrequest. Bei zunehmender Sensor Anzahl sollte dieser Wert
    // angepasst werden.
    char buffer[750];

    // Netwerk SSID und Schlüssel welche per Konstruktor übergeben werden.
    const char *ssid;
    const char *key;

    // Server Adresse, wird über den Konstruktor übergeben.
    const char *serverAddress;

    // WiFi101 client, sorgt für die Verbindung mit dem Wifi Modul.
    WiFiClient client; //WiFiSSLClient client;

    // Aktueller Status des Wifi Modules.
    int status = WL_IDLE_STATUS;

    // Netzwerk Webrequest Zeiten
    unsigned long millisNetworkPost;

    // Letzter Task der ausgeführt wurde, so kann unterschieden werden
    // welche post oder get Methode als letztes ausgeführt wurde.
    // 0: Keine
    // 1: getValuesFromUrl()
    // 2: postMeasurements()
    uint8 lastTask = 0;

    /**
     * 
     * Mit dieser Methode werden die Messungen aus 'measurements'
     * in den Puffer für den Webrequest geschrieben.
     * 
     **/
    void writeMeasurementsToClient()
    {
        for (uint8_t i = 0; i < num_measurements; i++)
        {
            sprintf_P(buffer, PSTR("%s,%9.2f\n"), measurements[i].sensorId,
                      measurements[i].value);

            client.print(buffer);
            DEBUG2(buffer);
        }

        // Setzte Messwerte Index auf 0 zurück
        num_measurements = 0;
    }

    String getValue(String data, char separator, int index)
    {
        int found = 0;
        int strIndex[] = {0, -1};
        int maxIndex = data.length() - 1;

        for (int i = 0; i <= maxIndex && found <= index; i++)
        {
            if (data.charAt(i) == separator || i == maxIndex)
            {
                found++;
                strIndex[0] = strIndex[1] + 1;
                strIndex[1] = (i == maxIndex) ? i + 1 : i;
            }
        }
        return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
    }

public:
    int windradSpeed = 0;
    int windradDirection = 0;
    double pm25 = 0.0;
    double pm10 = 0.0;

    /**
     * 
     * Fügt eine Messung hinzu, benötigt wird die SensorID welche
     * von der openSenseMap zugewiesen wurde und der Wert welcher von
     * einem Sensor zurückgegeben wurde.
     * 
     **/
    void addMeasurement(const char *sensorId, float value)
    {
        measurements[num_measurements].sensorId = sensorId;
        measurements[num_measurements].value = value;
        num_measurements++;
    }

    /**
     * 
     * Setze alle Messungen zurück, aber Achtung! die 'measurements' Array
     * ist davon nicht betroffen, es wird lediglich der index bzw die Position
     * zurückgesetzt.
     * 
     **/
    void clearMeasurmentIndex()
    {
        num_measurements = 0;
    }

    /**
     * 
     * WiFi Verbindung wird mit Netzwerk hergestellt.
     * 
     **/
    void connect()
    {
        WiFi.begin(ssid, key);
    }

    /**
     * 
     * WiFi Verbindung wird getrennt.
     * 
     **/
    void disconnect()
    {
        WiFi.disconnect();
    }

    /**
     * 
     * WiFi Verbindung wird geschlossen und im Anschluss neu aufgebaut.
     * zwischen dem trennen und dem Aufbau liegen 200ms.
     * 
     **/
    void reconnect()
    {
        disconnect();
        delay(200);
        connect();
    }

    void getValuesFromUrl(IPAddress *address, uint16 port)
    {
        bool connected = false;

        DEBUG(F("[Network] (GET) Connecting..."));
        connected = client.connect(*address, port);
        if (connected == true)
        {
            lastTask = 1;

            DEBUG(F("[Network] Connection success, start request..."));
            client.println(F("GET / HTTP/1.1"));
            //client.println(F("Host: 192.168.43.122"));
            client.println(F("Connection: close"));
            client.println();
        }
        else
        {
            DEBUG("Failed to connect...");
        }
    }

    /**
     * 
     * Webrequest wird gestartet, alle Messungen aus 'measurements'
     * werden in den Puffer geladen und als Post Request gesendet.
     * 
     **/
    void postMeasuremnts()
    {
        bool connected = false;

        Serial.println(F("connecting..."));
        connected = client.connectSSL(this->serverAddress, 443);
        if (connected == true)
        {
            lastTask = 2;

            DEBUG(F("Connection successful, transferring..."));
            // Erzeuge Header des HTTP Webrequests
            sprintf_P(buffer,
                      PSTR("POST /boxes/%s/data HTTP/1.1\nHost: %s\nContent-Type: "
                           "text/csv\nConnection: close\nContent-Length: %i\n\n"),
                      SENSEBOX_ID, this->serverAddress, num_measurements * 35);
            DEBUG(buffer);

            // Sende den Header des HTTP Webrequests
            client.print(buffer);

            // Sende die Messergebnisse
            this->writeMeasurementsToClient();

            // Sende am Ende eine Leere Zeile um den Webrequest abzuschließen
            client.println();

            /*while (client.available())
            {
                char c = client.read();
                DEBUG_WRITE(c);
                // Wenn der Server die Verbindung abbricht, stoppe client.
                if (!client.connected())
                {
                    DEBUG();
                    DEBUG(F("disconnecting from server."));
                    client.stop();
                    break;
                }
            }*/

            DEBUG(F("done!"));

            // Setze Index der Messungen auf Null.
            num_measurements = 0;
        }

        if (connected == false) {
            DEBUG(F("connection failed. Restarting System."));
            delay(5000);
            noInterrupts();
            NVIC_SystemReset();
            while (1)
                ;
        }
    }

    /**
     * 
     * Diese Methode gehört in die Hauptroutine des Programms.
     * Sie sorgt dafür das zur richtigen Zeit ein Postrequest ausgelöst
     * wird. Sollte es zu Verbindungsproblemen kommen sorgt diese Methode
     * ebenfalls für das Wiederverbinden mit dem Netzwerk.
     * 
     **/
    void networkHandle(void (*pre)())
    {
        // Bevor neue Tasks abarbeitet werden wird nach Daten
        // werden Daten im Client puffer abarbeitet.
        if (lastTask == 0)
        {
            client.read();
        }

        // getValuesFromUrl()
        if (lastTask == 1)
        {
            boolean content = false;

            while (client.available())
            {
                if (content == true)
                {
                    String value = client.readString();
                    DEBUG(value);
                    this->windradDirection = getValue(value, ',', 1).toInt();
                    this->windradSpeed = getValue(value, ',', 0).toInt();
                    this->pm25 = getValue(value, ',', 0).toDouble();
                    this->pm10 = getValue(value, ',', 0).toDouble();
                    continue;
                }

                char c;
                if ((c = client.read()) == '\r' && (c = client.read()) == '\n')
                {
                    if ((c = client.read()) == '\r' && (c = client.read()) == '\n')
                    {
                        content = true;
                    }
                }

                delay(1);
            }
        }

        // postMeasurements()
        if (lastTask == 2)
        {
            client.read();
        }

        // Wenn der Server die Verbindung abbricht, stoppe client.
        /*if (!client.connected())
        {
            DEBUG();
            DEBUG(F("disconnecting from server."));
            client.stop();
        }*/

        // Behandle Verbindungs Aufgaben
        if ((millis() - millisNetworkPost) >= 60e3)
        {
            DEBUG(F("[Network] Routine started..."));
            if (status == WL_CONNECTED)
            {
                // Führe eine pre Operation aus (z.b für das Sammeln von Daten)
                pre();

                DEBUG(F("[Network] Post data started..."));
                this->postMeasuremnts();
                DEBUG(F("[Network] Post data complete"));
            }
            else if (status == WL_IDLE_STATUS)
            {
                DEBUG(F("[Network] Status is WL_IDLE_STATUS, reconnect..."));
                reconnect();
                DEBUG(F("[Network] Reconnected..."));
            }
            else if (status == WL_CONNECT_FAILED)
            {
                DEBUG(F("[Network] Connection failed..."));
                disconnect();
                status = WL_IDLE_STATUS;
            }
            else if (status == WL_DISCONNECTED)
            {
                DEBUG(F("[Network] Status is WL_DISCONNECTED, reconnect..."));
                reconnect();
            }
            else
            {
                DEBUG(F("[Network] Status will be idle..."));
                disconnect();
                status = WL_IDLE_STATUS;
            }
            DEBUG(F("[Network] routine complete..."));

            millisNetworkPost = millis();
        }
    }

    /**
     * 
     * Initialisiert das WiFi Module, diese Methode sollte vor dem
     * ausführen der 'networkHandle' Methode einmalig ausgeführt werden.
     * 
     **/
    void initialize(const char *ssid, const char *key)
    {
        this->ssid = ssid;
        this->key = key;

        status = WiFi.status();
        if (status == WL_NO_SHIELD)
        {
            DEBUG(F("[Network] WiFi shield no present"));
        }

        while (status != WL_CONNECTED)
        {
            DEBUG(F("==[Network]=="));
            DEBUG2(F("Attempting to connect to SSID: "));
            DEBUG(ssid);
            status = WiFi.begin(this->ssid, this->key);
            DEBUG2(F("Waiting 10 secounds for connection..."));
            delay(10000);
            DEBUG(F("done"));
        }
    }

    /**
     * 
     * Erzeugt eine Klassen Instanz für das einfache übertragen
     * von Messungen an die openSenseMap.
     * 
     **/
    Network(const char *serverAddress)
    {
        this->serverAddress = serverAddress;
    }
};

#endif