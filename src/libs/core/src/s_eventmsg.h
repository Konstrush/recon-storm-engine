#pragma once

#include "message.h"
#include "platform/platform.hpp"
#include "s_vartab.h"

class S_EVENTMSG
{
  public:
    uint32_t nTime;
    uint32_t nPeriod;

  public:
    MESSAGE *pMessageClass;
    char *pEventName;
    bool bProcess;
    bool bInvalide;

    S_EVENTMSG()
    {
        bInvalide = false;
        bProcess = false;
        pMessageClass = nullptr;
        nTime = 0;
        nPeriod = 0;
        pEventName = nullptr;
    };

    S_EVENTMSG(const char *_name, MESSAGE *_pc, uint32_t _period)
    {
        bInvalide = false;
        bProcess = false;
        pMessageClass = _pc;
        nTime = 0;
        nPeriod = _period;

        const auto len = strlen(_name) + 1;
        pEventName = new char[len];
        strcpy_s(pEventName, len, _name);
    };

    ~S_EVENTMSG()
    {
        if (pMessageClass)
            delete pMessageClass;
        if (pEventName)
            delete pEventName;
    };

    bool ProcessTime(uint32_t _DeltaTime)
    {
        nTime += _DeltaTime;
        bProcess = true;
        if (nPeriod == 0)
            return true;
        if (nTime >= nPeriod)
            return true;
        return false;
    };

    void Invalidate()
    {
        bInvalide = true;
    };

    bool IsValid() const
    {
        return !bInvalide && pEventName;
    }

    bool StoreData(ATTRIBUTES *attr, std::unordered_map<void *, std::pair<std::string, std::vector<size_t>>> &varIndex,
                   VIRTUAL_COMPILER *compiler) const
    {

        auto period = 0;
        if (nTime < nPeriod)
        {
            period = nPeriod - nTime;
        }

        attr->CreateAttribute(std::string("nPeriod"), fmt::format("{}", period).c_str());
        attr->CreateAttribute(std::string("pEventName"), pEventName);

        if (pMessageClass)
        {
            auto ret = pMessageClass->StoreData(&attr->CreateAttribute(std::string("pEventMsg")), varIndex, compiler);
            if (!ret)
            {
                return false;
            }
        }
        return true;
    }

    static S_EVENTMSG *LoadData(ATTRIBUTES *attr, VarTable &VarTab, VIRTUAL_COMPILER *compiler)
    {
        auto nPeriodRec = attr->GetAttributeClass(std::string("nPeriod"));
        if (!nPeriodRec)
        {
            compiler->SetError("nPeriod not found");
            return nullptr;
        }

        auto nPeriodSVal = nPeriodRec->GetValue();
        uint32_t period = std::stoll(nPeriodSVal);

        auto pEventNameRec = attr->GetAttributeClass(std::string("pEventName"));
        if (!pEventNameRec)
        {
            compiler->SetError("pEventName not found");
            return nullptr;
        }

        auto &pEventNameVal = pEventNameRec->GetValue();

        MESSAGE *msg = nullptr;

        auto MessageRec = attr->GetAttributeClass(std::string("pEventMsg"));
        if (MessageRec)
        {
            msg = MESSAGE::LoadData(MessageRec, VarTab, compiler);
            if (!msg)
            {
                return nullptr;
            }
        } 

        return new S_EVENTMSG(pEventNameVal.c_str(), msg, period);
    }

    void FixEnitiyIDs()
    {
        if (pMessageClass)
        {
            pMessageClass->FixEnitiyIDs();
        }
    }
};
