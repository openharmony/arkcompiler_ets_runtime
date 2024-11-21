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

#ifndef ECMASCRIPT_COMPILER_PGO_TYPE_PGO_TYPE_PARSER_H
#define ECMASCRIPT_COMPILER_PGO_TYPE_PGO_TYPE_PARSER_H

#include "ecmascript/compiler/bytecode_info_collector.h"
#include "ecmascript/compiler/pgo_bc_info.h"
#include "ecmascript/compiler/pgo_type/pgo_hclass_generator.h"
#include "ecmascript/compiler/pgo_type/pgo_type_manager.h"
#include "ecmascript/compiler/pgo_type/pgo_type_recorder.h"
#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"

namespace panda::ecmascript::kungfu {
class BaseParser {
public:
    BaseParser(PGOTypeManager *ptManager, PGOBCInfo::Type type) : ptManager_(ptManager), type_(type) {}
    virtual ~BaseParser() = default;

    void Parse(const BytecodeInfoCollector &collector, const PGOTypeRecorder &typeRecorder,
        const PGOHClassGenerator &generator, uint32_t methodOffset);

protected:
    virtual bool RecordTypeInfo(const PGODefineOpType &defType, const PGOTypeLocation &loc) = 0;
    virtual void GenerateHClass(
        [[maybe_unused]] const PGOHClassGenerator &generator, [[maybe_unused]] const PGOTypeLocation &loc) {};

    PGOTypeManager *ptManager_ { nullptr };
    PGOBCInfo::Type type_;
};

class ClassParser final : public BaseParser {
public:
    ClassParser(PGOTypeManager *ptManager) : BaseParser(ptManager, PGOBCInfo::Type::CLASS) {}

private:
    virtual bool RecordTypeInfo(const PGODefineOpType &defType, const PGOTypeLocation &loc) override;
    virtual void GenerateHClass(const PGOHClassGenerator &generator, const PGOTypeLocation &loc) override;
};

class ArrayParser : public BaseParser {
public:
    ArrayParser(PGOTypeManager *ptManager, PGOBCInfo::Type type) : BaseParser(ptManager, type) {}

private:
    virtual bool RecordTypeInfo(const PGODefineOpType &defType, const PGOTypeLocation &loc) override;
};

class EmptyArrayParser final : public ArrayParser {
public:
    EmptyArrayParser(PGOTypeManager *ptManager) : ArrayParser(ptManager, PGOBCInfo::Type::EMPTY_ARRAY) {}
};

class ArrayLiteralParser final : public ArrayParser {
public:
    ArrayLiteralParser(PGOTypeManager *ptManager) : ArrayParser(ptManager, PGOBCInfo::Type::ARRAY_LITERAL) {}
};

class ObjectLiteralParser final : public BaseParser {
public:
    ObjectLiteralParser(PGOTypeManager *ptManager) : BaseParser(ptManager, PGOBCInfo::Type::OBJ_LITERAL) {}

private:
    virtual bool RecordTypeInfo(const PGODefineOpType &defType, const PGOTypeLocation &loc) override;
    virtual void GenerateHClass(const PGOHClassGenerator &generator, const PGOTypeLocation &loc) override;
};

class FunctionParser final : public BaseParser {
public:
    FunctionParser(PGOTypeManager *ptManager) : BaseParser(ptManager, PGOBCInfo::Type::FUNCTION) {}

private:
    virtual bool RecordTypeInfo(const PGODefineOpType &defType, const PGOTypeLocation &loc) override;
    virtual void GenerateHClass(const PGOHClassGenerator &generator, const PGOTypeLocation &loc) override;
};

class PGOTypeParser {
public:
    explicit PUBLIC_API PGOTypeParser(const PGOProfilerDecoder &decoder, PGOTypeManager *ptManager);
    ~PGOTypeParser() = default;

    void PUBLIC_API CreatePGOType(BytecodeInfoCollector &collector);

private:
    void GenerateHClassForNapiType(ProfileType rootType, const PGOHClassGenerator &generator);
    void GenerateHClassForClassType(ProfileType rootType, ProfileType protoPt, const PGOHClassGenerator &generator,
                                    bool isCache);
    void GenerateHClassForPrototype(ProfileType rootType, const PGOHClassGenerator &generator, bool isCache);
    bool SkipGenerateHClass(PGOTypeRecorder typeRecorder, ProfileType rootType, bool isCache);

    const PGOProfilerDecoder &decoder_;
    PGOTypeManager *ptManager_;
    CVector<std::unique_ptr<BaseParser>> parsers_;
};
} // namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_PGO_TYPE_PGO_TYPE_PARSER_H
