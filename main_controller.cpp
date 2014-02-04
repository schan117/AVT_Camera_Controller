#include "main_controller.h"
#include <QDateTime>
#include <QTcpSocket>
#include <QTest>
#include <QStringList>

Main_Controller::Main_Controller(QObject *parent) :
    QObject(parent)
{    
    socket = 0;

    Connect_Signals();

    // Initialize camera

    bool ok = Initialize_Cameras();

    if (ok)
    {
        qDebug() << Generate_Date_Time() << "Camera(s) setup ok!!!!!";

       bool ok = server.listen(QHostAddress::Any, 1234);

        qDebug() << Generate_Date_Time() << "Tcp server starts with return:" << ok;

        if (ok)
        {
            qDebug() << Generate_Date_Time() << "Start to listen at port 1234";
        }
    }
    else
    {
        qDebug() << Generate_Date_Time() << "Camera startup error!!!";
    }
}

void Main_Controller::Connect_Signals()
{
    connect(&server, SIGNAL(newConnection()), this, SLOT(On_New_Connection()));


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
    tPvErr error = PvCaptureQueueFrame(handles[index], &frames[index], NULL);
    qDebug() << Generate_Date_Time() << "   Queue frame for camera:" << index << "returns:" << error;
    if (error != ePvErrSuccess) return error;

    error = PvCommandRun(handles[index], "FrameStartTriggerSoftware");
    qDebug() << Generate_Date_Time() << "   Trigger frame returns:" << error;
    if (error != ePvErrSuccess) return error;

    error = PvCaptureWaitForFrameDone(handles[index], &frames[index], wait_time);
    if (error != ePvErrSuccess) return error;

    QTest::qWait(20);

    return ePvErrSuccess;
}

void Main_Controller::On_New_Connection()
{
    qDebug() << Generate_Date_Time() << "New connection coming in";

    socket = server.nextPendingConnection();
    connect(socket, SIGNAL(disconnected()), this, SLOT(Current_Socket_Disconnected()));
    connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(On_TCP_Received()));

    if (socket != NULL)
    {
        qDebug() << Generate_Date_Time() << "Incoming connection at:" << socket->peerAddress();
    }

}

void Main_Controller::Current_Socket_Disconnected()
{
    qDebug() << Generate_Date_Time() << "Current connection disconnected!!";

}

bool Main_Controller::Initialize_Cameras()
{
    tPvErr error = PvInitialize();

    qDebug() << Generate_Date_Time() << "Initialization API returns: " << error;

    int count = 0;

    while (count == 0)
    {
        count = PvCameraCount();
        qDebug() << Generate_Date_Time() << "Camera Count:" << count;
        // wait a while
        QTest::qWait(200);
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

        error = PvCameraOpen(list[i].UniqueId, ePvAccessMaster, &handles[i]);
        qDebug() << Generate_Date_Time() << "   Open Camera returns:" << error;
        if (error != ePvErrSuccess) return false;

        tPvUint32 frame_size;
        error = Get_Camera_Uint32_Features(handles[i], "TotalBytesPerFrame", &frame_size);
        qDebug() << Generate_Date_Time() << "   Get frame size returns" << error << "size:" << frame_size;
        if (error != ePvErrSuccess) return false;

        tPvUint32 width, height;
        error = Get_Camera_Uint32_Features(handles[i], "Width", &width);
        qDebug() << Generate_Date_Time() << "   Get width returns" << error << "size:" << width;
        if (error != ePvErrSuccess) return false;
        error = Get_Camera_Uint32_Features(handles[i], "Height", &height);
        qDebug() << Generate_Date_Time() << "   Get height returns" << error << "size:" << height;
        if (error != ePvErrSuccess) return false;


        camera_properties[i].buffer_size = frame_size;
        camera_properties[i].width = width;
        camera_properties[i].height = height;

        memset(frames + 1, 0, sizeof(tPvFrame));

        frames[i].ImageBuffer = new char[frame_size];
        frames[i].ImageBufferSize = frame_size;

        error = PvCaptureAdjustPacketSize(handles[i], 1500); // embedded linux typically runs only 100M
        qDebug() << Generate_Date_Time() << "   Adjust packet size returns:" << error;
        if (error != ePvErrSuccess) return false;

        error = PvAttrEnumSet(handles[i], "FrameStartTriggerMode", "Software");
        qDebug() << Generate_Date_Time() << "   Set software trigger returns" << error;
        if (error != ePvErrSuccess) return false;

        error = PvCaptureStart(handles[i]);
        qDebug() << Generate_Date_Time() << "   Start capture returns:" << error;
        if (error != ePvErrSuccess) return false;

        error = PvCommandRun(handles[i], "AcquisitionStart");
        qDebug() << Generate_Date_Time() << "   Acquisition start returns:" << error;
        if (error != ePvErrSuccess) return false;
    }


    return true;

}

void Main_Controller::On_TCP_Received()
{
    int bytes = socket->bytesAvailable();

    QByteArray qba;

    qba = socket->peek(bytes);

    QString command;

    if (((uchar) qba.at(bytes-1)) == '#')
    {
        command = QString(socket->read(bytes));

        command.remove(bytes-1, 1);
        qDebug() << Generate_Date_Time() << "Received command:" << command;
    }

    QStringList list = command.split(QRegExp("\\s+"), QString::SkipEmptyParts);

    if (list.count() != 0)
    {
        if (list[0] == "SET_EXP")
        {
            qDebug() << Generate_Date_Time() << "Request exposure set!";

            if (list.count() == 3)
            {
                bool ok;

                int index = list[1].toInt(&ok);
                if (!ok) return;

                int value = list[2].toInt(&ok);
                if (!ok) return;

                tPvErr error = Set_Camera_Uint32_Features(handles[index], "ExposureValue", value);
                qDebug() << Generate_Date_Time() << "Set exposure for camera:" << index << "returns:" << error;

                return;
            }
        }

        if (list[0] == "CAPTURE")
        {
            qDebug() << Generate_Date_Time() << "Request capture!";

            if (list.count() == 2)
            {
                bool ok;
                int index = list[1].toInt(&ok);
                if (!ok) return;

                tPvErr error = Trigger_Image(index, 2000);

                qDebug() << Generate_Date_Time() << "Capture index:" << index << "returns:" << error;

                QByteArray qba = QByteArray((const char*) frames[index].ImageBuffer, camera_properties[index].buffer_size);

                socket->write(qba, camera_properties[index].buffer_size);

                return;

            }
        }
        if (list[0] == "GET")
        {
            qDebug() << Generate_Date_Time() << "Request get image!";

            if (list.count() == 2)
            {
                bool ok;
                int index = list[1].toInt(&ok);
                if (!ok) return;

                QByteArray qba = QByteArray((const char*) frames[index].ImageBuffer, camera_properties[index].buffer_size);

                socket->write(qba, camera_properties[index].buffer_size);

            }
        }
    }

}

QString Main_Controller::Generate_Date_Time()
{
    return QDateTime::currentDateTime().toString("[dd.MM.yyyy hh:mm:ss:zzz]");
}
