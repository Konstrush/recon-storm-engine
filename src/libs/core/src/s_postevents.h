#pragma once

#include "s_eventmsg.h"
#include "s_vartab.h"

class POSTEVENTS_LIST
{
    S_EVENTMSG **pTable;
    uint32_t nClassesNum;

  public:
    POSTEVENTS_LIST()
    {
        nClassesNum = 0;
        pTable = nullptr;
    };

    ~POSTEVENTS_LIST()
    {
        Release();
    };

    void Release()
    {
        if (pTable)
        {
            for (uint32_t n = 0; n < nClassesNum; n++)
                delete pTable[n];
            free(pTable);
            pTable = nullptr;
        }
        nClassesNum = 0;
    };

    void Add(S_EVENTMSG *pClass)
    {
        uint32_t n = nClassesNum;
        nClassesNum++;
        pTable = (S_EVENTMSG **)realloc(pTable, nClassesNum * sizeof(S_EVENTMSG *));
        pTable[n] = pClass;
    };

    void Del(uint32_t _n)
    {
        if (_n >= nClassesNum)
            return;
        delete pTable[_n];
        for (uint32_t n = _n; n < (nClassesNum - 1); n++)
            pTable[n] = pTable[n + 1];
        nClassesNum--;
    }

    S_EVENTMSG *Read(uint32_t _n)
    {
        if (_n >= nClassesNum)
            return nullptr;
        return pTable[_n];
    };

    uint32_t GetClassesNum()
    {
        return nClassesNum;
    }

    void InvalidateAll()
    {
        if (pTable)
        {
            for (uint32_t n = 0; n < nClassesNum; n++)
                pTable[n]->Invalidate();
        }
    };

    void RemoveInvalidated()
    {
        if (pTable)
        {
            for (uint32_t n = 0; n < nClassesNum; n++)
            {
                if (pTable[n]->bInvalide)
                {
                    Del(n);
                    n = 0;
                }
            }
        }
    }

    bool StoreEventsData(ATTRIBUTES *attr,
                         std::unordered_map<void *, std::pair<std::string, std::vector<size_t>>> &varIndex,
                         VIRTUAL_COMPILER *compiler)
    {
        auto &eventTable = attr->CreateAttribute(std::string("postEventTable"));

        if (pTable)
        {
            uint32_t count = 0;
            for (uint32_t n = 0; n < nClassesNum; n++)
            {
                if (!pTable[n]->IsValid())
                    continue;
                auto &curPostEvent = eventTable.CreateAttribute(fmt::format("{}", count));
                auto ret = pTable[n]->StoreData(&curPostEvent, varIndex, compiler);
                if (!ret)
                {
                    return false;
                }
                count++;
            }
        }
        return true;
    }
    bool LoadEventsData(ATTRIBUTES *attr, VarTable &VarTab, VIRTUAL_COMPILER *compiler)
    {
        auto eventTable = attr->GetAttributeClass(std::string("postEventTable"));
        if (!eventTable)
        {
            compiler->SetError("postEventTable not found");
            return false;
        }


        auto eventCount = eventTable->GetAttributesNum();
        for (size_t i = 0; i < eventCount; i++)
        {
            auto curEventRecord = eventTable->GetAttributeClass(fmt::format("{}", i));
            if (!curEventRecord)
            {
                compiler->SetError("postEventTable.%u not found", (unsigned)i);
                return false;
            }

            auto msg = S_EVENTMSG::LoadData(curEventRecord, VarTab, compiler);
            if (!msg)
            {
                compiler->SetError("unable to load event message");
                return false;
            }
            Add(msg);
        }
        return true;
    }

    void FixEnitiyIDs()
    {
        if (pTable)
        {
            uint32_t count = 0;
            for (uint32_t n = 0; n < nClassesNum; n++)
            {
                if (!pTable[n]->IsValid())
                    continue;
                pTable[n]->FixEnitiyIDs();
            }
        }
    }
};
