/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MAPLEIR_VERIFY_PRAGMA_INFO_H
#define MAPLEIR_VERIFY_PRAGMA_INFO_H
#include <string>
#include <utility>

namespace maple {
enum PragmaInfoType { kThrowVerifyError, kAssignableCheck, kExtendFinalCheck, kOverrideFinalCheck };

class VerifyPragmaInfo {
public:
    VerifyPragmaInfo() = default;
    ;
    virtual ~VerifyPragmaInfo() = default;

    virtual PragmaInfoType GetPragmaType() const = 0;
    bool IsEqualTo(const VerifyPragmaInfo &pragmaInfo) const
    {
        return GetPragmaType() == pragmaInfo.GetPragmaType();
    }
    bool IsVerifyError() const
    {
        return GetPragmaType() == kThrowVerifyError;
    }
    bool IsAssignableCheck() const
    {
        return GetPragmaType() == kAssignableCheck;
    }
    bool IsExtendFinalCheck() const
    {
        return GetPragmaType() == kExtendFinalCheck;
    }
    bool IsOverrideFinalCheck() const
    {
        return GetPragmaType() == kOverrideFinalCheck;
    }
};

class ThrowVerifyErrorPragma : public VerifyPragmaInfo {
public:
    explicit ThrowVerifyErrorPragma(std::string errorMessage)
        : VerifyPragmaInfo(), errorMessage(std::move(errorMessage))
    {
    }
    ~ThrowVerifyErrorPragma() = default;

    PragmaInfoType GetPragmaType() const override
    {
        return kThrowVerifyError;
    }

    const std::string &GetMessage() const
    {
        return errorMessage;
    }

private:
    std::string errorMessage;
};

class AssignableCheckPragma : public VerifyPragmaInfo {
public:
    AssignableCheckPragma(std::string fromType, std::string toType)
        : VerifyPragmaInfo(), fromType(std::move(fromType)), toType(std::move(toType))
    {
    }
    ~AssignableCheckPragma() = default;

    PragmaInfoType GetPragmaType() const override
    {
        return kAssignableCheck;
    }

    bool IsEqualTo(const AssignableCheckPragma &pragma) const
    {
        return fromType == pragma.GetFromType() && toType == pragma.GetToType();
    }

    const std::string &GetFromType() const
    {
        return fromType;
    }

    const std::string &GetToType() const
    {
        return toType;
    }

private:
    std::string fromType;
    std::string toType;
};

class ExtendFinalCheckPragma : public VerifyPragmaInfo {
public:
    ExtendFinalCheckPragma() : VerifyPragmaInfo() {}
    ~ExtendFinalCheckPragma() = default;

    PragmaInfoType GetPragmaType() const override
    {
        return kExtendFinalCheck;
    }
};

class OverrideFinalCheckPragma : public VerifyPragmaInfo {
public:
    OverrideFinalCheckPragma() : VerifyPragmaInfo() {}
    ~OverrideFinalCheckPragma() = default;

    PragmaInfoType GetPragmaType() const override
    {
        return kOverrideFinalCheck;
    }
};
}  // namespace maple
#endif  // MAPLEIR_VERIFY_PRAGMA_INFO_H
