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
#ifndef MAPLEBE_INCLUDE_TARGET_REGISTRY_H
#define MAPLEBE_INCLUDE_TARGET_REGISTRY_H
#include "types_def.h"
#include "cg.h"
#if TARGX86_64
#include "x86_64/assembler/assembler.h"
#endif
#include "target_machine.h"
#include "mempool.h"


namespace maplebe {
using namespace maple;
class TargetMachine;
class Target {
using CGCtorFnTy = CG *(*)(MIRModule &mod, const CGOptions &opts, const std::vector<std::string> &nameVec,
                                   const std::unordered_map<std::string, std::vector<std::string>> &patternMap);
using EmitterCtorFnTy = std::function<Emitter*(CG&, const std::string&)>;
#if TARGX86_64
using DecoupledEmitterCtorFnTy = std::function<Emitter*(CG &cg, assembler::Assembler &newAssembler)>;
#endif
using TargetMachineCtorFnTy = std::function<TargetMachine*()>;
friend struct TargetRegistry;

public:
    Target() = default;
    
    const Target *getNext() const
    {
        return next;
    }

    const std::string getName() const
    {
        return name;
    }

    CG *createCG(MIRModule &mod, const CGOptions &opts, const std::vector<std::string> &nameVec,
        const std::unordered_map<std::string, std::vector<std::string>> &patternMap) const
    {
        if (!CGCtorFn) {
            return nullptr;
        }
        return CGCtorFn(mod, opts, nameVec, patternMap);
    }

    Emitter *createEmitter(CG &cg, const std::string &asmFileName) const
    {
        if (!EmitterCtorFn) {
            return nullptr;
        }
        return EmitterCtorFn(cg, asmFileName);
    }

#if TARGX86_64
    Emitter *createDecoupledEmitter(CG &cg, assembler::Assembler &newAssembler) const
    {
        if (!DecoupedEmitterCtorFn) {
            return nullptr;
        }
        return DecoupedEmitterCtorFn(cg, newAssembler);
    }
#endif
    TargetMachine *createTargetMachine() const
    {
        if (!TargetMachineCtorFn) {
            return nullptr;
        }
        return TargetMachineCtorFn();
    }

private:
    // Next - The next registered target in the linked list, maintained by the
    // TargetRegistry.
    Target *next = nullptr;
    // Name - The target name.
    std::string name;
    // Construction function for this target's CG, if
    // registered (default = nullptr).
    CGCtorFnTy CGCtorFn;
    // Construction function for this target's Emitter, if
    // registered (default = nullptr).
    EmitterCtorFnTy EmitterCtorFn;
#if TARGX86_64
    // Construction function for this target's DecoupledEmitter, if
    // registered (default = nullptr).
    DecoupledEmitterCtorFnTy DecoupedEmitterCtorFn;
#endif
    // Construction function for this target's TargetMachine, if
    // registered (default = nullptr).
    TargetMachineCtorFnTy TargetMachineCtorFn;
};

struct TargetRegistry {
    TargetRegistry() = delete;
    static void RegisterTarget(Target &t, const std::string name);
    static Target *lookupTarget(const std::string &targetName);

    static void RegisterCGFunc(Target &t, Target::CGCtorFnTy Fn)
    {
        t.CGCtorFn = Fn;
    }

    static void RegisterEmitter(Target &t, Target::EmitterCtorFnTy Fn)
    {
        t.EmitterCtorFn = Fn;
    }
#if TARGX86_64
    static void RegisterDecoupledEmitter(Target &t, Target::DecoupledEmitterCtorFnTy Fn)
    {
        t.DecoupedEmitterCtorFn = Fn;
    }
#endif
    static void RegisterTargetMachine(Target &t, Target::TargetMachineCtorFnTy Fn)
    {
        t.TargetMachineCtorFn = Fn;
    }
};

  
struct RegisterTarget {
    RegisterTarget(Target &t, const std::string name)
    {
        TargetRegistry::RegisterTarget(t, name);
    }
};

template <class CGImpl>
struct RegisterCGFUnc {
    RegisterCGFUnc(Target &T)
    {
        TargetRegistry::RegisterCGFunc(T, &Allocator);
    }

private:
    static CG *Allocator(MIRModule &mod, const CGOptions &opts, const std::vector<std::string> &nameVec,
                         const std::unordered_map<std::string, std::vector<std::string>> &patternMap)
    {
        return new CGImpl(mod, opts, nameVec, patternMap);
    }
};

template <class EmitterImpl>
struct RegisterEmitter {
    RegisterEmitter(Target &T, MemPool *m)
    {
        std::function<Emitter*(CG&, const std::string&)> Allocator = [m](CG &cg, const std::string &asmFileName) {
            return m->New<EmitterImpl>(cg, asmFileName);
        };
        TargetRegistry::RegisterEmitter(T, Allocator);
    }
};

#if TARGX86_64
template <class DecoupledEmitterImpl>
struct RegisterDecoupledEmitter {
    RegisterDecoupledEmitter(Target &T, MemPool *m)
    {
        std::function<Emitter*(CG &cg, assembler::Assembler &newAssembler)> Allocator = [m](CG &cg,
            assembler::Assembler &newAssembler) {
            return m->New<DecoupledEmitterImpl>(cg, newAssembler);
        };
        TargetRegistry::RegisterDecoupledEmitter(T, Allocator);
    }
};
#endif

template <class TargetMachineImpl>
struct RegisterTargetMachine {
    RegisterTargetMachine(Target &T, MemPool *m)
    {
        std::function<TargetMachine*()> Allocator = [m]() {
            return m->New<TargetMachineImpl>();
        };
        TargetRegistry::RegisterTargetMachine(T, Allocator);
    }
};

}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_TARGET_REGISTRY_H */
