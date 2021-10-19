#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QTimer>
#include <QTime>
#include <QLabel>
#include "settingsdialog.h"
#include "qpaintbox.h"

#define MAXTIMEOUT 6000 //Tiempo maximo del led apagado una vez jugando
#define MAXTIMEIN 2000 //Tiempo maximo del led encendido
#define MINTIMEOUT 1000 //Tiempo minimo del led apagado una vez jugando
#define MINTIMEIN 500 //Tiempo minimo del led encendido
#define INTERVAL 50 //Intervalo de ejecucion de la funcion juego
#define NUMLED 4 //Numero de leds
#define NUMBUT 4 //Numero de botones
#define TIMESTART 1000 //Tiempo para iniciar el juego
#define GAMETIME 30000 //Tiempo maximo de juego

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots: //Funciones privadas
    //Funcion para abrir puerto serie
    void openSerialPort();
    //Funcion para cerrar puerto serie
    void closeSerialPort();
    //Funcion ejecutada cuando el timer llega a 0
    void myTimerOnTime();
    //Funcion de decodificacion de protocolo de recepcion
    void dataRecived();
    //Funcion de decodificacion de datos recibidos
    void decodeData();
    //Funcion que prepara el comando con los datos a enviar
    void sendData();
    //Funcion que dibuja la interfaz
    void drawInter();
    //Funcion para chequear conexion
    void alive();
    //Funcion del juego
    void juegoTopos();
    //Funcion que pide el estado de los leds
    void getLeds();
    //Funcion que pide el estado de los botones
    void getButtons();
    //Funcion de la secuencia de comandos al conectar la bluepill
    void conectar();

private: //Declaracion de variables privadas
    Ui::MainWindow *ui;
    QSerialPort *mySerial;
    QTimer *myTimer, *gameTimer;
    QPaintBox *myPaintBox;
    SettingsDialog *mySettings;
    QLabel *estado;

    //Enumeracion de componentes del protocolo
    typedef enum{
        START,
        HEADER_1,
        HEADER_2,
        HEADER_3,
        NBYTES,
        TOKEN,
        PAYLOAD
    }_eProtocolo;

    _eProtocolo estadoProtocolo;

    //Enumeracion de comandos
    typedef enum{
        ACK = 0x0D,
        ALIVE=0xF0,
        GET_LEDS=0xFB,
        SET_LEDS=0xFC,
        BUTTONEVENT=0xFA,
        GET_BUTTONS=0xFD,
        OTHERS
    }_eID;

    _eID estadoComandos;

    //Enumeracion de estados del juego
    typedef enum{
        LOBBY,
        GAME,
        FINISH
    }_eESTADOS;

    _eESTADOS gameState;

    //Estructura de los comandos
    typedef struct{
        uint8_t timeOut;
        uint8_t cheksum;
        uint8_t payLoad[50];
        uint8_t nBytes;
        uint8_t index;
    }_sDatos ;

    _sDatos rxData, txData;

    //Union para la conversion de datos en bloques de 8 bits
    typedef union {
        float f32;
        int i32;
        unsigned int ui32;        
        unsigned short ui16[2];
        short i16[2];
        uint8_t ui8[4];
        char chr[4];
        unsigned char uchr[4];
    }_udat;

    _udat myWord;

    //Estructura de los botones
    typedef struct{
        uint8_t estado;
        uint8_t event;
        int32_t timeDown;
        int32_t timeDiff;
    }_sTeclas;

    _sTeclas ourButton[4];

    //Banderas
    typedef union{
        struct{
            uint8_t playing :1;
            uint8_t topoOut1 :1;
            uint8_t topoOut2 :1;
            uint8_t topoOut3 :1;
            uint8_t topoOut4 :1;
            uint8_t buttonUp:1;
            uint8_t Reserved :2;

        }individualFlags;
        uint8_t allFlags;
    }_bGeneralFlags;

    volatile _bGeneralFlags myFlags;

    uint16_t lastLeds, ledsRead, buttonsState;
    uint8_t buttonIndex, numLed, ledState, auxButtonState = 0, auxledTopos=0;
    uint16_t waitTime = 0, auxGameTime = 0, delay = 0, randomTimeOut[4], randomTimeIn[4], actualTime[4] = {0,0,0,0};
    uint8_t ledsMask[5]={1,2,4,8,15};
    uint8_t randomLed, aciertos = 0, fallos = 0, errores = 0;
    int puntaje = 0, puntajeMax = 0;
};
#endif // MAINWINDOW_H
