#include "config.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "measurement.h"

class WSDisplay
{
private:
    // Gibt die wie viele Seiten es gibt
    const int maxDisplayPages = 2;

    // Die aktuel dargestellte Seite
    int currentPage = 0;

    // Ermittelt ob das Display ein-/ausgeschaltet ist
    bool displayStandby = false;

    // Display Zeiten für (Display aktualisierung, Display ein-/ausschalten)
    unsigned long millisUpdateDisplay, millisShutdownDisplay;

    // Zeigt auf die Messwerte aus dem Hauptprogramm um diese darstellen zu können
    Measurment *data;

    Adafruit_SSD1306 display;

    /**
     * 
     * Zeigt auf dem Display die Status Seite an.
     * 
     **/
    void displayStatusPage()
    {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(1.4);
        display.setTextColor(BLACK, WHITE);
        display.println("> Status Nachricht\n");
        display.setTextColor(WHITE, BLACK);
        display.println();
        display.println("");
        display.display();
    }

    /**
     * 
     * Zeigt auf dem Display die Luft Werte an.
     * unter anderem die Temperatur, Luftdruck und die Höhe.
     * 
     **/
    void displayAirPage()
    {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(1.4);
        display.setTextColor(BLACK, WHITE);
        display.print("Luft Daten");
        display.setTextColor(WHITE, BLACK);
        display.println();
        display.println();
        display.print(this->data->Temperature);
        display.print((char)247);
        display.println("C");
        display.println(String(this->data->Pressure) + " mBar");
        display.println(String(this->data->Altitute) + " Meter");
        display.display();
    }

    /**
     * 
     * Zeigt auf dem Display Licht Daten an, wie z.b Lux und UV.
     * 
     **/
    void displayLightPage()
    {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(1.4);
        display.setTextColor(BLACK, WHITE);
        display.print("Licht Daten");
        display.setTextColor(WHITE, BLACK);
        display.println();
        display.println();
        display.println(String(this->data->Lux) + " Lux");
        display.println(String(this->data->UV) + " µW/cm² (UV)");
        display.display();
    }

    /**
     * 
     * Aktuallisiert das Display.
     * Wenn durch die 'handleDisplay' Methode der Wert 'displayStandby' wahr ist, dann
     * wird das Display geleert (ausgeschalten), ist dies nicht der Fall wird per
     * Wert 'currentPage' ermittelt welche Seite zuletzt offen war. Diese Seite
     * wird folglich aktualisiert.
     * 
     **/
    void updateDisplay()
    {
        if (displayStandby == true)
        {
            display.clearDisplay();
            display.display();
            return;
        }

        switch (currentPage)
        {
        case 0:
            displayStatusPage();
            break;
        case 1:
            displayAirPage();
            break;
        case 2:
            displayLightPage();
            break;
        }
    }

public:
    /**
     * 
     * Setzt die Display Seite und aktualisiert dieses im Anschluss.
     * 
     * Mögliche Seiten:
     *  0: Status Seite         - Zeigt aktuelle Ereignisse, Fehlermeldungen und Warnungen
     *  1: Luftsensor Seite     - Zeigt Luftwerte (Temperatur, Luftdruck, Höhe)
     *  2: Lichtsensor Seite    - Zeigt Lichtinformationen (Lumen, Ultraviolet in µW/cm²)
     * 
     **/
    void setDisplayPage(int page)
    {
        currentPage = page;
        updateDisplay();
    }

    /**
     * 
     * Setzt alle Display Zeiten zurück.
     * Die 'updateDisplay' Methode wird aufgerufen und sorgt dafür das nach dem
     * aktualisieren des Display's dieses auch wieder eingeschalten wird.
     * 
     **/
    void adjustmentMillis()
    {
        millisUpdateDisplay = millis();
        millisShutdownDisplay = millis();
        displayStandby = false;
        updateDisplay();
    }

    /**
     * 
     * Springt auf die nächste Seite und führt die
     * 'updateDisplay' Methode aus.
     * 
     **/
    void nextPage()
    {
        if (displayStandby == false)
        {
            currentPage++;
        }
        if (currentPage > maxDisplayPages)
        {
            currentPage = 0;
        }

        millisShutdownDisplay = millis();
        displayStandby = false;
        updateDisplay();
    }

    /**
     * 
     * Diese Methode muss in die Hauptroutine vom Programm,
     * sie sorgt für die Überprüfung der Display Zeiten.
     * Jenachdem was in der Konfiguration eingestellt ist wird das
     * display aktualisiert oder ein-/ausgeschalten.
     * Dafür wird zunächst überprüft, wann 'millisShutdownDisplay'
     * oder 'millisUpdateDisplay' zuletzt eine
     * Interaktion hatten. Diese werden bspw. über die Methoden 'nextPage',
     * 'adjustmentMillis', 'setDisplayPage' ausgeübt.
     * 
     **/
    void handleDisplay()
    {
        if ((millis() - millisShutdownDisplay) >= 30e3)
        {
            displayStandby = true;
            updateDisplay();
            millisShutdownDisplay = millis();
        }

        if ((millis() - millisUpdateDisplay) >= 10e3)
        {
            updateDisplay();
            millisUpdateDisplay = millis();
        }
    }

    /**
     * 
     * Erzeugt eine neue Klasseninstanz für das
     * Wetterstations Display.
     * 
     **/
    WSDisplay(Measurment *data) : display(SCREEN_RESET)
    {
        this->data = data;

        // Initialisiere Display, zeige Logo
        const unsigned char logo[] PROGMEM = BKB_LOGO;
        display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
        display.clearDisplay();
        display.drawBitmap(0, 0, logo, 128, 64, WHITE);
        display.display();
        delay(3000);
    }
};
