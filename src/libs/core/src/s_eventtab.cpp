#include "s_eventtab.h"

#include "string_compare.hpp"
#include "message.h"

#define HASHT_INDEX(x) (uint8_t)(x >> 24)
#define HASHT_CODE(x) (x & 0xffffff)
#define HASH2INDEX(x) (uint8_t)(x & 0x2f)

S_EVENTTAB::S_EVENTTAB()
{
    for (uint32_t n = 0; n < HASHTABLE_SIZE; n++)
    {
        Buffer_size[n] = 0;
        Event_num[n] = 0;
    }
}

S_EVENTTAB::~S_EVENTTAB()
{
    Release();
}

void S_EVENTTAB::Clear()
{
    for (uint32_t i = 0; i < HASHTABLE_SIZE; i++)
    {
        for (uint32_t n = 0; n < Event_num[i]; n++)
        {
            for (uint32_t m = 0; m < pTable[i][n].pFuncInfo.size(); m++)
            {
                if (!pTable[i][n].pFuncInfo[m].bStatic)
                    pTable[i][n].pFuncInfo[m].status = FSTATUS_DELETED;
            }


            for (auto &elem : pTable[i][n].pFuncInfoForObjects)
            {
                for (auto &handler : elem.second)
                {
                    if (!handler.bStatic)
                        handler.status = FSTATUS_DELETED;
                }
            }


            // if(pTable[n].pFuncInfo) delete pTable[n].pFuncInfo;
            // if(pTable[n].name) delete pTable[n].name;
        }
        // delete pTable; pTable = 0;
        // Buffer_size = 0;
        // Event_num = 0;
    }
    ProcessFrame();
}

void S_EVENTTAB::Release()
{
    for (uint32_t i = 0; i < HASHTABLE_SIZE; i++)
    {
        for (uint32_t n = 0; n < Event_num[i]; n++)
        {
            delete[] pTable[i][n].name;
        }

        Buffer_size[i] = 0;
        Event_num[i] = 0;
    }
}

bool S_EVENTTAB::GetEvent(EVENTINFO &ei, uint32_t event_code)
{
    const auto ti = HASHT_INDEX(event_code);
    const auto tc = HASHT_CODE(event_code);
    if (tc >= Event_num[ti])
        return false;
    ei = pTable[ti][tc];
    return true;
}


uint32_t S_EVENTTAB::AddEventHandler(ATTRIBUTES *pObject, const char *event_name, uint32_t func_code, uint32_t func_segment_id, int32_t flag,
                                     bool bStatic)
{
    uint32_t i;
    uint32_t eventPos = 0;
    const auto hash = MakeHashValue(event_name);

    const auto ti = HASH2INDEX(hash);

    EVENTINFO *ei = FindEventByName(event_name, true, &eventPos);

    auto funcInfoVecP = &ei->pFuncInfo;

    if (pObject)
    {
        auto it = ei->pFuncInfoForObjects.find(pObject);
        if (it == ei->pFuncInfoForObjects.end())
        {
            ei->pFuncInfoForObjects[pObject] = std::vector<EVENT_FUNC_INFO>();
            funcInfoVecP = &ei->pFuncInfoForObjects[pObject];
        }
        else
        {
            funcInfoVecP = &it->second;
        }
    }

    auto &funcInfoVec = *funcInfoVecP;

    for (i = 0; i < funcInfoVec.size(); i++)
    {
        // event handler function already set
        if (funcInfoVec[i].func_code == func_code)
        {
            funcInfoVec[i].status = FSTATUS_NORMAL;
            funcInfoVec[i].bStatic = bStatic;
            return (((ti << 24) & 0xff000000) | (eventPos & 0xffffff));
        }
    }
    // add function
    i = funcInfoVec.size();
    
    funcInfoVec.resize(i + 1);

    funcInfoVec[i].func_code = func_code;
    funcInfoVec[i].segment_id = func_segment_id;
    if (flag)
        funcInfoVec[i].status = FSTATUS_NEW;
    else
        funcInfoVec[i].status = FSTATUS_NORMAL;
    funcInfoVec[i].bStatic = bStatic;
    // return n;
    return (((ti << 24) & 0xff000000) | (eventPos & 0xffffff));
}

uint32_t S_EVENTTAB::MakeHashValue(const char *string)
{
    uint32_t hval = 0;
    while (*string != 0)
    {
        auto v = *string++;
        if ('A' <= v && v <= 'Z')
            v += 'a' - 'A'; // case independent
        hval = (hval << 4) + static_cast<uint32_t>(v);
        const uint32_t g = hval & (static_cast<uint32_t>(0xf) << (32 - 4));
        if (g != 0)
        {
            hval ^= g >> (32 - 8);
            hval ^= g;
        }
    }
    return hval;
}

void S_EVENTTAB::SetStatus(ATTRIBUTES *pObject, const char *event_name, uint32_t func_code, uint32_t status)
{
    if (event_name == nullptr)
        return;

    const auto hash = MakeHashValue(event_name);
    const auto ti = HASH2INDEX(hash);

    for (uint32_t n = 0; n < Event_num[ti]; n++)
    {
        if (pTable[ti][n].hash == hash)
            if (storm::iEquals(pTable[ti][n].name, event_name))
            {
                if (!pObject)
                {
                    for (uint32_t i = 0; i < pTable[ti][n].pFuncInfo.size(); i++)
                    {
                        if (pTable[ti][n].pFuncInfo[i].func_code == func_code)
                        {
                            pTable[ti][n].pFuncInfo[i].status = status;
                            return;
                        }
                    }
                }
                else
                {
                    auto it = pTable[ti][n].pFuncInfoForObjects.find(pObject);
                    if (it == pTable[ti][n].pFuncInfoForObjects.end())
                        return;

                    for (auto &handler : it->second)
                    {
                        if (handler.func_code == func_code)
                        {
                            handler.status = status;
                            return;
                        }
                    }
                }
                
            }
    }
}

bool S_EVENTTAB::DelEventHandler(std::vector<EVENT_FUNC_INFO> &funcInfo, uint32_t func_code,
                                 bool bDelStatic)
{

    if (!bDelStatic)
    {
        if (funcInfo[func_code].bStatic)
        {
            return false;
        }
    }

    for (auto n = func_code; n < funcInfo.size() - 1; n++)
    {
        funcInfo[n] = funcInfo[n + 1];
    }
    funcInfo.resize(funcInfo.size() - 1);
    return true;
}

void S_EVENTTAB::InvalidateBySegmentID(uint32_t segment_id)
{
    for (uint32_t ti = 0; ti < HASHTABLE_SIZE; ti++)
    {
        for (uint32_t n = 0; n < Event_num[ti]; n++)
        {
            for (uint32_t i = 0; i < pTable[ti][n].pFuncInfo.size(); i++)
            {
                if (pTable[ti][n].pFuncInfo[i].segment_id == segment_id)
                {
                    if (DelEventHandler(pTable[ti][n].pFuncInfo, i, true))
                        i = 0;
                }
            }

            for (auto &elem : pTable[ti][n].pFuncInfoForObjects)
            {
                for (uint32_t i = 0; i < elem.second.size(); i++)
                {
                    if (elem.second[i].segment_id == segment_id)
                    {
                        if (DelEventHandler(elem.second, i, true))
                            i--;
                    }
                }
            }

        }
    }
}

uint32_t S_EVENTTAB::FindEvent(const char *event_name)
{
    if (event_name == nullptr)
        return INVALID_EVENT_CODE;
    const auto hash = MakeHashValue(event_name);
    const auto ti = HASH2INDEX(hash);
    for (uint32_t n = 0; n < Event_num[ti]; n++)
    {
        if (pTable[ti][n].hash == hash)
            if (storm::iEquals(pTable[ti][n].name, event_name))
                return (((ti << 24) & 0xff000000) | (n & 0xffffff));
    }
    return INVALID_EVENT_CODE;
}

void S_EVENTTAB::ProcessFrame()
{
    for (uint32_t ti = 0; ti < HASHTABLE_SIZE; ti++)
        for (uint32_t n = 0; n < Event_num[ti]; n++)
        {
            // delete old handlers
            for (uint32_t i = 0; i < pTable[ti][n].pFuncInfo.size(); i++)
            {
                if (pTable[ti][n].pFuncInfo[i].status == FSTATUS_DELETED)
                {
                    DelEventHandler(pTable[ti][n].pFuncInfo, i);
                    i = 0;
                }
                else
                {
                    pTable[ti][n].pFuncInfo[i].status = FSTATUS_NORMAL;
                }
            }

            for (auto &elem : pTable[ti][n].pFuncInfoForObjects)
            {
                for (uint32_t i = 0; i < elem.second.size(); i++)
                {
                    if (elem.second[i].status == FSTATUS_DELETED)
                    {
                        if (DelEventHandler(elem.second, i, true))
                            i--;
                    }
                    else
                    {
                        elem.second[i].status = FSTATUS_NORMAL;
                    }
                }
            }
        }
}

bool S_EVENTTAB::StoreEventsData(ATTRIBUTES *attr, FuncTable &FuncTab,
                                 std::unordered_map<void *, std::pair<std::string, std::vector<size_t>>> &varIndex,
                                 VIRTUAL_COMPILER *compiler)
{
    auto &eventTable = attr->CreateAttribute(std::string("eventTable"));
    
    for (size_t ti = 0; ti < HASHTABLE_SIZE; ti++)
    {
        for (size_t n = 0; n < pTable[ti].size(); n++)
        {
            auto &curEvent = pTable[ti][n];

            auto eventName = curEvent.name;
            if (!eventName)
            {
                continue;
            }

            auto &curEventRecord = eventTable.CreateAttribute(std::string(curEvent.name));

            if (curEvent.format != std::nullopt)
            {
                curEventRecord.CreateAttribute(std::string("format"), curEvent.format.value().c_str());
            }

            auto &commonEventTable = curEventRecord.CreateAttribute(std::string("common"));
            for (size_t i = 0; i < curEvent.pFuncInfo.size(); i++)
            {
                auto &curFunInfo = curEvent.pFuncInfo[i];
                if (curFunInfo.status == FSTATUS_DELETED)
                    continue;

                FuncInfo fi;
                FuncTab.GetFunc(fi, curFunInfo.func_code);
                const char *isStatic = curFunInfo.bStatic ? "1" : "0";
                commonEventTable.CreateAttribute(fi.name, isStatic);
            }

            auto &objectEventTable = curEventRecord.CreateAttribute(std::string("object"));
            size_t counter = 0;
            for (auto& cur : curEvent.pFuncInfoForObjects)
            {
                auto &curObjectEventTable = objectEventTable.CreateAttribute(fmt::format("{}", counter));
                counter++;


                if (!StoreAttributesRef(&curObjectEventTable, cur.first, varIndex, compiler))
                {
                    compiler->SetError("Attempting to save handler for local object");
                    return false;
                }

                auto &curObjectHandlerTable = curObjectEventTable.CreateAttribute(std::string("handlers"));
                for (size_t i = 0; i < cur.second.size(); i++)
                {
                    auto &curFunInfo = cur.second[i];
                    if (curFunInfo.status == FSTATUS_DELETED)
                        continue;

                    FuncInfo fi;
                    FuncTab.GetFunc(fi, curFunInfo.func_code);
                    const char *isStatic = curFunInfo.bStatic ? "1" : "0";
                    curObjectHandlerTable.CreateAttribute(fi.name, isStatic);
                }
            }
        }
    }
    return true;
}
bool S_EVENTTAB::LoadEventsData(ATTRIBUTES *attr, FuncTable &FuncTab, VarTable &VarTab, VIRTUAL_COMPILER *compiler)
{
    auto eventTable = attr->GetAttributeClass(std::string("eventTable"));
    if (!eventTable)
    {
        compiler->SetError("eventTable not found");
        return false;
    }


    auto eventCount = eventTable->GetAttributesNum();
    for (size_t i = 0; i < eventCount; i++)
    {
        auto curEventRecord = eventTable->GetAttributeClass(i);
        if (!curEventRecord)
        {
            compiler->SetError("eventTable[%u] not found", (unsigned)i);
            return false;
        }

        auto eventName = curEventRecord->GetThisName();

        auto formatEventRecord = curEventRecord->GetAttributeClass(std::string("format"));
        if (formatEventRecord)
        {
            SetEventFormat(eventName, formatEventRecord->GetValue());
        }

        auto commonEventTable = curEventRecord->GetAttributeClass(std::string("common"));

        if (!commonEventTable)
        {
            compiler->SetError("eventTable[%u].common not found", (unsigned)i);
            return false;
        }
        auto eventFuncCount = commonEventTable->GetAttributesNum();

        for (size_t j = 0; j < eventFuncCount; j++)
        {
            auto curFuncRecord = commonEventTable->GetAttributeClass(j);
            if (!curFuncRecord)
            {
                compiler->SetError("eventTable.common.%s[%u] not found", eventName, (unsigned)j);
                return false;
            }
            auto funcName = curFuncRecord->GetThisName();


            FuncInfo fi;
            const uint32_t func_code = FuncTab.FindFunc(funcName);
            if (func_code == INVALID_FUNC_CODE)
            {
                compiler->SetError("Invalid function code douring event loading");
                return false;
            }

            if (!FuncTab.GetFunc(fi, func_code))
            {
                compiler->SetError("funcion not found error");
                return false;
            }

            bool isStatic = curFuncRecord->GetValue() == std::string("1");
            AddEventHandler(nullptr, eventName, func_code, fi.segment_id, 1, isStatic);
        }

        auto objectEventTable = curEventRecord->GetAttributeClass(std::string("object"));

        if (!objectEventTable)
        {
            compiler->SetError("eventTable[%u].object not found", (unsigned)i);
            return false;
        }

        auto objectHandlersCount = objectEventTable->GetAttributesNum();

        for (size_t k = 0; k < objectHandlersCount; k++)
        {
            auto curObjectHandlerRecord = objectEventTable->GetAttributeClass(fmt::format("{}", k));
            if (!curObjectHandlerRecord)
            {
                compiler->SetError("eventTable[%u].object[%u] not found", (unsigned)i, (unsigned)k);
                return false;
            }

            ATTRIBUTES *objectPointer = LoadAttributesRef(curObjectHandlerRecord, VarTab, compiler);
            if (!objectPointer)
            {
                compiler->SetError("stored attribute reference not found");
                return false;
            }

            auto curObjectHandlerListRecord = curObjectHandlerRecord->GetAttributeClass("handlers");
            if (!curObjectHandlerListRecord)
            {
                compiler->SetError("eventTable[%u].object[%u].handlers not found", (unsigned)i, (unsigned)k);
                return false;
            }

            auto eventFuncCount = curObjectHandlerListRecord->GetAttributesNum();

            for (size_t j = 0; j < eventFuncCount; j++)
            {
                auto curFuncRecord = curObjectHandlerListRecord->GetAttributeClass(j);
                if (!curFuncRecord)
                {
                    compiler->SetError("eventTable.common.%s[%u] not found", eventName, (unsigned)j);
                    return false;
                }
                auto funcName = curFuncRecord->GetThisName();

                FuncInfo fi;
                const uint32_t func_code = FuncTab.FindFunc(funcName);
                if (func_code == INVALID_FUNC_CODE)
                {
                    compiler->SetError("Invalid function code douring event loading");
                    return false;
                }

                if (!FuncTab.GetFunc(fi, func_code))
                {
                    compiler->SetError("function not found error");
                    return false;
                }

                bool isStatic = curFuncRecord->GetValue() == std::string("1");
                AddEventHandler(objectPointer, eventName, func_code, fi.segment_id, 1, isStatic);
            }
        }

    }
    return true;
}


EVENTINFO *S_EVENTTAB::FindEventByName(const char *eventName, bool createIsNotFound, uint32_t *pEventPos)
{
    uint32_t i;
    uint32_t eventPos = 0;
    const auto hash = MakeHashValue(eventName);

    const auto ti = HASH2INDEX(hash);

    EVENTINFO *ei = nullptr;
    for (; eventPos < Event_num[ti]; eventPos++)
    {
        if (pTable[ti][eventPos].hash == hash)
        {
            if (storm::iEquals(eventName, pTable[ti][eventPos].name))
            {
                // event already in list
                ei = &pTable[ti][eventPos];
                break;
            }
        }
    }

    if (!ei && !createIsNotFound)
    {
        return nullptr;
    } 
    else if (!ei)
    {
        if (Event_num[ti] >= Buffer_size[ti])
        {
            Buffer_size[ti] += BUFFER_BLOCK_SIZE;
            pTable[ti].resize(Buffer_size[ti]);
        }

        pTable[ti][Event_num[ti]].hash = hash;
        pTable[ti][Event_num[ti]].name = nullptr;

        if constexpr (true) // bKeepName)
        {
            if (eventName)
            {
                const auto len = strlen(eventName) + 1;
                pTable[ti][Event_num[ti]].name = new char[len];
                memcpy(pTable[ti][Event_num[ti]].name, eventName, len);
            }
        }
        ei = &pTable[ti][Event_num[ti]];
        eventPos = Event_num[ti];
        Event_num[ti]++;
    }
    if (pEventPos)
    {
        *pEventPos = eventPos;
    }
    return ei;
}

void S_EVENTTAB::SetEventFormat(const char *eventName, std::string format)
{
    auto ei = FindEventByName(eventName, true);
    ei->format = format;
}

std::optional<std::string> S_EVENTTAB::GetEventFormat(const char *eventName)
{
    auto ei = FindEventByName(eventName, false);

    if (!ei)
    {
        return std::nullopt;
    }

    return ei->format;
}
