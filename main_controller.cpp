#include "main_controller.h"
#include <QDateTime>
#include <QTcpSocket>
#include <QTest>
#include <QStringList>

Main_Controller::Main_Controller(QObject *parent) :
    QObject(parent)
{    
    control_socket = 0;

    for (int i=0; i<MAX_CAMERAS; i++)
    {
        cameras_ready[i] = false;
    }

    // Initialize camera
    Initialize_Cameras();

     bool ok0 = control_server.listen(QHostAddress::Any, 1117);
     bool ok1 = buffer_server.listen(QHostAddress::Any, 1118);

     qDebug() << Generate_Date_Time() << "Control Tcp server starts with return:" << ok0;
     qDebug() << Generate_Date_Time() << "Buffer Tcp server starts with return:" << ok1;

     if (ok0 && ok1)
     {
         qDebug() << Generate_Date_Time() << "Start to listen at port 1117 and 1118";
         Connect_Signals();
     }
}

void Main_Controller::Connect_Signals()
{
    connect(&control_server, SIGNAL(newConnection()), this, SLOT(On_Control_Port_New_Connection()));
    connect(&buffer_server, SIGNAL(newConnection()), this, SLOT(On_Buffer_Port_New_Connection()));


}

tPvErr Main_Controller::Get_Camera_Uint32_Features(tPvHandle handle, QString feature, tPvUint32 *value)
{
    tPvErr error;
    error = PvAttrUint32Get(handle, feature.toAscii().constData(), value);
    return error;
}

tPvErr Main_Controller::Set_Camera_Uint32_Features(tPvHandle handle, QString feature, tPvUint32 value)
{
    tPvErr error;
    error = PvAttrUint32Set(handle, feature.toAscii().constData(), value);
    return error;

}

tPvErr Main_Controller::Get_Camera_Float32_Features(tPvHandle handle, QString feature, tPvFloat32 *value)
{
    tPvErr error;
    error = PvAttrFloat32Get(handle, feature.toAscii().constData(), value);
    return error;
}

tPvErr Main_Controller::Set_Camera_Float32_Features(tPvHandle handle, QString feature, tPvFloat32 value)
{
    tPvErr error;
    error = PvAttrFloat32Set(handle, feature.toAscii().constData(), value);
    return error;
}

tPvErr Main_Controller::Trigger_Image(int index, long wait_time)
{
    QTime t;

   // t.start();

    tPvErr error = PvCaptureQueueFrame(handles[index], &frames[index], NULL);
    if (error != ePvErrSuccess)
    {
        qDebug() << Generate_Date_Time() << "   Queue frame for camera:" << index << "returns:" << error;
        return error;
    }

    error = PvCommandRun(handles[index], "FrameStartTriggerSoftware");
     if (error != ePvErrSuccess)
    {
        qDebug() << Generate_Date_Time() << "   Trigger frame returns:" << error;
        return error;
    }

    error = PvCaptureWaitForFrameDone(handles[index], &frames[index], wait_time);
    if (error != ePvErrSuccess)
    {
        qDebug() << Generate_Date_Time() << "   Wait for frame returns:" << error;
        return error;
    }

    //qDebug() << "Time elapsed in waiting for frame:" << t.elapsed();

   // QTest::qWait(20);

    return ePvErrSuccess;
}

void Main_Controller::On_Control_Port_New_Connection()
{
    qDebug() << Generate_Date_Time() << "New connection coming in for control port";

    control_socket = control_server.nextPendingConnection();
    connect(control_socket, SIGNAL(disconnected()), this, SLOT(Current_Control_Socket_Disconnected()));
    connect(control_socket, SIGNAL(disconnected()), control_socket, SLOT(deleteLater()));
    connect(control_socket, SIGNAL(readyRead()), this, SLOT(On_Control_Socket_TCP_Received()));

    if (control_socket != NULL)
    {
        qDebug() << Generate_Date_Time() << "Incoming connection at:" << control_socket->peerAddress();
    }

}

void Main_Controller::On_Buffer_Port_New_Connection()
{
    qDebug() << Generate_Date_Time() << "New connection coming in for buffer port";

    buffer_socket = buffer_server.nextPendingConnection();
    connect(buffer_socket, SIGNAL(disconnected()), this, SLOT(Current_Buffer_Socket_Disconnected()));
    connect(buffer_socket, SIGNAL(disconnected()), buffer_socket, SLOT(deleteLater()));

    if (buffer_socket != NULL)
    {
        qDebug() << Generate_Date_Time() << "Incoming connection at:" << buffer_socket->peerAddress();
    }

}

void Main_Controller::Current_Control_Socket_Disconnected()
{
    qDebug() << Generate_Date_Time() << "Current control connection disconnected!!";

}

void Main_Controller::Current_Buffer_Socket_Disconnected()
{
    qDebug() << Generate_Date_Time() << "Current buffer connection disconnected!!";
}

void Main_Controller::Initialize_Cameras()
{
    tPvErr error = PvInitialize();

    qDebug() << Generate_Date_Time() << "Initialization API returns: " << error;

    int count = 0;

    while (count == 0)
    {
        count = PvCameraCount();
        qDebug() << Generate_Date_Time() << "Camera Count:" << count;
        // wait a while
        QTest::qWait(1000);
    }

    tPvCameraInfoEx list[MAX_CAMERAS];
    unsigned long cameras;
    cameras = PvCameraListEx(list, MAX_CAMERAS, NULL,sizeof(tPvCameraInfoEx));
    // Print a list of the connected cameras
    for (unsigned long i = 0; i < cameras; i++)
    {
        qDebug() << "   Serial:" << list[i].SerialNumber;
        qDebug() << "   ID:" << list[i].UniqueId;
    }

    // open the first camera for now

    for (int i=0; i<1; i++)
    {
        qDebug() << Generate_Date_Time() << "Camera Index:" << i;
        // force only the provided camera to work with the board now

        /*
        if (i == 0)
        {
            qDebug() << Generate_Date_Time() << "   Check unique id!";

            if (list[i].UniqueId != 161443)
            {
                qDebug() << Generate_Date_Time() << "   Unique ID does not match, not starting service!";
                return false;
            }
        }
        */


        error = PvCameraOpen(list[i].UniqueId, ePvAccessMaster, &handles[i]);
        qDebug() << Generate_Date_Time() << "   Open Camera returns:" << error;
        if (error != ePvErrSuccess) return;


        tPvUint32 frame_size;
        error = Get_Camera_Uint32_Features(handles[i], "TotalBytesPerFrame", &frame_size);
        qDebug() << Generate_Date_Time() << "   Get frame size returns" << error << "size:" << frame_size;
        if (error != ePvErrSuccess) return;

        tPvUint32 width, height;
        error = Get_Camera_Uint32_Features(handles[i], "Width", &width);
        qDebug() << Generate_Date_Time() << "   Get width returns" << error << "size:" << width;
        if (error != ePvErrSuccess) return;
        error = Get_Camera_Uint32_Features(handles[i], "Height", &height);
        qDebug() << Generate_Date_Time() << "   Get height returns" << error << "size:" << height;
        if (error != ePvErrSuccess) return;


        camera_properties[i].buffer_size = frame_size;
        camera_properties[i].width = width;
        camera_properties[i].height = height;

        memset(frames + 1, 0, sizeof(tPvFrame));

        frames[i].ImageBuffer = new char[frame_size];
        frames[i].ImageBufferSize = frame_size;

        error = PvCaptureAdjustPacketSize(handles[i], 1500); // embedded linux typically runs only 100M
        qDebug() << Generate_Date_Time() << "   Adjust packet size returns:" << error;
        if (error != ePvErrSuccess) return;

        uint sbs = 124000000;

        error = PvAttrUint32Set(handles[i], "StreamBytesPerSecond", sbs);
        qDebug() << Generate_Date_Time() << "    Adjust stream bytes per second to:" << sbs << "returns:" << error;
        if (error != ePvErrSuccess) return;


        error = PvAttrEnumSet(handles[i], "FrameStartTriggerMode", "Software");
        qDebug() << Generate_Date_Time() << "   Set software trigger returns" << error;
        if (error != ePvErrSuccess) return;

        error = PvCaptureStart(handles[i]);
        qDebug() << Generate_Date_Time() << "   Start capture returns:" << error;
        if (error != ePvErrSuccess) return;

        error = PvCommandRun(handles[i], "AcquisitionStart");
        qDebug() << Generate_Date_Time() << "   Acquisition start returns:" << error;
        if (error != ePvErrSuccess) return;

        cameras_ready[i] = true;
    }

    for (int i=0; i<MAX_CAMERAS; i++)
    {
        qDebug() << "   Camera:" << i << "ready:" << cameras_ready[i];
    }
}

void Main_Controller::On_Control_Socket_TCP_Received()
{
    int bytes = control_socket->bytesAvailable();

    QByteArray qba;

    qba = control_socket->peek(bytes);

    QString command;

    if (((uchar) qba.at(bytes-1)) == '#')
    {
        command = QString(control_socket->read(bytes));

        command.remove(bytes-1, 1);

        QStringList commands = command.split('#');
      //  qDebug() << Generate_Date_Time() << "Commands recevied:" << commands.count();

        for (int i=0; i<commands.count(); i++)
        {
            QString line = commands[i];

            //qDebug() << Generate_Date_Time() << "Received command:" << line;

            QStringList list = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);

            if (list.count() != 0)
            {

                if (list[0] == "SET_UINT32")
                {
                    qDebug() << Generate_Date_Time() << "Request set uint32!";

                    if (list.count() == 4)
                    {
                        bool ok;


                        int index = list[1].toInt(&ok);
                        if (!ok) return;

                        QString attribute = list[2];

                        int value = list[3].toInt(&ok);
                        if (!ok) return;

                        tPvErr error = Set_Camera_Uint32_Features(handles[index], attribute.toAscii().constData(), value);
                        qDebug() << Generate_Date_Time() << "Set Uint32 Attribute:" << attribute << "index:" << index << "with value:" << value << "returns:" << error;

                        QString reply = QString("SET_UINT32_ACK %1 %2 %3 %4").arg(index).arg(attribute).arg(value).arg(error);

                        control_socket->write(reply.toAscii().constData(), reply.size());
                    }
                }

                if (list[0] == "GET_UINT32")
                {
                    qDebug() << Generate_Date_Time() << "Request get uint32!";

                    if  (list.count() == 3)
                    {
                        bool ok;

                        int index = list[1].toInt(&ok);
                        if (!ok) return;

                        QString attribute = list[2];

                        tPvUint32 value;

                        tPvErr error = Get_Camera_Uint32_Features(handles[index], attribute.toAscii().constData(), &value);
                        qDebug() << Generate_Date_Time() << "Get Uint32 Attribute:" << attribute << "index:" << index << "with value:" << value << "returns:" << error;

                        QString reply = QString("GET_UINT32_ACK %1 %2 %3 %4#").arg(index).arg(attribute).arg(value).arg(error);

                        control_socket->write(reply.toAscii().constData(), reply.size());
                    }
                }

                if (list[0] == "SET_FLOAT32")
                {
                    qDebug() << Generate_Date_Time() << "Request set float32!";

                    if (list.count() == 4)
                    {
                        bool ok;


                        int index = list[1].toInt(&ok);
                        if (!ok) return;

                        QString attribute = list[2];

                        tPvFloat32 value = list[3].toFloat(&ok);
                        if (!ok) return;

                        tPvErr error = Set_Camera_Float32_Features(handles[index], attribute.toAscii().constData(), value);
                        qDebug() << Generate_Date_Time() << "Set Float32 Attribute:" << attribute << "index:" << index << "with value:" << value << "returns:" << error;

                        QString reply = QString("SET_FLOAT32_ACK %1 %2 %3 %4").arg(index).arg(attribute).arg(value).arg(error);

                        control_socket->write(reply.toAscii().constData(), reply.size());
                    }
                }

                if (list[0] == "GET_FLOAT32")
                {
                    qDebug() << Generate_Date_Time() << "Request get float32!";

                    if  (list.count() == 3)
                    {
                        bool ok;

                        int index = list[1].toInt(&ok);
                        if (!ok) return;

                        QString attribute = list[2];

                        tPvFloat32 value;

                        tPvErr error = Get_Camera_Float32_Features(handles[index], attribute.toAscii().constData(), &value);
                        qDebug() << Generate_Date_Time() << "Get Float32 Attribute:" << attribute << "index:" << index << "with value:" << value << "returns:" << error;

                        QString reply = QString("GET_FLOAT32_ACK %1 %2 %3 %4#").arg(index).arg(attribute).arg(value).arg(error);

                        control_socket->write(reply.toAscii().constData(), reply.size());
                    }

                }

                if (list[0] == "CAPTURE")
                {
                   // qDebug() << Generate_Date_Time() << "Request capture!";

                    if (list.count() == 2)
                    {
                        bool ok;
                        int index = list[1].toInt(&ok);
                        if (!ok) return;

                        if (cameras_ready[index] == true && buffer_socket != NULL)
                        {
                            tPvErr error = Trigger_Image(index, 2000);

                           // qDebug() << Generate_Date_Time() << "Capture index:" << index << "returns:" << error;

                            if (error == ePvErrSuccess)
                            {
                                QByteArray qba = QByteArray((const char*) frames[index].ImageBuffer, camera_properties[index].buffer_size);
                                buffer_socket->write(qba, camera_properties[index].buffer_size);
                            }

                                QString reply = QString("CAPTURE_OK %1 %2#").arg(index).arg(error);
                                control_socket->write(reply.toAscii().constData(), reply.size());
                        }
                        else
                        {
                            qDebug() << Generate_Date_Time() << "Camera not ready at index:" << index;
                            QString reply = QString("CAPTURE_OK %1 %2#").arg(index).arg(-1);
                            control_socket->write(reply.toAscii().constData(), reply.size());
                        }
                    }
                }

            }
        }
    }

}

QString Main_Controller::Generate_Date_Time()
{
    return QDateTime::currentDateTime().toString("[dd.MM.yyyy hh:mm:ss:zzz]");
}
