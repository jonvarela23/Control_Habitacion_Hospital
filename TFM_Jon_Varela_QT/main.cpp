/*
 * TFM: Control Habitación Hospital
 *
 * Máster Universitario en Sistemas Electrónicos para Entornos Inteligentes
 *
 * Archivo: main.cpp
 *
 * Descripción: Archivo inicial de aplicación de PC
 *
 * Autor: Jon Varela Gonzalez
*/

#include <QtWidgets/QApplication>
#include "guipanel.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    GUIPanel w;
    w.show();
    
    return a.exec();
}
