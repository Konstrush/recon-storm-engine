#pragma once

#include <cstdarg>
#include <cstring>
#include <variant>
#include <unordered_map>

#include "c_vector.h"
#include "entity.h"

class VDATA;
class DATA;
class VIRTUAL_COMPILER;
class VarTable;

namespace storm
{
using MessageParam = std::variant<uint8_t, uint16_t, uint32_t, int32_t, float, double, ATTRIBUTES *, entid_t, VDATA *,
                                  CVECTOR, std::string>;

namespace detail {

template<typename T>
MessageParam convertMessageParam(T value)
{
    return MessageParam(value);
}

template<>
inline MessageParam convertMessageParam(const bool value)
{
    return {value ? 1 : 0};
}

// Convert uint32_t to int32_t to prevent conversion issues
template<>
inline MessageParam convertMessageParam(const uint32_t value)
{
    return MessageParam(static_cast<int32_t>(value));
}

} // namespace detail

} // namespace storm

class MESSAGE final
{
  public:
    void Move2Start();

    uint8_t Byte();
    uint16_t Word();
    int32_t Long();
    int32_t Dword();
    float Float();
    double Double();
    uintptr_t Pointer();
    ATTRIBUTES *AttributePointer();
    entid_t EntityID();
    VDATA *ScriptVariablePointer();
    CVECTOR CVector();
    const std::string &String();


    bool Set(uint8_t value);
    bool Set(uint16_t value);
    bool Set(int32_t value);
    bool Set(uint32_t value);
    bool Set(uintptr_t value);
    bool Set(float value);
    bool Set(double value);
    bool Set(std::string value);
    bool SetEntity(entid_t value);
    bool Set(VDATA *value);
    bool Set(ATTRIBUTES *value);
    bool Set(CVECTOR value);

    void ValidateFormat(char c);
    void Reset(const std::string_view &format);

    template<typename... Args>
    void Reset(const std::string_view &format, Args... args)
    {
        Assert(format.size() == sizeof...(args));
        index = 0;
        format_ = format;
        params_ = {storm::detail::convertMessageParam(args)...};
    }

    void ResetVA(const std::string_view &format, va_list &args);
    char GetCurrentFormatType();;
    const char *StringPointer();

    [[nodiscard]] std::string_view GetFormat() const;
    [[nodiscard]] bool ParamValid() const;

    size_t GetParametersCount() const;
    void GetData(DATA *vd, COMPILER *comp);
    bool StoreData(ATTRIBUTES *attr, std::unordered_map<void *, std::pair<std::string, std::vector<size_t>>> &varIndex,
                   VIRTUAL_COMPILER *compiler) const;
    static MESSAGE *LoadData(ATTRIBUTES *attr, VarTable &VarTab, VIRTUAL_COMPILER *compiler);
    void FixEnitiyIDs();

    ATTRIBUTES *GetThisObject();
    void SetThisObject(ATTRIBUTES *);
  private:
    static storm::MessageParam GetParamValue(const char c, va_list &args);

    std::string format_;
    std::vector<storm::MessageParam> params_;
    int32_t index{};
    bool hasThisObject_{false};
};

ATTRIBUTES *LoadAttributesRef(ATTRIBUTES *attr, VarTable &VarTab, VIRTUAL_COMPILER *compiler);

//false - local variable reference; otherwise - true
bool StoreAttributesRef(ATTRIBUTES *attr, const ATTRIBUTES *val,
                        std::unordered_map<void *, std::pair<std::string, std::vector<size_t>>> &varIndex,
                        VIRTUAL_COMPILER *compiler);
