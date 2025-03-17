/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "ecmascript/global_env_constants.h"

#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/free_object.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/jobs/pending_job.h"
#include "ecmascript/js_api/js_api_arraylist_iterator.h"
#include "ecmascript/js_api/js_api_bitvector_iterator.h"
#include "ecmascript/js_api/js_api_deque_iterator.h"
#include "ecmascript/js_api/js_api_lightweightmap_iterator.h"
#include "ecmascript/js_api/js_api_lightweightset_iterator.h"
#include "ecmascript/js_api/js_api_linked_list_iterator.h"
#include "ecmascript/js_api/js_api_list_iterator.h"
#include "ecmascript/js_api/js_api_plain_array_iterator.h"
#include "ecmascript/js_api/js_api_queue_iterator.h"
#include "ecmascript/js_api/js_api_stack_iterator.h"
#include "ecmascript/js_api/js_api_hashmap_iterator.h"
#include "ecmascript/js_api/js_api_hashset_iterator.h"
#include "ecmascript/js_api/js_api_tree_map_iterator.h"
#include "ecmascript/js_api/js_api_tree_set_iterator.h"
#include "ecmascript/js_api/js_api_vector_iterator.h"
#include "ecmascript/js_async_generator_object.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_finalization_registry.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_realm.h"
#include "ecmascript/js_regexp_iterator.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/marker_cell.h"
#include "ecmascript/runtime.h"
#include "ecmascript/shared_objects/js_shared_array.h"
#include "ecmascript/shared_objects/js_shared_array_iterator.h"
#include "ecmascript/shared_objects/js_shared_map_iterator.h"
#include "ecmascript/shared_objects/js_shared_set_iterator.h"
#include "ecmascript/vtable.h"

namespace panda::ecmascript {
void GlobalEnvConstants::Init(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (Runtime::GetInstance()->SharedConstInited()) {
        auto *globalConst = Runtime::GetInstance()->GetGlobalEnvConstants();
        // 1. Copy shareds.
        for (size_t i = static_cast<size_t>(ConstantIndex::SHARED_BEGIN);
            i <= static_cast<size_t>(ConstantIndex::SHARED_END); i++) {
            constants_[i] = globalConst->constants_[i];
        }
    } else {
        InitSharedRootsClasses(factory);
        InitSharedMiscellanious(thread, factory);
        InitSharedStrings(factory);
    }
    // 2. Init non-shareds.
    InitMiscellanious(thread, factory);
    InitRootsClasses(factory);
}

void GlobalEnvConstants::CopySharedConstantsFrom(const GlobalEnvConstants *src)
{
    for (size_t i = 0; i < static_cast<size_t>(ConstantIndex::SHARED_BEGIN); i++) {
        constants_[i] = JSTaggedValue::Hole();
    }
    for (size_t i = static_cast<size_t>(ConstantIndex::SHARED_BEGIN);
        i <= static_cast<size_t>(ConstantIndex::SHARED_END); i++) {
        constants_[i] = src->constants_[i];
    }
    for (size_t i = static_cast<size_t>(ConstantIndex::SHARED_END) + 1;
        i < static_cast<size_t>(ConstantIndex::CONSTANT_COUNT); i++) {
        constants_[i] = JSTaggedValue::Hole();
    }
}

void GlobalEnvConstants::InitSharedStrings(ObjectFactory *factory)
{
    #define INIT_GLOBAL_ENV_CONSTANT_STRING(Name, Index, Token) \
        SetConstant(ConstantIndex::Index, factory->NewFromASCIIReadOnly(Token));
        SHARED_GLOBAL_ENV_CONSTANT_STRING(INIT_GLOBAL_ENV_CONSTANT_STRING)
    #undef INIT_GLOBAL_ENV_CONSTANT_STRING
}

void GlobalEnvConstants::InitSharedRootsClasses(ObjectFactory *factory)
{
    // Global constants are readonly.
    JSHClass *hClass = *factory->InitSClassClass();

    SetConstant(ConstantIndex::HCLASS_CLASS_INDEX, JSTaggedValue(hClass));
    // To reverse the order, the hclass of string needs to load default supers
    SetConstant(ConstantIndex::TAGGED_ARRAY_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, 0, JSType::TAGGED_ARRAY));
    SetConstant(ConstantIndex::FREE_OBJECT_WITH_NONE_FIELD_CLASS_INDEX,
                factory->NewSEcmaReadOnlyHClass(hClass, FreeObject::NEXT_OFFSET, JSType::FREE_OBJECT_WITH_NONE_FIELD));
    SetConstant(ConstantIndex::FREE_OBJECT_WITH_ONE_FIELD_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, FreeObject::SIZE_OFFSET, JSType::FREE_OBJECT_WITH_ONE_FIELD));
    SetConstant(ConstantIndex::FREE_OBJECT_WITH_TWO_FIELD_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, FreeObject::SIZE, JSType::FREE_OBJECT_WITH_TWO_FIELD));
    SetConstant(ConstantIndex::LINE_STRING_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, 0, JSType::LINE_STRING));
    SetConstant(ConstantIndex::SLICED_STRING_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, 0, JSType::SLICED_STRING));
    SetConstant(ConstantIndex::CONSTANT_STRING_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, 0, JSType::CONSTANT_STRING));
    SetConstant(ConstantIndex::TREE_STRING_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, 0, JSType::TREE_STRING));
    SetConstant(ConstantIndex::BYTE_ARRAY_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, 0, JSType::BYTE_ARRAY));
    SetConstant(ConstantIndex::CONSTANT_POOL_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, 0, JSType::CONSTANT_POOL));
    SetConstant(ConstantIndex::PROFILE_TYPE_INFO_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, 0, JSType::PROFILE_TYPE_INFO));
    SetConstant(ConstantIndex::AOT_LITERAL_INFO_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, 0, JSType::AOT_LITERAL_INFO));
    SetConstant(ConstantIndex::EXTRA_PROFILE_TYPE_INFO_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, ExtraProfileTypeInfo::SIZE, JSType::EXTRA_PROFILE_TYPE_INFO));
    SetConstant(ConstantIndex::PROFILE_TYPE_INFO_CELL_0_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, ProfileTypeInfoCell::SIZE, JSType::PROFILE_TYPE_INFO_CELL_0));
    SetConstant(ConstantIndex::PROFILE_TYPE_INFO_CELL_1_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, ProfileTypeInfoCell::SIZE, JSType::PROFILE_TYPE_INFO_CELL_1));
    SetConstant(ConstantIndex::PROFILE_TYPE_INFO_CELL_N_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, ProfileTypeInfoCell::SIZE, JSType::PROFILE_TYPE_INFO_CELL_N));
    SetConstant(ConstantIndex::FUNCTION_TEMPLATE_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, FunctionTemplate::SIZE, JSType::FUNCTION_TEMPLATE));
    SetConstant(ConstantIndex::VTABLE_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, 0, JSType::VTABLE));
    SetConstant(ConstantIndex::COW_MUTANT_TAGGED_ARRAY_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, 0, JSType::COW_MUTANT_TAGGED_ARRAY));
    SetConstant(ConstantIndex::MUTANT_TAGGED_ARRAY_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, 0, JSType::MUTANT_TAGGED_ARRAY));
    SetConstant(ConstantIndex::DICTIONARY_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, 0, JSType::TAGGED_DICTIONARY));
    SetConstant(ConstantIndex::COW_ARRAY_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, 0, JSType::COW_TAGGED_ARRAY));
    SetConstant(ConstantIndex::BIGINT_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, BigInt::SIZE, JSType::BIGINT));
    SetConstant(ConstantIndex::SENDABLE_JS_NATIVE_POINTER_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, JSNativePointer::SIZE, JSType::JS_NATIVE_POINTER));
    SetConstant(ConstantIndex::ENV_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, 0, JSType::LEXICAL_ENV));
    SetConstant(ConstantIndex::SYMBOL_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, JSSymbol::SIZE, JSType::SYMBOL));
    SetConstant(ConstantIndex::ACCESSOR_DATA_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, AccessorData::SIZE, JSType::ACCESSOR_DATA));
    SetConstant(ConstantIndex::INTERNAL_ACCESSOR_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, AccessorData::SIZE, JSType::INTERNAL_ACCESSOR));
    SetConstant(ConstantIndex::COMPLETION_RECORD_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, CompletionRecord::SIZE, JSType::COMPLETION_RECORD));
    SetConstant(ConstantIndex::GENERATOR_CONTEST_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, GeneratorContext::SIZE, JSType::JS_GENERATOR_CONTEXT));
    SetConstant(ConstantIndex::ASYNC_GENERATOR_REQUEST_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, AsyncGeneratorRequest::SIZE,
                                        JSType::ASYNC_GENERATOR_REQUEST));
    SetConstant(ConstantIndex::ASYNC_ITERATOR_RECORD_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, AsyncIteratorRecord::SIZE, JSType::ASYNC_ITERATOR_RECORD));
    SetConstant(ConstantIndex::CAPABILITY_RECORD_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, PromiseCapability::SIZE, JSType::PROMISE_CAPABILITY));
    SetConstant(ConstantIndex::REACTIONS_RECORD_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, PromiseReaction::SIZE, JSType::PROMISE_REACTIONS));
    SetConstant(ConstantIndex::PROMISE_ITERATOR_RECORD_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, PromiseIteratorRecord::SIZE,
                                        JSType::PROMISE_ITERATOR_RECORD));
    SetConstant(ConstantIndex::PROMISE_RECORD_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, PromiseRecord::SIZE, JSType::PROMISE_RECORD));
    SetConstant(ConstantIndex::PROMISE_RESOLVING_FUNCTIONS_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, ResolvingFunctionsRecord::SIZE,
                                        JSType::RESOLVING_FUNCTIONS_RECORD));
    SetConstant(ConstantIndex::MICRO_JOB_QUEUE_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, job::MicroJobQueue::SIZE, JSType::MICRO_JOB_QUEUE));
    SetConstant(ConstantIndex::PENDING_JOB_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, job::PendingJob::SIZE, JSType::PENDING_JOB));
    SetConstant(ConstantIndex::PROTO_CHANGE_MARKER_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, ProtoChangeMarker::SIZE, JSType::PROTO_CHANGE_MARKER));
    SetConstant(ConstantIndex::PROTO_CHANGE_DETAILS_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, ProtoChangeDetails::SIZE, JSType::PROTOTYPE_INFO));
    SetConstant(ConstantIndex::MARKER_CELL_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, MarkerCell::SIZE, JSType::MARKER_CELL));
    SetConstant(ConstantIndex::TRACK_INFO_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, TrackInfo::SIZE, JSType::TRACK_INFO));
    SetConstant(ConstantIndex::PROTOTYPE_HANDLER_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, PrototypeHandler::SIZE, JSType::PROTOTYPE_HANDLER));
    SetConstant(ConstantIndex::TRANSITION_HANDLER_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, TransitionHandler::SIZE, JSType::TRANSITION_HANDLER));
    SetConstant(ConstantIndex::TRANS_WITH_PROTO_HANDLER_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, TransWithProtoHandler::SIZE, JSType::TRANS_WITH_PROTO_HANDLER));
    SetConstant(ConstantIndex::STORE_TS_HANDLER_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, StoreAOTHandler::SIZE, JSType::STORE_TS_HANDLER));
    SetConstant(ConstantIndex::PROPERTY_BOX_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, PropertyBox::SIZE, JSType::PROPERTY_BOX));
    SetConstant(ConstantIndex::PROGRAM_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, Program::SIZE, JSType::PROGRAM));
    SetConstant(ConstantIndex::IMPORT_ENTRY_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, ImportEntry::SIZE, JSType::IMPORTENTRY_RECORD));
    SetConstant(ConstantIndex::LOCAL_EXPORT_ENTRY_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, LocalExportEntry::SIZE, JSType::LOCAL_EXPORTENTRY_RECORD));
    SetConstant(ConstantIndex::INDIRECT_EXPORT_ENTRY_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, IndirectExportEntry::SIZE,
                                        JSType::INDIRECT_EXPORTENTRY_RECORD));
    SetConstant(ConstantIndex::STAR_EXPORT_ENTRY_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, StarExportEntry::SIZE, JSType::STAR_EXPORTENTRY_RECORD));
    SetConstant(ConstantIndex::SOURCE_TEXT_MODULE_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, SourceTextModule::SIZE, JSType::SOURCE_TEXT_MODULE_RECORD));
    SetConstant(ConstantIndex::RESOLVED_BINDING_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, ResolvedBinding::SIZE, JSType::RESOLVEDBINDING_RECORD));
    SetConstant(ConstantIndex::RESOLVED_INDEX_BINDING_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, ResolvedIndexBinding::SIZE, JSType::RESOLVEDINDEXBINDING_RECORD));
    SetConstant(ConstantIndex::MACHINE_CODE_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, 0, JSType::MACHINE_CODE_OBJECT));
    SetConstant(ConstantIndex::CLASS_INFO_EXTRACTOR_HCLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, ClassInfoExtractor::SIZE,
                                        JSType::CLASS_INFO_EXTRACTOR));
    SetConstant(ConstantIndex::CELL_RECORD_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, CellRecord::SIZE, JSType::CELL_RECORD));
    SetConstant(ConstantIndex::METHOD_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, Method::SIZE, JSType::METHOD));
    SetConstant(ConstantIndex::LINKED_NODE_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, LinkedNode::SIZE, JSType::LINKED_NODE));
    SetConstant(ConstantIndex::RB_TREENODE_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, RBTreeNode::SIZE, JSType::RB_TREENODE));
    SetConstant(ConstantIndex::CLASS_LITERAL_HCLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, ClassLiteral::SIZE, JSType::CLASS_LITERAL));
    SetConstant(ConstantIndex::RESOLVED_RECORD_INEDX_BINDING_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, ResolvedRecordIndexBinding::SIZE,
        JSType::RESOLVEDRECORDINDEXBINDING_RECORD));
    SetConstant(ConstantIndex::RESOLVED_RECORD_BINDING_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, ResolvedRecordBinding::SIZE, JSType::RESOLVEDRECORDBINDING_RECORD));
    SetConstant(ConstantIndex::SENDABLE_ENV_CLASS_INDEX,
        factory->NewSEcmaReadOnlyHClass(hClass, 0, JSType::SENDABLE_ENV));
}

void GlobalEnvConstants::InitSharedMiscellanious(JSThread *thread, ObjectFactory *factory)
{
    // Accessors
    auto accessor = factory->NewSInternalAccessor(reinterpret_cast<void *>(JSFunction::PrototypeSetter),
                                                  reinterpret_cast<void *>(JSFunction::PrototypeGetter));
    SetConstant(ConstantIndex::FUNCTION_PROTOTYPE_ACCESSOR, accessor);
    accessor = factory->NewSInternalAccessor(nullptr, reinterpret_cast<void *>(JSFunction::NameGetter));
    SetConstant(ConstantIndex::FUNCTION_NAME_ACCESSOR, accessor);
    accessor = factory->NewSInternalAccessor(nullptr, reinterpret_cast<void *>(JSFunction::LengthGetter));
    SetConstant(ConstantIndex::FUNCTION_LENGTH_ACCESSOR, accessor);
    accessor = factory->NewSInternalAccessor(reinterpret_cast<void *>(JSArray::LengthSetter),
                                             reinterpret_cast<void *>(JSArray::LengthGetter));
    SetConstant(ConstantIndex::ARRAY_LENGTH_ACCESSOR, accessor);
    accessor = factory->NewSInternalAccessor(reinterpret_cast<void *>(JSSharedArray::DummyLengthSetter),
                                             reinterpret_cast<void *>(JSSharedArray::LengthGetter));
    SetConstant(ConstantIndex::SHARED_ARRAY_LENGTH_ACCESSOR, accessor);
    // Specials
    SetConstant(ConstantIndex::UNDEFINED_INDEX, JSTaggedValue::Undefined());
    SetConstant(ConstantIndex::NULL_INDEX, JSTaggedValue::Null());
    SetConstant(ConstantIndex::HOLE_INDEX, JSTaggedValue::Hole());
    SetConstant(ConstantIndex::TRUE_INDEX, JSTaggedValue::True());
    SetConstant(ConstantIndex::FALSE_INDEX, JSTaggedValue::False());
    // Constant Symbols
    SetConstant(ConstantIndex::PROTO_TRANS_ROOT_HCLASS_SYMBOL_INDEX, factory->NewSConstantPrivateSymbol());
    // Emptys
    auto vm = thread->GetEcmaVM();
    SetConstant(ConstantIndex::EMPTY_STRING_OBJECT_INDEX, JSTaggedValue(EcmaStringAccessor::CreateEmptyString(vm)));
    SetConstant(ConstantIndex::SINGLE_CHAR_TABLE_INDEX, SingleCharTable::CreateSingleCharTable(thread));
    SetConstant(ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX, factory->NewSEmptyArray());
    SetConstant(ConstantIndex::EMPTY_MUTANT_ARRAY_OBJECT_INDEX, factory->NewSEmptyMutantArray());
    SetConstant(ConstantIndex::EMPTY_SLAYOUT_INFO_OBJECT_INDEX, factory->NewSEmptyLayoutInfo());
    SetConstant(ConstantIndex::UINT64_MAX_BIGINT_INDEX, BigInt::CreateUint64MaxBigInt(thread));
    SetConstant(ConstantIndex::INT64_MAX_BIGINT_INDEX, BigInt::CreateInt64MaxBigInt(thread));
    SetConstant(ConstantIndex::EMPTY_PROFILE_TYPE_INFO_CELL_INDEX, factory->NewSEmptyProfileTypeInfoCell());
    // Native function method
    SetConstant(ConstantIndex::NORMAL_FUNCTION_METHOD_INDEX,
                factory->NewSEmptyNativeFunctionMethod(FunctionKind::NORMAL_FUNCTION));
    SetConstant(ConstantIndex::GETTER_FUNCTION_METHOD_INDEX,
                factory->NewSEmptyNativeFunctionMethod(FunctionKind::GETTER_FUNCTION));
    SetConstant(ConstantIndex::SETTER_FUNCTION_METHOD_INDEX,
                factory->NewSEmptyNativeFunctionMethod(FunctionKind::SETTER_FUNCTION));
    SetConstant(ConstantIndex::ARROW_FUNCTION_METHOD_INDEX,
                factory->NewSEmptyNativeFunctionMethod(FunctionKind::ARROW_FUNCTION));
    SetConstant(ConstantIndex::ASYNC_ARROW_FUNCTION_METHOD_INDEX,
                factory->NewSEmptyNativeFunctionMethod(FunctionKind::ASYNC_ARROW_FUNCTION));
    SetConstant(ConstantIndex::CONCURRENT_FUNCTION_METHOD_INDEX,
                factory->NewSEmptyNativeFunctionMethod(FunctionKind::CONCURRENT_FUNCTION));
    SetConstant(ConstantIndex::ASYNC_FUNCTION_METHOD_INDEX,
                factory->NewSEmptyNativeFunctionMethod(FunctionKind::ASYNC_FUNCTION));
    SetConstant(ConstantIndex::BASE_CONSTRUCTOR_METHOD_INDEX,
                factory->NewSEmptyNativeFunctionMethod(FunctionKind::BASE_CONSTRUCTOR));
    SetConstant(ConstantIndex::CLASS_CONSTRUCTOR_METHOD_INDEX,
                factory->NewSEmptyNativeFunctionMethod(FunctionKind::CLASS_CONSTRUCTOR));
    SetConstant(ConstantIndex::BUILTIN_PROXY_CONSTRUCTOR_METHOD_INDEX,
                factory->NewSEmptyNativeFunctionMethod(FunctionKind::BUILTIN_PROXY_CONSTRUCTOR));
    SetConstant(ConstantIndex::BUILTIN_CONSTRUCTOR_METHOD_INDEX,
                factory->NewSEmptyNativeFunctionMethod(FunctionKind::BUILTIN_CONSTRUCTOR));
    SetConstant(ConstantIndex::DERIVED_CONSTRUCTOR_METHOD_INDEX,
                factory->NewSEmptyNativeFunctionMethod(FunctionKind::DERIVED_CONSTRUCTOR));
    SetConstant(ConstantIndex::GENERATOR_FUNCTION_METHOD_INDEX,
                factory->NewSEmptyNativeFunctionMethod(FunctionKind::GENERATOR_FUNCTION));
    SetConstant(ConstantIndex::NONE_FUNCTION_METHOD_INDEX,
                factory->NewSEmptyNativeFunctionMethod(FunctionKind::NONE_FUNCTION));
    SetConstant(ConstantIndex::ASYNC_GENERATOR_FUNCTION_METHOD_INDEX,
                factory->NewSEmptyNativeFunctionMethod(FunctionKind::ASYNC_GENERATOR_FUNCTION));
}

void GlobalEnvConstants::InitRootsClassesPartOne(JSHClass *hClass, ObjectFactory *factory)
{
    SetConstant(ConstantIndex::JS_API_LINKED_LIST_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSAPILinkedListIterator::SIZE, JSType::JS_API_LINKED_LIST_ITERATOR));
    SetConstant(ConstantIndex::JS_API_LIST_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSAPIListIterator::SIZE, JSType::JS_API_LIST_ITERATOR));
    SetConstant(ConstantIndex::JS_API_PLAIN_ARRAY_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSAPIPlainArrayIterator::SIZE, JSType::JS_API_PLAIN_ARRAY_ITERATOR));
    SetConstant(ConstantIndex::JS_API_QUEUE_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSAPIQueueIterator::SIZE, JSType::JS_API_QUEUE_ITERATOR));
    SetConstant(ConstantIndex::JS_API_STACK_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSAPIStackIterator::SIZE, JSType::JS_API_STACK_ITERATOR));
    SetConstant(ConstantIndex::JS_API_VECTOR_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSAPIVectorIterator::SIZE, JSType::JS_API_VECTOR_ITERATOR));
    SetConstant(ConstantIndex::JS_API_HASH_MAP_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSAPIHashMapIterator::SIZE, JSType::JS_API_HASHMAP_ITERATOR));
    SetConstant(ConstantIndex::JS_API_HASH_SET_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSAPIHashSetIterator::SIZE, JSType::JS_API_HASHSET_ITERATOR));
    SetConstant(ConstantIndex::JS_API_TREE_MAP_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSAPITreeMapIterator::SIZE, JSType::JS_API_TREEMAP_ITERATOR));
    SetConstant(ConstantIndex::JS_API_TREE_SET_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSAPITreeSetIterator::SIZE, JSType::JS_API_TREESET_ITERATOR));
    SetConstant(ConstantIndex::OBJECT_HCLASS_INDEX, factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT));
    SetConstant(ConstantIndex::CLASS_PROTOTYPE_HCLASS_INDEX,
                factory->CreateDefaultClassPrototypeHClass(hClass));
    SetConstant(ConstantIndex::CLASS_CONSTRUCTOR_HCLASS_INDEX,
                factory->CreateDefaultClassConstructorHClass(hClass));
    SetConstant(ConstantIndex::JS_PROXY_ORDINARY_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSProxy::SIZE, JSType::JS_PROXY));
    // napi_wrap need set NativePointer to object, so only mark the shared bit in shared JSNativePointer hclass.
    SetConstant(ConstantIndex::JS_NATIVE_POINTER_CLASS_INDEX,
        factory->NewEcmaReadOnlyHClass(hClass, JSNativePointer::SIZE, JSType::JS_NATIVE_POINTER));
}

void GlobalEnvConstants::InitRootsClassesPartTwo(JSHClass *hClass, ObjectFactory *factory)
{
    SetConstant(ConstantIndex::JS_REALM_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSRealm::SIZE, JSType::JS_REALM));
    SetConstant(ConstantIndex::JS_REGEXP_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSRegExpIterator::SIZE, JSType::JS_REG_EXP_ITERATOR));
    SetConstant(ConstantIndex::JS_SET_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSSetIterator::SIZE, JSType::JS_SET_ITERATOR, 0)); // 0: no inlined props
    SetConstant(ConstantIndex::JS_SHARED_SET_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSSharedSetIterator::SIZE,
                JSType::JS_SHARED_SET_ITERATOR, 0)); // 0: no inlined props
    SetConstant(ConstantIndex::JS_MAP_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSMapIterator::SIZE, JSType::JS_MAP_ITERATOR, 0)); // 0: no inlined props
    SetConstant(ConstantIndex::JS_SHARED_MAP_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSSharedMapIterator::SIZE,
                JSType::JS_SHARED_MAP_ITERATOR, 0)); // 0: no inlined props
    SetConstant(ConstantIndex::JS_ARRAY_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSArrayIterator::SIZE, JSType::JS_ARRAY_ITERATOR, 0));
    SetConstant(ConstantIndex::JS_SHARED_ARRAY_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSSharedArrayIterator::SIZE, JSType::JS_SHARED_ARRAY_ITERATOR, 0));
    SetConstant(ConstantIndex::JS_API_ARRAYLIST_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSAPIArrayListIterator::SIZE, JSType::JS_API_ARRAYLIST_ITERATOR));
    SetConstant(ConstantIndex::JS_API_DEQUE_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSAPIDequeIterator::SIZE, JSType::JS_API_DEQUE_ITERATOR));
    SetConstant(ConstantIndex::JS_API_LIGHTWEIGHTMAP_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSAPILightWeightMapIterator::SIZE,
                JSType::JS_API_LIGHT_WEIGHT_MAP_ITERATOR));
    SetConstant(ConstantIndex::JS_API_LIGHTWEIGHTSET_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSAPILightWeightSetIterator::SIZE,
                JSType::JS_API_LIGHT_WEIGHT_SET_ITERATOR));
    SetConstant(ConstantIndex::JS_API_BITVECTOR_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSAPIBitVectorIterator::SIZE, JSType::JS_API_BITVECTOR_ITERATOR));
}

void GlobalEnvConstants::InitRootsClasses(ObjectFactory *factory)
{
    JSHClass *hClass = JSHClass::Cast(GetHClassClass().GetTaggedObject());

    InitRootsClassesPartOne(hClass, factory);
    InitRootsClassesPartTwo(hClass, factory);
    // JS_PROXY_CALLABLE_CLASS_INDEX
    JSHClass *jsProxyCallableClass = *factory->NewEcmaHClass(hClass, JSProxy::SIZE, JSType::JS_PROXY);
    jsProxyCallableClass->SetCallable(true);
    SetConstant(ConstantIndex::JS_PROXY_CALLABLE_CLASS_INDEX, JSTaggedValue(jsProxyCallableClass));
    // JS_PROXY_CONSTRUCT_CLASS_INDEX
    JSHClass *jsProxyConstructClass = *factory->NewEcmaHClass(hClass, JSProxy::SIZE, JSType::JS_PROXY);
    jsProxyConstructClass->SetCallable(true);
    jsProxyConstructClass->SetConstructor(true);
    SetConstant(ConstantIndex::JS_PROXY_CONSTRUCT_CLASS_INDEX, JSTaggedValue(jsProxyConstructClass));
}

void GlobalEnvConstants::InitMiscellanious(JSThread *thread, ObjectFactory *factory)
{
    SetConstant(ConstantIndex::EMPTY_LAYOUT_INFO_OBJECT_INDEX, factory->CreateLayoutInfo(0));
    SetConstant(ConstantIndex::EMPTY_TAGGED_QUEUE_OBJECT_INDEX, factory->NewTaggedQueue(0));
    SetConstant(ConstantIndex::DEFAULT_SUPERS_INDEX,
                WeakVector::Create(thread, VTable::DEFAULT_SUPERS_CAPACITY, MemSpaceType::NON_MOVABLE));

    // non ECMA standard jsapi containers iterators, init to Undefined first
    InitJSAPIContainers();

    InitGlobalCaches();
}

void GlobalEnvConstants::InitGlobalCaches()
{
    SetConstant(ConstantIndex::CACHED_JSCOLLATOR_LOCALES_INDEX, JSTaggedValue::Undefined());
}

void GlobalEnvConstants::SetCachedLocales(JSTaggedValue value)
{
    JSTaggedValue cached = GetCachedJSCollatorLocales();
    if (cached.IsUndefined()) {
        SetConstant(ConstantIndex::CACHED_JSCOLLATOR_LOCALES_INDEX, value);
    }
}

void GlobalEnvConstants::InitJSAPIContainers()
{
    for (size_t i = GetJSAPIContainersBegin(); i <= GetJSAPIContainersEnd(); i++) {
        SetConstant(static_cast<ConstantIndex>(i), JSTaggedValue::Undefined());
    }
}

void GlobalEnvConstants::InitSpecialForSnapshot()
{
    SetConstant(ConstantIndex::UNDEFINED_INDEX, JSTaggedValue::Undefined());
    SetConstant(ConstantIndex::NULL_INDEX, JSTaggedValue::Null());
    InitJSAPIContainers();
}

void GlobalEnvConstants::InitElementKindHClass(const JSThread *thread, JSHandle<JSHClass> originHClass)
{
    {
        JSHandle<JSHClass> hclass;
#define INIT_ARRAY_HCLASS_INDEX_ARRAYS(name)                                                        \
        hclass = JSHClass::CloneWithElementsKind(thread, originHClass, ElementsKind::name, false);  \
        SetConstant(ConstantIndex::ELEMENT_##name##_HCLASS_INDEX, hclass);

        ELEMENTS_KIND_INIT_HCLASS_LIST(INIT_ARRAY_HCLASS_INDEX_ARRAYS)
#undef INIT_ARRAY_HCLASS_INDEX_ARRAYS
    }
    SetConstant(ConstantIndex::ELEMENT_HOLE_TAGGED_HCLASS_INDEX, originHClass);
    {
        JSHandle<JSHClass> hclass;
#define INIT_ARRAY_HCLASS_INDEX_ARRAYS(name)                                                       \
        hclass = JSHClass::CloneWithElementsKind(thread, originHClass, ElementsKind::name, true);  \
        SetConstant(ConstantIndex::ELEMENT_##name##_PROTO_HCLASS_INDEX, hclass);

        ELEMENTS_KIND_INIT_HCLASS_LIST(INIT_ARRAY_HCLASS_INDEX_ARRAYS)
#undef INIT_ARRAY_HCLASS_INDEX_ARRAYS
    }
}
}  // namespace panda::ecmascript
