#pragma once

#include "vma.hpp"

#include "script_libriary.h"
#include "../string_service.h"
#include <unordered_set>

//-----------SDEVICE-----------
class STRSERVICE : public VSTRSERVICE
{
    struct UsersStringBlock
    {
        int32_t nref;
        char *fileName;
        int32_t blockID;
        int32_t nStringsQuantity;
        char **psStrName;
        char **psString;

        UsersStringBlock *next;
    };

  public:
    STRSERVICE();
    ~STRSERVICE() override;

    bool Init() override;
    bool LoadState(ENTITY_STATE *state) override;
    bool CreateState(ENTITY_STATE_GEN *state_gen) override;
    void RunStart() override;
    void RunEnd() override;

    void SetLanguage(const char *sLanguage) override;
    char *GetLanguage() override;

    void LoadCommonIniFile(const std::string &sFileName) override;
    void LoadCommonIniFileImpl(const std::string &sFileName);

    char *GetString(const char *stringName, char *sBuffer = nullptr, std::size_t bufferSize = 0) override;
    int32_t GetStringNum(const char *stringName) override;
    char *GetString(int32_t strNum) override;
    char *GetStringName(int32_t strNum) override;

    int32_t OpenUsersStringFile(const char *fileName) override;
    void CloseUsersStringFile(int32_t id) override;
    char *TranslateFromUsers(int32_t id, const char *inStr) override;

    void SetDialogSourceFile(const char *fileName);

    char *TranslateForDialog(const char *str)
    {
        return TranslateFromUsers(m_nDialogSourceFile, (char *)str);
    }

  protected:
    void LoadIni();
    int32_t GetFreeUsersID() const;
    bool GetNextUsersString(char *src, int32_t &idx, char **strName, char **strData) const;

  protected:
    char *m_sLanguage;
    char *m_sIniFileName;
    char *m_sLanguageDir;

    int32_t m_nStringQuantity;
    std::vector<char *> m_psStrName;
    std::vector<char *> m_psString;

    UsersStringBlock *m_pUsersBlocks;

    int32_t m_nDialogSourceFile;
    std::vector<std::string> m_sFilesToReload;
    std::unordered_set<std::string> m_sFilesLoaded;
};

class SCRIPT_INTERFACE_FUNCTIONS : public SCRIPT_LIBRIARY
{
  public:
    SCRIPT_INTERFACE_FUNCTIONS(){};

    ~SCRIPT_INTERFACE_FUNCTIONS() override{};
    bool Init() override;
};
