/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_ESCAPE_ANALYSIS_H
#define ECMASCRIPT_COMPILER_ESCAPE_ANALYSIS_H

#include "ecmascript/compiler/combined_pass_visitor.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {

class EscapeAnalysis;
class FieldLocation;

class VirtualObject {
public:
    VirtualObject(size_t numIn, Chunk* chunk);
    void SetEscaped();
    bool IsEscaped() const;
    ChunkVector<GateRef>& GetUsers();
    void ClearUsers();
    void AddUser(GateRef gate);
    FieldLocation GetField(size_t offset);
private:
    ChunkVector<FieldLocation> fields_;
    // Gates which use this virtual object
    ChunkVector<GateRef> users_;
    bool escaped_ {false};
};

class FieldLocation {
public:
    FieldLocation() : id_(maxid++) {}
    FieldLocation(size_t id) : id_(id) {}
    bool operator == (const FieldLocation &v) const
    {
        return id_ == v.id_;
    }
    bool operator != (const FieldLocation &v) const
    {
        return id_ != v.id_;
    }
    bool operator < (const FieldLocation &v) const
    {
        return id_ < v.id_;
    }
    static constexpr size_t invalid = -1;
    static FieldLocation Invalid()
    {
        return FieldLocation(invalid);
    }
private:
    static size_t maxid;
    size_t id_;
};

class State {
public:
    explicit State(Chunk* chunk) : map_(chunk) {}
    void SetFieldValue(FieldLocation field, GateRef gate);
    GateRef GetFieldValue(FieldLocation field) const;
    bool IsMapEqual(const State &state) const;
    auto begin() const { return map_.begin(); }
    auto end() const { return map_.end(); }
private:
    ChunkMap<FieldLocation, GateRef> map_;
};

// When we visit a gate, we first create a GateInfo which maintains results and all information
// that may be used in this visit. After finishing this visit, virtual object and replacement
// of this gate must be reset.
class GateInfo {
public:
    GateInfo(Circuit* circuit, GateRef curGate, EscapeAnalysis* escapeAnalysis, Chunk* chunk);
    ~GateInfo();
    GateRef GetCurrentGate() const;
    GateRef GetFieldValue(FieldLocation field) const;
    void SetFieldValue(FieldLocation field, GateRef value);
    void SetEliminated();
    void SetVirtualObject(VirtualObject* object);
    State MergeState(GateRef gate);
    void SetReplacement(GateRef replacement);
private:
    Circuit* circuit_;
    GateAccessor acc_;
    GateRef curGate_;
    EscapeAnalysis* escapeAnalysis_;
    State state_;
    VirtualObject* object_ {nullptr};
    GateRef replacement_ {Circuit::NullGate()};
};

class EscapeAnalysis : public PassVisitor {
public:
    EscapeAnalysis(Circuit* circuit, RPOVisitor* visitor, Chunk* chunk, bool isTraced)
        : PassVisitor(circuit, chunk, visitor), circuit_(circuit), replacements_(chunk), gateToVirtualObject_(chunk),
          gateToState_(chunk), chunk_(chunk), isTraced_(isTraced) {}
    
    GateRef VisitGate(GateRef gate) override;
    void SetEscaped(GateRef gate);
    GateRef GetCurrentGate(GateRef gate) const;
    void RevisitUser(VirtualObject* vObj);
    GateRef TryGetReplacement(GateRef gate) const;
    void SetReplacement(GateRef gate, GateRef replacement);
    VirtualObject* TryGetVirtualObject(GateRef gate) const;
    VirtualObject* TryGetVirtualObjectAndAddUser(GateRef gate, GateRef currentGate);
    void SetVirtualObject(GateRef gate, VirtualObject* object);
    VirtualObject* GetOrCreateVirtualObject(size_t numIn, GateInfo* info);
    State& GetOrCreateState(GateRef gate);
    void SetState(GateRef gate, State state);
    void RevisitGate(GateRef gate);
    void SetReplaceGate(GateRef gate);
private:
    GateRef VisitCreateObjectWithBuffer(GateRef gate, GateInfo* info);
    GateRef VisitLoadProperty(GateRef gate, GateInfo* info);
    GateRef VisitLoadConstOffset(GateRef gate, GateInfo* info);
    GateRef VisitStoreProperty(GateRef gate, GateInfo* info);
    GateRef VisitObjectTypeCheck(GateRef gate, GateInfo* info);
    
    Circuit *circuit_ {nullptr};
    ChunkMap<GateRef, GateRef> replacements_;
    ChunkMap<GateRef, VirtualObject*> gateToVirtualObject_;
    ChunkMap<GateRef, State> gateToState_;
    GateRef replaceGate_ {Circuit::NullGate()};
    Chunk* chunk_;
    bool isTraced_;
};

};

#endif