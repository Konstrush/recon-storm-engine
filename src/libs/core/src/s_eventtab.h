#pragma once

#include "data.h"
#include "s_functab.h"
#include <optional>

#define BUFFER_BLOCK_SIZE 4
#define INVALID_EVENT_CODE 0xffffffff
#define INVALID_SEGMENT_ID 0xffffffff

#define FSTATUS_NEW 0
#define FSTATUS_NORMAL 1
#define FSTATUS_DELETED 2

struct EVENT_FUNC_INFO
{
    uint32_t segment_id;
    uint32_t func_code;
    uint32_t status;
    bool bStatic;
};

struct EVENTINFO
{
    uint32_t hash;
    std::vector<EVENT_FUNC_INFO> pFuncInfo;
    std::unordered_map<ATTRIBUTES *, std::vector<EVENT_FUNC_INFO>> pFuncInfoForObjects;
    char *name;
    std::optional<std::string> format{std::nullopt};
};

#define HASHTABLE_SIZE 64

class S_EVENTTAB
{
    uint32_t Buffer_size[HASHTABLE_SIZE];
    uint32_t Event_num[HASHTABLE_SIZE];
    std::vector<EVENTINFO> pTable[HASHTABLE_SIZE];

    bool DelEventHandler(std::vector<EVENT_FUNC_INFO> &funcInfo, uint32_t func_code, bool bDelStatic = false);
    EVENTINFO *FindEventByName(const char *eventName, bool createIsNotFound, uint32_t *pEventPos = nullptr);
  public:
    S_EVENTTAB();
    ~S_EVENTTAB();

    void SetStatus(ATTRIBUTES *pObject, const char *event_name, uint32_t func_code, uint32_t status);
    uint32_t AddEventHandler(ATTRIBUTES *objectPtr, const char *event_name, uint32_t func_code,
                             uint32_t func_segment_id, int32_t flag, bool bStatic = false);
    
    
    bool GetEvent(EVENTINFO &ei, uint32_t event_code); // return true if var registred and loaded
    uint32_t MakeHashValue(const char *string);
    //    void  KeepNameMode(bool on){bKeepName = on;};
    void Release();
    void Clear();
    void InvalidateBySegmentID(uint32_t segment_id);
    uint32_t FindEvent(const char *event_name);
    void ProcessFrame();
    bool StoreEventsData(ATTRIBUTES *attr, FuncTable &FuncTab,
                         std::unordered_map<void *, std::pair<std::string, std::vector<size_t>>> &varIndex,
                         VIRTUAL_COMPILER *compiler);
    bool LoadEventsData(ATTRIBUTES *attr, FuncTable &FuncTab, VarTable &VarTab, VIRTUAL_COMPILER* compiler);

    void SetEventFormat(const char *event_name, std::string format);
    std::optional<std::string> GetEventFormat(const char *event_name);
};
