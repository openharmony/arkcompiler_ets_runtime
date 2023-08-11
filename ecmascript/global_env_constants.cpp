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

#include "ecmascript/accessor_data.h"
#include "ecmascript/builtins/builtins.h"
#include "ecmascript/builtins/builtins_global.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/free_object.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/property_box.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/jobs/pending_job.h"
#include "ecmascript/js_api/js_api_arraylist_iterator.h"
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
#include "ecmascript/jspandafile/class_info_extractor.h"
#include "ecmascript/jspandafile/class_literal.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_finalization_registry.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_realm.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_regexp_iterator.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_symbol.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/method.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/subtyping_operator.h"
#include "ecmascript/tagged_node.h"
#include "ecmascript/ts_types/ts_type.h"

namespace panda::ecmascript {
void GlobalEnvConstants::Init(JSThread *thread, JSHClass *hClass)
{
    InitRootsClass(thread, hClass);
    InitGlobalConstant(thread);
    InitGlobalCaches();
}

void GlobalEnvConstants::InitRootsClass(JSThread *thread, JSHClass *hClass)
{
    // Global constants are readonly.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    SetConstant(ConstantIndex::HCLASS_CLASS_INDEX, JSTaggedValue(hClass));
    SetConstant(ConstantIndex::FREE_OBJECT_WITH_NONE_FIELD_CLASS_INDEX,
        factory->NewEcmaReadOnlyHClass(hClass, FreeObject::NEXT_OFFSET, JSType::FREE_OBJECT_WITH_NONE_FIELD));
    SetConstant(ConstantIndex::FREE_OBJECT_WITH_ONE_FIELD_CLASS_INDEX,
        factory->NewEcmaReadOnlyHClass(hClass, FreeObject::SIZE_OFFSET, JSType::FREE_OBJECT_WITH_ONE_FIELD));
    SetConstant(ConstantIndex::FREE_OBJECT_WITH_TWO_FIELD_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, FreeObject::SIZE, JSType::FREE_OBJECT_WITH_TWO_FIELD));
    SetConstant(ConstantIndex::LINE_STRING_CLASS_INDEX, factory->NewEcmaReadOnlyHClass(hClass, 0, JSType::LINE_STRING));
    SetConstant(ConstantIndex::CONSTANT_STRING_CLASS_INDEX,
        factory->NewEcmaReadOnlyHClass(hClass, 0, JSType::CONSTANT_STRING));
    SetConstant(ConstantIndex::TREE_STRING_CLASS_INDEX, factory->NewEcmaReadOnlyHClass(hClass, 0, JSType::TREE_STRING));
    SetConstant(ConstantIndex::ARRAY_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, 0, JSType::TAGGED_ARRAY));
    SetConstant(ConstantIndex::BYTE_ARRAY_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, 0, JSType::BYTE_ARRAY));
    SetConstant(ConstantIndex::CONSTANT_POOL_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, 0, JSType::CONSTANT_POOL));
    SetConstant(ConstantIndex::AOT_LITERAL_INFO_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, 0, JSType::AOT_LITERAL_INFO));
    SetConstant(ConstantIndex::VTABLE_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, 0, JSType::VTABLE));
    InitGlobalConstantSpecial(thread);
    SetConstant(ConstantIndex::DICTIONARY_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, 0, JSType::TAGGED_DICTIONARY));
    SetConstant(ConstantIndex::COW_ARRAY_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, 0, JSType::COW_TAGGED_ARRAY));
    SetConstant(ConstantIndex::BIGINT_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, BigInt::SIZE, JSType::BIGINT));
    SetConstant(ConstantIndex::JS_NATIVE_POINTER_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, JSNativePointer::SIZE, JSType::JS_NATIVE_POINTER));
    SetConstant(ConstantIndex::ENV_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, 0, JSType::LEXICAL_ENV));
    SetConstant(ConstantIndex::SYMBOL_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, JSSymbol::SIZE, JSType::SYMBOL));
    SetConstant(ConstantIndex::ACCESSOR_DATA_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, AccessorData::SIZE, JSType::ACCESSOR_DATA));
    SetConstant(ConstantIndex::INTERNAL_ACCESSOR_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, AccessorData::SIZE, JSType::INTERNAL_ACCESSOR));
    SetConstant(ConstantIndex::JS_PROXY_ORDINARY_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSProxy::SIZE, JSType::JS_PROXY));
    SetConstant(ConstantIndex::COMPLETION_RECORD_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, CompletionRecord::SIZE, JSType::COMPLETION_RECORD));
    SetConstant(ConstantIndex::GENERATOR_CONTEST_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, GeneratorContext::SIZE, JSType::JS_GENERATOR_CONTEXT));
    SetConstant(ConstantIndex::ASYNC_GENERATOR_REQUEST_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, AsyncGeneratorRequest::SIZE,
                                                 JSType::ASYNC_GENERATOR_REQUEST));
    SetConstant(ConstantIndex::ASYNC_ITERATOR_RECORD_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, AsyncIteratorRecord::SIZE, JSType::ASYNC_ITERATOR_RECORD));
    SetConstant(ConstantIndex::CAPABILITY_RECORD_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, PromiseCapability::SIZE, JSType::PROMISE_CAPABILITY));
    SetConstant(ConstantIndex::REACTIONS_RECORD_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, PromiseReaction::SIZE, JSType::PROMISE_REACTIONS));
    SetConstant(ConstantIndex::PROMISE_ITERATOR_RECORD_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, PromiseIteratorRecord::SIZE,
                                                 JSType::PROMISE_ITERATOR_RECORD));
    SetConstant(ConstantIndex::PROMISE_RECORD_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, PromiseRecord::SIZE, JSType::PROMISE_RECORD));
    SetConstant(ConstantIndex::PROMISE_RESOLVING_FUNCTIONS_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, ResolvingFunctionsRecord::SIZE,
                                                 JSType::RESOLVING_FUNCTIONS_RECORD));
    SetConstant(ConstantIndex::MICRO_JOB_QUEUE_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, job::MicroJobQueue::SIZE, JSType::MICRO_JOB_QUEUE));
    SetConstant(ConstantIndex::PENDING_JOB_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, job::PendingJob::SIZE, JSType::PENDING_JOB));
    SetConstant(ConstantIndex::PROTO_CHANGE_MARKER_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, ProtoChangeMarker::SIZE, JSType::PROTO_CHANGE_MARKER));
    SetConstant(ConstantIndex::PROTO_CHANGE_DETAILS_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, ProtoChangeDetails::SIZE, JSType::PROTOTYPE_INFO));
    SetConstant(ConstantIndex::PROTOTYPE_HANDLER_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, PrototypeHandler::SIZE, JSType::PROTOTYPE_HANDLER));
    SetConstant(ConstantIndex::TRANSITION_HANDLER_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, TransitionHandler::SIZE, JSType::TRANSITION_HANDLER));
    SetConstant(ConstantIndex::TRANS_WITH_PROTO_HANDLER_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, TransWithProtoHandler::SIZE, JSType::TRANS_WITH_PROTO_HANDLER));
    SetConstant(ConstantIndex::STORE_TS_HANDLER_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, StoreTSHandler::SIZE, JSType::STORE_TS_HANDLER));
    SetConstant(ConstantIndex::PROPERTY_BOX_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, PropertyBox::SIZE, JSType::PROPERTY_BOX));
    SetConstant(ConstantIndex::PROGRAM_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, Program::SIZE, JSType::PROGRAM));
    SetConstant(
        ConstantIndex::IMPORT_ENTRY_CLASS_INDEX,
        factory->NewEcmaReadOnlyHClass(hClass, ImportEntry::SIZE, JSType::IMPORTENTRY_RECORD));
    SetConstant(
        ConstantIndex::LOCAL_EXPORT_ENTRY_CLASS_INDEX,
        factory->NewEcmaReadOnlyHClass(hClass, LocalExportEntry::SIZE, JSType::LOCAL_EXPORTENTRY_RECORD));
    SetConstant(
        ConstantIndex::INDIRECT_EXPORT_ENTRY_CLASS_INDEX,
        factory->NewEcmaReadOnlyHClass(hClass, IndirectExportEntry::SIZE,
                                       JSType::INDIRECT_EXPORTENTRY_RECORD));
    SetConstant(
        ConstantIndex::STAR_EXPORT_ENTRY_CLASS_INDEX,
        factory->NewEcmaReadOnlyHClass(hClass, StarExportEntry::SIZE, JSType::STAR_EXPORTENTRY_RECORD));
    SetConstant(
        ConstantIndex::SOURCE_TEXT_MODULE_CLASS_INDEX,
        factory->NewEcmaReadOnlyHClass(hClass, SourceTextModule::SIZE, JSType::SOURCE_TEXT_MODULE_RECORD));
    SetConstant(
        ConstantIndex::RESOLVED_BINDING_CLASS_INDEX,
        factory->NewEcmaReadOnlyHClass(hClass, ResolvedBinding::SIZE, JSType::RESOLVEDBINDING_RECORD));
    SetConstant(
        ConstantIndex::RESOLVED_INDEX_BINDING_CLASS_INDEX,
        factory->NewEcmaReadOnlyHClass(hClass, ResolvedIndexBinding::SIZE, JSType::RESOLVEDINDEXBINDING_RECORD));

    JSHClass *jsProxyCallableClass = *factory->NewEcmaHClass(hClass, JSProxy::SIZE, JSType::JS_PROXY);

    jsProxyCallableClass->SetCallable(true);
    SetConstant(ConstantIndex::JS_PROXY_CALLABLE_CLASS_INDEX, JSTaggedValue(jsProxyCallableClass));

    JSHClass *jsProxyConstructClass = *factory->NewEcmaHClass(hClass, JSProxy::SIZE, JSType::JS_PROXY);

    jsProxyConstructClass->SetCallable(true);
    jsProxyConstructClass->SetConstructor(true);
    SetConstant(ConstantIndex::JS_PROXY_CONSTRUCT_CLASS_INDEX, JSTaggedValue(jsProxyConstructClass));

    SetConstant(ConstantIndex::JS_REALM_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSRealm::SIZE, JSType::JS_REALM));
    SetConstant(ConstantIndex::MACHINE_CODE_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, 0, JSType::MACHINE_CODE_OBJECT));
    SetConstant(ConstantIndex::CLASS_INFO_EXTRACTOR_HCLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, ClassInfoExtractor::SIZE,
                                                 JSType::CLASS_INFO_EXTRACTOR));
    SetConstant(ConstantIndex::TS_OBJECT_TYPE_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, TSObjectType::SIZE, JSType::TS_OBJECT_TYPE));
    SetConstant(ConstantIndex::TS_CLASS_TYPE_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, TSClassType::SIZE, JSType::TS_CLASS_TYPE));
    SetConstant(ConstantIndex::TS_UNION_TYPE_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, TSUnionType::SIZE, JSType::TS_UNION_TYPE));
    SetConstant(ConstantIndex::TS_INTERFACE_TYPE_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, TSInterfaceType::SIZE, JSType::TS_INTERFACE_TYPE));
    SetConstant(ConstantIndex::TS_CLASS_INSTANCE_TYPE_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, TSClassInstanceType::SIZE,
                                                 JSType::TS_CLASS_INSTANCE_TYPE));
    SetConstant(ConstantIndex::TS_FUNCTION_TYPE_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, TSFunctionType::SIZE, JSType::TS_FUNCTION_TYPE));
    SetConstant(ConstantIndex::TS_ARRAY_TYPE_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, TSArrayType::SIZE, JSType::TS_ARRAY_TYPE));
    SetConstant(ConstantIndex::TS_ITERATOR_INSTANCE_TYPE_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, TSIteratorInstanceType::SIZE,
                JSType::TS_ITERATOR_INSTANCE_TYPE));
    SetConstant(ConstantIndex::TS_NAMESPACE_TYPE_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, TSNamespaceType::SIZE, JSType::TS_NAMESPACE_TYPE));
    SetConstant(ConstantIndex::JS_REGEXP_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSRegExpIterator::SIZE, JSType::JS_REG_EXP_ITERATOR));
    SetConstant(ConstantIndex::JS_SET_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSSetIterator::SIZE, JSType::JS_SET_ITERATOR));
    SetConstant(ConstantIndex::JS_MAP_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSMapIterator::SIZE, JSType::JS_MAP_ITERATOR));
    SetConstant(ConstantIndex::JS_ARRAY_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSArrayIterator::SIZE, JSType::JS_ARRAY_ITERATOR));
    SetConstant(
        ConstantIndex::JS_API_ARRAYLIST_ITERATOR_CLASS_INDEX,
        factory->NewEcmaHClass(hClass, JSAPIArrayListIterator::SIZE, JSType::JS_API_ARRAYLIST_ITERATOR));
    SetConstant(ConstantIndex::JS_API_DEQUE_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSAPIDequeIterator::SIZE, JSType::JS_API_DEQUE_ITERATOR));
    SetConstant(ConstantIndex::JS_API_LIGHTWEIGHTMAP_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSAPILightWeightMapIterator::SIZE,
                JSType::JS_API_LIGHT_WEIGHT_MAP_ITERATOR));
    SetConstant(ConstantIndex::JS_API_LIGHTWEIGHTSET_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSAPILightWeightSetIterator::SIZE,
                JSType::JS_API_LIGHT_WEIGHT_SET_ITERATOR));
    SetConstant(
        ConstantIndex::JS_API_LINKED_LIST_ITERATOR_CLASS_INDEX,
        factory->NewEcmaHClass(hClass, JSAPILinkedListIterator::SIZE, JSType::JS_API_LINKED_LIST_ITERATOR));
    SetConstant(ConstantIndex::JS_API_LIST_ITERATOR_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, JSAPIListIterator::SIZE, JSType::JS_API_LIST_ITERATOR));
    SetConstant(
        ConstantIndex::JS_API_PLAIN_ARRAY_ITERATOR_CLASS_INDEX,
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
    SetConstant(ConstantIndex::LINKED_NODE_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, LinkedNode::SIZE, JSType::LINKED_NODE));
    SetConstant(ConstantIndex::RB_TREENODE_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, RBTreeNode::SIZE, JSType::RB_TREENODE));
    SetConstant(ConstantIndex::CELL_RECORD_CLASS_INDEX,
                factory->NewEcmaReadOnlyHClass(hClass, CellRecord::SIZE, JSType::CELL_RECORD));
    SetConstant(ConstantIndex::OBJECT_HCLASS_INDEX, factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT));
    SetConstant(ConstantIndex::METHOD_CLASS_INDEX,
                factory->NewEcmaHClass(hClass, Method::SIZE, JSType::METHOD));
    SetConstant(ConstantIndex::CLASS_LITERAL_HCLASS_INDEX,
                factory->NewEcmaHClass(hClass, ClassLiteral::SIZE, JSType::CLASS_LITERAL));
}

void GlobalEnvConstants::InitGlobalConstantSpecial(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // SPECIAL INIT
    SetConstant(ConstantIndex::UNDEFINED_INDEX, JSTaggedValue::Undefined());
    SetConstant(ConstantIndex::NULL_INDEX, JSTaggedValue::Null());
    auto vm = thread->GetEcmaVM();
    SetConstant(ConstantIndex::EMPTY_STRING_OBJECT_INDEX, JSTaggedValue(EcmaStringAccessor::CreateEmptyString(vm)));
    SetConstant(ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX, factory->NewEmptyArray());
    SetConstant(ConstantIndex::DEFAULT_SUPERS_INDEX,
                WeakVector::Create(thread, SubtypingOperator::DEFAULT_SUPERS_CAPACITY));
    SetConstant(ConstantIndex::EMPTY_LAYOUT_INFO_OBJECT_INDEX, factory->CreateLayoutInfo(0));
    SetConstant(ConstantIndex::EMPTY_TAGGED_QUEUE_OBJECT_INDEX, factory->NewTaggedQueue(0));
}

// NOLINTNEXTLINE(readability-function-size)
void GlobalEnvConstants::InitGlobalConstant(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] auto test = EcmaString::Cast(GetHandledEmptyString().GetObject<EcmaString>());
    SetConstant(ConstantIndex::CONSTRUCTOR_STRING_INDEX, factory->NewFromASCIINonMovable("constructor"));
    SetConstant(ConstantIndex::PROTOTYPE_STRING_INDEX, factory->NewFromASCIINonMovable("prototype"));
    SetConstant(ConstantIndex::LENGTH_STRING_INDEX, factory->NewFromASCIINonMovable("length"));
    SetConstant(ConstantIndex::VALUE_STRING_INDEX, factory->NewFromASCIINonMovable("value"));
    SetConstant(ConstantIndex::SET_STRING_INDEX, factory->NewFromASCIINonMovable("set"));
    SetConstant(ConstantIndex::GET_STRING_INDEX, factory->NewFromASCIINonMovable("get"));
    SetConstant(ConstantIndex::WRITABLE_STRING_INDEX, factory->NewFromASCIINonMovable("writable"));
    SetConstant(ConstantIndex::ENUMERABLE_STRING_INDEX, factory->NewFromASCIINonMovable("enumerable"));
    SetConstant(ConstantIndex::CONFIGURABLE_STRING_INDEX, factory->NewFromASCIINonMovable("configurable"));
    /* non ECMA standard jsapi containers iterators, init to Undefined first */
    InitJSAPIContainers();
    /* SymbolTable *RegisterSymbols */
    SetConstant(ConstantIndex::NAME_STRING_INDEX, factory->NewFromASCIINonMovable("name"));
    SetConstant(ConstantIndex::GETPROTOTYPEOF_STRING_INDEX, factory->NewFromASCIINonMovable("getPrototypeOf"));
    SetConstant(ConstantIndex::SETPROTOTYPEOF_STRING_INDEX, factory->NewFromASCIINonMovable("setPrototypeOf"));
    SetConstant(ConstantIndex::ISEXTENSIBLE_STRING_INDEX, factory->NewFromASCIINonMovable("isExtensible"));
    SetConstant(ConstantIndex::PREVENTEXTENSIONS_STRING_INDEX,
                factory->NewFromASCIINonMovable("preventExtensions"));
    SetConstant(ConstantIndex::GETOWNPROPERTYDESCRIPTOR_STRING_INDEX,
                factory->NewFromASCIINonMovable("getOwnPropertyDescriptor"));
    SetConstant(ConstantIndex::DEFINEPROPERTY_STRING_INDEX, factory->NewFromASCIINonMovable("defineProperty"));
    SetConstant(ConstantIndex::HAS_STRING_INDEX, factory->NewFromASCIINonMovable("has"));
    SetConstant(ConstantIndex::DELETEPROPERTY_STRING_INDEX, factory->NewFromASCIINonMovable("deleteProperty"));
    SetConstant(ConstantIndex::ENUMERATE_STRING_INDEX, factory->NewFromASCIINonMovable("enumerate"));
    SetConstant(ConstantIndex::OWNKEYS_STRING_INDEX, factory->NewFromASCIINonMovable("ownKeys"));
    SetConstant(ConstantIndex::APPLY_STRING_INDEX, factory->NewFromASCIINonMovable("apply"));
    SetConstant(ConstantIndex::NEGATIVE_ZERO_STRING_INDEX, factory->NewFromASCIINonMovable("-0"));
    SetConstant(ConstantIndex::DONE_STRING_INDEX, factory->NewFromASCIINonMovable("done"));
    SetConstant(ConstantIndex::PROXY_STRING_INDEX, factory->NewFromASCIINonMovable("proxy"));
    SetConstant(ConstantIndex::REVOKE_STRING_INDEX, factory->NewFromASCIINonMovable("revoke"));
    SetConstant(ConstantIndex::NEXT_STRING_INDEX, factory->NewFromASCIINonMovable("next"));
    SetConstant(ConstantIndex::TO_STRING_STRING_INDEX, factory->NewFromASCIINonMovable("toString"));
    SetConstant(ConstantIndex::TO_LOCALE_STRING_STRING_INDEX, factory->NewFromASCIINonMovable("toLocaleString"));
    SetConstant(ConstantIndex::VALUE_OF_STRING_INDEX, factory->NewFromASCIINonMovable("valueOf"));
    SetConstant(ConstantIndex::UNDEFINED_STRING_INDEX, factory->NewFromASCIINonMovable("undefined"));
    SetConstant(ConstantIndex::NULL_STRING_INDEX, factory->NewFromASCIINonMovable("null"));
    SetConstant(ConstantIndex::BOOLEAN_STRING_INDEX, factory->NewFromASCIINonMovable("boolean"));
    SetConstant(ConstantIndex::NUMBER_STRING_INDEX, factory->NewFromASCIINonMovable("number"));
    SetConstant(ConstantIndex::BIGINT_STRING_INDEX, factory->NewFromASCIINonMovable("bigint"));
    SetConstant(ConstantIndex::FUNCTION_STRING_INDEX, factory->NewFromASCIINonMovable("function"));
    SetConstant(ConstantIndex::STRING_STRING_INDEX, factory->NewFromASCIINonMovable("string"));
    SetConstant(ConstantIndex::SYMBOL_STRING_INDEX, factory->NewFromASCIINonMovable("symbol"));
    SetConstant(ConstantIndex::OBJECT_STRING_INDEX, factory->NewFromASCIINonMovable("object"));
    SetConstant(ConstantIndex::TRUE_STRING_INDEX, factory->NewFromASCIINonMovable("true"));
    SetConstant(ConstantIndex::FALSE_STRING_INDEX, factory->NewFromASCIINonMovable("false"));
    SetConstant(ConstantIndex::RETURN_STRING_INDEX, factory->NewFromASCIINonMovable("return"));
    SetConstant(ConstantIndex::PROXY_CONSTRUCT_STRING_INDEX, factory->NewFromASCIINonMovable("construct"));
    SetConstant(ConstantIndex::PROXY_CALL_STRING_INDEX, factory->NewFromASCIINonMovable("call"));
    SetConstant(ConstantIndex::PROMISE_THEN_STRING_INDEX, factory->NewFromASCIINonMovable("then"));
    SetConstant(ConstantIndex::PROMISE_CATCH_STRING_INDEX, factory->NewFromASCIINonMovable("catch"));
    SetConstant(ConstantIndex::PROMISE_FINALLY_STRING_INDEX, factory->NewFromASCII("finally"));
    SetConstant(ConstantIndex::PROMISE_STATUS_STRING_INDEX, factory->NewFromASCII("status"));
    SetConstant(ConstantIndex::PROMISE_FULFILLED_STRING_INDEX, factory->NewFromASCII("fulfilled"));
    SetConstant(ConstantIndex::PROMISE_REJECTED_STRING_INDEX, factory->NewFromASCII("rejected"));
    SetConstant(ConstantIndex::PROMISE_REASON_STRING_INDEX, factory->NewFromASCII("reason"));
    SetConstant(ConstantIndex::SCRIPT_JOB_STRING_INDEX, factory->NewFromASCIINonMovable("ScriptJobs"));
    SetConstant(ConstantIndex::PROMISE_STRING_INDEX, factory->NewFromASCIINonMovable("PrimiseJobs"));
    SetConstant(ConstantIndex::THROWER_STRING_INDEX, factory->NewFromASCIINonMovable("Thrower"));
    SetConstant(ConstantIndex::IDENTITY_STRING_INDEX, factory->NewFromASCIINonMovable("Identity"));
    SetConstant(ConstantIndex::CALLER_STRING_INDEX, factory->NewFromASCIINonMovable("caller"));
    SetConstant(ConstantIndex::CALLEE_STRING_INDEX, factory->NewFromASCIINonMovable("callee"));
    SetConstant(ConstantIndex::INT8_ARRAY_STRING_INDEX, factory->NewFromASCIINonMovable("Int8Array"));
    SetConstant(ConstantIndex::UINT8_ARRAY_STRING_INDEX, factory->NewFromASCIINonMovable("Uint8Array"));
    SetConstant(ConstantIndex::UINT8_CLAMPED_ARRAY_STRING_INDEX,
                factory->NewFromASCIINonMovable("Uint8ClampedArray"));
    SetConstant(ConstantIndex::INT16_ARRAY_STRING_INDEX, factory->NewFromASCIINonMovable("Int16Array"));
    SetConstant(ConstantIndex::UINT16_ARRAY_STRING_INDEX, factory->NewFromASCIINonMovable("Uint16Array"));
    SetConstant(ConstantIndex::INT32_ARRAY_STRING_INDEX, factory->NewFromASCIINonMovable("Int32Array"));
    SetConstant(ConstantIndex::UINT32_ARRAY_STRING_INDEX, factory->NewFromASCIINonMovable("Uint32Array"));
    SetConstant(ConstantIndex::FLOAT32_ARRAY_STRING_INDEX, factory->NewFromASCIINonMovable("Float32Array"));
    SetConstant(ConstantIndex::FLOAT64_ARRAY_STRING_INDEX, factory->NewFromASCIINonMovable("Float64Array"));
    SetConstant(ConstantIndex::BIGINT64_ARRAY_STRING_INDEX, factory->NewFromASCIINonMovable("BigInt64Array"));
    SetConstant(ConstantIndex::BIGUINT64_ARRAY_STRING_INDEX, factory->NewFromASCIINonMovable("BigUint64Array"));
    SetConstant(ConstantIndex::ASYNC_FUNCTION_STRING_INDEX, factory->NewFromASCIINonMovable("AsyncFunction"));
    SetConstant(ConstantIndex::PROMISE_RESOLVE_STRING_INDEX, factory->NewFromASCIINonMovable("resolve"));
    SetConstant(ConstantIndex::ID_STRING_INDEX, factory->NewFromASCIINonMovable("id"));
    SetConstant(ConstantIndex::METHOD_STRING_INDEX, factory->NewFromASCIINonMovable("method"));
    SetConstant(ConstantIndex::PARAMS_STRING_INDEX, factory->NewFromASCIINonMovable("params"));
    SetConstant(ConstantIndex::RESULT_STRING_INDEX, factory->NewFromASCIINonMovable("result"));
    SetConstant(ConstantIndex::TO_JSON_STRING_INDEX, factory->NewFromASCIINonMovable("toJSON"));
    SetConstant(ConstantIndex::GLOBAL_STRING_INDEX, factory->NewFromASCIINonMovable("global"));
    SetConstant(ConstantIndex::MESSAGE_STRING_INDEX, factory->NewFromASCIINonMovable("message"));
    SetConstant(ConstantIndex::CAUSE_STRING_INDEX, factory->NewFromASCIINonMovable("cause"));
    SetConstant(ConstantIndex::ERROR_STRING_INDEX, factory->NewFromASCIINonMovable("Error"));
    SetConstant(ConstantIndex::ERRORS_STRING_INDEX, factory->NewFromASCII("errors"));
    SetConstant(ConstantIndex::AGGREGATE_ERROR_STRING_INDEX, factory->NewFromASCII("AggregateError"));
    SetConstant(ConstantIndex::RANGE_ERROR_STRING_INDEX, factory->NewFromASCIINonMovable("RangeError"));
    SetConstant(ConstantIndex::REFERENCE_ERROR_STRING_INDEX, factory->NewFromASCIINonMovable("ReferenceError"));
    SetConstant(ConstantIndex::TYPE_ERROR_STRING_INDEX, factory->NewFromASCIINonMovable("TypeError"));
    SetConstant(ConstantIndex::URI_ERROR_STRING_INDEX, factory->NewFromASCIINonMovable("URIError"));
    SetConstant(ConstantIndex::SYNTAX_ERROR_STRING_INDEX, factory->NewFromASCIINonMovable("SyntaxError"));
    SetConstant(ConstantIndex::EVAL_ERROR_STRING_INDEX, factory->NewFromASCIINonMovable("EvalError"));
    SetConstant(ConstantIndex::OOM_ERROR_STRING_INDEX, factory->NewFromASCIINonMovable("OutOfMemoryError"));
    SetConstant(ConstantIndex::ERROR_FUNC_STRING_INDEX, factory->NewFromASCIINonMovable("errorfunc"));
    SetConstant(ConstantIndex::STACK_STRING_INDEX, factory->NewFromASCIINonMovable("stack"));
    SetConstant(ConstantIndex::STACK_EMPTY_STRING_INDEX, factory->NewFromASCIINonMovable("stackisempty"));
    SetConstant(ConstantIndex::OBJ_NOT_COERCIBLE_STRING_INDEX,
                factory->NewFromASCIINonMovable("objectnotcoercible"));
    /* for Intl. */
    SetConstant(ConstantIndex::LANGUAGE_STRING_CLASS_INDEX, factory->NewFromASCIINonMovable("language"));
    SetConstant(ConstantIndex::SCRIPT_STRING_CLASS_INDEX, factory->NewFromASCIINonMovable("script"));
    SetConstant(ConstantIndex::REGION_STRING_CLASS_INDEX, factory->NewFromASCIINonMovable("region"));
    SetConstant(ConstantIndex::BASE_NAME_STRING_CLASS_INDEX, factory->NewFromASCIINonMovable("baseName"));
    SetConstant(ConstantIndex::CALENDAR_STRING_CLASS_INDEX, factory->NewFromASCIINonMovable("calendar"));
    SetConstant(ConstantIndex::COLLATION_STRING_CLASS_INDEX, factory->NewFromASCIINonMovable("collation"));
    SetConstant(ConstantIndex::HOUR_CYCLE_STRING_CLASS_INDEX, factory->NewFromASCIINonMovable("hourCycle"));
    SetConstant(ConstantIndex::CASE_FIRST_STRING_CLASS_INDEX, factory->NewFromASCIINonMovable("caseFirst"));
    SetConstant(ConstantIndex::NUMERIC_STRING_CLASS_INDEX, factory->NewFromASCIINonMovable("numeric"));
    SetConstant(ConstantIndex::NUMBERING_SYSTEM_STRING_CLASS_INDEX,
                factory->NewFromASCIINonMovable("numberingSystem"));
    SetConstant(ConstantIndex::TYPE_STRING_INDEX, factory->NewFromASCIINonMovable("type"));
    SetConstant(ConstantIndex::LOCALE_MATCHER_STRING_INDEX, factory->NewFromASCIINonMovable("localeMatcher"));
    SetConstant(ConstantIndex::FORMAT_MATCHER_STRING_INDEX, factory->NewFromASCIINonMovable("formatMatcher"));
    SetConstant(ConstantIndex::HOUR12_STRING_INDEX, factory->NewFromASCIINonMovable("hour12"));
    SetConstant(ConstantIndex::H11_STRING_INDEX, factory->NewFromASCIINonMovable("h11"));
    SetConstant(ConstantIndex::H12_STRING_INDEX, factory->NewFromASCIINonMovable("h12"));
    SetConstant(ConstantIndex::H23_STRING_INDEX, factory->NewFromASCIINonMovable("h23"));
    SetConstant(ConstantIndex::H24_STRING_INDEX, factory->NewFromASCIINonMovable("h24"));
    SetConstant(ConstantIndex::WEEK_DAY_STRING_INDEX, factory->NewFromASCIINonMovable("weekday"));
    SetConstant(ConstantIndex::ERA_STRING_INDEX, factory->NewFromASCIINonMovable("era"));
    SetConstant(ConstantIndex::YEAR_STRING_INDEX, factory->NewFromASCIINonMovable("year"));
    SetConstant(ConstantIndex::QUARTER_STRING_INDEX, factory->NewFromASCIINonMovable("quarter"));
    SetConstant(ConstantIndex::MONTH_STRING_INDEX, factory->NewFromASCIINonMovable("month"));
    SetConstant(ConstantIndex::DAY_STRING_INDEX, factory->NewFromASCIINonMovable("day"));
    SetConstant(ConstantIndex::HOUR_STRING_INDEX, factory->NewFromASCIINonMovable("hour"));
    SetConstant(ConstantIndex::MINUTE_STRING_INDEX, factory->NewFromASCIINonMovable("minute"));
    SetConstant(ConstantIndex::SECOND_STRING_INDEX, factory->NewFromASCIINonMovable("second"));
    SetConstant(ConstantIndex::YEARS_STRING_INDEX, factory->NewFromASCIINonMovable("years"));
    SetConstant(ConstantIndex::QUARTERS_STRING_INDEX, factory->NewFromASCIINonMovable("quarters"));
    SetConstant(ConstantIndex::MONTHS_STRING_INDEX, factory->NewFromASCIINonMovable("months"));
    SetConstant(ConstantIndex::DAYS_STRING_INDEX, factory->NewFromASCIINonMovable("days"));
    SetConstant(ConstantIndex::HOURS_STRING_INDEX, factory->NewFromASCIINonMovable("hours"));
    SetConstant(ConstantIndex::MINUTES_STRING_INDEX, factory->NewFromASCIINonMovable("minutes"));
    SetConstant(ConstantIndex::SECONDS_STRING_INDEX, factory->NewFromASCIINonMovable("seconds"));
    SetConstant(ConstantIndex::TIME_ZONE_NAME_STRING_INDEX, factory->NewFromASCIINonMovable("timeZoneName"));
    SetConstant(ConstantIndex::LOCALE_STRING_INDEX, factory->NewFromASCIINonMovable("locale"));
    SetConstant(ConstantIndex::TIME_ZONE_STRING_INDEX, factory->NewFromASCIINonMovable("timeZone"));
    SetConstant(ConstantIndex::LITERAL_STRING_INDEX, factory->NewFromASCIINonMovable("literal"));
    SetConstant(ConstantIndex::YEAR_NAME_STRING_INDEX, factory->NewFromASCIINonMovable("yearName"));
    SetConstant(ConstantIndex::DAY_PERIOD_STRING_INDEX, factory->NewFromASCIINonMovable("dayPeriod"));
    SetConstant(ConstantIndex::FRACTIONAL_SECOND_DIGITS_STRING_INDEX,
                factory->NewFromASCIINonMovable("fractionalSecondDigits"));
    SetConstant(ConstantIndex::FRACTIONAL_SECOND_STRING_INDEX, factory->NewFromASCIINonMovable("fractionalSecond"));
    SetConstant(ConstantIndex::RELATED_YEAR_STRING_INDEX, factory->NewFromASCIINonMovable("relatedYear"));
    SetConstant(ConstantIndex::LOOK_UP_STRING_INDEX, factory->NewFromASCIINonMovable("lookup"));
    SetConstant(ConstantIndex::BEST_FIT_STRING_INDEX, factory->NewFromASCIINonMovable("bestfit"));
    SetConstant(ConstantIndex::DATE_STYLE_STRING_INDEX, factory->NewFromASCIINonMovable("dateStyle"));
    SetConstant(ConstantIndex::TIME_STYLE_STRING_INDEX, factory->NewFromASCIINonMovable("timeStyle"));
    SetConstant(ConstantIndex::UTC_STRING_INDEX, factory->NewFromASCIINonMovable("UTC"));
    SetConstant(ConstantIndex::WEEK_STRING_INDEX, factory->NewFromASCIINonMovable("week"));
    SetConstant(ConstantIndex::WEEKS_STRING_INDEX, factory->NewFromASCIINonMovable("weeks"));
    SetConstant(ConstantIndex::SOURCE_STRING_INDEX, factory->NewFromASCIINonMovable("source"));
    SetConstant(ConstantIndex::FORMAT_STRING_INDEX, factory->NewFromASCIINonMovable("format"));
    SetConstant(ConstantIndex::EN_US_STRING_INDEX, factory->NewFromASCIINonMovable("en-US"));
    SetConstant(ConstantIndex::UND_STRING_INDEX, factory->NewFromASCIINonMovable("und"));
    SetConstant(ConstantIndex::LATN_STRING_INDEX, factory->NewFromASCIINonMovable("latn"));
    SetConstant(ConstantIndex::STYLE_STRING_INDEX, factory->NewFromASCIINonMovable("style"));
    SetConstant(ConstantIndex::UNIT_STRING_INDEX, factory->NewFromASCIINonMovable("unit"));
    SetConstant(ConstantIndex::INTEGER_STRING_INDEX, factory->NewFromASCIINonMovable("integer"));
    SetConstant(ConstantIndex::NAN_STRING_INDEX, factory->NewFromASCIINonMovable("nan"));
    SetConstant(ConstantIndex::INFINITY_STRING_INDEX, factory->NewFromASCIINonMovable("infinity"));
    SetConstant(ConstantIndex::FRACTION_STRING_INDEX, factory->NewFromASCIINonMovable("fraction"));
    SetConstant(ConstantIndex::DECIMAL_STRING_INDEX, factory->NewFromASCIINonMovable("decimal"));
    SetConstant(ConstantIndex::GROUP_STRING_INDEX, factory->NewFromASCIINonMovable("group"));
    SetConstant(ConstantIndex::GROUPS_STRING_INDEX, factory->NewFromASCIINonMovable("groups"));
    SetConstant(ConstantIndex::CURRENCY_STRING_INDEX, factory->NewFromASCIINonMovable("currency"));
    SetConstant(ConstantIndex::CURRENCY_SIGN_STRING_INDEX, factory->NewFromASCIINonMovable("currencySign"));
    SetConstant(ConstantIndex::CURRENCY_DISPLAY_STRING_INDEX, factory->NewFromASCIINonMovable("currencyDisplay"));
    SetConstant(ConstantIndex::PERCENT_SIGN_STRING_INDEX, factory->NewFromASCIINonMovable("percentSign"));
    SetConstant(ConstantIndex::PERCENT_STRING_INDEX, factory->NewFromASCIINonMovable("percent"));
    SetConstant(ConstantIndex::MINUS_SIGN_STRING_INDEX, factory->NewFromASCIINonMovable("minusSign"));
    SetConstant(ConstantIndex::PLUS_SIGN_STRING_INDEX, factory->NewFromASCIINonMovable("plusSign"));
    SetConstant(ConstantIndex::EXPONENT_SEPARATOR_STRING_INDEX,
                factory->NewFromASCIINonMovable("exponentSeparator"));
    SetConstant(ConstantIndex::EXPONENT_MINUS_SIGN_INDEX, factory->NewFromASCIINonMovable("exponentMinusSign"));
    SetConstant(ConstantIndex::EXPONENT_INTEGER_STRING_INDEX, factory->NewFromASCIINonMovable("exponentInteger"));
    SetConstant(ConstantIndex::LONG_STRING_INDEX, factory->NewFromASCIINonMovable("long"));
    SetConstant(ConstantIndex::SHORT_STRING_INDEX, factory->NewFromASCIINonMovable("short"));
    SetConstant(ConstantIndex::FULL_STRING_INDEX, factory->NewFromASCIINonMovable("full"));
    SetConstant(ConstantIndex::MEDIUM_STRING_INDEX, factory->NewFromASCIINonMovable("medium"));
    SetConstant(ConstantIndex::NARROW_STRING_INDEX, factory->NewFromASCIINonMovable("narrow"));
    SetConstant(ConstantIndex::ALWAYS_STRING_INDEX, factory->NewFromASCIINonMovable("always"));
    SetConstant(ConstantIndex::AUTO_STRING_INDEX, factory->NewFromASCIINonMovable("auto"));
    SetConstant(ConstantIndex::THROW_STRING_INDEX, factory->NewFromASCIINonMovable("throw"));
    SetConstant(ConstantIndex::UNIT_DISPLAY_INDEX, factory->NewFromASCIINonMovable("unitDisplay"));
    SetConstant(ConstantIndex::NOTATION_INDEX, factory->NewFromASCIINonMovable("notation"));
    SetConstant(ConstantIndex::COMPACT_DISPALY_INDEX, factory->NewFromASCIINonMovable("compactDisplay"));
    SetConstant(ConstantIndex::USER_GROUPING_INDEX, factory->NewFromASCIINonMovable("useGrouping"));
    SetConstant(ConstantIndex::SIGN_DISPLAY_INDEX, factory->NewFromASCIINonMovable("signDisplay"));
    SetConstant(ConstantIndex::CODE_INDEX, factory->NewFromASCIINonMovable("code"));
    SetConstant(ConstantIndex::NARROW_SYMBOL_INDEX, factory->NewFromASCIINonMovable("narrowSymbol"));
    SetConstant(ConstantIndex::STANDARD_INDEX, factory->NewFromASCIINonMovable("standard"));
    SetConstant(ConstantIndex::ACCOUNTING_INDEX, factory->NewFromASCIINonMovable("accounting"));
    SetConstant(ConstantIndex::SCIENTIFIC_INDEX, factory->NewFromASCIINonMovable("scientific"));
    SetConstant(ConstantIndex::ENGINEERING_INDEX, factory->NewFromASCIINonMovable("engineering"));
    SetConstant(ConstantIndex::COMPACT_STRING_INDEX, factory->NewFromASCIINonMovable("compact"));
    SetConstant(ConstantIndex::NEVER_INDEX, factory->NewFromASCIINonMovable("never"));
    SetConstant(ConstantIndex::EXPECT_ZERO_INDEX, factory->NewFromASCIINonMovable("exceptZero"));
    SetConstant(ConstantIndex::MINIMUM_INTEGER_DIGITS_INDEX,
                factory->NewFromASCIINonMovable("minimumIntegerDigits"));
    SetConstant(ConstantIndex::MINIMUM_FRACTIONDIGITS_INDEX,
                factory->NewFromASCIINonMovable("minimumFractionDigits"));
    SetConstant(ConstantIndex::MAXIMUM_FRACTIONDIGITS_INDEX,
                factory->NewFromASCIINonMovable("maximumFractionDigits"));
    SetConstant(ConstantIndex::MINIMUM_SIGNIFICANTDIGITS_INDEX,
                factory->NewFromASCIINonMovable("minimumSignificantDigits"));
    SetConstant(ConstantIndex::MAXIMUM_SIGNIFICANTDIGITS_INDEX,
                factory->NewFromASCIINonMovable("maximumSignificantDigits"));
    SetConstant(ConstantIndex::INVALID_DATE_INDEX, factory->NewFromASCIINonMovable("Invalid Date"));
    SetConstant(ConstantIndex::USAGE_INDEX, factory->NewFromASCIINonMovable("usage"));
    SetConstant(ConstantIndex::COMPARE_INDEX, factory->NewFromASCIINonMovable("compare"));
    SetConstant(ConstantIndex::SENSITIVITY_INDEX, factory->NewFromASCIINonMovable("sensitivity"));
    SetConstant(ConstantIndex::IGNORE_PUNCTUATION_INDEX, factory->NewFromASCIINonMovable("ignorePunctuation"));
    SetConstant(ConstantIndex::CARDINAL_INDEX, factory->NewFromASCIINonMovable("cardinal"));
    SetConstant(ConstantIndex::ORDINAL_INDEX, factory->NewFromASCIINonMovable("ordinal"));
    SetConstant(ConstantIndex::EXEC_INDEX, factory->NewFromASCIINonMovable("exec"));
    SetConstant(ConstantIndex::LAST_INDEX_INDEX, factory->NewFromASCIINonMovable("lastIndex"));
    SetConstant(ConstantIndex::PLURAL_CATEGORIES_INDEX, factory->NewFromASCIINonMovable("pluralCategories"));
    SetConstant(ConstantIndex::SORT_INDEX, factory->NewFromASCIINonMovable("sort"));
    SetConstant(ConstantIndex::SEARCH_INDEX, factory->NewFromASCIINonMovable("search"));
    SetConstant(ConstantIndex::BASE_INDEX, factory->NewFromASCIINonMovable("base"));
    SetConstant(ConstantIndex::ACCENT_INDEX, factory->NewFromASCIINonMovable("accent"));
    SetConstant(ConstantIndex::CASE_INDEX, factory->NewFromASCIINonMovable("case"));
    SetConstant(ConstantIndex::VARIANT_INDEX, factory->NewFromASCIINonMovable("variant"));
    SetConstant(ConstantIndex::EN_US_POSIX_STRING_INDEX, factory->NewFromASCIINonMovable("en-US-POSIX"));
    SetConstant(ConstantIndex::UPPER_INDEX, factory->NewFromASCIINonMovable("upper"));
    SetConstant(ConstantIndex::LOWER_INDEX, factory->NewFromASCIINonMovable("lower"));
    SetConstant(ConstantIndex::DEFAULT_INDEX, factory->NewFromASCIINonMovable("default"));
    SetConstant(ConstantIndex::SHARED_INDEX, factory->NewFromASCIINonMovable("shared"));
    SetConstant(ConstantIndex::START_RANGE_INDEX, factory->NewFromASCIINonMovable("startRange"));
    SetConstant(ConstantIndex::END_RANGE_INDEX, factory->NewFromASCIINonMovable("endRange"));
    SetConstant(ConstantIndex::ISO8601_INDEX, factory->NewFromASCIINonMovable("iso8601"));
    SetConstant(ConstantIndex::GREGORY_INDEX, factory->NewFromASCIINonMovable("gregory"));
    SetConstant(ConstantIndex::ETHIOAA_INDEX, factory->NewFromASCIINonMovable("ethioaa"));
    SetConstant(ConstantIndex::STICKY_INDEX, factory->NewFromASCIINonMovable("sticky"));
    SetConstant(ConstantIndex::U_INDEX, factory->NewFromASCIINonMovable("u"));
    SetConstant(ConstantIndex::INDEX_INDEX, factory->NewFromASCIINonMovable("index"));
    SetConstant(ConstantIndex::INPUT_INDEX, factory->NewFromASCIINonMovable("input"));
    SetConstant(ConstantIndex::UNICODE_INDEX, factory->NewFromASCIINonMovable("unicode"));
    SetConstant(ConstantIndex::ZERO_INDEX, factory->NewFromASCIINonMovable("0"));
    SetConstant(ConstantIndex::VALUES_INDEX, factory->NewFromASCIINonMovable("values"));
    SetConstant(ConstantIndex::ADD_INDEX, factory->NewFromASCIINonMovable("add"));
    SetConstant(ConstantIndex::AMBIGUOUS_INDEX, factory->NewFromASCIINonMovable("ambiguous"));
    SetConstant(ConstantIndex::MODULE_INDEX, factory->NewFromASCIINonMovable("Module"));
    SetConstant(ConstantIndex::STAR_INDEX, factory->NewFromASCIINonMovable("*"));
    SetConstant(ConstantIndex::DATETIMEFIELD_INDEX, factory->NewFromASCIINonMovable("datetimefield"));
    SetConstant(ConstantIndex::CONJUNCTION_INDEX, factory->NewFromASCIINonMovable("conjunction"));
    SetConstant(ConstantIndex::NONE_INDEX, factory->NewFromASCIINonMovable("none"));
    SetConstant(ConstantIndex::FALLBACK_INDEX, factory->NewFromASCIINonMovable("fallback"));
    SetConstant(ConstantIndex::DISJUNCTION_INDEX, factory->NewFromASCIINonMovable("disjunction"));
    SetConstant(ConstantIndex::ELEMENT_INDEX, factory->NewFromASCIINonMovable("element"));
    SetConstant(ConstantIndex::FLAGS_INDEX, factory->NewFromASCIINonMovable("flags"));
    SetConstant(ConstantIndex::G_INDEX, factory->NewFromASCIINonMovable("g"));
    SetConstant(ConstantIndex::NFC_INDEX, factory->NewFromASCIINonMovable("NFC"));
    SetConstant(ConstantIndex::ENTRIES_INDEX, factory->NewFromASCIINonMovable("entries"));
    SetConstant(ConstantIndex::LEFT_SQUARE_BRACKET_INDEX, factory->NewFromASCIINonMovable("["));
    SetConstant(ConstantIndex::RIGHT_SQUARE_BRACKET_INDEX, factory->NewFromASCIINonMovable("]"));
    SetConstant(ConstantIndex::Y_INDEX, factory->NewFromASCIINonMovable("y"));
    SetConstant(ConstantIndex::DOLLAR_INDEX, factory->NewFromASCIINonMovable("$"));
    SetConstant(ConstantIndex::COMMA_INDEX, factory->NewFromASCIINonMovable(","));
    SetConstant(ConstantIndex::JOIN_INDEX, factory->NewFromASCIINonMovable("join"));
    SetConstant(ConstantIndex::COPY_WITHIN_INDEX, factory->NewFromASCIINonMovable("copyWithin"));
    SetConstant(ConstantIndex::FILL_INDEX, factory->NewFromASCIINonMovable("fill"));
    SetConstant(ConstantIndex::FIND_INDEX, factory->NewFromASCIINonMovable("find"));
    SetConstant(ConstantIndex::FIND_INDEX_INDEX, factory->NewFromASCIINonMovable("findIndex"));
    SetConstant(ConstantIndex::FLAT_INDEX, factory->NewFromASCIINonMovable("flat"));
    SetConstant(ConstantIndex::FLATMAP_INDEX, factory->NewFromASCIINonMovable("flatMap"));
    SetConstant(ConstantIndex::INCLUDES_INDEX, factory->NewFromASCIINonMovable("includes"));
    SetConstant(ConstantIndex::KEYS_INDEX, factory->NewFromASCIINonMovable("keys"));
    SetConstant(ConstantIndex::BOUND_INDEX, factory->NewFromASCIINonMovable("bound"));
    SetConstant(ConstantIndex::BACKSLASH_INDEX, factory->NewFromASCIINonMovable("/"));
    SetConstant(ConstantIndex::SPACE_INDEX, factory->NewFromASCIINonMovable(" "));
    SetConstant(ConstantIndex::NAN_INDEX, factory->NewFromASCIINonMovable("NaN"));
    SetConstant(ConstantIndex::NOT_EQUAL_INDEX, factory->NewFromASCIINonMovable("not-equal"));
    SetConstant(ConstantIndex::OK_INDEX, factory->NewFromASCIINonMovable("ok"));
    SetConstant(ConstantIndex::TIMEOUT_INDEX, factory->NewFromASCIINonMovable("timed-out"));
    SetConstant(ConstantIndex::CJS_EXPORTS_INDEX, factory->NewFromASCIINonMovable("exports"));
    SetConstant(ConstantIndex::CJS_CACHE_INDEX, factory->NewFromASCIINonMovable("_cache"));
    SetConstant(ConstantIndex::NAPI_WRAPPER_INDEX, factory->NewFromASCIINonMovable("_napiwrapper"));
    // for require native module
    SetConstant(ConstantIndex::REQUIRE_NATIVE_MOUDULE_FUNC_INDEX,
                factory->NewFromASCIINonMovable("requireNativeModule"));
    SetConstant(ConstantIndex::REQUIRE_NAPI_FUNC_INDEX, factory->NewFromASCIINonMovable("requireNapi"));
    SetConstant(ConstantIndex::DOLLAR_STRING_ONE_INDEX, factory->NewFromASCIINonMovable("$1"));
    SetConstant(ConstantIndex::DOLLAR_STRING_TWO_INDEX, factory->NewFromASCIINonMovable("$2"));
    SetConstant(ConstantIndex::DOLLAR_STRING_THREE_INDEX, factory->NewFromASCIINonMovable("$3"));
    SetConstant(ConstantIndex::DOLLAR_STRING_FOUR_INDEX, factory->NewFromASCIINonMovable("$4"));
    SetConstant(ConstantIndex::DOLLAR_STRING_FIVE_INDEX, factory->NewFromASCIINonMovable("$5"));
    SetConstant(ConstantIndex::DOLLAR_STRING_SIX_INDEX, factory->NewFromASCIINonMovable("$6"));
    SetConstant(ConstantIndex::DOLLAR_STRING_SEVEN_INDEX, factory->NewFromASCIINonMovable("$7"));
    SetConstant(ConstantIndex::DOLLAR_STRING_EIGHT_INDEX, factory->NewFromASCIINonMovable("$8"));
    SetConstant(ConstantIndex::DOLLAR_STRING_NINE_INDEX, factory->NewFromASCIINonMovable("$9"));

    auto accessor = factory->NewInternalAccessor(reinterpret_cast<void *>(JSFunction::PrototypeSetter),
                                                 reinterpret_cast<void *>(JSFunction::PrototypeGetter));
    SetConstant(ConstantIndex::FUNCTION_PROTOTYPE_ACCESSOR, accessor);
    accessor = factory->NewInternalAccessor(nullptr, reinterpret_cast<void *>(JSFunction::NameGetter));
    SetConstant(ConstantIndex::FUNCTION_NAME_ACCESSOR, accessor);
    accessor = factory->NewInternalAccessor(reinterpret_cast<void *>(JSArray::LengthSetter),
                                            reinterpret_cast<void *>(JSArray::LengthGetter));
    SetConstant(ConstantIndex::ARRAY_LENGTH_ACCESSOR, accessor);
    SetConstant(ConstantIndex::CLASS_PROTOTYPE_HCLASS_INDEX,
                factory->CreateDefaultClassPrototypeHClass(JSHClass::Cast(GetHClassClass().GetTaggedObject())));
    SetConstant(ConstantIndex::CLASS_CONSTRUCTOR_HCLASS_INDEX,
                factory->CreateDefaultClassConstructorHClass(JSHClass::Cast(GetHClassClass().GetTaggedObject())));
    InitClassConstructorOptimizedClass(factory);
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

void GlobalEnvConstants::InitClassConstructorOptimizedClass(ObjectFactory *factory)
{
    auto optimized = factory->CreateDefaultClassConstructorHClass(JSHClass::Cast(GetHClassClass().GetTaggedObject()));
    optimized->SetIsOptimized(true);
    SetConstant(ConstantIndex::CLASS_CONSTRUCTOR_OPTIMIZED_HCLASS_INDEX, optimized);
    auto fastCall = factory->CreateDefaultClassConstructorHClass(JSHClass::Cast(GetHClassClass().GetTaggedObject()));
    fastCall->SetIsOptimized(true);
    fastCall->SetCanFastCall(true);
    SetConstant(ConstantIndex::CLASS_CONSTRUCTOR_OPTIMIZED_WITH_FAST_CALL_HCLASS_INDEX, fastCall);
}

void GlobalEnvConstants::InitElementKindHClass(const JSThread *thread, JSHandle<JSHClass> originHClass)
{
    auto map = thread->GetArrayHClassIndexMap();
    for (auto iter : map) {
        JSHandle<JSHClass> hclass = originHClass;
        if (iter.first != ElementsKind::GENERIC) {
            hclass = JSHClass::Clone(thread, originHClass);
            hclass->SetElementsKind(iter.first);
        }
        SetConstant(iter.second, hclass);
    }
}
}  // namespace panda::ecmascript
