/*
 * TFM: Control Habitación Hospital
 *
 * Máster Universitario en Sistemas Electrónicos para Entornos Inteligentes
 *
 * Archivo: guipanel.h
 *
 * Descripción: Archivo header de aplicación de PC
 *
 * Autor: Jon Varela Gonzalez
*/

#ifndef GUIPANEL_H
#define GUIPANEL_H

#include <QWidget>
#include <QtSerialPort/qserialport.h>
#include "qmqtt.h"

namespace Ui {
class GUIPanel;
}

//QT4:QT_USE_NAMESPACE_SERIALPORT

class GUIPanel : public QWidget
{
    Q_OBJECT
    
public:
    //GUIPanel(QWidget *parent = 0);
    explicit GUIPanel(QWidget *parent = 0);
    ~GUIPanel(); // Da problemas
    
private slots:

    void onMQTT_Received(const QMQTT::Message &message);
    void onMQTT_Connected(void);
    void onMQTT_error(QMQTT::ClientError err);

    void onMQTT_subscribed(const QString &topic);

    void on_Conectar_clicked();

    void revisarHoraYEnviar();

    void on_ActuarTermostato_clicked();
    void on_ActuarPersianas_clicked();
    void on_ActuarLlamadaRuido_clicked();
    void on_ActuarLEDRuido_clicked();
    void on_PresenciaPC_clicked();

private: // funciones privadas
    void startClient();
    void activateRunButton();
    void nuevaMedicion(int habitacion, int temperatura, int humedad);

    void SendMessageActuarTermostato(int Numero_Habitacion);
    void SendMessageActuarRuido(int Numero_Habitacion);
    void SendMessageActuarLEDRuido(int Numero_Habitacion);
    void SendMessageActuarPersianas(int Numero_Habitacion);
    void SendMessagePresenciaPC(int Numero_Habitacion);
    void SendMessageHora(int Numero_Habitacion, QTime HoraActual);

    void setupConnections();

private:
    Ui::GUIPanel *ui;
    int transactionCount;
    QMQTT::Client *_client;
    bool connected;
    QTimer* timerEnvioHora;

    static const int NUM_HABITACIONES = 16;
    int sumaTemperatura[NUM_HABITACIONES] = {0};
    int sumaHumedad[NUM_HABITACIONES] = {0};
    int cantidadMuestras[NUM_HABITACIONES] = {0};

};

#endif // GUIPANEL_H
