/*
 * TFM: Control Habitación Hospital
 *
 * Máster Universitario en Sistemas Electrónicos para Entornos Inteligentes
 *
 * Archivo: guipanel.cpp
 *
 * Descripción: Archivo principal de funcionamiento de aplicación de PC
 *
 * Autor: Jon Varela Gonzalez
*/

#include "guipanel.h"
#include "ui_guipanel.h"

#include <QJsonObject>
#include <QJsonDocument>

#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDateTime>

#include <stdint.h>
#include <stdbool.h>

// Constructor de la clase
GUIPanel::GUIPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GUIPanel)
  , transactionCount(0)
{
    ui->setupUi(this);
    setWindowTitle(tr("Interfaz de Control Hospital"));

    _client=new QMQTT::Client(QHostAddress::LocalHost, 1883);

    connect(_client, SIGNAL(connected()), this, SLOT(onMQTT_Connected())); // Se conecta la señal de conectado al slot interno de conectado
    connect(_client, SIGNAL(received(const QMQTT::Message &)), this, SLOT(onMQTT_Received(const QMQTT::Message &))); // Se conecta la señal de recibido al slot interno de recibido
    connect(_client, SIGNAL(subscribed(const QString &)), this, SLOT(onMQTT_subscribed(const QString &))); // Se conecta la señal de suscrito al slot interno de suscrito
    connect(_client, SIGNAL(error(const QMQTT::ClientError)), this, SLOT(onMQTT_error(QMQTT::ClientError))); // Se conecta la señal de error al slot interno de error

    connected=false; // Todavía no se ha establecido la conexión

    timerEnvioHora = new QTimer(this);

    connect(timerEnvioHora, &QTimer::timeout, this, &GUIPanel::revisarHoraYEnviar);
    timerEnvioHora->start(1000); // Timer cada 1000 ms = 1 segundo

    // Deshabilita los tabs de todas las habitaciones hasta que se establezca la conexion
    ui->Conectar->setEnabled(true); // Habilita el botón de Conectar
    ui->Habitaciones->setEnabled(false);
    ui->Habitacion1->setEnabled(false);
    ui->Habitacion2->setEnabled(false);
    ui->Habitacion3->setEnabled(false);
    ui->Habitacion4->setEnabled(false);
    ui->Habitacion5->setEnabled(false);
    ui->Habitacion6->setEnabled(false);
    ui->Habitacion7->setEnabled(false);
    ui->Habitacion8->setEnabled(false);
    ui->Habitacion9->setEnabled(false);
    ui->Habitacion10->setEnabled(false);
    ui->Habitacion11->setEnabled(false);
    ui->Habitacion12->setEnabled(false);
    ui->Habitacion13->setEnabled(false);
    ui->Habitacion14->setEnabled(false);
    ui->Habitacion15->setEnabled(false);
    ui->Habitacion16->setEnabled(false);

    setupConnections();
}

// Timer que entra cada segundo
void GUIPanel::revisarHoraYEnviar()
{
    QTime horaActual = QTime::currentTime();

    if (horaActual.minute() == 0 && horaActual.second() == 0)
    {
        SendMessageHora(1, horaActual);

        QString carpetaBase = "C:/Historico_Habitaciones";

        QString fechaSubcarpeta = QDate::currentDate().toString("yyyy_MM");
        QString carpetaCompleta = carpetaBase + "/" + fechaSubcarpeta;

        QDir dir;
        if (!dir.exists(carpetaCompleta)) {
            dir.mkpath(carpetaCompleta);
        }

        for (int idx = 0; idx < NUM_HABITACIONES; ++idx)
        {
            // Si ha habido comunicación con los datos de los sensores genera el histórico para cada hora
            if (cantidadMuestras[idx] > 0)
            {
                double mediaTemperatura = sumaTemperatura[idx] / cantidadMuestras[idx];
                double mediaHumedad = sumaHumedad[idx] / cantidadMuestras[idx];

                QString nombreArchivo = QString("Historico_Habitacion%1_%2.csv")
                                            .arg(idx + 1)
                                            .arg(fechaSubcarpeta);
                QString rutaCompleta = carpetaCompleta + "/" + nombreArchivo;

                QFile file(rutaCompleta);
                bool nuevoArchivo = false;

                if (!QFile::exists(rutaCompleta)) {
                    nuevoArchivo = true;
                }

                if (file.open(QIODevice::Append | QIODevice::Text)) {
                    QTextStream out(&file);

                    if (nuevoArchivo) {
                        out << "Fecha,Temperatura,Humadad\n";
                    }

                    out << QDateTime::currentDateTime().toString(Qt::ISODate)
                        << "," << QString::number(mediaTemperatura, 'f', 2)
                        << "," << QString::number(mediaHumedad, 'f', 2)
                        << "\n";

                    file.close();
                }

                // Resetear acumuladores de esta habitación
                sumaTemperatura[idx] = 0;
                sumaHumedad[idx] = 0;
                cantidadMuestras[idx] = 0;
            }
        }
    }
}

// Destructor de la clase
GUIPanel::~GUIPanel()
{
    delete ui;
}

// Se ha conectado al cliente MQTT
void GUIPanel::onMQTT_Connected()
{
    // Se habilitan todas las funcionalidades una vez conectado
    ui->Habitaciones->setEnabled(true);
    // Deshabilita el botón de Conectar
    ui->Conectar->setEnabled(false);

    connected=true; // Ya se ha conectado

    ui->Conectado->setText(tr("PC Conectado")); // Se establece el texto de que la aplicación se ha conectado

    _client->subscribe("/TFM/jvg/ESP/#",0);  // Se suscribe a todos los Topics que contengan /TFM/jvg/ESP/

    QJsonObject objeto_json;

    objeto_json["PC"] = "Conectado";

    QJsonDocument mensaje(objeto_json);

    QMQTT::Message msg(0,"/TFM/jvg/PC", mensaje.toJson(),0,1);

    _client->publish(msg);
}

// Se desea conectar al cliente MQTT
void GUIPanel::startClient()
{
    _client->setWillTopic("/TFM/jvg/PC");

    _client->setWillRetain((bool)1);

    QJsonObject objeto_json;

    objeto_json["PC"] = "Desconectado";

    QJsonDocument mensaje(objeto_json);

    _client->setWillMessage(mensaje.toJson());

    _client->setHostName("broker.emqx.io");
    _client->setPort(1883);
    _client->setKeepAlive(300);
    _client->setCleanSession(true);
    _client->connectToHost();
}

// Se suscribe al topic definido
void GUIPanel::onMQTT_subscribed(const QString &topic)
{

}

// Respuesta ante ERRORES MQTT
void GUIPanel::onMQTT_error(QMQTT::ClientError err)
{
    QString errInfo;
    switch(err) {
    //  0	The connection was refused by the peer (or timed out).
    case QAbstractSocket::ConnectionRefusedError:
        errInfo = tr("Connection Refused");
        break;
    //	1	The remote host closed the connection. Note that the client socket (i.e., this socket) will be closed after the remote close notification has been sent.
    case QAbstractSocket::RemoteHostClosedError:
        errInfo = tr("Remote Host Closed");
        break;
    //	2	The host address was not found.
    case QAbstractSocket::HostNotFoundError:
        errInfo = tr("Host Not Found Error");
        break;
    // 	3	The socket operation failed because the application lacked the required privileges.
    case QAbstractSocket::SocketAccessError:
        errInfo = tr("Socket Access Error");
        break;
    // 	4	The local system ran out of resources (e.g., too many sockets).
    case QAbstractSocket::SocketResourceError:
        errInfo = tr("Socket Resource Error");
        break;
    // 	5	The socket operation timed out.
    case QAbstractSocket::SocketTimeoutError:
        errInfo = tr("Socket Timeout Error");
        break;
    default:
        errInfo = tr("Socket Error");
    }
    ui->Conectado->setText(tr("PC Desconectado")); // Se establece el texto de que la aplicación se ha conectado
    ui->Error->setText(tr("%1").arg(errInfo)); // Se escribe el error que ha saltado
    activateRunButton(); // Activa el botón RUN
}

// Cuando se recibe una nueva medición de una variable
void GUIPanel::nuevaMedicion(int habitacion, int temperatura, int humedad)
{
    if (habitacion >= 1 && habitacion <= NUM_HABITACIONES)
    {
        int idx = habitacion - 1; // Convertir 1-16 a 0-15
        sumaTemperatura[idx] += temperatura;
        sumaHumedad[idx] += humedad;
        cantidadMuestras[idx]++;
    }
}

// Función cuando se recibe un mensaje MQTT
void GUIPanel::onMQTT_Received(const QMQTT::Message &message)
{
    QString topic = message.topic();
    QRegularExpression regex("^/TFM/jvg/ESP/Habitacion(\\d+)$"); // Busca Habitacion seguido de un número
    QRegularExpressionMatch match = regex.match(topic);

    if (match.hasMatch() && connected)
    {
        int habitacion = match.captured(1).toInt(); // Captura el número de habitación

        if (habitacion >= 1 && habitacion <= 16) // Sólo se aceptan habitaciones entre 1 y 16
        {
            QJsonParseError error;
            QJsonDocument mensaje = QJsonDocument::fromJson(message.payload(), &error);

            if ((error.error == QJsonParseError::NoError) && (mensaje.isObject()))
            {
                QJsonObject objeto_json = mensaje.object();

                QJsonValue entradahabitacion = objeto_json["Habitacion"];
                QJsonValue entradaTemperatura = objeto_json["Temperatura"];
                QJsonValue entradaHumedad = objeto_json["Humedad"];
                QJsonValue entradaLlamadaPaciente = objeto_json["LlamadaPaciente"];
                QJsonValue entradaLlamadaPacienteRuido = objeto_json["LlamadaPacienteRuido"];
                QJsonValue entradaPresencia = objeto_json["Presencia"];

                if (entradahabitacion.isString() && entradahabitacion.toString() == "conectado")
                {
                    this->findChild<QWidget*>(QString("Habitacion%1").arg(habitacion))->setEnabled(true);
                    this->findChild<QLabel*>(QString("Conectado%1").arg(habitacion))->setText(tr("Habitacion %1 Conectada").arg(habitacion));

                    if (this->findChild<QCheckBox*>(QString("ActuarTermostato%1").arg(habitacion))->isChecked())
                        SendMessageActuarTermostato(habitacion);

                    if (this->findChild<QCheckBox*>(QString("ActuarPersianas%1").arg(habitacion))->isChecked())
                        SendMessageActuarPersianas(habitacion);

                    if (this->findChild<QCheckBox*>(QString("ActuarLlamadaRuido%1").arg(habitacion))->isChecked())
                        SendMessageActuarRuido(habitacion);

                    if (this->findChild<QCheckBox*>(QString("ActuarLEDRuido%1").arg(habitacion))->isChecked())
                        SendMessageActuarLEDRuido(habitacion);

                    if (this->findChild<QCheckBox*>(QString("PresenciaPC%1").arg(habitacion))->isChecked())
                        SendMessagePresenciaPC(habitacion);

                    QTime horaActual = QTime::currentTime();
                    SendMessageHora(habitacion, horaActual);
                }

                if (entradahabitacion.isString() && entradahabitacion.toString() == "desconectado")
                {
                    this->findChild<QLabel*>(QString("Conectado%1").arg(habitacion))->setText(tr("Habitacion %1 Desconectada").arg(habitacion));
                    this->findChild<QWidget*>(QString("Habitacion%1").arg(habitacion))->setEnabled(false);
                }

                if (entradaTemperatura.isDouble())
                {
                    this->findChild<QLCDNumber*>(QString("Temperatura%1").arg(habitacion))->display(entradaTemperatura.toInt());
                }

                if (entradaHumedad.isDouble())
                {
                    this->findChild<QLCDNumber*>(QString("Humedad%1").arg(habitacion))->display(entradaHumedad.toInt());
                    nuevaMedicion(habitacion, entradaTemperatura.toInt(), entradaHumedad.toInt());
                }

                if (entradaLlamadaPaciente.isBool())
                {
                    this->findChild<Led*>(QString("LlamadaPaciente%1").arg(habitacion))->setChecked(entradaLlamadaPaciente.toBool());
                }

                if (entradaLlamadaPacienteRuido.isBool())
                {
                    this->findChild<Led*>(QString("LlamadaPacienteRuido%1").arg(habitacion))->setChecked(entradaLlamadaPacienteRuido.toBool());
                }

                if (entradaPresencia.isBool())
                {
                    this->findChild<Led*>(QString("Presencia%1").arg(habitacion))->setChecked(entradaPresencia.toBool());

                    if (entradaPresencia.toBool() == 0)
                    {
                        QCheckBox *presenciaPC = this->findChild<QCheckBox*>(QString("PresenciaPC%1").arg(habitacion));
                        bool previousBlockState = presenciaPC->blockSignals(true);
                        presenciaPC->setChecked(entradaPresencia.toBool());
                        presenciaPC->blockSignals(previousBlockState);
                    }
                }
            }
        }
    }
}

// Función para habilitar el botón para conectar
void GUIPanel::activateRunButton()
{
    ui->Conectar->setEnabled(true);
}

// Si se pulsa el botón de INICIO
void GUIPanel::on_Conectar_clicked()
{
    startClient();
}

// Función para mandar el mensaje de ActuarTermostato
void GUIPanel::SendMessageActuarTermostato(int Numero_Habitacion)
{

    if (connected){

        QJsonObject objeto_json;

        // Construir el nombre según el número de habitación
        QString nombreWidget = QString("ActuarTermostato%1").arg(Numero_Habitacion);

        // Buscar el checkbox por nombre
        QCheckBox* checkBox = this->findChild<QCheckBox*>(nombreWidget);

        objeto_json["ActuarTermostato"] = static_cast<uint8_t>(checkBox->isChecked());

        QJsonDocument mensaje(objeto_json);

        // Construir el topic según el número de habitación
        QString topic = QString("/TFM/jvg/PC/Habitacion%1").arg(Numero_Habitacion);

        QMQTT::Message msg(0, topic, mensaje.toJson());
        _client->publish(msg);

    }

}

// Función para mandar el mensaje de ActuarRuido
void GUIPanel::SendMessageActuarRuido(int Numero_Habitacion)
{

    if (connected){

        QJsonObject objeto_json;

        // Construir el nombre según el número de habitación
        QString nombreWidget = QString("ActuarLlamadaRuido%1").arg(Numero_Habitacion);

        // Buscar el checkbox por nombre
        QCheckBox* checkBox = this->findChild<QCheckBox*>(nombreWidget);

        objeto_json["ActuarRuido"] = static_cast<uint8_t>(checkBox->isChecked());

        QJsonDocument mensaje(objeto_json);

        // Construir el topic según el número de habitación
        QString topic = QString("/TFM/jvg/PC/Habitacion%1").arg(Numero_Habitacion);

        QMQTT::Message msg(0, topic, mensaje.toJson());
        _client->publish(msg);

    }

}

// Función para mandar el mensaje de ActuarLEDRuido
void GUIPanel::SendMessageActuarLEDRuido(int Numero_Habitacion)
{

    if (connected){

        QJsonObject objeto_json;

        // Construir el nombre según el número de habitación
        QString nombreWidget = QString("ActuarLEDRuido%1").arg(Numero_Habitacion);

        // Buscar el checkbox por nombre
        QCheckBox* checkBox = this->findChild<QCheckBox*>(nombreWidget);

        objeto_json["ActuarLEDRuido"] = static_cast<uint8_t>(checkBox->isChecked());

        QJsonDocument mensaje(objeto_json);

        // Construir el topic según el número de habitación
        QString topic = QString("/TFM/jvg/PC/Habitacion%1").arg(Numero_Habitacion);

        QMQTT::Message msg(0, topic, mensaje.toJson());
        _client->publish(msg);

    }

}

// Función para mandar el mensaje de ActuarPersianas
void GUIPanel::SendMessageActuarPersianas(int Numero_Habitacion)
{

    if (connected){

        QJsonObject objeto_json;

        // Construir el nombre según el número de habitación
        QString nombreWidget = QString("ActuarPersianas%1").arg(Numero_Habitacion);

        // Buscar el checkbox por nombre
        QCheckBox* checkBox = this->findChild<QCheckBox*>(nombreWidget);

        objeto_json["ActuarPersianas"] = static_cast<uint8_t>(checkBox->isChecked());

        QJsonDocument mensaje(objeto_json);

        // Construir el topic según el número de habitación
        QString topic = QString("/TFM/jvg/PC/Habitacion%1").arg(Numero_Habitacion);

        QMQTT::Message msg(0, topic, mensaje.toJson());
        _client->publish(msg);

    }

}

// Función para mandar el mensaje de PresenciaPC
void GUIPanel::SendMessagePresenciaPC(int Numero_Habitacion)
{

    if (connected){

        QJsonObject objeto_json;

        // Construir el nombre según el número de habitación
        QString nombreWidget = QString("PresenciaPC%1").arg(Numero_Habitacion);

        // Buscar el checkbox por nombre
        QCheckBox* checkBox = this->findChild<QCheckBox*>(nombreWidget);

        objeto_json["PresenciaPC"] = static_cast<uint8_t>(checkBox->isChecked());

        QJsonDocument mensaje(objeto_json);

        // Construir el topic según el número de habitación
        QString topic = QString("/TFM/jvg/PC/Habitacion%1").arg(Numero_Habitacion);

        QMQTT::Message msg(0, topic, mensaje.toJson());
        _client->publish(msg);

    }

}

// Función para mandar el mensaje de Hora Minuto y Segundo
void GUIPanel::SendMessageHora(int Numero_Habitacion, QTime HoraActual)
{

    if (connected){

        QJsonObject objeto_json;
        objeto_json["Hora"] = HoraActual.hour();
        objeto_json["Minuto"] = HoraActual.minute();
        objeto_json["Segundo"] = HoraActual.second();

        QJsonDocument mensaje(objeto_json);

        // Construir el topic según el número de habitación
        QString topic = QString("/TFM/jvg/PC/Habitacion%1").arg(Numero_Habitacion);

        QMQTT::Message msg(0, topic, mensaje.toJson());
        _client->publish(msg);

    }

}

// Para generar las conexiones para las inputs del sistema
void GUIPanel::setupConnections()
{
    // Actuar Termostato
    for (int i = 1; i <= 16; ++i)
    {
        QCheckBox *checkbox = this->findChild<QCheckBox*>(QString("ActuarTermostato%1").arg(i));
        if (checkbox)
            connect(checkbox, &QCheckBox::clicked, this, &GUIPanel::on_ActuarTermostato_clicked);
    }

    // Actuar Persianas
    for (int i = 1; i <= 16; ++i)
    {
        QCheckBox *checkbox = this->findChild<QCheckBox*>(QString("ActuarPersianas%1").arg(i));
        if (checkbox)
            connect(checkbox, &QCheckBox::clicked, this, &GUIPanel::on_ActuarPersianas_clicked);
    }

    // Actuar Llamada Ruido
    for (int i = 1; i <= 16; ++i)
    {
        QCheckBox *checkbox = this->findChild<QCheckBox*>(QString("ActuarLlamadaRuido%1").arg(i));
        if (checkbox)
            connect(checkbox, &QCheckBox::clicked, this, &GUIPanel::on_ActuarLlamadaRuido_clicked);
    }

    // Actuar LED Ruido
    for (int i = 1; i <= 16; ++i)
    {
        QCheckBox *checkbox = this->findChild<QCheckBox*>(QString("ActuarLEDRuido%1").arg(i));
        if (checkbox)
            connect(checkbox, &QCheckBox::clicked, this, &GUIPanel::on_ActuarLEDRuido_clicked);
    }

    // Presencia PC
    for (int i = 1; i <= 16; ++i)
    {
        QCheckBox *checkbox = this->findChild<QCheckBox*>(QString("PresenciaPC%1").arg(i));
        if (checkbox)
            connect(checkbox, &QCheckBox::clicked, this, &GUIPanel::on_PresenciaPC_clicked);
    }
}


// Slot para Actuar Termostato
void GUIPanel::on_ActuarTermostato_clicked()
{
    QCheckBox *checkbox = qobject_cast<QCheckBox*>(sender());
    if (checkbox)
    {
        QString name = checkbox->objectName();
        QRegularExpression re("^ActuarTermostato(\\d+)$");
        QRegularExpressionMatch match = re.match(name);

        if (match.hasMatch())
        {
            int habitacion = match.captured(1).toInt();
            SendMessageActuarTermostato(habitacion);
        }
    }
}

// Slot para Actuar Persianas
void GUIPanel::on_ActuarPersianas_clicked()
{
    QCheckBox *checkbox = qobject_cast<QCheckBox*>(sender());
    if (checkbox)
    {
        QString name = checkbox->objectName();
        QRegularExpression re("^ActuarPersianas(\\d+)$");
        QRegularExpressionMatch match = re.match(name);

        if (match.hasMatch())
        {
            int habitacion = match.captured(1).toInt();
            SendMessageActuarPersianas(habitacion);
        }
    }
}

// Slot para Actuar Llamada Ruido
void GUIPanel::on_ActuarLlamadaRuido_clicked()
{
    QCheckBox *checkbox = qobject_cast<QCheckBox*>(sender());
    if (checkbox)
    {
        QString name = checkbox->objectName();
        QRegularExpression re("^ActuarLlamadaRuido(\\d+)$");
        QRegularExpressionMatch match = re.match(name);

        if (match.hasMatch())
        {
            int habitacion = match.captured(1).toInt();
            SendMessageActuarRuido(habitacion);
        }
    }
}

// Slot para Actuar LED Ruido
void GUIPanel::on_ActuarLEDRuido_clicked()
{
    QCheckBox *checkbox = qobject_cast<QCheckBox*>(sender());
    if (checkbox)
    {
        QString name = checkbox->objectName();
        QRegularExpression re("^ActuarLEDRuido(\\d+)$");
        QRegularExpressionMatch match = re.match(name);

        if (match.hasMatch())
        {
            int habitacion = match.captured(1).toInt();
            SendMessageActuarLEDRuido(habitacion);
        }
    }
}

// Slot para Presencia PC
void GUIPanel::on_PresenciaPC_clicked()
{
    QCheckBox *checkbox = qobject_cast<QCheckBox*>(sender());
    if (checkbox)
    {
        QString name = checkbox->objectName();
        QRegularExpression re("^PresenciaPC(\\d+)$");
        QRegularExpressionMatch match = re.match(name);

        if (match.hasMatch())
        {
            int habitacion = match.captured(1).toInt();
            SendMessagePresenciaPC(habitacion);
        }
    }
}
