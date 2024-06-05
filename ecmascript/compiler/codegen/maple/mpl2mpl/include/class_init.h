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

#ifndef MPL2MPL_INCLUDE_CLASS_INIT_H
#define MPL2MPL_INCLUDE_CLASS_INIT_H
#include "phase_impl.h"
#include "class_hierarchy_phase.h"
#include "maple_phase_manager.h"

namespace maple {
class ClassInit : public FuncOptimizeImpl {
public:
    ClassInit(MIRModule &mod, KlassHierarchy *kh, bool dump) : FuncOptimizeImpl(mod, kh, dump) {}
    ~ClassInit() = default;

    FuncOptimizeImpl *Clone() override
    {
        return new ClassInit(*this);
    }

private:
    void GenClassInitCheckProfile(MIRFunction &func, const MIRSymbol &classInfo, StmtNode *clinit) const;
    void GenPreClassInitCheck(MIRFunction &func, const MIRSymbol &classInfo, const StmtNode *clinit) const;
    void GenPostClassInitCheck(MIRFunction &func, const MIRSymbol &classInfo, const StmtNode *clinit) const;
    MIRSymbol *GetClassInfo(const std::string &classname);
    bool CanRemoveClinitCheck(const std::string &clinitClassname) const;
};

}  // namespace maple
#endif  // MPL2MPL_INCLUDE_CLASS_INIT_H
