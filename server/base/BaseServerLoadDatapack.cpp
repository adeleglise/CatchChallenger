#include "BaseServer.h"
#include "GlobalServerData.h"
#include <openssl/sha.h>

#include "../../general/base/CommonSettingsCommon.h"

using namespace CatchChallenger;

void BaseServer::preload_the_datapack()
{
    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    std::vector<std::string> extensionAllowedTemp=stringsplit(std::string(CATCHCHALLENGER_EXTENSION_ALLOWED)+";"+std::string(CATCHCHALLENGER_EXTENSION_COMPRESSED),';');
    BaseServerMasterSendDatapack::extensionAllowed=std::unordered_set<std::string>(extensionAllowedTemp.begin(),extensionAllowedTemp.end());
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    std::vector<std::string> compressedExtensionAllowedTemp=stringsplit(std::string(CATCHCHALLENGER_EXTENSION_COMPRESSED),';');
    BaseServerMasterSendDatapack::compressedExtension=std::unordered_set<std::string>(compressedExtensionAllowedTemp.begin(),compressedExtensionAllowedTemp.end());
    #endif
    Client::datapack_list_cache_timestamp_base=0;
    Client::datapack_list_cache_timestamp_main=0;
    Client::datapack_list_cache_timestamp_sub=0;
    #else
    std::vector<std::string> extensionAllowedTemp=stringsplit(std::string(CATCHCHALLENGER_EXTENSION_ALLOWED+std::string(";")+CATCHCHALLENGER_EXTENSION_COMPRESSED),';');
    std::unordered_set<std::string> extensionAllowed=std::unordered_set<std::string>(extensionAllowedTemp.begin(),extensionAllowedTemp.end());
    #endif

    GlobalServerData::serverPrivateVariables.mainDatapackFolder=
            GlobalServerData::serverSettings.datapack_basePath+
            "map/main/"+
            CommonSettingsServer::commonSettingsServer.mainDatapackCode+
            "/";
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        if(CommonSettingsServer::commonSettingsServer.mainDatapackCode.size()==0)
        {
            std::cerr << "CommonSettingsServer::commonSettingsServer.mainDatapackCode.isEmpty" << std::endl;
            abort();
        }
        if(!regex_search(CommonSettingsServer::commonSettingsServer.mainDatapackCode,std::regex(CATCHCHALLENGER_CHECK_MAINDATAPACKCODE)))
        {
            std::cerr << "CommonSettingsServer::commonSettingsServer.mainDatapackCode not match CATCHCHALLENGER_CHECK_MAINDATAPACKCODE "
                      << CommonSettingsServer::commonSettingsServer.mainDatapackCode
                      << " not contain "
                      << CATCHCHALLENGER_CHECK_MAINDATAPACKCODE;
            abort();
        }
        if(!FacilityLibGeneral::isDir(GlobalServerData::serverPrivateVariables.mainDatapackFolder))
        {
            std::cerr << GlobalServerData::serverPrivateVariables.mainDatapackFolder << " don't exists" << std::endl;
            abort();
        }
    }
    #endif
    if(CommonSettingsServer::commonSettingsServer.subDatapackCode.size()>0)
    {
        GlobalServerData::serverPrivateVariables.subDatapackFolder=
                GlobalServerData::serverSettings.datapack_basePath+
                "map/main/"+
                CommonSettingsServer::commonSettingsServer.mainDatapackCode+
                "/sub/"+
                CommonSettingsServer::commonSettingsServer.subDatapackCode+
                "/";
        if(!FacilityLibGeneral::isDir(GlobalServerData::serverPrivateVariables.subDatapackFolder))
        {
            std::cerr << GlobalServerData::serverPrivateVariables.subDatapackFolder << " don't exists, drop spec" << std::endl;
            GlobalServerData::serverPrivateVariables.subDatapackFolder.clear();
            CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();
        }
    }

    #ifdef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    if(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.empty() || CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.empty())
    {
        std::cerr << "CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR, only send datapack by mirror is supported" << std::endl;
        abort();
    }
    #endif

    //do the base
    {
        SHA256_CTX hashBase;
        if(SHA224_Init(&hashBase)!=1)
        {
            std::cerr << "SHA224_Init(&hashBase)!=1" << std::endl;
            abort();
        }
        #ifdef _WIN32
        const std::unordered_map<std::string,BaseServerMasterSendDatapack::DatapackCacheFile> &pair=Client::datapack_file_list(GlobalServerData::serverSettings.datapack_basePath,"",false);
        #else
        const std::unordered_map<std::string,BaseServerMasterSendDatapack::DatapackCacheFile> &pair=Client::datapack_file_list(GlobalServerData::serverSettings.datapack_basePath,"map/main/",false);
        #endif
        if(pair.size()==0)
        {
            std::cout << "Client::datapack_file_list(GlobalServerData::serverSettings.datapack_basePath,\"map/main/\",false) empty (abort)" << std::endl;
            abort();
        }
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            auto i=pair.begin();
            while(i!=pair.cend())
            {
                if(i->first.find("map/main/")!=std::string::npos)
                {
                    std::cerr << "map/main/ found into: " << i->first << " (abort)" << std::endl;
                    abort();
                }
                ++i;
            }
        }
        #endif
        std::vector<std::string> datapack_file_temp=unordered_map_keys_vector(pair);
        std::sort(datapack_file_temp.begin(), datapack_file_temp.end());
        #if defined(CATCHCHALLENGER_EXTRA_CHECK) || defined(_WIN32)
        const std::string mainDatapackBaseFilter("map/main/");
        #endif
        /// \todo check under windows
        #ifdef _WIN32
        const std::string mainDatapackBaseFilterW("map\\main\\");
        #endif
        unsigned int index=0;
        while(index<datapack_file_temp.size()) {
            //right file name + extention check already done into Client::datapack_file_list() by if(regex_search(fileName,GlobalServerData::serverPrivateVariables.datapack_rightFileName))
            {
                const std::string &fileName=datapack_file_temp.at(index);
                std::string fullPathFileToOpen=GlobalServerData::serverSettings.datapack_basePath+fileName;
                #ifdef Q_OS_WIN32
                stringreplaceAll(fullPathFileToOpen,"/","\\");
                #endif
                FILE *filedesc=fopen(fullPathFileToOpen.c_str(),"rb");
                if(filedesc!=NULL)
                {
                    //read and load the file
                    const std::vector<char> &data=FacilityLibGeneral::readAllFileAndClose(filedesc);


                    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
                    if((1+fileName.size()+4+data.size())>=CATCHCHALLENGER_MAX_PACKET_SIZE)
                    {
                        if(GlobalServerData::serverSettings.max_players>1)//if not internal
                        {
                            if(BaseServerMasterSendDatapack::compressedExtension.find(FacilityLibGeneral::getSuffix(fileName))!=BaseServerMasterSendDatapack::compressedExtension.end())
                            {
                                if(ProtocolParsing::compressionTypeServer==ProtocolParsing::CompressionType::None)
                                {
                                    std::cerr << "The file " << GlobalServerData::serverSettings.datapack_basePath << fileName << " is over the maximum packet size, but can be compressed, try enable the compression" << std::endl;
                                    abort();
                                }
                            }
                            else
                            {
                                std::cerr << "The file " << GlobalServerData::serverSettings.datapack_basePath << fileName << " is over the maximum packet size" << std::endl;
                                abort();
                            }
                        }
                    }
                    #endif

                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    bool skip=false;
                    if(fileName.size()>mainDatapackBaseFilter.size())
                        if(stringStartWith(fileName,mainDatapackBaseFilter)
                            #ifdef _WIN32
                            || stringStartWith(fileName,mainDatapackBaseFilterW)
                            #endif
                                )
                            skip=true;
                    //switch the data to correct hash or drop it
                    if(skip)
                    {
                        #ifndef _WIN32
                        abort();
                        #endif
                    }
                    else
                    #endif
                    {
                        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                        #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
                        SHA256_CTX hashFile;
                        if(SHA224_Init(&hashFile)!=1)
                        {
                            std::cerr << "SHA224_Init(&hashBase)!=1" << std::endl;
                            abort();
                        }
                        SHA224_Update(&hashFile,data.data(),data.size());
                        BaseServerMasterSendDatapack::DatapackCacheFile cacheFile;
                        SHA224_Final(reinterpret_cast<unsigned char *>(ProtocolParsingBase::tempBigBufferForOutput),&hashFile);
                        ::memcpy(&cacheFile.partialHash,ProtocolParsingBase::tempBigBufferForOutput,sizeof(uint32_t));
                        BaseServerMasterSendDatapack::datapack_file_hash_cache_base[fileName]=cacheFile;
                        #endif
                        #endif

                        SHA224_Update(&hashBase,data.data(),data.size());
                    }
                }
                else
                {
                    std::cerr << "Stop now! Unable to open the file " << GlobalServerData::serverSettings.datapack_basePath << fileName << " to do the datapack checksum for the mirror" << std::endl;
                    abort();
                }
            }
            index++;
        }
        if(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size()==CATCHCHALLENGER_SHA224HASH_SIZE)
        {
            SHA224_Final(reinterpret_cast<unsigned char *>(ProtocolParsingBase::tempBigBufferForOutput),&hashBase);
            if(memcmp(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.data(),ProtocolParsingBase::tempBigBufferForOutput,CATCHCHALLENGER_SHA224HASH_SIZE)!=0)
            {
                std::cerr << "datapackHashBase sha224 sum not match master "
                          << binarytoHexa(CommonSettingsCommon::commonSettingsCommon.datapackHashBase)
                          << " != master "
                          << binarytoHexa(ProtocolParsingBase::tempBigBufferForOutput,CATCHCHALLENGER_SHA224HASH_SIZE)
                          << " (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                abort();
            }
        }
        else
        {
            CommonSettingsCommon::commonSettingsCommon.datapackHashBase.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
            SHA224_Final(reinterpret_cast<unsigned char *>(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.data()),&hashBase);
        }
    }
    /// \todo check if big file is compressible under 1MB

    //do the main
    {
        SHA256_CTX hashMain;
        if(SHA224_Init(&hashMain)!=1)
        {
            std::cerr << "SHA224_Init(&hashBase)!=1" << std::endl;
            abort();
        }
        const std::unordered_map<std::string,BaseServerMasterSendDatapack::DatapackCacheFile> &pair=Client::datapack_file_list(GlobalServerData::serverPrivateVariables.mainDatapackFolder,"sub/",false);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            auto i=pair.begin();
            while(i!=pair.cend())
            {
                if(i->first.find("sub/")!=std::string::npos)
                {
                    std::cerr << "sub/ found into: " << i->first << " (abort)" << std::endl;
                    abort();
                }
                ++i;
            }
        }
        #endif
        std::vector<std::string> datapack_file_temp=unordered_map_keys_vector(pair);
        std::sort(datapack_file_temp.begin(), datapack_file_temp.end());
        #if defined(CATCHCHALLENGER_EXTRA_CHECK) || defined(_WIN32)
        const std::string mainDatapackFolderFilter("sub/");
        #endif
        #ifdef _WIN32
        const std::string mainDatapackFolderFilterW("sub\\");
        #endif
        unsigned int index=0;
        while(index<datapack_file_temp.size()) {
            //right file name + extention check already done into Client::datapack_file_list() by if(regex_search(fileName,GlobalServerData::serverPrivateVariables.datapack_rightFileName))
            {
                const std::string &fileName=datapack_file_temp.at(index);
                std::string fullPathFileToOpen=GlobalServerData::serverPrivateVariables.mainDatapackFolder+fileName;
                #ifdef Q_OS_WIN32
                stringreplaceAll(fullPathFileToOpen,"/","\\");
                #endif
                FILE *filedesc=fopen(fullPathFileToOpen.c_str(),"rb");
                if(filedesc!=NULL)
                {
                    //read and load the file
                    const std::vector<char> &data=FacilityLibGeneral::readAllFileAndClose(filedesc);

                    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
                    if((1+fileName.size()+4+data.size())>=CATCHCHALLENGER_MAX_PACKET_SIZE)
                    {
                        if(GlobalServerData::serverSettings.max_players>1)//if not internal
                        {
                            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                            if(BaseServerMasterSendDatapack::compressedExtension.find(FacilityLibGeneral::getSuffix(fileName))!=BaseServerMasterSendDatapack::compressedExtension.end())
                            {
                                if(ProtocolParsing::compressionTypeServer==ProtocolParsing::CompressionType::None)
                                {
                                    std::cerr << "The file " << GlobalServerData::serverSettings.datapack_basePath << fileName << " is over the maximum packet size, but can be compressed, try enable the compression" << std::endl;
                                    abort();
                                }
                            }
                            else
                            #endif
                            {
                                std::cerr << "The file " << GlobalServerData::serverSettings.datapack_basePath << fileName << " is over the maximum packet size" << std::endl;
                                abort();
                            }
                        }
                    }
                    #endif

                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    bool skip=false;
                    if(fileName.size()>mainDatapackFolderFilter.size())
                        if(stringStartWith(fileName,mainDatapackFolderFilter)
                            #ifdef _WIN32
                            || stringStartWith(fileName,mainDatapackFolderFilterW)
                            #endif
                                )
                            skip=true;
                    //switch the data to correct hash or drop it
                    if(skip)
                    {
                        #ifndef _WIN32
                        abort();
                        #endif
                    }
                    else
                    #endif
                    {
                        #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
                        SHA256_CTX hashFile;
                        if(SHA224_Init(&hashFile)!=1)
                        {
                            std::cerr << "SHA224_Init(&hashBase)!=1" << std::endl;
                            abort();
                        }
                        SHA224_Update(&hashFile,data.data(),data.size());
                        BaseServerMasterSendDatapack::DatapackCacheFile cacheFile;
                        SHA224_Final(reinterpret_cast<unsigned char *>(ProtocolParsingBase::tempBigBufferForOutput),&hashFile);
                        ::memcpy(&cacheFile.partialHash,ProtocolParsingBase::tempBigBufferForOutput,sizeof(uint32_t));
                        Client::datapack_file_hash_cache_main[fileName]=cacheFile;
                        #endif

                        SHA224_Update(&hashMain,data.data(),data.size());
                    }
                }
                else
                {
                    std::cerr << "Stop now! Unable to open the file " << GlobalServerData::serverSettings.datapack_basePath << fileName << " to do the datapack checksum for the mirror" << std::endl;
                    abort();
                }
            }
            index++;
        }
        CommonSettingsServer::commonSettingsServer.datapackHashServerMain.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
        SHA224_Final(reinterpret_cast<unsigned char *>(CommonSettingsServer::commonSettingsServer.datapackHashServerMain.data()),&hashMain);
    }
    //do the sub
    if(GlobalServerData::serverPrivateVariables.subDatapackFolder.size()>0)
    {
        SHA256_CTX hashSub;
        if(SHA224_Init(&hashSub)!=1)
        {
            std::cerr << "SHA224_Init(&hashBase)!=1" << std::endl;
            abort();
        }
        const std::unordered_map<std::string,BaseServerMasterSendDatapack::DatapackCacheFile> &pair=Client::datapack_file_list(GlobalServerData::serverPrivateVariables.subDatapackFolder,"",false);
        std::vector<std::string> datapack_file_temp=unordered_map_keys_vector(pair);
        std::sort(datapack_file_temp.begin(), datapack_file_temp.end());
        unsigned int index=0;
        while(index<datapack_file_temp.size()) {
            //right file name + extention check already done into Client::datapack_file_list() by if(regex_search(fileName,GlobalServerData::serverPrivateVariables.datapack_rightFileName))
            {
                const std::string &fileName=datapack_file_temp.at(index);
                std::string fullPathFileToOpen=GlobalServerData::serverPrivateVariables.subDatapackFolder+fileName;
                #ifdef Q_OS_WIN32
                stringreplaceAll(fullPathFileToOpen,"/","\\");
                #endif
                FILE *filedesc=fopen(fullPathFileToOpen.c_str(),"rb");
                if(filedesc!=NULL)
                {
                    //read and load the file
                    const std::vector<char> &data=FacilityLibGeneral::readAllFileAndClose(filedesc);

                    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
                    if((1+fileName.size()+4+data.size())>=CATCHCHALLENGER_MAX_PACKET_SIZE)
                    {
                        if(GlobalServerData::serverSettings.max_players>1)//if not internal
                        {
                            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                            if(BaseServerMasterSendDatapack::compressedExtension.find(FacilityLibGeneral::getSuffix(fileName))!=BaseServerMasterSendDatapack::compressedExtension.end())
                            {
                                if(ProtocolParsing::compressionTypeServer==ProtocolParsing::CompressionType::None)
                                {
                                    std::cerr << "The file " << GlobalServerData::serverSettings.datapack_basePath << fileName << " is over the maximum packet size, but can be compressed, try enable the compression" << std::endl;
                                    abort();
                                }
                            }
                            else
                            #endif
                            {
                                std::cerr << "The file " << GlobalServerData::serverSettings.datapack_basePath << fileName << " is over the maximum packet size" << std::endl;
                                abort();
                            }
                        }
                    }
                    #endif

                    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
                    //switch the data to correct hash or drop it
                    SHA256_CTX hashFile;
                    if(SHA224_Init(&hashFile)!=1)
                    {
                        std::cerr << "SHA224_Init(&hashBase)!=1" << std::endl;
                        abort();
                    }
                    SHA224_Update(&hashFile,data.data(),data.size());
                    BaseServerMasterSendDatapack::DatapackCacheFile cacheFile;
                    SHA224_Final(reinterpret_cast<unsigned char *>(ProtocolParsingBase::tempBigBufferForOutput),&hashFile);
                    ::memcpy(&cacheFile.partialHash,ProtocolParsingBase::tempBigBufferForOutput,sizeof(uint32_t));
                    Client::datapack_file_hash_cache_sub[fileName]=cacheFile;
                    #endif

                    SHA224_Update(&hashSub,data.data(),data.size());
                }
                else
                {
                    std::cerr << "Stop now! Unable to open the file " << GlobalServerData::serverSettings.datapack_basePath << fileName << " to do the datapack checksum for the mirror" << std::endl;
                    abort();
                }
            }
            index++;
        }
        CommonSettingsServer::commonSettingsServer.datapackHashServerSub.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
        SHA224_Final(reinterpret_cast<unsigned char *>(CommonSettingsServer::commonSettingsServer.datapackHashServerSub.data()),&hashSub);
    }

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    if(BaseServerMasterSendDatapack::datapack_file_hash_cache_base.size()==0)
    {
        std::cout << "0 file for datapack loaded base (abort)" << std::endl;
        abort();
    }
    #endif
    #endif
    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    if(Client::datapack_file_hash_cache_main.size()==0)
    {
        std::cout << "0 file for datapack loaded main (abort)" << std::endl;
        abort();
    }

    std::cout << BaseServerMasterSendDatapack::datapack_file_hash_cache_base.size()
              << " file for datapack loaded base, "
              << Client::datapack_file_hash_cache_main.size()
              << " file for datapack loaded main, "
              << Client::datapack_file_hash_cache_sub.size()
              << " file for datapack loaded sub" << std::endl;
    #endif
    std::cout << binarytoHexa(CommonSettingsCommon::commonSettingsCommon.datapackHashBase)
              << " hash for datapack loaded base, "
              << binarytoHexa(CommonSettingsServer::commonSettingsServer.datapackHashServerMain)
              << " hash for datapack loaded main, "
              << binarytoHexa(CommonSettingsServer::commonSettingsServer.datapackHashServerSub)
              << " hash for datapack loaded sub" << std::endl;
}
