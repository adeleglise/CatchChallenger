#ifndef BOTTARGETLIST_H
#define BOTTARGETLIST_H

#include <QDialog>
#include <QHash>
#include <QListWidgetItem>

#include "../bot/MultipleBotConnection.h"
#include "../bot/actions/ActionsAction.h"
#include "WaitScreen.h"

namespace Ui {
class BotTargetList;
}

class BotTargetList : public QDialog
{
    Q_OBJECT

public:
    explicit BotTargetList(QHash<CatchChallenger::Api_client_real *,MultipleBotConnection::CatchChallengerClient *> apiToCatchChallengerClient,
    QHash<CatchChallenger::ConnectedSocket *,MultipleBotConnection::CatchChallengerClient *> connectedSocketToCatchChallengerClient,
    QHash<QSslSocket *,MultipleBotConnection::CatchChallengerClient *> sslSocketToCatchChallengerClient,
    ActionsAction *actionsAction);
    ~BotTargetList();

    struct SimplifiedMapForPathFinding
    {
        struct PathToGo
        {
            std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > left;
            std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > right;
            std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > top;
            std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > bottom;
        };
        std::unordered_map<std::pair<uint8_t,uint8_t>,PathToGo,pairhash> pathToGo;
        std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> pointQueued;
    };

    void loadAllBotsInformation();
    void loadAllBotsInformation2();
    void updateLayerElements();
    void updateMapInformation();
    void updateMapContent();
    void updatePlayerInformation();
    void updatePlayerMapSlot();
    void updatePlayerMap(const bool &force=false);
    void updatePlayerStep();
    void startPlayerMove();
    std::vector<std::string> contentToGUI(const MapServerMini::BlockObject * const blockObject,const MultipleBotConnection::CatchChallengerClient * const client, QListWidget *listGUI=NULL);
    std::vector<std::string> contentToGUI(const MultipleBotConnection::CatchChallengerClient * const client, QListWidget *listGUI, const std::unordered_map<const MapServerMini::BlockObject *, MapServerMini::BlockObjectPathFinding> &resolvedBlockList, const bool &displayTooHard=true);
    std::string graphStepNearMap(const MultipleBotConnection::CatchChallengerClient * const client,const MapServerMini::BlockObject * const currentNearBlock, const unsigned int &depth=2);
    std::string graphLocalMap();
    std::pair<uint8_t,uint8_t> getNextPosition(const MapServerMini::BlockObject * const blockObject, const ActionsBotInterface::GlobalTarget &target);
    std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > pathFinding(
            const MapServerMini::BlockObject * const blockObject,
            const CatchChallenger::Orientation &source_orientation,const uint8_t &source_x,const uint8_t &source_y,
            const CatchChallenger::Orientation &destination_orientation,const uint8_t &destination_x,const uint8_t &destination_y);
    static std::string stepToString(const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &returnPath);
signals:
    void start_preload_the_map();
private slots:
    void on_bots_itemSelectionChanged();
    void on_comboBox_Layer_activated(int index);
    void on_localTargets_itemActivated(QListWidgetItem *item);
    void on_comboBoxStep_currentIndexChanged(int index);
    void on_browseMap_clicked();
    void on_searchDeep_editingFinished();
    void on_globalTargets_itemActivated(QListWidgetItem *item);
    void on_tooHard_clicked();

    void on_trackThePlayer_clicked();

private:
    Ui::BotTargetList *ui;
    QHash<CatchChallenger::Api_client_real *,MultipleBotConnection::CatchChallengerClient *> apiToCatchChallengerClient;
    QHash<CatchChallenger::ConnectedSocket *,MultipleBotConnection::CatchChallengerClient *> connectedSocketToCatchChallengerClient;
    QHash<QSslSocket *,MultipleBotConnection::CatchChallengerClient *> sslSocketToCatchChallengerClient;
    ActionsAction *actionsAction;
    QHash<QString,MultipleBotConnection::CatchChallengerClient *> pseudoToBot;

    bool botsInformationLoaded;
    uint32_t mapId;
    WaitScreen waitScreen;

    std::vector<uint32_t> mapIdListLocalTarget;
    std::vector<ActionsBotInterface::GlobalTarget> targetListGlobalTarget;
    bool alternateColor;
    static std::string pathFindingToString(const MapServerMini::BlockObjectPathFinding &resolvedBlock);
    static bool isSame(const CatchChallenger::MonstersCollisionValue::MonstersCollisionContent &monstersCollisionContentA,const CatchChallenger::MonstersCollisionValue::MonstersCollisionContent &monstersCollisionContentB);

    //std::unodered_map<const CatchChallenger::MonstersCollisionValue::MonstersCollisionContent,un> monsterCollisionContentDuplicate;
};

#endif // BOTTARGETLIST_H