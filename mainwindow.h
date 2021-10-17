#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QTimer>
#include <QTime>
#include <QLabel>
#include "settingsdialog.h"
#include "qpaintbox.h"

#define MAXTIMEOUT 6000
#define MAXTIMEIN 2000
#define MINTIMEOUT 1000
#define MINTIMEIN 500
#define INTERVAL 50
#define NUMLED 4
#define NUMBUT 4
#define TIMESTART 1000
#define GAMETIME 30000

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openSerialPort();

    void closeSerialPort();

    void myTimerOnTime();

    void dataRecived();

    void decodeData();

    void sendData();

    void drawInter();

    void alive();

    void juegoTopos();

    void getLeds();

    void getButtons();

    void conectar();

private:
    Ui::MainWindow *ui;
    QSerialPort *mySerial;
    QTimer *myTimer, *gameTimer;
    QPaintBox *myPaintBox;
    SettingsDialog *mySettings;
    QLabel *estado;




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

    typedef enum{
        LOBBY,
        GAME,
        FINISH
    }_eESTADOS;

    _eESTADOS gameState;

    typedef struct{
        uint8_t timeOut;
        uint8_t cheksum;
        uint8_t payLoad[50];
        uint8_t nBytes;
        uint8_t index;
    }_sDatos ;

    _sDatos rxData, txData;

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

    typedef struct{
        uint8_t estado;
        uint8_t event;
        int32_t timeDown;
        int32_t timeDiff;
    }_sTeclas;

    _sTeclas ourButton[4];

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
