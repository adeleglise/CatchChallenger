#ifndef CATCHCHALLENGER_MAPCONTROLLERMP_H
#define CATCHCHALLENGER_MAPCONTROLLERMP_H

#include "../../fight/interface/MapVisualiserPlayerWithFight.h"
#include "../Api_protocol.h"
#include "PathFinding.h"

#include <QString>
#include <QList>
#include <QStringList>
#include <QHash>
#include <QTimer>

class MapControllerMP : public MapVisualiserPlayerWithFight
{
    Q_OBJECT
public:
    explicit MapControllerMP(const bool &centerOnPlayer=true, const bool &debugTags=false, const bool &useCache=true, const bool &OpenGL=false);
    ~MapControllerMP();

    virtual void resetAll();
    virtual void connectAllSignals(CatchChallenger::Api_protocol * client);
    void setScale(const float &scaleSize);
public slots:
    //map move Qt
    void insert_player(const CatchChallenger::Player_public_informations &player,const uint32_t &mapId,const uint16_t &x,const uint16_t &y,const CatchChallenger::Direction &direction);
    void move_player(const uint16_t &id, const QList<QPair<uint8_t,CatchChallenger::Direction> > &movement);
    void remove_player(const uint16_t &id);
    void reinsert_player(const uint16_t &id, const uint8_t &x, const uint8_t &y, const CatchChallenger::Direction &direction);
    void full_reinsert_player(const uint16_t &id, const uint32_t &mapId, const uint8_t &x, const uint8_t &y, const CatchChallenger::Direction &direction);
    void dropAllPlayerOnTheMap();
    //map move
    bool insert_player_final(const CatchChallenger::Player_public_informations &player,const uint32_t &mapId,const uint16_t &x,const uint16_t &y,const CatchChallenger::Direction &direction,bool inReplayMode);
    bool move_player_final(const uint16_t &id, const QList<QPair<uint8_t,CatchChallenger::Direction> > &movement, bool inReplayMode);
    bool remove_player_final(const uint16_t &id, bool inReplayMode);
    bool reinsert_player_final(const uint16_t &id, const uint8_t &x, const uint8_t &y, const CatchChallenger::Direction &direction, bool inReplayMode);
    bool full_reinsert_player_final(const uint16_t &id, const uint32_t &mapId, const uint8_t &x, const uint8_t &y, const CatchChallenger::Direction &direction, bool inReplayMode);
    bool dropAllPlayerOnTheMap_final(bool inReplayMode);

    void teleportTo(const uint32_t &mapId,const uint16_t &x,const uint16_t &y,const CatchChallenger::Direction &direction);
    virtual bool asyncMapLoaded(const QString &fileName,MapVisualiserThread::Map_full * tempMapObject);

    //player info
    void have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations);

    //the datapack
    virtual void datapackParsed();
    virtual void datapackParsedMainSub();
    virtual void reinject_signals();
    virtual void reinject_signals_on_valid_map();
private:
    //the other player
    struct OtherPlayer
    {
        Tiled::MapObject * playerMapObject;
        Tiled::Tileset * playerTileset;
        int moveStep;
        CatchChallenger::Direction direction;
        uint8_t x,y;
        bool inMove;
        bool stepAlternance;
        QString current_map;
        QSet<QString> mapUsed;
        CatchChallenger::Player_public_informations informations;
        Tiled::MapObject * labelMapObject;
        Tiled::Tileset * labelTileset;
        uint32_t playerSpeed;
        bool animationDisplayed;

        //presumed map
        MapVisualiserThread::Map_full *presumed_map;
        uint8_t presumed_x,presumed_y;
        CatchChallenger::Direction presumed_direction;
        //pointer to allow copy of OtherPlayer
        QTimer *oneStepMore;
        QTimer *moveAnimationTimer;
    };
    PathFinding pathFinding;
    QHash<uint16_t,OtherPlayer> otherPlayerList;
    QHash<QTimer *,uint16_t> otherPlayerListByTimer,otherPlayerListByAnimationTimer;
    QHash<QString,uint16_t> mapUsedByOtherPlayer;

    //datapack
    QStringList skinFolderList;

    //the delayed action
    struct DelayedInsert
    {
        CatchChallenger::Player_public_informations player;
        uint32_t mapId;
        uint16_t x;
        uint16_t y;
        CatchChallenger::Direction direction;
    };
    struct DelayedMove
    {
        uint16_t id;
        QList<QPair<uint8_t,CatchChallenger::Direction> > movement;
    };
    struct DelayedReinsertSingle
    {
        uint16_t id;
        uint8_t x;
        uint8_t y;
        CatchChallenger::Direction direction;
    };
    struct DelayedReinsertFull
    {
        uint16_t id;
        uint32_t mapId;
        uint8_t x;
        uint8_t y;
        CatchChallenger::Direction direction;
    };
    enum DelayedType
    {
        DelayedType_Insert,
        DelayedType_Move,
        DelayedType_Remove,
        DelayedType_Reinsert_single,
        DelayedType_Reinsert_full,
        DelayedType_Drop_all
    };
    struct DelayedMultiplex
    {
        DelayedType type;
        DelayedInsert insert;
        DelayedMove move;
        uint16_t remove;
        DelayedReinsertSingle reinsert_single;
        DelayedReinsertFull reinsert_full;
    };
    QList<DelayedMultiplex> delayedActions;

    struct DelayedTeleportTo
    {
        uint32_t mapId;
        uint16_t x;
        uint16_t y;
        CatchChallenger::Direction direction;
    };
    QList<DelayedTeleportTo> delayedTeleportTo;
    float scaleSize;
    bool isTeleported;
    static QFont playerpseudofont;
    static QPixmap *imgForPseudoAdmin;
    static QPixmap *imgForPseudoDev;
    static QPixmap *imgForPseudoPremium;
    struct PathResolved
    {
        struct StartPoint
        {
            QString map;
            uint8_t x,y;
        };
        StartPoint startPoint;
        QList<QPair<CatchChallenger::Orientation,uint8_t> > path;
    };
    QList<PathResolved> pathList;

    CatchChallenger::Api_protocol * client;
protected:
    //current player
    CatchChallenger::Player_private_and_public_informations player_informations;
    bool player_informations_is_set;
private slots:
    bool loadPlayerMap(const QString &fileName,const uint8_t &x,const uint8_t &y);
    void moveOtherPlayerStepSlot();
    void moveOtherPlayerStepSlotWithPlayer(OtherPlayer &otherPlayer);
    void doMoveOtherAnimation();
    /// \warning all ObjectGroupItem destroyed into removeMap()
    virtual void destroyMap(MapVisualiserThread::Map_full *map);
    void eventOnMap(CatchChallenger::MapEvent event, MapVisualiserThread::Map_full * tempMapObject, uint8_t x, uint8_t y);
    CatchChallenger::Direction moveFromPath();
    //virtual QSet<QString> loadMap(MapVisualiserThread::Map_full *map,const bool &display);
protected slots:
    virtual void finalPlayerStep();
    //call after enter on new map
    virtual void loadOtherPlayerFromMap(OtherPlayer otherPlayer, const bool &display=true);
    //call before leave the old map (and before loadPlayerFromCurrentMap())
    virtual void unloadOtherPlayerFromMap(OtherPlayer otherPlayer);
    void pathFindingResult(const QString &current_map, const uint8_t &x, const uint8_t &y, const QList<QPair<CatchChallenger::Orientation, uint8_t> > &path);
    bool nextPathStep();//true if have step
    virtual void keyPressParse();
signals:
    void searchPath(QList<MapVisualiserThread::Map_full> mapList);
    void pathFindingNotFound();
};

#endif // CATCHCHALLENGER_MAPCONTROLLERMP_H
