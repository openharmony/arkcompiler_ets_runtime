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

#ifndef ECMASCRIPT_MEM_OBJECT_XRAY_H
#define ECMASCRIPT_MEM_OBJECT_XRAY_H

#include <cstdint>

#include "ecmascript/byte_array.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_async_from_sync_iterator.h"
#include "ecmascript/global_env.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/jobs/pending_job.h"
#include "ecmascript/jspandafile/class_info_extractor.h"
#include "ecmascript/jspandafile/class_literal.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_api/js_api_arraylist_iterator.h"
#include "ecmascript/js_api/js_api_bitvector.h"
#include "ecmascript/js_api/js_api_bitvector_iterator.h"
#include "ecmascript/js_api/js_api_buffer.h"
#include "ecmascript/js_api/js_api_deque.h"
#include "ecmascript/js_api/js_api_deque_iterator.h"
#include "ecmascript/js_api/js_api_hashmap.h"
#include "ecmascript/js_api/js_api_hashmap_iterator.h"
#include "ecmascript/js_api/js_api_hashset.h"
#include "ecmascript/js_api/js_api_hashset_iterator.h"
#include "ecmascript/js_api/js_api_lightweightmap.h"
#include "ecmascript/js_api/js_api_lightweightmap_iterator.h"
#include "ecmascript/js_api/js_api_lightweightset.h"
#include "ecmascript/js_api/js_api_lightweightset_iterator.h"
#include "ecmascript/js_api/js_api_linked_list.h"
#include "ecmascript/js_api/js_api_linked_list_iterator.h"
#include "ecmascript/js_api/js_api_list.h"
#include "ecmascript/js_api/js_api_list_iterator.h"
#include "ecmascript/js_api/js_api_plain_array.h"
#include "ecmascript/js_api/js_api_plain_array_iterator.h"
#include "ecmascript/js_api/js_api_queue.h"
#include "ecmascript/js_api/js_api_queue_iterator.h"
#include "ecmascript/js_api/js_api_stack.h"
#include "ecmascript/js_api/js_api_stack_iterator.h"
#include "ecmascript/js_api/js_api_tree_map.h"
#include "ecmascript/js_api/js_api_tree_map_iterator.h"
#include "ecmascript/js_api/js_api_tree_set.h"
#include "ecmascript/js_api/js_api_tree_set_iterator.h"
#include "ecmascript/js_api/js_api_vector.h"
#include "ecmascript/js_api/js_api_vector_iterator.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_async_function.h"
#include "ecmascript/js_async_generator_object.h"
#include "ecmascript/js_collator.h"
#include "ecmascript/js_dataview.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_finalization_registry.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_intl.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_number_format.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_plural_rules.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_realm.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_regexp_iterator.h"
#include "ecmascript/js_relative_time_format.h"
#include "ecmascript/js_segmenter.h"
#include "ecmascript/js_segments.h"
#include "ecmascript/js_segment_iterator.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_displaynames.h"
#include "ecmascript/js_list_format.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_string_iterator.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/jspandafile/class_info_extractor.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_weak_ref.h"
#include "ecmascript/marker_cell.h"
#include "ecmascript/mem/machine_code.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/module/js_module_namespace.h"
#include "ecmascript/dfx/native_module_failure_info.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/js_shared_module.h"
#include "ecmascript/shared_objects/js_shared_array.h"
#include "ecmascript/shared_objects/js_sendable_arraybuffer.h"
#include "ecmascript/shared_objects/js_shared_array_iterator.h"
#include "ecmascript/shared_objects/js_shared_map.h"
#include "ecmascript/shared_objects/js_shared_map_iterator.h"
#include "ecmascript/shared_objects/js_shared_set.h"
#include "ecmascript/shared_objects/js_shared_set_iterator.h"
#include "ecmascript/shared_objects/js_shared_typed_array.h"
#include "ecmascript/tagged_node.h"
#include "ecmascript/require/js_cjs_module.h"
#include "ecmascript/require/js_cjs_require.h"
#include "ecmascript/require/js_cjs_exports.h"

namespace panda::ecmascript {
class ObjectXRay {
public:
    ObjectXRay() = default;
    ~ObjectXRay() = default;

    static inline void VisitVMRoots(EcmaVM *vm, RootVisitor &visitor, VMRootVisitType type)
    {
        vm->Iterate(visitor, type);
        vm->GetAssociatedJSThread()->Iterate(visitor);
    }

    static inline void VisitJitCodeMap(EcmaVM *vm, const JitCodeMapVisitor &updater)
    {
        vm->GetJSThread()->IterateJitCodeMap(updater);
    }

    template<VisitType visitType, class DerivedVisitor>
    static inline void VisitObjectBody(TaggedObject *object, JSHClass *klass,
                                       BaseObjectVisitor<DerivedVisitor> &visitor)
    {
        // handle body
        JSType type = klass->GetObjectType();
        switch (type) {
            case JSType::JS_OBJECT:
            case JSType::JS_XREF_OBJECT:
            case JSType::JS_ERROR:
            case JSType::JS_EVAL_ERROR:
            case JSType::JS_RANGE_ERROR:
            case JSType::JS_REFERENCE_ERROR:
            case JSType::JS_TYPE_ERROR:
            case JSType::JS_AGGREGATE_ERROR:
            case JSType::JS_URI_ERROR:
            case JSType::JS_SYNTAX_ERROR:
            case JSType::JS_OOM_ERROR:
            case JSType::JS_TERMINATION_ERROR:
            case JSType::JS_ASYNCITERATOR:
            case JSType::JS_ITERATOR:
                JSObject::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_SHARED_OBJECT: {
                auto jsSharedObject = JSSharedObject::Cast(object);
                jsSharedObject->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_ASYNC_FROM_SYNC_ITERATOR:
                JSAsyncFromSyncIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_GLOBAL_OBJECT:
                JSGlobalObject::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_FUNCTION_BASE: {
                auto jsFunctionBase = JSFunctionBase::Cast(object);
                jsFunctionBase->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_FUNCTION: {
                auto jsFunction = JSFunction::Cast(object);
                jsFunction->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_SHARED_FUNCTION: {
                auto jsSharedFunction = JSSharedFunction::Cast(object);
                jsSharedFunction->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_GENERATOR_FUNCTION: {
                auto jsGeneratorFunction = JSGeneratorFunction::Cast(object);
                jsGeneratorFunction->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_ASYNC_GENERATOR_FUNCTION: {
                auto jsGeneratorFunction = JSAsyncGeneratorFunction::Cast(object);
                jsGeneratorFunction->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_PROXY_REVOC_FUNCTION: {
                auto jsProxyRevocFunction = JSProxyRevocFunction::Cast(object);
                jsProxyRevocFunction->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_PROMISE_REACTIONS_FUNCTION: {
                auto jsPromiseReactionsFunction = JSPromiseReactionsFunction::Cast(object);
                jsPromiseReactionsFunction->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_PROMISE_EXECUTOR_FUNCTION: {
                auto jsPromiseExecutorFunction = JSPromiseExecutorFunction::Cast(object);
                jsPromiseExecutorFunction->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_ASYNC_MODULE_FULFILLED_FUNCTION: {
                auto jsAsyncModuleFulfilledFunction = JSAsyncModuleFulfilledFunction::Cast(object);
                jsAsyncModuleFulfilledFunction->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_ASYNC_MODULE_REJECTED_FUNCTION: {
                auto jsAsyncModuleRejectedFunction = JSAsyncModuleRejectedFunction::Cast(object);
                jsAsyncModuleRejectedFunction->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_ASYNC_FROM_SYNC_ITER_UNWARP_FUNCTION: {
                auto jsAsyncFromSyncIterUnwarpFunction = JSAsyncFromSyncIterUnwarpFunction::Cast(object);
                jsAsyncFromSyncIterUnwarpFunction->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION: {
                auto jsPromiseAllResolveElementFunction = JSPromiseAllResolveElementFunction::Cast(object);
                jsPromiseAllResolveElementFunction->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_ASYNC_GENERATOR_RESUME_NEXT_RETURN_PROCESSOR_RST_FTN: {
                auto jsAsyGeneratorRseNextRtnProRstFtn = JSAsyncGeneratorResNextRetProRstFtn::Cast(object);
                jsAsyGeneratorRseNextRtnProRstFtn->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_PROMISE_ANY_REJECT_ELEMENT_FUNCTION: {
                auto jsPromiseAnyRejectElementFunction = JSPromiseAnyRejectElementFunction::Cast(object);
                jsPromiseAnyRejectElementFunction->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_PROMISE_ALL_SETTLED_ELEMENT_FUNCTION: {
                auto jsPromiseAllSettledElementFunction = JSPromiseAllSettledElementFunction::Cast(object);
                jsPromiseAllSettledElementFunction->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_PROMISE_FINALLY_FUNCTION: {
                auto jsPromiseFinallyFunction = JSPromiseFinallyFunction::Cast(object);
                jsPromiseFinallyFunction->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_PROMISE_VALUE_THUNK_OR_THROWER_FUNCTION: {
                auto jsPromiseValueThunkOrThrowerFunction = JSPromiseValueThunkOrThrowerFunction::Cast(object);
                jsPromiseValueThunkOrThrowerFunction->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_ASYNC_FUNCTION:
            case JSType::JS_SHARED_ASYNC_FUNCTION: {
                auto jsAsyncFunction = JSAsyncFunction::Cast(object);
                jsAsyncFunction->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION: {
                auto jsAsyncAwaitStatusFunction = JSAsyncAwaitStatusFunction::Cast(object);
                jsAsyncAwaitStatusFunction->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_REG_EXP:
                JSRegExp::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_SET:
                JSSet::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_SHARED_SET:
                JSSharedSet::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_MAP:
                JSMap::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_SHARED_MAP:
                JSSharedMap::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_WEAK_MAP:
                JSWeakMap::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_WEAK_SET:
                JSWeakSet::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_WEAK_REF:
                JSWeakRef::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_FINALIZATION_REGISTRY:
                JSFinalizationRegistry::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::CELL_RECORD:
                CellRecord::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_DATE:
                JSDate::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_FORIN_ITERATOR:
                JSForInIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_MAP_ITERATOR:
                JSMapIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_SHARED_MAP_ITERATOR:
                JSSharedMapIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_SET_ITERATOR:
                JSSetIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_SHARED_SET_ITERATOR:
                JSSharedSetIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_REG_EXP_ITERATOR:
                JSRegExpIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_ARRAY_ITERATOR:
                JSArrayIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_SHARED_ARRAY_ITERATOR:
                JSSharedArrayIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_STRING_ITERATOR:
                JSStringIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_ARRAY_BUFFER:
                JSArrayBuffer::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_SENDABLE_ARRAY_BUFFER:
                JSSendableArrayBuffer::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_SHARED_ARRAY_BUFFER:
                JSArrayBuffer::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_PROMISE:
                JSPromise::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_DATA_VIEW:
                JSDataView::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_BOUND_FUNCTION: {
                auto jsBoundFunction = JSBoundFunction::Cast(object);
                jsBoundFunction->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_ARGUMENTS:
                JSArguments::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_GENERATOR_OBJECT:
                JSGeneratorObject::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_ASYNC_GENERATOR_OBJECT:
                JSAsyncGeneratorObject::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_ASYNC_FUNC_OBJECT:
                JSAsyncFuncObject::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_ARRAY:
                JSArray::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_SHARED_ARRAY:
                JSSharedArray::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_TYPED_ARRAY:
            case JSType::JS_INT8_ARRAY:
            case JSType::JS_UINT8_ARRAY:
            case JSType::JS_UINT8_CLAMPED_ARRAY:
            case JSType::JS_INT16_ARRAY:
            case JSType::JS_UINT16_ARRAY:
            case JSType::JS_INT32_ARRAY:
            case JSType::JS_UINT32_ARRAY:
            case JSType::JS_FLOAT32_ARRAY:
            case JSType::JS_FLOAT64_ARRAY:
            case JSType::JS_BIGINT64_ARRAY:
            case JSType::JS_BIGUINT64_ARRAY:
                JSTypedArray::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_SHARED_TYPED_ARRAY:
            case JSType::JS_SHARED_INT8_ARRAY:
            case JSType::JS_SHARED_UINT8_ARRAY:
            case JSType::JS_SHARED_UINT8_CLAMPED_ARRAY:
            case JSType::JS_SHARED_INT16_ARRAY:
            case JSType::JS_SHARED_UINT16_ARRAY:
            case JSType::JS_SHARED_INT32_ARRAY:
            case JSType::JS_SHARED_UINT32_ARRAY:
            case JSType::JS_SHARED_FLOAT32_ARRAY:
            case JSType::JS_SHARED_FLOAT64_ARRAY:
            case JSType::JS_SHARED_BIGINT64_ARRAY:
            case JSType::JS_SHARED_BIGUINT64_ARRAY:
                JSSharedTypedArray::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::BYTE_ARRAY:
                if constexpr (visitType == VisitType::ALL_VISIT) {
                    ByteArray::Cast(object)->VisitRangeSlot<visitType>(visitor);
                }
                break;
            case JSType::JS_PRIMITIVE_REF:
                JSPrimitiveRef::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_PROXY: {
                auto jsProxy = JSProxy::Cast(object);
                jsProxy->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::HCLASS:
                // semi gc is not needed to visit hclass
                if constexpr (visitType != VisitType::SEMI_GC_VISIT) {
                    JSHClass::Cast(object)->VisitRangeSlot<visitType>(visitor);
                }
                break;
            case JSType::LINE_STRING:
                if constexpr (visitType == VisitType::ALL_VISIT) {
                    LineEcmaString::Cast(object)->VisitRangeSlot<visitType>(visitor);
                }
                break;
            case JSType::TREE_STRING:
                TreeEcmaString::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::SLICED_STRING:
                SlicedEcmaString::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_NATIVE_POINTER:
                if constexpr ((visitType == VisitType::SNAPSHOT_VISIT) || (visitType == VisitType::ALL_VISIT)) {
                    JSNativePointer::Cast(object)->VisitRangeSlot<visitType>(visitor);
                }
                break;
            case JSType::PROFILE_TYPE_INFO_CELL_0:
            case JSType::PROFILE_TYPE_INFO_CELL_1:
            case JSType::PROFILE_TYPE_INFO_CELL_N:
                ProfileTypeInfoCell::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::FUNCTION_TEMPLATE:
                FunctionTemplate::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::EXTRA_PROFILE_TYPE_INFO:
                ExtraProfileTypeInfo::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::TAGGED_ARRAY:
            case JSType::TAGGED_DICTIONARY:
            case JSType::TEMPLATE_MAP:
            case JSType::LEXICAL_ENV:
            case JSType::SFUNCTION_ENV:
            case JSType::SENDABLE_ENV:
            case JSType::AOT_LITERAL_INFO:
            case JSType::VTABLE:
            case JSType::COW_TAGGED_ARRAY:
                TaggedArray::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::MUTANT_TAGGED_ARRAY:
                if constexpr (visitType == VisitType::ALL_VISIT) {
                    MutantTaggedArray::Cast(object)->VisitRangeSlot<visitType>(visitor);
                }
                break;
            case JSType::COW_MUTANT_TAGGED_ARRAY:
                if constexpr (visitType == VisitType::ALL_VISIT) {
                    COWMutantTaggedArray::Cast(object)->VisitRangeSlot<visitType>(visitor);
                }
                break;
            case JSType::CONSTANT_POOL:
                ConstantPool::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::PROFILE_TYPE_INFO:
                ProfileTypeInfo::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::GLOBAL_ENV:
                GlobalEnv::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::ACCESSOR_DATA:
                AccessorData::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::INTERNAL_ACCESSOR:
                if constexpr (visitType == VisitType::ALL_VISIT) {
                    InternalAccessor::Cast(object)->VisitRangeSlot<visitType>(visitor);
                }
                break;
            case JSType::SYMBOL:
                JSSymbol::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::ENUM_CACHE:
                EnumCache::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_GENERATOR_CONTEXT:
                GeneratorContext::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::PROTOTYPE_HANDLER:
                PrototypeHandler::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::TRANSITION_HANDLER:
                TransitionHandler::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::TRANS_WITH_PROTO_HANDLER:
                TransWithProtoHandler::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::STORE_TS_HANDLER:
                StoreAOTHandler::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::PROPERTY_BOX:
                PropertyBox::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::PROTO_CHANGE_MARKER:
                if constexpr (visitType == VisitType::ALL_VISIT) {
                    ProtoChangeMarker::Cast(object)->VisitRangeSlot<visitType>(visitor);
                }
                break;
            case JSType::MARKER_CELL:
                if constexpr (visitType == VisitType::ALL_VISIT) {
                    MarkerCell::Cast(object)->VisitRangeSlot<visitType>(visitor);
                }
                break;
            case JSType::TRACK_INFO:
                TrackInfo::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::PROTOTYPE_INFO:
                ProtoChangeDetails::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::PROMISE_CAPABILITY:
                PromiseCapability::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::ASYNC_GENERATOR_REQUEST:
                AsyncGeneratorRequest::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::ASYNC_ITERATOR_RECORD:
                AsyncIteratorRecord::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::PROMISE_RECORD:
                PromiseRecord::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::RESOLVING_FUNCTIONS_RECORD:
                ResolvingFunctionsRecord::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::PROMISE_REACTIONS:
                PromiseReaction::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::PROMISE_ITERATOR_RECORD:
                PromiseIteratorRecord::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::MICRO_JOB_QUEUE:
                job::MicroJobQueue::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::PENDING_JOB:
                job::PendingJob::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::COMPLETION_RECORD:
                CompletionRecord::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::PROGRAM:
                Program::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_INTL:
                JSIntl::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_NUMBER_FORMAT:
                JSNumberFormat::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_LOCALE:
                JSLocale::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_DATE_TIME_FORMAT:
                JSDateTimeFormat::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_RELATIVE_TIME_FORMAT:
                JSRelativeTimeFormat::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_INTL_BOUND_FUNCTION: {
                auto jsIntlBoundFunction = JSIntlBoundFunction::Cast(object);
                jsIntlBoundFunction->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_REALM:
                JSRealm::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_COLLATOR:
                JSCollator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_PLURAL_RULES:
                JSPluralRules::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_DISPLAYNAMES:
                JSDisplayNames::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_SEGMENTER:
                JSSegmenter::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_SEGMENTS:
                JSSegments::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_SEGMENT_ITERATOR:
                JSSegmentIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_LIST_FORMAT:
                JSListFormat::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::MACHINE_CODE_OBJECT:
                MachineCode::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::CLASS_INFO_EXTRACTOR: {
                auto classInfoExtractor = ClassInfoExtractor::Cast(object);
                classInfoExtractor->VisitRangeSlot<visitType>(visitor);
                break;
            }
            case JSType::JS_API_QUEUE:
                JSAPIQueue::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_QUEUE_ITERATOR:
                JSAPIQueueIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_ARRAY_LIST:
                JSAPIArrayList::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_ARRAYLIST_ITERATOR:
                JSAPIArrayListIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_LIGHT_WEIGHT_MAP:
                JSAPILightWeightMap::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_LIGHT_WEIGHT_MAP_ITERATOR:
                JSAPILightWeightMapIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_LIGHT_WEIGHT_SET:
                JSAPILightWeightSet::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_LIGHT_WEIGHT_SET_ITERATOR:
                JSAPILightWeightSetIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::RB_TREENODE:
                RBTreeNode::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::LINKED_NODE:
                LinkedNode::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_HASH_MAP:
                JSAPIHashMap::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_HASH_SET:
                JSAPIHashSet::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_HASHMAP_ITERATOR:
                JSAPIHashMapIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_HASHSET_ITERATOR:
                JSAPIHashSetIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_TREE_MAP:
                JSAPITreeMap::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_TREE_SET:
                JSAPITreeSet::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_TREEMAP_ITERATOR:
                JSAPITreeMapIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_TREESET_ITERATOR:
                JSAPITreeSetIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_PLAIN_ARRAY:
                JSAPIPlainArray::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_PLAIN_ARRAY_ITERATOR:
                JSAPIPlainArrayIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_DEQUE:
                JSAPIDeque::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_DEQUE_ITERATOR:
                JSAPIDequeIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_STACK:
                JSAPIStack::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_STACK_ITERATOR:
                JSAPIStackIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_VECTOR:
                JSAPIVector::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_BITVECTOR:
                JSAPIBitVector::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_VECTOR_ITERATOR:
                JSAPIVectorIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_BITVECTOR_ITERATOR:
                JSAPIBitVectorIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_FAST_BUFFER:
                JSAPIFastBuffer::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_LIST:
                JSAPIList::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_LINKED_LIST:
                JSAPILinkedList::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_LIST_ITERATOR:
                JSAPIListIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_API_LINKED_LIST_ITERATOR:
                JSAPILinkedListIterator::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::BIGINT:
                if constexpr (visitType == VisitType::ALL_VISIT) {
                    BigInt::Cast(object)->VisitRangeSlot<visitType>(visitor);
                }
                break;
            case JSType::SOURCE_TEXT_MODULE_RECORD:
                SourceTextModule::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::IMPORTENTRY_RECORD:
                ImportEntry::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::LOCAL_EXPORTENTRY_RECORD:
                LocalExportEntry::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::INDIRECT_EXPORTENTRY_RECORD:
                IndirectExportEntry::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::STAR_EXPORTENTRY_RECORD:
                StarExportEntry::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::RESOLVEDBINDING_RECORD:
                ResolvedBinding::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::RESOLVEDINDEXBINDING_RECORD:
                ResolvedIndexBinding::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::RESOLVEDRECORDINDEXBINDING_RECORD:
                ResolvedRecordIndexBinding::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::RESOLVEDRECORDBINDING_RECORD:
                ResolvedRecordBinding::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_MODULE_NAMESPACE:
                ModuleNamespace::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_CJS_EXPORTS:
                CjsExports::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_CJS_MODULE:
                CjsModule::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::JS_CJS_REQUIRE:
                CjsRequire::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::METHOD:
                Method::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::CLASS_LITERAL:
                ClassLiteral::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
            case JSType::NATIVE_MODULE_FAILURE_INFO:
                NativeModuleFailureInfo::Cast(object)->VisitRangeSlot<visitType>(visitor);
                break;
#ifdef USE_CMC_GC
            case JSType::FREE_OBJECT_WITH_NONE_FIELD:
            case JSType::FREE_OBJECT_WITH_ONE_FIELD:
            case JSType::FREE_OBJECT_WITH_TWO_FIELD:
                break;
#endif
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable, type: " << static_cast<size_t>(type);
                UNREACHABLE();
        }
    }
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_OBJECT_XRAY_H
