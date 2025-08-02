#pragma once

#include "message.h"

#include <algorithm>

#include <fmt/format.h>
#include <cassert>
#include <v_data.h>
#include "s_vartab.h"
#include "core_impl.h"
#include "data.h"

void MESSAGE::Move2Start()
{
    index = 0;
}

uint8_t MESSAGE::Byte()
{
    ValidateFormat('b');
    return get<uint8_t>(params_[index - 1]);
}

uint16_t MESSAGE::Word()
{
    ValidateFormat('w');
    return get<uint16_t>(params_[index - 1]);
}

int32_t MESSAGE::Long()
{
    ValidateFormat('l');
    return get<int32_t>(params_[index - 1]);
}

int32_t MESSAGE::Dword()
{
    ValidateFormat('u');
    return static_cast<int32_t>(get<uint32_t>(params_[index - 1]));
}

float MESSAGE::Float()
{
    ValidateFormat('f');
    return get<float>(params_[index - 1]);
}

double MESSAGE::Double()
{
    ValidateFormat('d');
    return get<double>(params_[index - 1]);
}

uintptr_t MESSAGE::Pointer()
{
    ValidateFormat('p');
    return get<uintptr_t>(params_[index - 1]);
}

ATTRIBUTES * MESSAGE::AttributePointer()
{
    ValidateFormat('a');
    return get<ATTRIBUTES *>(params_[index - 1]);
}

entid_t MESSAGE::EntityID()
{
    ValidateFormat('i');
    return get<entid_t>(params_[index - 1]);
}

VDATA * MESSAGE::ScriptVariablePointer()
{
    ValidateFormat('e');
    return get<VDATA *>(params_[index - 1]);
}

CVECTOR MESSAGE::CVector()
{
    ValidateFormat('c');
    return get<CVECTOR>(params_[index - 1]);
}

const std::string & MESSAGE::String()
{
    ValidateFormat('s');
    return get<std::string>(params_[index - 1]);
}

bool MESSAGE::Set(uint8_t value)
{
    ValidateFormat('b');
    params_[index - 1] = value;
    return true;
}

bool MESSAGE::Set(uint16_t value)
{
    ValidateFormat('w');
    params_[index - 1] = value;
    return true;
}

bool MESSAGE::Set(int32_t value)
{
    ValidateFormat('l');
    params_[index - 1] = value;
    return true;
}

bool MESSAGE::Set(uint32_t value)
{
    ValidateFormat('u');
    params_[index - 1] = value;
    return true;
}

bool MESSAGE::Set(uintptr_t value)
{
    ValidateFormat('p');
    params_[index - 1] = value;
    return true;
}

bool MESSAGE::Set(float value)
{
    ValidateFormat('f');
    params_[index - 1] = value;
    return true;
}

bool MESSAGE::Set(double value)
{
    ValidateFormat('d');
    params_[index - 1] = value;
    return true;
}

bool MESSAGE::Set(std::string value)
{
    ValidateFormat('s');
    params_[index - 1] = std::move(value);
    return true;
}

bool MESSAGE::SetEntity(entid_t value)
{
    ValidateFormat('i');
    params_[index - 1] = value;
    return true;
}

bool MESSAGE::Set(VDATA *value)
{
    ValidateFormat('e');
    params_[index - 1] = value;
    return true;
}

bool MESSAGE::Set(ATTRIBUTES *value)
{
    ValidateFormat('a');
    params_[index - 1] = value;
    return true;
}

bool MESSAGE::Set(CVECTOR value)
{
    ValidateFormat('c');
    params_[index - 1] = value;
    return true;
}

void MESSAGE::ValidateFormat(char c)
{
    if (format_.empty())
        throw std::runtime_error("Read from empty message");
    if (format_[index] != c)
        throw std::runtime_error("Incorrect message data");
    index++;
}

void MESSAGE::Reset(const std::string_view &format)
{
    if (hasThisObject_ && index == 1 && format_ == "a")
    {
        format_ += format;
    }
    else
    {
        index = 0;
        format_ = format;
    }
    
    params_.resize(format_.size());
}

void MESSAGE::ResetVA(const std::string_view &format, va_list&args)
{
    if (hasThisObject_ && index == 1 && format_ == "a")
    {
        format_ += format;
    }
    else
    {
        index = 0;
        format_ = format;
    }

    params_.resize(format_.size());
    std::transform(format_.begin(), format_.end(), params_.begin(),
                   [&](const char c) { return GetParamValue(c, args); });
}

char MESSAGE::GetCurrentFormatType()
{
    return format_[index];
}

const char * MESSAGE::StringPointer()
{
    return String().c_str();
}

std::string_view MESSAGE::GetFormat() const
{
    return format_;
}

size_t MESSAGE::GetParametersCount() const
{
    return format_.size();
}

bool MESSAGE::ParamValid() const
{
    return index < format_.length();
}

storm::MessageParam MESSAGE::GetParamValue(const char c, va_list&args)
{
    switch (c)
    {
    case 'b':
        return va_arg(args, uint8_t);
    case 'w':
        return va_arg(args, uint16_t);
    case 'l':
        return va_arg(args, int32_t);
    case 'u':
        return va_arg(args, uint32_t);
    case 'f':
        return static_cast<float>(va_arg(args, double));
    case 'd':
        return va_arg(args, double);
    case 'p':
        return va_arg(args, uintptr_t);
    case 'a':
        return va_arg(args, ATTRIBUTES *);
    case 'i':
        return va_arg(args, entid_t);
    case 'e':
        return va_arg(args, VDATA *);
    case 'c':
        return va_arg(args, CVECTOR);
    case 's': {
        char *ptr = va_arg(args, char *);
        return std::string(ptr);
    }
    default:
        throw std::runtime_error(fmt::format("Unknown message format: '{}'", c));
    }
}

void MESSAGE::GetData(DATA *vd, COMPILER *comp)
{
    char format_sym;
    format_sym = GetCurrentFormatType();
    if (format_sym == 0)
    {
        comp->SetError("No (more) data on this event");
        return;
    }
    entid_t ent;
    std::string str;
    switch (format_sym)
    {
    case 'a':
        vd->SetType(VAR_AREFERENCE);
        vd->SetAReference(AttributePointer());
        return;
    case 'l':
        vd->Set(Long());
        return;
    case 'f':
        vd->Set(Float());
        return;
    case 's':
        str = String();
        vd->Set(str.c_str());
        return;
    case 'i':
        vd->SetType(VAR_AREFERENCE);
        ent = EntityID();
        vd->Set(ent);
        vd->SetAReference(core_internal.Entity_GetAttributePointer(ent));
        return;
    case 'e':
        DATA *pE;
        pE = static_cast<DATA *>(ScriptVariablePointer());
        vd->SetType(VAR_REFERENCE);
        vd->SetReference(pE);
        return;
    default:
        comp->SetError("Invalid data type in event message: '%c'", format_sym);
        return;
    }
}

ATTRIBUTES *MESSAGE::GetThisObject()
{
    if (!hasThisObject_)
    {
        return nullptr;
    }

    if (!format_.size() || format_[0] != 'a')
    {
        throw std::runtime_error(fmt::format("Wrong message format: '{}'; First elem should be an aref", format_));
    }

    return std::get<ATTRIBUTES *>(params_[0]);
}
void MESSAGE::SetThisObject(ATTRIBUTES *attr)
{
    if (format_.size())
    {
        throw std::runtime_error(fmt::format("Should be called for an empty object", format_));
    }
    format_ = "a";
    hasThisObject_ = true;
    index = 1;
    params_.push_back(attr);
}


bool StoreAttributesRef(ATTRIBUTES *attr, const ATTRIBUTES *val,
                        std::unordered_map<void *, std::pair<std::string, std::vector<size_t>>> &varIndex,
                        VIRTUAL_COMPILER *compiler)
{
    if (!val)
    {
        return true;
    }

    std::vector<std::string> attributes;
    const ATTRIBUTES *curAttr = val;
    while (true)
    {
        if (curAttr->GetParent() == nullptr)
        {
            break;
        }
        attributes.push_back(curAttr->GetThisName());
        curAttr = curAttr->GetParent();
    }

    auto globalVarData = varIndex.find((void *)curAttr);

    if (globalVarData == varIndex.end())
    {
        return false;
    }

    attr->CreateAttribute(std::string("varName"), globalVarData->second.first.c_str());
    auto indexes = globalVarData->second.second;

    if (indexes.size())
    {
        auto &indexesRec = attr->CreateAttribute(std::string("indexes"));
        for (size_t i = 0; i < indexes.size(); i++)
        {
            indexesRec.CreateAttribute(fmt::format("{}", i), fmt::format("{}", indexes[i]).c_str());
        }
    }

    if (attributes.size())
    {
        auto &attributesRec = attr->CreateAttribute(std::string("attributes"));
        for (size_t i = 0; i < attributes.size(); i++)
        {
            attributesRec.CreateAttribute(fmt::format("{}", i), attributes[i].c_str());
        }
    }
    return true;
}

static bool StoreParam(ATTRIBUTES *attr, const uint8_t &val)
{
    attr->CreateAttribute(std::string("value"), fmt::format("{}", val).c_str());
    return true;
}

static bool StoreParam(ATTRIBUTES *attr, const uint16_t &val)
{
    attr->CreateAttribute(std::string("value"), fmt::format("{}", val).c_str());
    return true;
}

static bool StoreParam(ATTRIBUTES *attr, const uint32_t &val)
{
    attr->CreateAttribute(std::string("value"), fmt::format("{}", val).c_str());
    return true;
}

static bool StoreParam(ATTRIBUTES *attr, const int32_t &val)
{
    attr->CreateAttribute(std::string("value"), fmt::format("{}", val).c_str());
    return true;
}

static bool StoreParam(ATTRIBUTES *attr, const float &val)
{
    attr->CreateAttribute(std::string("value"), fmt::format("{}", val).c_str());
    return true;
}

static bool StoreParam(ATTRIBUTES *attr, const double &val)
{
    attr->CreateAttribute(std::string("value"), fmt::format("{}", val).c_str());
    return true;
}

static bool StoreParam(ATTRIBUTES *attr, const uintptr_t &val)
{
    attr->CreateAttribute(std::string("value"), fmt::format("{}", val).c_str());
    return true;
}

static bool StoreParam(ATTRIBUTES *attr, const ATTRIBUTES *val,
                       std::unordered_map<void *, std::pair<std::string, std::vector<size_t>>> &varIndex,
                       VIRTUAL_COMPILER *compiler)
{
    if (!StoreAttributesRef(attr, val, varIndex, compiler))
    {
        auto &data = attr->CreateAttribute(std::string("value"));
        data = val->Copy();
    }
    return true;
}

static bool StoreParamEntity(ATTRIBUTES *attr, const entid_t &val,
                             std::unordered_map<void *, std::pair<std::string, std::vector<size_t>>> &varIndex,
                             VIRTUAL_COMPILER *compiler)
{
    auto data = core_internal.Entity_GetAttributePointer(val);
    return StoreParam(attr, data, varIndex, compiler);
}

static bool StoreParam(ATTRIBUTES *attr, VDATA *val,
                       std::unordered_map<void *, std::pair<std::string, std::vector<size_t>>> &varIndex,
                       VIRTUAL_COMPILER *compiler)
{
    entid_t eid = val->GetEntityID();
    attr->CreateAttribute(std::string("objectId"), fmt::format("{}", eid).c_str());
    auto attributes = val->GetAClass();
    return StoreParam(attr, attributes, varIndex, compiler);
}

static bool StoreParam(ATTRIBUTES *attr, const CVECTOR &val)
{
    attr->CreateAttribute(std::string("x"), fmt::format("{}", val.x).c_str());
    attr->CreateAttribute(std::string("y"), fmt::format("{}", val.y).c_str());
    attr->CreateAttribute(std::string("z"), fmt::format("{}", val.z).c_str());
    return true;
}

static bool StoreParam(ATTRIBUTES *attr, const std::string &val)
{
    attr->CreateAttribute(std::string("value"), fmt::format("{}", val).c_str());
    return true;
}


bool MESSAGE::StoreData(ATTRIBUTES *attr,
                        std::unordered_map<void *, std::pair<std::string, std::vector<size_t>>> &varIndex,
                        VIRTUAL_COMPILER *compiler) const
{
    attr->CreateAttribute(std::string("format"), format_.c_str());
    int ito = hasThisObject_;
    
    attr->CreateAttribute(std::string("isThisObject"), fmt::format("{}", ito).c_str());

    auto &paramsRecord = attr->CreateAttribute(std::string("params"));

    assert(format_.size() == params_.size());
    for (uint32_t i = 0; i < format_.size(); i++)
    {
        auto &curParamRecord = paramsRecord.CreateAttribute(fmt::format("{}", i));
        switch (format_[i])
        {
        case 'b':
            StoreParam(&curParamRecord, std::get<uint8_t>(params_[i]));
            continue;
        case 'w':
            StoreParam(&curParamRecord, std::get<uint16_t>(params_[i]));
            continue;
        case 'l':
            StoreParam(&curParamRecord, std::get<int32_t>(params_[i]));
            continue;
        case 'u':
            StoreParam(&curParamRecord, std::get<uint32_t>(params_[i]));
            continue;
        case 'f':
            StoreParam(&curParamRecord, std::get<float>(params_[i]));
            continue;
        case 'd':
            StoreParam(&curParamRecord, std::get<double>(params_[i]));
            continue;
        case 'p':
            StoreParam(&curParamRecord, std::get<uintptr_t>(params_[i]));
            continue;
        case 'a':
            StoreParam(&curParamRecord, std::get<ATTRIBUTES *>(params_[i]), varIndex, compiler);
            continue;
        case 'i':
            StoreParamEntity(&curParamRecord, std::get<entid_t>(params_[i]), varIndex, compiler);
            continue;
        case 'e':
            StoreParam(&curParamRecord, std::get<VDATA *>(params_[i]), varIndex, compiler);
            continue;
        case 'c':
            StoreParam(&curParamRecord, std::get<CVECTOR>(params_[i]));
            continue;
        case 's': {
            StoreParam(&curParamRecord, std::get<std::string>(params_[i]));
            continue;
        }
        default:
            compiler->SetError("Unknown message format: '%c'", format_[i]);
            return false;
        }
    }
    return true;
}

template <typename T> T LoadParam(ATTRIBUTES *attr, VIRTUAL_COMPILER *compiler, bool &status)
{
    static_assert(false, "Not implemented");
}

template <typename T> T LoadParam(ATTRIBUTES *attr, VarTable &VarTab, VIRTUAL_COMPILER *compiler, bool &status)
{
    static_assert(false, "Not implemented");
}


template <> uint8_t LoadParam<uint8_t>(ATTRIBUTES *attr, VIRTUAL_COMPILER *compiler, bool &status)
{
    auto valueAttr = attr->GetAttributeClass(std::string("value"));
    if (!valueAttr)
    {
        compiler->SetError("value not found");
        status = false;
        return 0;
    }

    auto &str = valueAttr->GetValue();
    status = true;
    return std::stoll(str.c_str());
}

template <> uint16_t LoadParam<uint16_t>(ATTRIBUTES *attr, VIRTUAL_COMPILER *compiler, bool &status)
{
    auto valueAttr = attr->GetAttributeClass(std::string("value"));
    if (!valueAttr)
    {
        compiler->SetError("value not found");
        status = false;
        return 0;
    }

    auto &str = valueAttr->GetValue();
    status = true;
    return std::stoll(str.c_str());
}

template <> uint32_t LoadParam<uint32_t>(ATTRIBUTES *attr, VIRTUAL_COMPILER *compiler, bool &status)
{
    auto valueAttr = attr->GetAttributeClass(std::string("value"));
    if (!valueAttr)
    {
        compiler->SetError("value not found");
        status = false;
        return 0;
    }

    auto &str = valueAttr->GetValue();
    status = true;
    return std::stoll(str.c_str());
}

template <> int32_t LoadParam<int32_t>(ATTRIBUTES *attr, VIRTUAL_COMPILER *compiler, bool &status)
{
    auto valueAttr = attr->GetAttributeClass(std::string("value"));
    if (!valueAttr)
    {
        compiler->SetError("value not found");
        status = false;
        return 0;
    }

    auto &str = valueAttr->GetValue();
    status = true;
    return std::stoll(str.c_str());
}

template <> float LoadParam<float>(ATTRIBUTES *attr, VIRTUAL_COMPILER *compiler, bool &status)
{
    auto valueAttr = attr->GetAttributeClass(std::string("value"));
    if (!valueAttr)
    {
        compiler->SetError("value not found");
        status = false;
        return 0;
    }

    auto &str = valueAttr->GetValue();
    status = true;
    return std::stold(str.c_str());
}

template <> double LoadParam<double>(ATTRIBUTES *attr, VIRTUAL_COMPILER *compiler, bool &status)
{
    auto valueAttr = attr->GetAttributeClass(std::string("value"));
    if (!valueAttr)
    {
        compiler->SetError("value not found");
        status = false;
        return 0;
    }

    auto &str = valueAttr->GetValue();
    status = true;
    return std::stold(str.c_str());
}

template <> uintptr_t LoadParam<uintptr_t>(ATTRIBUTES *attr, VIRTUAL_COMPILER *compiler, bool &status)
{
    compiler->SetWarning("'p' parameter is not supproted for messages");
    auto valueAttr = attr->GetAttributeClass(std::string("value"));
    if (!valueAttr)
    {
        compiler->SetError("value not found");
        status = false;
        return 0;
    }

    auto &str = valueAttr->GetValue();
    status = true;
    return std::stoll(str.c_str());
}

static DATA *FindVariable(std::string name, std::vector<size_t> indexes, VarTable &VarTab, VIRTUAL_COMPILER *compiler)
{
    auto varCode = VarTab.FindVar(name);
    const VarInfo *realVar;
    DATA *dt;
    if (varCode == INVALID_VAR_CODE)
    {
        compiler->SetError("Load warning - variable: '%s' not found", name.c_str());
        return nullptr;
    }
    else
    {
        realVar = VarTab.GetVarX(varCode);
        if (realVar == nullptr)
        {
            compiler->SetError("Load warning - variable: '%s' has invalid var code", name.c_str());
            return nullptr;
        }
        else
        {
            dt = realVar->value.get();
        }
    }

    for (auto idx : indexes)
    {
        dt = dt->GetArrayElement(idx);
        if (!dt)
        {
            compiler->SetError("Load warning - variable: '%s' has not index %u", name.c_str(), (unsigned)idx);
            return nullptr;
        }
    }

    return dt;
}

static DATA *LoadVariable(ATTRIBUTES *attr, VarTable &VarTab, VIRTUAL_COMPILER *compiler)
{
    auto varNameAttr = attr->GetAttributeClass(std::string("varName"));
    if (!varNameAttr)
    {
        compiler->SetError("varName not found");
        return nullptr;
    }

    auto &varName = varNameAttr->GetValue();

    std::vector<size_t> indexes;
    auto indexesRec = attr->GetAttributeClass(std::string("indexes"));
    if (indexesRec)
    {
        auto indexCount = indexesRec->GetAttributesNum();
        for (size_t i = 0; i < indexCount; i++)
        {
            auto curIndexRecord = indexesRec->GetAttributeClass(fmt::format("{}", i));
            if (!curIndexRecord)
            {
                compiler->SetError("indexes.%u not found", (unsigned)i);
                return nullptr;
            }

            indexes.push_back(std::stoll(curIndexRecord->GetValue().c_str()));
        }
    }

    return FindVariable(varName, indexes, VarTab, compiler);
}

ATTRIBUTES *LoadAttributesRef(ATTRIBUTES *attr, VarTable &VarTab, VIRTUAL_COMPILER *compiler)
{
    auto var = LoadVariable(attr, VarTab, compiler);
    if (!var)
    {
        return nullptr;
    }
    auto attributes = var->GetAClass();
    if (!attributes)
    {
        return nullptr;
    }

    auto attributesRec = attr->GetAttributeClass(std::string("attributes"));
    if (attributesRec)
    {
        auto attributesCount = attributesRec->GetAttributesNum();
        for (size_t i = 0; i < attributesCount; i++)
        {
            auto curAttributeRecord = attributesRec->GetAttributeClass(fmt::format("{}", i));
            if (!curAttributeRecord)
            {
                compiler->SetError("attributes.%u not found", (unsigned)i);
                return nullptr;
            }

            attributes = attributes->GetAttributeClass(curAttributeRecord->GetValue());
            if (!attributes)
            {
                compiler->SetError("missing attribute '%s'", curAttributeRecord->GetValue().c_str());
                return nullptr;
            }
        }
    }
    return attributes;
}


template <>
ATTRIBUTES *LoadParam<ATTRIBUTES *>(ATTRIBUTES *attr, VarTable &VarTab, VIRTUAL_COMPILER *compiler, bool &status)
{
    auto valueAttr = attr->GetAttributeClass(std::string("value"));
    if (valueAttr)
    {
        return valueAttr;
    }


    auto ret = LoadAttributesRef(attr, VarTab, compiler);
    status = ret != nullptr;
    return ret;
}

static DATA *LoadParamEntity(ATTRIBUTES *attr, VarTable &VarTab, VIRTUAL_COMPILER *compiler, bool &status)
{
    DATA *dt = LoadVariable(attr, VarTab, compiler);
    status = dt != nullptr;
    return dt;
}

template <> VDATA *LoadParam<VDATA *>(ATTRIBUTES *attr, VarTable &VarTab, VIRTUAL_COMPILER *compiler, bool &status)
{
    auto valueAttr = attr->GetAttributeClass(std::string("value"));
    if (valueAttr)
    {
        auto objectIdAttr = attr->GetAttributeClass(std::string("objectId"));
        if (!objectIdAttr)
        {
            compiler->SetError("objectId not found");
            status = false;
            return nullptr;
        }

        entid_t objectId = std::stoll(objectIdAttr->GetValue());
        status = true;
        return new DATA(objectId, valueAttr);
    }

    DATA *dt = LoadVariable(attr, VarTab, compiler);
    status = dt != nullptr;
    return dt;
}

template <> CVECTOR LoadParam<CVECTOR>(ATTRIBUTES *attr, VIRTUAL_COMPILER *compiler, bool &status)
{
    CVECTOR ret;

    auto xAttr = attr->GetAttributeClass(std::string("x"));
    if (!xAttr)
    {
        compiler->SetError("x not found");
        status = false;
        return ret;
    }

    auto yAttr = attr->GetAttributeClass(std::string("y"));
    if (!yAttr)
    {
        compiler->SetError("y not found");
        status = false;
        return ret;
    }

    auto zAttr = attr->GetAttributeClass(std::string("z"));
    if (!zAttr)
    {
        compiler->SetError("z not found");
        status = false;
        return ret;
    }
    ret.x = std::stold(xAttr->GetValue().c_str());
    ret.y = std::stold(yAttr->GetValue().c_str());
    ret.z = std::stold(zAttr->GetValue().c_str());
    status = true;
    return ret;
}

template <> std::string LoadParam<std::string>(ATTRIBUTES *attr, VIRTUAL_COMPILER *compiler, bool &status)
{
    auto valueAttr = attr->GetAttributeClass(std::string("value"));
    if (!valueAttr)
    {
        compiler->SetError("value not found");
        status = false;
        return "error";
    }

    status = true;
    return valueAttr->GetValue();
}

MESSAGE *MESSAGE::LoadData(ATTRIBUTES *attr, VarTable &VarTab, VIRTUAL_COMPILER *compiler)
{

    auto formatAttr = attr->GetAttributeClass(std::string("format"));
    if (!formatAttr)
    {
        compiler->SetError("format not found");
        return nullptr;
    }

    auto &formatStr = formatAttr->GetValue();


    auto isThisObjectAttr = attr->GetAttributeClass(std::string("isThisObject"));
    if (!isThisObjectAttr)
    {
        compiler->SetError("isThisObject not found");
        return nullptr;
    }

    auto ito = stoll(isThisObjectAttr->GetValue());

    auto paramsAttr = attr->GetAttributeClass(std::string("params"));
    if (!paramsAttr)
    {
        compiler->SetError("params not found");
        return nullptr;
    }

    auto paramsCount = paramsAttr->GetAttributesNum();
    if (paramsCount != formatStr.size())
    {
        compiler->SetError("params count != format length");
        return nullptr;
    }

    MESSAGE msg;
    msg.Reset(formatStr);
    msg.hasThisObject_ = (bool)ito;
    for (size_t i = 0; i < paramsCount; i++)
    {
        auto curParamRecord = paramsAttr->GetAttributeClass(fmt::format("{}", i));
        if (!curParamRecord)
        {
            compiler->SetError("params.%u not found", (unsigned)i);
            return nullptr;
        }
        DATA *ent = nullptr;
        bool status = false;
        switch (formatStr[i])
        {
        case 'b':
        {
            auto ret = LoadParam<uint8_t>(curParamRecord, compiler, status);
            if (!status)
            {
                return nullptr;
            }
            msg.Set(ret);
            break;
        }
        case 'w': 
        {
            auto ret = LoadParam<uint16_t>(curParamRecord, compiler, status);
            if (!status)
            {
                return nullptr;
            }
            msg.Set(ret);
            break;
        } 
        case 'l': 
        {
            auto ret = LoadParam<int32_t>(curParamRecord, compiler, status);
            if (!status)
            {
                return nullptr;
            }
            msg.Set(ret);
            break;
        }
        case 'u': 
        {
            auto ret = LoadParam<uint32_t>(curParamRecord, compiler, status);
            if (!status)
            {
                return nullptr;
            }
            msg.Set(ret);
            break;
        }
        case 'f': 
        {
            auto ret = LoadParam<float>(curParamRecord, compiler, status);
            if (!status)
            {
                return nullptr;
            }
            msg.Set(ret);
            break;
        }
        case 'd': 
        {
            auto ret = LoadParam<double>(curParamRecord, compiler, status);
            if (!status)
            {
                return nullptr;
            }
            msg.Set(ret);
            break;
        }
        case 'p': 
        {
            auto ret = LoadParam<uintptr_t>(curParamRecord, compiler, status);
            if (!status)
            {
                return nullptr;
            }
            msg.Set(ret);
            break;
        }
        case 'a': 
        {
            auto ret = LoadParam<ATTRIBUTES *>(curParamRecord, VarTab, compiler, status);
            if (!status)
            {
                return nullptr;
            }
            msg.Set(ret);
            break;
        }
        case 'i':
        {
            ent = LoadParamEntity(curParamRecord, VarTab, compiler, status);
            if (!status)
            {
                return nullptr;
            }
            msg.SetEntity(ent->GetEntityID());
            // Впоследсвии будет перезаписана новым GetEntityID(), после обновления сущностей
            msg.params_[i] = ent;
            break;
        }
        case 'e': 
        {
            auto ret = LoadParam<VDATA *>(curParamRecord, VarTab, compiler, status);
            if (!status)
            {
                return nullptr;
            }
            msg.Set(ret);
            break;
        }
        case 'c': 
        {
            auto ret = LoadParam<CVECTOR>(curParamRecord, compiler, status);
            if (!status)
            {
                return nullptr;
            }
            msg.Set(ret);
            break;
        }
        case 's': 
        {
            auto ret = LoadParam<std::string>(curParamRecord, compiler, status);
            if (!status)
            {
                return nullptr;
            }
            msg.Set(ret);
            break;
        }
        default:
            compiler->SetError("Unknown message format: '%c'", formatStr[i]);
            return nullptr;
        }
    }

    return new MESSAGE(std::move(msg));
}


void MESSAGE::FixEnitiyIDs()
{
    for (uint32_t i = 0; i < format_.size(); i++)
    {
        if (format_[i] == 'i')
        {
            params_[i] = std::get<VDATA*>(params_[i])->GetEntityID();
        }
    }
}
