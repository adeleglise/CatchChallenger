#include <QObject>
#include <QSettings>
#include <QTcpServer>
#include <QDebug>
#include <QTimer>
#include <QCoreApplication>
#include <QList>
#include <QByteArray>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDir>
#include <QSemaphore>

#include "../general/base/DebugClass.h"
#include "base/ServerStructures.h"
#include "base/Client.h"
#include "../general/base/Map_loader.h"
#include "base/Bot/FakeBot.h"
#include "../general/base/ProtocolParsing.h"
#include "../general/base/QFakeServer.h"
#include "../general/base/QFakeSocket.h"
#include "base/Map_server.h"

#ifndef POKECRAFT_EVENTDISPATCHER_H
#define POKECRAFT_EVENTDISPATCHER_H

namespace Pokecraft {
class EventDispatcher : public QObject
{
	Q_OBJECT
public:
	explicit EventDispatcher();
	~EventDispatcher();
    void setSettings(ServerSettings settings);
	//stat function
	bool isListen();
	bool isStopped();
	bool isInBenchmark();
	quint16 player_current();
	quint16 player_max();
public slots:
	//to manipulate the server for restart and stop
	void start_server();
	void stop_server();
	void start_benchmark(quint16 second,quint16 number_of_client,bool benchmark_map);
	//todo
	/*void send_system_message(QString text);
	void send_pm_message(QString pseudo,QString text);*/
private:
	//to load/unload the content
	struct Map_semi
	{
		//conversion x,y to position: x+y*width
		Map* map;
		Map_semi_border border;
		Map_to_send old_map;
	};
	void load_settings();
	void preload_the_data();
	void preload_the_map();
	void preload_the_visibility_algorithm();
	void unload_the_data();
	void unload_the_map();
	void unload_the_visibility_algorithm();
	//internal usefull function
	QStringList listFolder(const QString& folder,const QString& suffix);
	QString listenIpAndPort(QString server_ip,quint16 server_port);
	void connect_the_last_client();
	//store about the network
	QString server_ip;
	QTcpServer *server;
	//store benchmark related
	bool in_benchmark_mode;
	int benchmark_latency;
	QTimer *timer_benchmark_stop;
	QTime time_benchmark_first_client;
	//to check double instance
	//shared into all the program
	static bool oneInstanceRunning;

	/// \brief To lunch event only when the event loop is setup
	QTimer *lunchInitFunction;

	//to keep client list
	QList<Client *> client_list;

	//stat
	enum ServerStat
	{
		Down=0,
		InUp=1,
		Up=2,
		InDown=3
	};
	ServerStat stat;

	//bot related
	void removeBots();
	void addBot();
	QTimer nextStep;//all function call singal sync, then not pointer needed
	bool initialize_the_database();
    //player related
    ClientMapManagement * getClientMapManagement();
private slots:
	//new connection
	void newConnection();
	//remove all finished client
	void removeOneClient();
	void removeOneBot();
	//parse general order from the client
	void serverCommand(QString command,QString extraText);
	//init, constructor, destructor
	void initAll();//call before all
	//starting function
	void stop_internal_server();
	void stop_benchmark();
	void check_if_now_stopped();
	void start_internal_benchmark(quint16 second,quint16 number_of_client,const bool &benchmark_map);
	void start_internal_server();
signals:
	//async the call
	void try_stop_server();
	void try_initAll();
	void need_be_stopped();
	void need_be_restarted();
	void need_be_started();
	void try_start_benchmark(const quint16 &second,const quint16 &number_of_client,const bool &benchmark_map);
	//stat
	void is_started(bool);
	//stat player
	void new_player_is_connected(const Player_internal_informations &player);
	void player_is_disconnected(const QString &pseudo);
	void new_chat_message(const QString &pseudo,const Chat_type &type,const QString &text);
	void error(const QString &error);
	//benchmark
	void benchmark_result(const int &latency,const double &TX_speed,const double &RX_speed,const double &TX_size,const double &RX_size,const double &second);
};
}

#endif // EVENTDISPATCHER_H