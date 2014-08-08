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

    void On_Control_Port_New_Connection();
    void On_Buffer_Port_New_Connection();
    void Current_Control_Socket_Disconnected();
    void Current_Buffer_Socket_Disconnected();
    void Initialize_Cameras();
    void On_Control_Socket_TCP_Received();

private:

    QTcpServer control_server;
    QTcpServer buffer_server;

    QString Generate_Date_Time();

    QTcpSocket* control_socket;
    QTcpSocket* buffer_socket;

    tPvHandle handles[MAX_CAMERAS];
    tPvFrame  frames[MAX_CAMERAS];

    Camera_Properties camera_properties[MAX_CAMERAS];

    bool cameras_ready[MAX_CAMERAS];

};

#endif // MAIN_CONTROLLER_H
