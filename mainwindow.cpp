#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    myTimer = new QTimer(this);
    gameTimer = new QTimer(this);
    mySerial = new QSerialPort(this);
    mySettings = new SettingsDialog();

    myPaintBox = new QPaintBox(0,0,ui->widget);
    estado = new QLabel;
    estado->setText("Desconectado............");
    ui->statusbar->addWidget(estado);
    ui->actionDesconectar->setEnabled(false);
    ui->menuALIVE->setEnabled(false);
    ui->screenState->setText("DESCONECTADO");
    ui->screenState->setAlignment(Qt::AlignCenter);
    //ui->widget->setVisible(false);

    estadoProtocolo=START; //Recibe
    gameState = LOBBY;
    myFlags.individualFlags.playing = false;
    myFlags.individualFlags.topoOut1 = false;
    myFlags.individualFlags.topoOut2 = false;
    myFlags.individualFlags.topoOut3 = false;
    myFlags.individualFlags.topoOut4 = false;
    srand(time(NULL));

    estadoComandos=ALIVE; //Envia

    ///Conexión de eventos
    connect(mySerial,&QSerialPort::readyRead,this, &MainWindow::dataRecived ); //Si llega recibir
    connect(myTimer, &QTimer::timeout,this, &MainWindow::myTimerOnTime); //intervalo de tiempo    
    connect(gameTimer,&QTimer::timeout,this,&MainWindow::juegoTopos); //intervalo de tiempo
    connect(ui->actionEscaneo_de_Puertos, &QAction::triggered, mySettings, &SettingsDialog::show); //Esaneo de puerto
    connect(ui->actionConectar,&QAction::triggered,this, &MainWindow::openSerialPort); //Abrir puerto
    connect(ui->actionConectar, &QAction::triggered,this, &MainWindow::conectar);
    connect(ui->actionDesconectar, &QAction::triggered, this, &MainWindow::closeSerialPort); //Cerrar puerto
    connect(ui->actionSalir,&QAction::triggered,this,&MainWindow::close ); //Cerrar programa
    connect(ui->actionALIVE,&QAction::triggered,this,&MainWindow::alive ); //Enviar Alive
    connect(ui->actionGET_BUTTONS,&QAction::triggered,this,&MainWindow::getButtons); //Enviar GET BUTTON
    connect(ui->actionGET_LEDS,&QAction::triggered,this,&MainWindow::getLeds); //ENVIAR GET LEDS


    myTimer->start(50);
    gameTimer->start(INTERVAL);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openSerialPort()
{
    SettingsDialog::Settings p = mySettings->settings();
    //Configuracion de comunicacion
    mySerial->setPortName(p.name);
    mySerial->setBaudRate(p.baudRate);
    mySerial->setDataBits(p.dataBits);
    mySerial->setParity(p.parity);
    mySerial->setStopBits(p.stopBits);
    mySerial->setFlowControl(p.flowControl);
    mySerial->open(QSerialPort::ReadWrite);
    if(mySerial->isOpen()){
        ui->actionConectar->setEnabled(false);
        ui->actionDesconectar->setEnabled(true);
        ui->menuALIVE->setEnabled(true);
        estado->setText(tr("Conectado a  %1 : %2, %3, %4, %5, %6  %7")
                                         .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                                         .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl).arg(p.fabricante));
    }
    else{
        QMessageBox::warning(this,"Menu Conectar","No se pudo abrir el puerto Serie!!!!");
    }
}

void MainWindow::closeSerialPort()
{
    if(mySerial->isOpen()){
        mySerial->close();
        ui->actionDesconectar->setEnabled(false);
        ui->actionConectar->setEnabled(true);
        estado->setText("Desconectado................");
        ui->menuALIVE->setEnabled(false);
    }
    else{
         estado->setText("Desconectado................");
         ui->menuALIVE->setEnabled(false);
    }

}

void MainWindow::myTimerOnTime()
{
    //Si timeout verificar si hay datos para recibir
    if(rxData.timeOut!=0){
        rxData.timeOut--;
    }else{
        estadoProtocolo=START;
    }
}

void MainWindow::dataRecived()
{
    unsigned char *incomingBuffer;
    int count;
    //numero de bytes
    count = mySerial->bytesAvailable();

    if(count<=0)
        return;

    incomingBuffer = new unsigned char[count];

    mySerial->read((char *)incomingBuffer,count);

    QString str="";

    for(int i=0; i<=count; i++){
        if(isalnum(incomingBuffer[i]))
            str = str + QString("%1").arg((char)incomingBuffer[i]);
        else
            str = str +"{" + QString("%1").arg(incomingBuffer[i],2,16,QChar('0')) + "}";
    }

    rxData.timeOut=5;
    for(int i=0;i<count; i++){
        switch (estadoProtocolo) {
            case START:
                if (incomingBuffer[i]=='U'){
                    estadoProtocolo=HEADER_1;
                    rxData.cheksum=0;
                }
                break;
            case HEADER_1:
                if (incomingBuffer[i]=='N')
                   estadoProtocolo=HEADER_2;
                else{
                    i--;
                    estadoProtocolo=START;
                }
                break;
            case HEADER_2:
                if (incomingBuffer[i]=='E')
                    estadoProtocolo=HEADER_3;
                else{
                    i--;
                   estadoProtocolo=START;
                }
                break;
        case HEADER_3:
            if (incomingBuffer[i]=='R')
                estadoProtocolo=NBYTES;
            else{
                i--;
               estadoProtocolo=START;
            }
            break;
            case NBYTES:
                rxData.nBytes=incomingBuffer[i];
               estadoProtocolo=TOKEN;
                break;
            case TOKEN:
                if (incomingBuffer[i]==':'){
                   estadoProtocolo=PAYLOAD;
                    rxData.cheksum='U'^'N'^'E'^'R'^ rxData.nBytes^':';
                    rxData.payLoad[0]=rxData.nBytes;
                    rxData.index=1;
                }
                else{
                    i--;
                    estadoProtocolo=START;
                }
                break;
            case PAYLOAD:
                if (rxData.nBytes>1){
                    rxData.payLoad[rxData.index++]=incomingBuffer[i];
                    rxData.cheksum^=incomingBuffer[i];
                }
                rxData.nBytes--;
                if(rxData.nBytes==0){
                    estadoProtocolo=START;
                    if(rxData.cheksum==incomingBuffer[i]){
//                        ui->textBrowser->setTextColor(Qt::blue);
//                        ui->textBrowser->append(">>MBED-->PC :: RECEIVED ::");
                        decodeData();
                    }
                }
                break;
            default:
                estadoProtocolo=START;
                break;
        }
    }
    delete [] incomingBuffer;
    if(waitTime >= TIMESTART){
        waitTime = 0;
        myFlags.individualFlags.playing = !myFlags.individualFlags.playing;
        return juegoTopos();
    }
}

void MainWindow::decodeData()
{
    QString str="", s="";
    //for(int a=1; a < rxData.index; a++){
    switch (rxData.payLoad[1]) {
        case ALIVE:
            str = "::*ID Válido* (ALIVE)::";
            break;
        case BUTTONEVENT:
            buttonIndex = rxData.payLoad[2];
            ourButton[buttonIndex].estado = !rxData.payLoad[3];
            myWord.ui8[0] = rxData.payLoad[4];
            myWord.ui8[1] = rxData.payLoad[5];
            myWord.ui8[2] = rxData.payLoad[6];
            myWord.ui8[3] = rxData.payLoad[7];
            if(rxData.payLoad[3]){
                ourButton[buttonIndex].timeDiff = myWord.ui32;
                waitTime = myWord.ui32;
//                str = "::*ID Válido* (BUTTON UP)::";

            }else{
                ourButton[buttonIndex].timeDown = myWord.ui32;
//                str = "::*ID Válido* (BUTTON DOWN)::";
                myFlags.individualFlags.buttonUp = true;
            }
            buttonsState = ourButton[buttonIndex].estado;
            drawInter();
            break;
        case SET_LEDS:
            myWord.ui8[0] = rxData.payLoad[2];
            myWord.ui8[1] = rxData.payLoad[3];
            lastLeds = myWord.ui16[0];
//            s.setNum(lastLeds);
//            str = "::*ID Válido* (LEDS SET)::";
            drawInter();
            break;
        case GET_LEDS:
            myWord.ui8[0] = rxData.payLoad[2];
            myWord.ui8[1] = rxData.payLoad[3];
            lastLeds = myWord.ui16[0];
//            s.setNum(lastLeds);
//            str = "::*ID Válido* (LEDS GOT)::\n stateLeds: "+ s +"";
            drawInter();
            break;
        case GET_BUTTONS:
            myWord.ui8[0] = rxData.payLoad[2];
            myWord.ui8[1] = rxData.payLoad[3];
            for(uint8_t i = 0;i<4;i++){
            auxButtonState = ~myWord.ui8[0] -240;
            }
//            s.setNum(auxButtonState);
//            str = "::*ID Válido* (BUTTONS GOT)::\n buttonState: "+ s +"";
            break;
        default:
//            str=((char *)rxData.payLoad);
//            str= ("::*ID Invalido * (" + str + ")::");
            break;
    }
    //}
    //escribe en la pantalla el mensaje
//    ui->textBrowser->setTextColor(Qt::black);
//    ui->textBrowser->append(str);

}
void MainWindow::sendData()
{
    //carga el header y token
    txData.index=0;
    txData.payLoad[txData.index++]='U';
    txData.payLoad[txData.index++]='N';
    txData.payLoad[txData.index++]='E';
    txData.payLoad[txData.index++]='R';
    txData.payLoad[txData.index++]=0;
    txData.payLoad[txData.index++]=':';
    //carga el ID y nBytes
    switch (estadoComandos) {
    case ALIVE:
        txData.payLoad[txData.index++]=ALIVE;
        txData.payLoad[NBYTES]=0x02;
        break;
    case BUTTONEVENT:
        txData.payLoad[txData.index++]=BUTTONEVENT;
        txData.payLoad[NBYTES]=0x02;
        break;
    case SET_LEDS:
        txData.payLoad[txData.index++]=SET_LEDS;
        txData.payLoad[txData.index++]= numLed;
        txData.payLoad[txData.index++]= ledState;
        txData.payLoad[NBYTES]=0x04;
        break;
    case GET_LEDS:
        txData.payLoad[txData.index++]=GET_LEDS;
        txData.payLoad[NBYTES]=0x02;
        break;
    case GET_BUTTONS:
        txData.payLoad[txData.index++]=GET_BUTTONS;
        txData.payLoad[NBYTES]=0x02;
        break;
    default:
        break;
    }
   txData.cheksum=0;
   //recuenta los bytes y carga el checksum
   QString str="";
   for(int a=0 ;a<txData.index;a++)
       txData.cheksum^=txData.payLoad[a];
    txData.payLoad[txData.index]=txData.cheksum;
    if(mySerial->isWritable()){
        mySerial->write((char *)txData.payLoad,txData.payLoad[NBYTES]+6);
    }
    //escribe en pantalla el mensaje enviado
//    for(int i=0; i<=txData.index; i++){
//        if(isalnum(txData.payLoad[i]))
//            str = str + QString("%1").arg((char)txData.payLoad[i]);
//        else
//            str = str +"{" + QString("%1").arg(txData.payLoad[i],2,16,QChar('0')) + "}";
//    }
//    ui->textBrowser->setTextColor(Qt::green);
//    ui->textBrowser->append(">>PC-->MBED :: SENT ::");

}

void MainWindow::alive()
{
    estadoComandos = ALIVE;
    sendData();
}

void MainWindow::drawInter(){
    uint16_t auxled = 0, largo, radio = 40, posX, posY, alto;
    QPainter paint(myPaintBox->getCanvas());
    QPen pen;
    myPaintBox->getCanvas()->fill(Qt::black);

    largo=myPaintBox->width()/4;
    posX = (largo/2)-(radio);
    alto = (myPaintBox->height()/3);
    posY = alto/2;

    //Led 1
    auxled|=1<<0;
    if(auxled & lastLeds)
        paint.setBrush(Qt::yellow);
    else
        paint.setBrush(Qt::white);


    paint.drawEllipse(posX, posY, radio, radio);
    posX+=largo;
    auxled=0;

    //Led 2
    auxled|=1<<1;
    if(auxled & lastLeds)
        paint.setBrush(Qt::red);
    else
        paint.setBrush(Qt::white);

    paint.drawEllipse(posX, posY, radio, radio);
    posX+=largo;
    auxled=0;

    //Led 3
    auxled|=1<<2;
    if(auxled & lastLeds)
        paint.setBrush(Qt::blue);
    else
        paint.setBrush(Qt::white);

    paint.drawEllipse(posX, posY, radio, radio);
    posX+=largo;
    auxled=0;

    //Led 4
    auxled|=1<<3;
    if(auxled & lastLeds)
        paint.setBrush(Qt::green);
    else
        paint.setBrush(Qt::white);

    paint.drawEllipse(posX, posY, radio, radio);

    posY += alto ;
    posX = (largo/2)-(radio);
    pen.setWidth(3);
    pen.setColor(Qt::white);
    paint.setPen(pen);

    for(int i = 0; i < NUMBUT;i++){
        if(ourButton[i].estado == true){
            switch(ourButton[i].estado){
                case 1:
                    paint.setBrush(Qt::darkGray);
                break;
                default:
                    paint.setBrush(Qt::black);
                break;
            }

        }else
            paint.setBrush(Qt::black);

        paint.drawRect(posX,posY,radio,radio);
        posX += largo;

    }

    myPaintBox->update();
}


void MainWindow::juegoTopos(){
    if(myFlags.individualFlags.playing){
        switch(gameState){
        case LOBBY:
            delay++;
            ui->screenState->setText("PLAYING");
            ui->screenState->setAlignment(Qt::AlignCenter);
            estadoComandos = SET_LEDS;
            numLed = ledsMask[NUMLED];
            ledState = 1;
            sendData();
            if(delay - auxGameTime >= (TIMESTART/INTERVAL)){
                delay = 0;
                auxGameTime = 0;
                ledState = 0;
                sendData();
                estadoComandos = GET_LEDS;
                sendData();
                gameState = GAME;
            }
            break;
        case GAME:
            delay++;
            if((delay < (GAMETIME/INTERVAL))){
                if(delay >= actualTime[0]){
                    if(myFlags.individualFlags.topoOut1 == false){
                        randomTimeOut[0] = ((rand() % (MAXTIMEOUT-MINTIMEOUT)) + MINTIMEOUT)/INTERVAL;
                        randomTimeIn[0] = ((rand() % (MAXTIMEIN-MINTIMEIN)) + MINTIMEIN)/INTERVAL;
                        myFlags.individualFlags.topoOut1 = true;
                    }else{
                        auxledTopos = 0;
                        auxledTopos |= 1<<0;
                        if(lastLeds & auxledTopos){
                            actualTime[0] += (randomTimeOut[0]);
                            ledState = 0;
                            fallos++;
                            puntaje -= 10;
                            ui->screenFallos->display(QString().number(fallos));
                            ui->screenPuntaje->display(QString().number(puntaje));
                            myFlags.individualFlags.topoOut1 = false;
                        }else{
                            actualTime[0] += (randomTimeIn[0]);
                            ledState = 1;

                        }
                        numLed = ledsMask[0];
                        estadoComandos = SET_LEDS;
                        sendData();

                    }
                }

                if(delay >= actualTime[1]){
                    if(myFlags.individualFlags.topoOut2 == false){
                        randomTimeOut[1] = ((rand() % (MAXTIMEOUT-MINTIMEOUT)) + MINTIMEOUT)/INTERVAL;
                        randomTimeIn[1] = ((rand() % (MAXTIMEIN-MINTIMEIN)) + MINTIMEIN)/INTERVAL;
                        myFlags.individualFlags.topoOut2 = true;
                    }else{
                        auxledTopos = 0;
                        auxledTopos |= (1<<1);
                        if(lastLeds & auxledTopos){
                            actualTime[1] += (randomTimeOut[1]);
                            ledState = 0;
                            fallos++;
                            puntaje -= 10;
                            ui->screenFallos->display(QString().number(fallos));
                            ui->screenPuntaje->display(QString().number(puntaje));
                            myFlags.individualFlags.topoOut2 = false;
                        }else{
                            actualTime[1] += (randomTimeIn[1]);
                            ledState = 1;
                        }
                        numLed = ledsMask[1];
                        estadoComandos = SET_LEDS;
                        sendData();

                    }
                }
                if(delay >= actualTime[2]){
                    if(myFlags.individualFlags.topoOut3 == false){
                        randomTimeOut[2] = ((rand() % (MAXTIMEOUT-MINTIMEOUT)) + MINTIMEOUT)/INTERVAL;
                        randomTimeIn[2] = ((rand() % (MAXTIMEIN-MINTIMEIN)) + MINTIMEIN)/INTERVAL;
                        myFlags.individualFlags.topoOut3 = true;
                    }else{
                        auxledTopos = 0;
                        auxledTopos |= 1<<2;
                        if(lastLeds & auxledTopos){
                            actualTime[2] += (randomTimeOut[2]);
                            ledState = 0;
                            fallos++;
                            puntaje -= 10;
                            ui->screenFallos->display(QString().number(fallos));
                            ui->screenPuntaje->display(QString().number(puntaje));
                            myFlags.individualFlags.topoOut3 = false;
                        }else{
                            actualTime[2] += (randomTimeIn[2]);
                            ledState = 1;
                        }
                        numLed = ledsMask[2];
                        estadoComandos = SET_LEDS;
                        sendData();

                    }
                }

                if(delay >= actualTime[3]){
                    if(myFlags.individualFlags.topoOut4 == false){
                        randomTimeOut[3] = ((rand() % (MAXTIMEOUT-MINTIMEOUT)) + MINTIMEOUT)/INTERVAL;
                        randomTimeIn[3] = ((rand() % (MAXTIMEIN-MINTIMEIN)) + MINTIMEIN)/INTERVAL;
                        myFlags.individualFlags.topoOut4 = true;
                    }else{
                        auxledTopos = 0;
                        auxledTopos |= 1<<3;
                        if(lastLeds & auxledTopos){
                            actualTime[3] = delay + (randomTimeOut[3]);
                            ledState = 0;
                            fallos++;
                            puntaje -= 10;
                            ui->screenFallos->display(QString().number(fallos));
                            ui->screenPuntaje->display(QString().number(puntaje));
                            myFlags.individualFlags.topoOut4 = false;
                        }else{
                            actualTime[3] = delay + (randomTimeIn[3]);
                            ledState = 1;
                        }
                        numLed = ledsMask[3];
                        estadoComandos = SET_LEDS;
                        sendData();

                    }
                }
                for(uint8_t i =0;i<NUMBUT;i++){
                    if(myFlags.individualFlags.buttonUp && (ourButton[i].estado == 1)){
                        auxledTopos = 0;
                        auxledTopos |= 1<<i;
                        if(lastLeds & auxledTopos){
                            aciertos++;
                            puntaje += (randomTimeOut[i]*randomTimeOut[i])/(randomTimeIn[i]*(actualTime[i]-delay));
                            numLed = ledsMask[i];
                            ledState = 0;
                            estadoComandos = SET_LEDS;
                            sendData();
                        }else{
                            errores++;
                            puntaje -= 20;
                        }
                        ui->screenAciertos->display(QString().number(aciertos));
                        ui->screenErrores->display(QString().number(errores));
                        ui->screenPuntaje->display(QString().number(puntaje));
                        myFlags.individualFlags.buttonUp = false;
                    }
                }
            }else{
                gameState = FINISH;
            }

            break;
        case FINISH:
            if(puntaje > puntajeMax){
                puntajeMax = puntaje;
                ui->screenPuntajeMax->display(QString().number(puntajeMax,10));
            }
            ledState = 0;
            numLed = ledsMask[NUMLED];
            estadoComandos = SET_LEDS;
            sendData();
            gameState = LOBBY;
            delay = 0;
            auxGameTime = 0;
            myFlags.individualFlags.playing = false;
            fallos = 0;
            aciertos = 0;
            errores = 0;
            puntaje = 0;
            for(uint8_t i = 0; i<4;i++){
                randomTimeOut[i] = 0;
                randomTimeIn[i]=0;
                actualTime[i] = 0;
            }
            ui->screenErrores->display(QString().number(errores,10));
            ui->screenAciertos->display(QString().number(aciertos,10));
            ui->screenFallos->display(QString().number(fallos,10));
            ui->screenPuntaje->display(QString().number(puntaje,10));
            ui->screenState->setText("AWAITING PLAYER");
            ui->screenState->setAlignment(Qt::AlignCenter);
            break;
        default:
            break;

        }
    }

}

void MainWindow::conectar(){
    for(uint8_t step = 0;step < 3;step++){
        switch (estadoComandos) {
            case ALIVE:
                sendData();
                estadoComandos = GET_BUTTONS;
                break;
            case GET_BUTTONS:
                sendData();
                estadoComandos = GET_LEDS;
                break;
            case GET_LEDS:
                sendData();
                ui->screenState->setText("AWAITING PLAYER");
                ui->screenState->setAlignment(Qt::AlignCenter);
                break;
            default:
                break;
        }
    }
}


void MainWindow::getLeds()
{
  estadoComandos = GET_LEDS;
    sendData();
}


void MainWindow::getButtons()
{
    estadoComandos = GET_BUTTONS;
      sendData();
}

