#ifndef MAIN_CONTROLLER_H
#define MAIN_CONTROLLER_H

#include <QObject>
#include <QTcpServer>
#include "PvApi.h"

#define MAX_CAMERAS 10


struct Camera_Properties
{
    int width;
    int height;
    unsigned long buffer_size;
};


class Main_Controller : public QObject
{
    Q_OBJECT
public:
    explicit Main_Controller(QObject *parent = 0);

    void Connect_Signals();
    tPvErr Get_Camera_Uint32_Features(tPvHandle handle, QString feature, tPvUint32* value);
    tPvErr Set_Camera_Uint32_Features(tPvHandle handle, QString feature, tPvUint32 value);
    tPvErr Get_Camera_Float32_Features(tPvHandle handle, QString feature, tPvFloat32* value);
    tPvErr Set_Camera_Float32_Features(tPvHandle handle, QString feature, tPvFloat32 value);

    tPvErr Trigger_Image(int index, long wait_time);


signals:

public slots:

    void On_New_Connection();
    void Current_Socket_Disconnected();
    bool Initialize_Cameras();
    void On_TCP_Received();


private:

    QTcpServer server;

    QString Generate_Date_Time();

    QTcpSocket* socket;

    tPvHandle handles[MAX_CAMERAS];
    tPvFrame  frames[MAX_CAMERAS];

    Camera_Properties camera_properties[MAX_CAMERAS];

};

#endif // MAIN_CONTROLLER_H
