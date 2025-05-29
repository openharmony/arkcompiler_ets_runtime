/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/snapshot/mem/snapshot_processor.h"

#include "ecmascript/builtins/builtins_ark_tools.h"
#include "ecmascript/builtins/builtins_array.h"
#include "ecmascript/builtins/builtins_arraybuffer.h"
#include "ecmascript/builtins/builtins_async_function.h"
#include "ecmascript/builtins/builtins_async_generator.h"
#include "ecmascript/builtins/builtins_async_iterator.h"
#include "ecmascript/builtins/builtins_atomics.h"
#include "ecmascript/builtins/builtins_bigint.h"
#include "ecmascript/builtins/builtins_boolean.h"
#include "ecmascript/builtins/builtins_cjs_exports.h"
#include "ecmascript/builtins/builtins_cjs_module.h"
#include "ecmascript/builtins/builtins_cjs_require.h"
#include "ecmascript/builtins/builtins_dataview.h"
#include "ecmascript/builtins/builtins_date.h"
#include "ecmascript/builtins/builtins_errors.h"
#include "ecmascript/builtins/builtins_finalization_registry.h"
#include "ecmascript/builtins/builtins_function.h"
#include "ecmascript/builtins/builtins_generator.h"
#include "ecmascript/builtins/builtins_global.h"
#include "ecmascript/builtins/builtins_iterator.h"
#include "ecmascript/builtins/builtins_json.h"
#include "ecmascript/builtins/builtins_map.h"
#include "ecmascript/builtins/builtins_math.h"
#include "ecmascript/builtins/builtins_number.h"
#include "ecmascript/builtins/builtins_object.h"
#include "ecmascript/builtins/builtins_promise.h"
#include "ecmascript/builtins/builtins_promise_handler.h"
#include "ecmascript/builtins/builtins_promise_job.h"
#include "ecmascript/builtins/builtins_proxy.h"
#include "ecmascript/builtins/builtins_reflect.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/builtins/builtins_set.h"
#include "ecmascript/builtins/builtins_sharedarraybuffer.h"
#include "ecmascript/builtins/builtins_string_iterator.h"
#include "ecmascript/builtins/builtins_symbol.h"
#include "ecmascript/builtins/builtins_typedarray.h"
#include "ecmascript/builtins/builtins_weak_map.h"
#include "ecmascript/builtins/builtins_weak_ref.h"
#include "ecmascript/builtins/builtins_weak_set.h"
#include "ecmascript/containers/containers_arraylist.h"
#include "ecmascript/containers/containers_deque.h"
#include "ecmascript/containers/containers_hashmap.h"
#include "ecmascript/containers/containers_hashset.h"
#include "ecmascript/containers/containers_lightweightmap.h"
#include "ecmascript/containers/containers_lightweightset.h"
#include "ecmascript/containers/containers_linked_list.h"
#include "ecmascript/containers/containers_list.h"
#include "ecmascript/containers/containers_plainarray.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/containers/containers_queue.h"
#include "ecmascript/containers/containers_stack.h"
#include "ecmascript/containers/containers_treemap.h"
#include "ecmascript/containers/containers_treeset.h"
#include "ecmascript/containers/containers_vector.h"
#include "ecmascript/containers/containers_bitvector.h"
#include "ecmascript/runtime_lock.h"
#ifdef ENABLE_NEXT_OPTIMIZATION
#include "ecmascript/ecma_string_table_optimization-inl.h"
#endif
#ifdef ARK_SUPPORT_INTL
#include "ecmascript/builtins/builtins_collator.h"
#include "ecmascript/builtins/builtins_date_time_format.h"
#include "ecmascript/builtins/builtins_displaynames.h"
#include "ecmascript/builtins/builtins_intl.h"
#include "ecmascript/builtins/builtins_list_format.h"
#include "ecmascript/builtins/builtins_locale.h"
#include "ecmascript/builtins/builtins_number_format.h"
#include "ecmascript/builtins/builtins_plural_rules.h"
#include "ecmascript/builtins/builtins_relative_time_format.h"
#include "ecmascript/builtins/builtins_segmenter.h"
#include "ecmascript/builtins/builtins_segments.h"
#include "ecmascript/builtins/builtins_segment_iterator.h"
#endif

namespace panda::ecmascript {
using Number = builtins::BuiltinsNumber;
using BuiltinsBigInt = builtins::BuiltinsBigInt;
using Object = builtins::BuiltinsObject;
using Date = builtins::BuiltinsDate;
using Symbol = builtins::BuiltinsSymbol;
using Boolean = builtins::BuiltinsBoolean;
using BuiltinsMap = builtins::BuiltinsMap;
using BuiltinsSet = builtins::BuiltinsSet;
using BuiltinsWeakMap = builtins::BuiltinsWeakMap;
using BuiltinsWeakSet = builtins::BuiltinsWeakSet;
using BuiltinsWeakRef = builtins::BuiltinsWeakRef;
using BuiltinsFinalizationRegistry = builtins::BuiltinsFinalizationRegistry;
using BuiltinsArray = builtins::BuiltinsArray;
using BuiltinsTypedArray = builtins::BuiltinsTypedArray;
using BuiltinsIterator = builtins::BuiltinsIterator;
using BuiltinsAsyncIterator = builtins::BuiltinsAsyncIterator;
using Error = builtins::BuiltinsError;
using RangeError = builtins::BuiltinsRangeError;
using ReferenceError = builtins::BuiltinsReferenceError;
using TypeError = builtins::BuiltinsTypeError;
using AggregateError = builtins::BuiltinsAggregateError;
using URIError = builtins::BuiltinsURIError;
using SyntaxError = builtins::BuiltinsSyntaxError;
using EvalError = builtins::BuiltinsEvalError;
using OOMError = builtins::BuiltinsOOMError;
using ErrorType = base::ErrorType;
using Global = builtins::BuiltinsGlobal;
using BuiltinsString = builtins::BuiltinsString;
using StringIterator = builtins::BuiltinsStringIterator;
using RegExp = builtins::BuiltinsRegExp;
using Function = builtins::BuiltinsFunction;
using Math = builtins::BuiltinsMath;
using Atomics = builtins::BuiltinsAtomics;
using ArrayBuffer = builtins::BuiltinsArrayBuffer;
using SharedArrayBuffer = builtins::BuiltinsSharedArrayBuffer;
using Json = builtins::BuiltinsJson;
using Proxy = builtins::BuiltinsProxy;
using Reflect = builtins::BuiltinsReflect;
using AsyncFunction = builtins::BuiltinsAsyncFunction;
using GeneratorObject = builtins::BuiltinsGenerator;
using AsyncGeneratorObject = builtins::BuiltinsAsyncGenerator;
using Promise = builtins::BuiltinsPromise;
using BuiltinsPromiseHandler = builtins::BuiltinsPromiseHandler;
using BuiltinsPromiseJob = builtins::BuiltinsPromiseJob;
using BuiltinsCjsExports = builtins::BuiltinsCjsExports;
using BuiltinsCjsModule = builtins::BuiltinsCjsModule;
using BuiltinsCjsRequire = builtins::BuiltinsCjsRequire;
using ArkTools = builtins::BuiltinsArkTools;
using ErrorType = base::ErrorType;
using DataView = builtins::BuiltinsDataView;
using ArrayList = containers::ContainersArrayList;
using HashMap = containers::ContainersHashMap;
using HashSet = containers::ContainersHashSet;
using LightWeightMap = containers::ContainersLightWeightMap;
using LightWeightSet = containers::ContainersLightWeightSet;
using TreeMap = containers::ContainersTreeMap;
using TreeSet = containers::ContainersTreeSet;
using Vector = containers::ContainersVector;
using BitVector = containers::ContainersBitVector;
using Queue = containers::ContainersQueue;
using List = containers::ContainersList;
using LinkedList = containers::ContainersLinkedList;
using PlainArray = containers::ContainersPlainArray;
using Deque = containers::ContainersDeque;
using ContainerStack = panda::ecmascript::containers::ContainersStack;
using ContainersPrivate = containers::ContainersPrivate;
#ifdef ARK_SUPPORT_INTL
using DisplayNames = builtins::BuiltinsDisplayNames;
using ListFormat = builtins::BuiltinsListFormat;
using Intl = builtins::BuiltinsIntl;
using Locale = builtins::BuiltinsLocale;
using DateTimeFormat = builtins::BuiltinsDateTimeFormat;
using NumberFormat = builtins::BuiltinsNumberFormat;
using RelativeTimeFormat = builtins::BuiltinsRelativeTimeFormat;
using Collator = builtins::BuiltinsCollator;
using PluralRules = builtins::BuiltinsPluralRules;
using Segmenter = builtins::BuiltinsSegmenter;
using Segments = builtins::BuiltinsSegments;
#endif

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
static uintptr_t g_nativeTable[] = {
    reinterpret_cast<uintptr_t>(nullptr),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Species),
    reinterpret_cast<uintptr_t>(StringIterator::Next),
    reinterpret_cast<uintptr_t>(Function::FunctionPrototypeInvokeSelf),
    reinterpret_cast<uintptr_t>(Function::FunctionConstructor),
    reinterpret_cast<uintptr_t>(JSFunction::AccessCallerArgumentsThrowTypeError),
    reinterpret_cast<uintptr_t>(Function::FunctionPrototypeApply),
    reinterpret_cast<uintptr_t>(Function::FunctionPrototypeBind),
    reinterpret_cast<uintptr_t>(Function::FunctionPrototypeCall),
    reinterpret_cast<uintptr_t>(Function::FunctionPrototypeToString),
    reinterpret_cast<uintptr_t>(Object::ObjectConstructor),
    reinterpret_cast<uintptr_t>(Object::FromEntries),
    reinterpret_cast<uintptr_t>(Error::ErrorConstructor),
    reinterpret_cast<uintptr_t>(Error::ToString),
    reinterpret_cast<uintptr_t>(RangeError::RangeErrorConstructor),
    reinterpret_cast<uintptr_t>(RangeError::ToString),
    reinterpret_cast<uintptr_t>(ReferenceError::ReferenceErrorConstructor),
    reinterpret_cast<uintptr_t>(ReferenceError::ToString),
    reinterpret_cast<uintptr_t>(TypeError::TypeErrorConstructor),
    reinterpret_cast<uintptr_t>(TypeError::ToString),
    reinterpret_cast<uintptr_t>(TypeError::ThrowTypeError),
    reinterpret_cast<uintptr_t>(AggregateError::AggregateErrorConstructor),
    reinterpret_cast<uintptr_t>(AggregateError::ToString),
    reinterpret_cast<uintptr_t>(URIError::URIErrorConstructor),
    reinterpret_cast<uintptr_t>(URIError::ToString),
    reinterpret_cast<uintptr_t>(SyntaxError::SyntaxErrorConstructor),
    reinterpret_cast<uintptr_t>(SyntaxError::ToString),
    reinterpret_cast<uintptr_t>(EvalError::EvalErrorConstructor),
    reinterpret_cast<uintptr_t>(EvalError::ToString),
    reinterpret_cast<uintptr_t>(OOMError::OOMErrorConstructor),
    reinterpret_cast<uintptr_t>(OOMError::ToString),
    reinterpret_cast<uintptr_t>(Number::NumberConstructor),
    reinterpret_cast<uintptr_t>(Number::ToExponential),
    reinterpret_cast<uintptr_t>(Number::ToFixed),
    reinterpret_cast<uintptr_t>(Number::ToLocaleString),
    reinterpret_cast<uintptr_t>(Number::ToPrecision),
    reinterpret_cast<uintptr_t>(Number::ToString),
    reinterpret_cast<uintptr_t>(Number::ValueOf),
    reinterpret_cast<uintptr_t>(Number::IsFinite),
    reinterpret_cast<uintptr_t>(Number::IsInteger),
    reinterpret_cast<uintptr_t>(Number::IsNaN),
    reinterpret_cast<uintptr_t>(Number::IsSafeInteger),
    reinterpret_cast<uintptr_t>(Number::ParseFloat),
    reinterpret_cast<uintptr_t>(Number::ParseInt),
    reinterpret_cast<uintptr_t>(Symbol::SymbolConstructor),
    reinterpret_cast<uintptr_t>(Symbol::For),
    reinterpret_cast<uintptr_t>(Symbol::KeyFor),
    reinterpret_cast<uintptr_t>(Symbol::DescriptionGetter),
    reinterpret_cast<uintptr_t>(Symbol::ToPrimitive),
    reinterpret_cast<uintptr_t>(Symbol::ToString),
    reinterpret_cast<uintptr_t>(Symbol::ValueOf),
    reinterpret_cast<uintptr_t>(Function::FunctionPrototypeHasInstance),
    reinterpret_cast<uintptr_t>(Date::DateConstructor),
    reinterpret_cast<uintptr_t>(Date::GetDate),
    reinterpret_cast<uintptr_t>(Date::GetDay),
    reinterpret_cast<uintptr_t>(Date::GetFullYear),
    reinterpret_cast<uintptr_t>(Date::GetHours),
    reinterpret_cast<uintptr_t>(Date::GetMilliseconds),
    reinterpret_cast<uintptr_t>(Date::GetMinutes),
    reinterpret_cast<uintptr_t>(Date::GetMonth),
    reinterpret_cast<uintptr_t>(Date::GetSeconds),
    reinterpret_cast<uintptr_t>(Date::GetTime),
    reinterpret_cast<uintptr_t>(Date::GetTimezoneOffset),
    reinterpret_cast<uintptr_t>(Date::GetUTCDate),
    reinterpret_cast<uintptr_t>(Date::GetUTCDay),
    reinterpret_cast<uintptr_t>(Date::GetUTCFullYear),
    reinterpret_cast<uintptr_t>(Date::GetUTCHours),
    reinterpret_cast<uintptr_t>(Date::GetUTCMilliseconds),
    reinterpret_cast<uintptr_t>(Date::GetUTCMinutes),
    reinterpret_cast<uintptr_t>(Date::GetUTCMonth),
    reinterpret_cast<uintptr_t>(Date::GetUTCSeconds),
    reinterpret_cast<uintptr_t>(Date::SetDate),
    reinterpret_cast<uintptr_t>(Date::SetFullYear),
    reinterpret_cast<uintptr_t>(Date::SetHours),
    reinterpret_cast<uintptr_t>(Date::SetMilliseconds),
    reinterpret_cast<uintptr_t>(Date::SetMinutes),
    reinterpret_cast<uintptr_t>(Date::SetMonth),
    reinterpret_cast<uintptr_t>(Date::SetSeconds),
    reinterpret_cast<uintptr_t>(Date::SetTime),
    reinterpret_cast<uintptr_t>(Date::SetUTCDate),
    reinterpret_cast<uintptr_t>(Date::SetUTCFullYear),
    reinterpret_cast<uintptr_t>(Date::SetUTCHours),
    reinterpret_cast<uintptr_t>(Date::SetUTCMilliseconds),
    reinterpret_cast<uintptr_t>(Date::SetUTCMinutes),
    reinterpret_cast<uintptr_t>(Date::SetUTCMonth),
    reinterpret_cast<uintptr_t>(Date::SetUTCSeconds),
    reinterpret_cast<uintptr_t>(Date::ToDateString),
    reinterpret_cast<uintptr_t>(Date::ToISOString),
    reinterpret_cast<uintptr_t>(Date::ToJSON),
    reinterpret_cast<uintptr_t>(Date::ToLocaleDateString),
    reinterpret_cast<uintptr_t>(Date::ToLocaleString),
    reinterpret_cast<uintptr_t>(Date::ToLocaleTimeString),
    reinterpret_cast<uintptr_t>(Date::ToString),
    reinterpret_cast<uintptr_t>(Date::ToTimeString),
    reinterpret_cast<uintptr_t>(Date::ToUTCString),
    reinterpret_cast<uintptr_t>(Date::ValueOf),
    reinterpret_cast<uintptr_t>(Date::ToPrimitive),
    reinterpret_cast<uintptr_t>(Date::Now),
    reinterpret_cast<uintptr_t>(Date::Parse),
    reinterpret_cast<uintptr_t>(Date::UTC),
    reinterpret_cast<uintptr_t>(Object::Assign),
    reinterpret_cast<uintptr_t>(Object::Create),
    reinterpret_cast<uintptr_t>(Object::DefineProperties),
    reinterpret_cast<uintptr_t>(Object::DefineProperty),
    reinterpret_cast<uintptr_t>(Object::Freeze),
    reinterpret_cast<uintptr_t>(Object::GetOwnPropertyDescriptor),
    reinterpret_cast<uintptr_t>(Object::GetOwnPropertyDescriptors),
    reinterpret_cast<uintptr_t>(Object::GetOwnPropertyNames),
    reinterpret_cast<uintptr_t>(Object::GetOwnPropertySymbols),
    reinterpret_cast<uintptr_t>(Object::GetPrototypeOf),
    reinterpret_cast<uintptr_t>(Object::Is),
    reinterpret_cast<uintptr_t>(Object::IsExtensible),
    reinterpret_cast<uintptr_t>(Object::IsFrozen),
    reinterpret_cast<uintptr_t>(Object::IsSealed),
    reinterpret_cast<uintptr_t>(Object::Keys),
    reinterpret_cast<uintptr_t>(Object::Values),
    reinterpret_cast<uintptr_t>(Object::PreventExtensions),
    reinterpret_cast<uintptr_t>(Object::Seal),
    reinterpret_cast<uintptr_t>(Object::SetPrototypeOf),
    reinterpret_cast<uintptr_t>(Object::HasOwnProperty),
    reinterpret_cast<uintptr_t>(Object::IsPrototypeOf),
    reinterpret_cast<uintptr_t>(Object::PropertyIsEnumerable),
    reinterpret_cast<uintptr_t>(Object::ToLocaleString),
    reinterpret_cast<uintptr_t>(Object::ToString),
    reinterpret_cast<uintptr_t>(Object::ValueOf),
    reinterpret_cast<uintptr_t>(Object::ProtoGetter),
    reinterpret_cast<uintptr_t>(Object::ProtoSetter),
    reinterpret_cast<uintptr_t>(Object::CreateRealm),
    reinterpret_cast<uintptr_t>(Object::Entries),
    reinterpret_cast<uintptr_t>(Object::HasOwn),
    reinterpret_cast<uintptr_t>(Boolean::BooleanConstructor),
    reinterpret_cast<uintptr_t>(Boolean::BooleanPrototypeToString),
    reinterpret_cast<uintptr_t>(Boolean::BooleanPrototypeValueOf),
    reinterpret_cast<uintptr_t>(RegExp::RegExpConstructor),
    reinterpret_cast<uintptr_t>(RegExp::Exec),
    reinterpret_cast<uintptr_t>(RegExp::Test),
    reinterpret_cast<uintptr_t>(RegExp::ToString),
    reinterpret_cast<uintptr_t>(RegExp::GetFlags),
    reinterpret_cast<uintptr_t>(RegExp::GetSource),
    reinterpret_cast<uintptr_t>(RegExp::GetGlobal),
    reinterpret_cast<uintptr_t>(RegExp::GetHasIndices),
    reinterpret_cast<uintptr_t>(RegExp::GetIgnoreCase),
    reinterpret_cast<uintptr_t>(RegExp::GetMultiline),
    reinterpret_cast<uintptr_t>(RegExp::GetDotAll),
    reinterpret_cast<uintptr_t>(RegExp::GetSticky),
    reinterpret_cast<uintptr_t>(RegExp::GetUnicode),
    reinterpret_cast<uintptr_t>(RegExp::Split),
    reinterpret_cast<uintptr_t>(RegExp::Search),
    reinterpret_cast<uintptr_t>(RegExp::Match),
    reinterpret_cast<uintptr_t>(RegExp::MatchAll),
    reinterpret_cast<uintptr_t>(RegExp::Replace),
    reinterpret_cast<uintptr_t>(BuiltinsSet::SetConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsSet::Add),
    reinterpret_cast<uintptr_t>(BuiltinsSet::Clear),
    reinterpret_cast<uintptr_t>(BuiltinsSet::Delete),
    reinterpret_cast<uintptr_t>(BuiltinsSet::Has),
    reinterpret_cast<uintptr_t>(BuiltinsSet::ForEach),
    reinterpret_cast<uintptr_t>(BuiltinsSet::Entries),
    reinterpret_cast<uintptr_t>(BuiltinsSet::Values),
    reinterpret_cast<uintptr_t>(BuiltinsSet::GetSize),
    reinterpret_cast<uintptr_t>(BuiltinsSet::Species),
    reinterpret_cast<uintptr_t>(BuiltinsMap::MapConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Set),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Clear),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Delete),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Has),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Get),
    reinterpret_cast<uintptr_t>(BuiltinsMap::ForEach),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Keys),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Values),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Entries),
    reinterpret_cast<uintptr_t>(BuiltinsMap::GetSize),
    reinterpret_cast<uintptr_t>(BuiltinsWeakMap::WeakMapConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsWeakMap::Set),
    reinterpret_cast<uintptr_t>(BuiltinsWeakMap::Delete),
    reinterpret_cast<uintptr_t>(BuiltinsWeakMap::Has),
    reinterpret_cast<uintptr_t>(BuiltinsWeakMap::Get),
    reinterpret_cast<uintptr_t>(BuiltinsWeakSet::WeakSetConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsWeakSet::Add),
    reinterpret_cast<uintptr_t>(BuiltinsWeakSet::Delete),
    reinterpret_cast<uintptr_t>(BuiltinsWeakSet::Has),
    reinterpret_cast<uintptr_t>(BuiltinsWeakRef::WeakRefConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsWeakRef::Deref),
    reinterpret_cast<uintptr_t>(BuiltinsFinalizationRegistry::FinalizationRegistryConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsFinalizationRegistry::Register),
    reinterpret_cast<uintptr_t>(BuiltinsFinalizationRegistry::Unregister),
    reinterpret_cast<uintptr_t>(BuiltinsArray::ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Concat),
    reinterpret_cast<uintptr_t>(BuiltinsArray::CopyWithin),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Entries),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Every),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Fill),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Filter),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Find),
    reinterpret_cast<uintptr_t>(BuiltinsArray::FindIndex),
    reinterpret_cast<uintptr_t>(BuiltinsArray::FindLast),
    reinterpret_cast<uintptr_t>(BuiltinsArray::FindLastIndex),
    reinterpret_cast<uintptr_t>(BuiltinsArray::ForEach),
    reinterpret_cast<uintptr_t>(BuiltinsArray::IndexOf),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Join),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Keys),
    reinterpret_cast<uintptr_t>(BuiltinsArray::LastIndexOf),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Map),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Pop),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Push),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Reduce),
    reinterpret_cast<uintptr_t>(BuiltinsArray::ReduceRight),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Reverse),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Shift),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Slice),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Some),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Sort),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Splice),
    reinterpret_cast<uintptr_t>(BuiltinsArray::ToLocaleString),
    reinterpret_cast<uintptr_t>(BuiltinsArray::ToString),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Unshift),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Values),
    reinterpret_cast<uintptr_t>(BuiltinsArray::From),
    reinterpret_cast<uintptr_t>(BuiltinsArray::IsArray),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Of),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Species),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Includes),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Flat),
    reinterpret_cast<uintptr_t>(BuiltinsArray::FlatMap),
    reinterpret_cast<uintptr_t>(BuiltinsArray::At),
    reinterpret_cast<uintptr_t>(BuiltinsArray::ToReversed),
    reinterpret_cast<uintptr_t>(BuiltinsArray::With),
    reinterpret_cast<uintptr_t>(BuiltinsArray::ToSorted),
    reinterpret_cast<uintptr_t>(BuiltinsArray::ToSpliced),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::TypedArrayBaseConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::CopyWithin),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Entries),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Every),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Fill),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Filter),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Find),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::FindIndex),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::FindLast),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::FindLastIndex),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::ForEach),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::IndexOf),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Join),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Keys),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::LastIndexOf),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Map),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Reduce),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::ReduceRight),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Reverse),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Set),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Slice),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Some),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Sort),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::ToSorted),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Subarray),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::ToLocaleString),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Values),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::With),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::GetBuffer),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::GetByteLength),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::GetByteOffset),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::GetLength),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::ToStringTag),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::At),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::ToReversed),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::From),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Of),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Species),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Includes),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Int8ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Uint8ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Uint8ClampedArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Int16ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Uint16ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Int32ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Uint32ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Float32ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Float64ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::BigInt64ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::BigUint64ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsString::StringConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsString::At),
    reinterpret_cast<uintptr_t>(BuiltinsString::CharAt),
    reinterpret_cast<uintptr_t>(BuiltinsString::CharCodeAt),
    reinterpret_cast<uintptr_t>(BuiltinsString::CodePointAt),
    reinterpret_cast<uintptr_t>(BuiltinsString::IsWellFormed),
    reinterpret_cast<uintptr_t>(BuiltinsString::ToWellFormed),
    reinterpret_cast<uintptr_t>(BuiltinsString::Concat),
    reinterpret_cast<uintptr_t>(BuiltinsString::EndsWith),
    reinterpret_cast<uintptr_t>(BuiltinsString::Includes),
    reinterpret_cast<uintptr_t>(BuiltinsString::IndexOf),
    reinterpret_cast<uintptr_t>(BuiltinsString::LastIndexOf),
    reinterpret_cast<uintptr_t>(BuiltinsString::LocaleCompare),
    reinterpret_cast<uintptr_t>(BuiltinsString::Match),
    reinterpret_cast<uintptr_t>(BuiltinsString::MatchAll),
    reinterpret_cast<uintptr_t>(BuiltinsString::Normalize),
    reinterpret_cast<uintptr_t>(BuiltinsString::PadEnd),
    reinterpret_cast<uintptr_t>(BuiltinsString::PadStart),
    reinterpret_cast<uintptr_t>(BuiltinsString::Repeat),
    reinterpret_cast<uintptr_t>(BuiltinsString::Replace),
    reinterpret_cast<uintptr_t>(BuiltinsString::ReplaceAll),
    reinterpret_cast<uintptr_t>(BuiltinsString::Search),
    reinterpret_cast<uintptr_t>(BuiltinsString::Slice),
    reinterpret_cast<uintptr_t>(BuiltinsString::Split),
    reinterpret_cast<uintptr_t>(BuiltinsString::StartsWith),
    reinterpret_cast<uintptr_t>(BuiltinsString::Substring),
    reinterpret_cast<uintptr_t>(BuiltinsString::SubStr),
    reinterpret_cast<uintptr_t>(BuiltinsString::ToLocaleLowerCase),
    reinterpret_cast<uintptr_t>(BuiltinsString::ToLocaleUpperCase),
    reinterpret_cast<uintptr_t>(BuiltinsString::ToLowerCase),
    reinterpret_cast<uintptr_t>(BuiltinsString::ToString),
    reinterpret_cast<uintptr_t>(BuiltinsString::ToUpperCase),
    reinterpret_cast<uintptr_t>(BuiltinsString::Trim),
    reinterpret_cast<uintptr_t>(BuiltinsString::TrimStart),
    reinterpret_cast<uintptr_t>(BuiltinsString::TrimEnd),
    reinterpret_cast<uintptr_t>(BuiltinsString::TrimLeft),
    reinterpret_cast<uintptr_t>(BuiltinsString::TrimRight),
    reinterpret_cast<uintptr_t>(BuiltinsString::ValueOf),
    reinterpret_cast<uintptr_t>(BuiltinsString::GetStringIterator),
    reinterpret_cast<uintptr_t>(BuiltinsString::FromCharCode),
    reinterpret_cast<uintptr_t>(BuiltinsString::FromCodePoint),
    reinterpret_cast<uintptr_t>(BuiltinsString::Raw),
    reinterpret_cast<uintptr_t>(BuiltinsString::GetLength),
    reinterpret_cast<uintptr_t>(ArrayBuffer::ArrayBufferConstructor),
    reinterpret_cast<uintptr_t>(ArrayBuffer::Slice),
    reinterpret_cast<uintptr_t>(ArrayBuffer::IsView),
    reinterpret_cast<uintptr_t>(ArrayBuffer::Species),
    reinterpret_cast<uintptr_t>(ArrayBuffer::GetByteLength),
    reinterpret_cast<uintptr_t>(SharedArrayBuffer::SharedArrayBufferConstructor),
    reinterpret_cast<uintptr_t>(SharedArrayBuffer::IsSharedArrayBuffer),
    reinterpret_cast<uintptr_t>(SharedArrayBuffer::Species),
    reinterpret_cast<uintptr_t>(SharedArrayBuffer::GetByteLength),
    reinterpret_cast<uintptr_t>(SharedArrayBuffer::Slice),
    reinterpret_cast<uintptr_t>(DataView::DataViewConstructor),
    reinterpret_cast<uintptr_t>(DataView::GetFloat32),
    reinterpret_cast<uintptr_t>(DataView::GetFloat64),
    reinterpret_cast<uintptr_t>(DataView::GetInt8),
    reinterpret_cast<uintptr_t>(DataView::GetInt16),
    reinterpret_cast<uintptr_t>(DataView::GetInt32),
    reinterpret_cast<uintptr_t>(DataView::GetUint8),
    reinterpret_cast<uintptr_t>(DataView::GetUint16),
    reinterpret_cast<uintptr_t>(DataView::GetUint32),
    reinterpret_cast<uintptr_t>(DataView::SetFloat32),
    reinterpret_cast<uintptr_t>(DataView::SetFloat64),
    reinterpret_cast<uintptr_t>(DataView::GetBigInt64),
    reinterpret_cast<uintptr_t>(DataView::GetBigUint64),
    reinterpret_cast<uintptr_t>(DataView::SetInt8),
    reinterpret_cast<uintptr_t>(DataView::SetInt16),
    reinterpret_cast<uintptr_t>(DataView::SetInt32),
    reinterpret_cast<uintptr_t>(DataView::SetUint8),
    reinterpret_cast<uintptr_t>(DataView::SetUint16),
    reinterpret_cast<uintptr_t>(DataView::SetUint32),
    reinterpret_cast<uintptr_t>(DataView::GetBuffer),
    reinterpret_cast<uintptr_t>(DataView::GetByteLength),
    reinterpret_cast<uintptr_t>(DataView::GetOffset),
    reinterpret_cast<uintptr_t>(DataView::SetBigInt64),
    reinterpret_cast<uintptr_t>(DataView::SetBigUint64),
    reinterpret_cast<uintptr_t>(Global::PrintEntrypoint),
    reinterpret_cast<uintptr_t>(Global::NotSupportEval),
    reinterpret_cast<uintptr_t>(Global::IsFinite),
    reinterpret_cast<uintptr_t>(Global::IsNaN),
    reinterpret_cast<uintptr_t>(Global::DecodeURI),
    reinterpret_cast<uintptr_t>(Global::DecodeURIComponent),
    reinterpret_cast<uintptr_t>(Global::EncodeURI),
    reinterpret_cast<uintptr_t>(Global::EncodeURIComponent),
    reinterpret_cast<uintptr_t>(Math::Abs),
    reinterpret_cast<uintptr_t>(Math::Acos),
    reinterpret_cast<uintptr_t>(Math::Acosh),
    reinterpret_cast<uintptr_t>(Math::Asin),
    reinterpret_cast<uintptr_t>(Math::Asinh),
    reinterpret_cast<uintptr_t>(Math::Atan),
    reinterpret_cast<uintptr_t>(Math::Atanh),
    reinterpret_cast<uintptr_t>(Math::Atan2),
    reinterpret_cast<uintptr_t>(Math::Cbrt),
    reinterpret_cast<uintptr_t>(Math::Ceil),
    reinterpret_cast<uintptr_t>(Math::Clz32),
    reinterpret_cast<uintptr_t>(Math::Cos),
    reinterpret_cast<uintptr_t>(Math::Cosh),
    reinterpret_cast<uintptr_t>(Math::Exp),
    reinterpret_cast<uintptr_t>(Math::Expm1),
    reinterpret_cast<uintptr_t>(Math::Floor),
    reinterpret_cast<uintptr_t>(Math::Fround),
    reinterpret_cast<uintptr_t>(Math::Hypot),
    reinterpret_cast<uintptr_t>(Math::Imul),
    reinterpret_cast<uintptr_t>(Math::Log),
    reinterpret_cast<uintptr_t>(Math::Log1p),
    reinterpret_cast<uintptr_t>(Math::Log10),
    reinterpret_cast<uintptr_t>(Math::Log2),
    reinterpret_cast<uintptr_t>(Math::Max),
    reinterpret_cast<uintptr_t>(Math::Min),
    reinterpret_cast<uintptr_t>(Math::Pow),
    reinterpret_cast<uintptr_t>(Math::Random),
    reinterpret_cast<uintptr_t>(Math::Round),
    reinterpret_cast<uintptr_t>(Math::Sign),
    reinterpret_cast<uintptr_t>(Math::Sin),
    reinterpret_cast<uintptr_t>(Math::Sinh),
    reinterpret_cast<uintptr_t>(Math::Sqrt),
    reinterpret_cast<uintptr_t>(Math::Tan),
    reinterpret_cast<uintptr_t>(Math::Tanh),
    reinterpret_cast<uintptr_t>(Math::Trunc),
    reinterpret_cast<uintptr_t>(Atomics::Wait),
    reinterpret_cast<uintptr_t>(Atomics::Exchange),
    reinterpret_cast<uintptr_t>(Atomics::CompareExchange),
    reinterpret_cast<uintptr_t>(Atomics::IsLockFree),
    reinterpret_cast<uintptr_t>(Atomics::Store),
    reinterpret_cast<uintptr_t>(Atomics::Load),
    reinterpret_cast<uintptr_t>(Atomics::Notify),
    reinterpret_cast<uintptr_t>(Atomics::Xor),
    reinterpret_cast<uintptr_t>(Atomics::Or),
    reinterpret_cast<uintptr_t>(Atomics::Sub),
    reinterpret_cast<uintptr_t>(Atomics::And),
    reinterpret_cast<uintptr_t>(Atomics::Add),
    reinterpret_cast<uintptr_t>(Json::Parse),
    reinterpret_cast<uintptr_t>(Json::Stringify),
    reinterpret_cast<uintptr_t>(BuiltinsIterator::Next),
    reinterpret_cast<uintptr_t>(BuiltinsIterator::Return),
    reinterpret_cast<uintptr_t>(BuiltinsIterator::Throw),
    reinterpret_cast<uintptr_t>(BuiltinsIterator::GetIteratorObj),
    reinterpret_cast<uintptr_t>(BuiltinsAsyncIterator::Next),
    reinterpret_cast<uintptr_t>(BuiltinsAsyncIterator::Return),
    reinterpret_cast<uintptr_t>(BuiltinsAsyncIterator::Throw),
    reinterpret_cast<uintptr_t>(BuiltinsAsyncIterator::GetAsyncIteratorObj),
    reinterpret_cast<uintptr_t>(JSForInIterator::Next),
    reinterpret_cast<uintptr_t>(JSRegExpIterator::Next),
    reinterpret_cast<uintptr_t>(JSSetIterator::Next),
    reinterpret_cast<uintptr_t>(JSMapIterator::Next),
    reinterpret_cast<uintptr_t>(JSArrayIterator::Next),
    reinterpret_cast<uintptr_t>(Proxy::ProxyConstructor),
    reinterpret_cast<uintptr_t>(Proxy::Revocable),
    reinterpret_cast<uintptr_t>(Reflect::ReflectApply),
    reinterpret_cast<uintptr_t>(Reflect::ReflectConstruct),
    reinterpret_cast<uintptr_t>(Reflect::ReflectDefineProperty),
    reinterpret_cast<uintptr_t>(Reflect::ReflectDeleteProperty),
    reinterpret_cast<uintptr_t>(Reflect::ReflectGet),
    reinterpret_cast<uintptr_t>(Reflect::ReflectGetOwnPropertyDescriptor),
    reinterpret_cast<uintptr_t>(Reflect::ReflectGetPrototypeOf),
    reinterpret_cast<uintptr_t>(Reflect::ReflectHas),
    reinterpret_cast<uintptr_t>(Reflect::ReflectIsExtensible),
    reinterpret_cast<uintptr_t>(Reflect::ReflectOwnKeys),
    reinterpret_cast<uintptr_t>(Reflect::ReflectPreventExtensions),
    reinterpret_cast<uintptr_t>(Reflect::ReflectSet),
    reinterpret_cast<uintptr_t>(Reflect::ReflectSetPrototypeOf),
    reinterpret_cast<uintptr_t>(AsyncFunction::AsyncFunctionConstructor),
    reinterpret_cast<uintptr_t>(GeneratorObject::GeneratorPrototypeNext),
    reinterpret_cast<uintptr_t>(GeneratorObject::GeneratorPrototypeReturn),
    reinterpret_cast<uintptr_t>(GeneratorObject::GeneratorPrototypeThrow),
    reinterpret_cast<uintptr_t>(GeneratorObject::GeneratorFunctionConstructor),
    reinterpret_cast<uintptr_t>(AsyncGeneratorObject::AsyncGeneratorPrototypeNext),
    reinterpret_cast<uintptr_t>(AsyncGeneratorObject::AsyncGeneratorPrototypeReturn),
    reinterpret_cast<uintptr_t>(AsyncGeneratorObject::AsyncGeneratorPrototypeThrow),
    reinterpret_cast<uintptr_t>(AsyncGeneratorObject::AsyncGeneratorFunctionConstructor),
    reinterpret_cast<uintptr_t>(Promise::PromiseConstructor),
    reinterpret_cast<uintptr_t>(Promise::All),
    reinterpret_cast<uintptr_t>(Promise::Race),
    reinterpret_cast<uintptr_t>(Promise::Resolve),
    reinterpret_cast<uintptr_t>(Promise::Reject),
    reinterpret_cast<uintptr_t>(Promise::Catch),
    reinterpret_cast<uintptr_t>(Promise::Then),
    reinterpret_cast<uintptr_t>(Promise::Finally),
    reinterpret_cast<uintptr_t>(Promise::Any),
    reinterpret_cast<uintptr_t>(Promise::AllSettled),
    reinterpret_cast<uintptr_t>(Promise::GetSpecies),
    reinterpret_cast<uintptr_t>(BuiltinsPromiseJob::PromiseReactionJob),
    reinterpret_cast<uintptr_t>(BuiltinsPromiseJob::PromiseResolveThenableJob),
    reinterpret_cast<uintptr_t>(BuiltinsPromiseJob::DynamicImportJob),
    reinterpret_cast<uintptr_t>(BuiltinsCjsExports::CjsExportsConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsCjsModule::CjsModuleConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsCjsModule::Compiler),
    reinterpret_cast<uintptr_t>(BuiltinsCjsModule::Load),
    reinterpret_cast<uintptr_t>(BuiltinsCjsModule::Require),
    reinterpret_cast<uintptr_t>(BuiltinsCjsModule::GetExportsForCircularRequire),
    reinterpret_cast<uintptr_t>(BuiltinsCjsModule::UpdateChildren),
    reinterpret_cast<uintptr_t>(BuiltinsCjsModule::ResolveFilename),
    reinterpret_cast<uintptr_t>(BuiltinsCjsRequire::CjsRequireConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsCjsRequire::Main),
    reinterpret_cast<uintptr_t>(BuiltinsCjsRequire::Resolve),
    reinterpret_cast<uintptr_t>(ArkTools::ObjectDump),
    reinterpret_cast<uintptr_t>(ArkTools::CompareHClass),
    reinterpret_cast<uintptr_t>(ArkTools::DumpHClass),
    reinterpret_cast<uintptr_t>(BuiltinsBigInt::BigIntConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsBigInt::AsUintN),
    reinterpret_cast<uintptr_t>(BuiltinsBigInt::AsIntN),
    reinterpret_cast<uintptr_t>(BuiltinsBigInt::ToLocaleString),
    reinterpret_cast<uintptr_t>(BuiltinsBigInt::ToString),
    reinterpret_cast<uintptr_t>(BuiltinsBigInt::ValueOf),
#ifdef ARK_SUPPORT_INTL
    reinterpret_cast<uintptr_t>(DisplayNames::DisplayNamesConstructor),
    reinterpret_cast<uintptr_t>(DisplayNames::SupportedLocalesOf),
    reinterpret_cast<uintptr_t>(DisplayNames::Of),
    reinterpret_cast<uintptr_t>(DisplayNames::ResolvedOptions),
    reinterpret_cast<uintptr_t>(Intl::GetCanonicalLocales),
    reinterpret_cast<uintptr_t>(Locale::LocaleConstructor),
    reinterpret_cast<uintptr_t>(Locale::Maximize),
    reinterpret_cast<uintptr_t>(Locale::Minimize),
    reinterpret_cast<uintptr_t>(Locale::ToString),
    reinterpret_cast<uintptr_t>(Locale::GetBaseName),
    reinterpret_cast<uintptr_t>(Locale::GetCalendar),
    reinterpret_cast<uintptr_t>(Locale::GetCaseFirst),
    reinterpret_cast<uintptr_t>(Locale::GetCollation),
    reinterpret_cast<uintptr_t>(Locale::GetHourCycle),
    reinterpret_cast<uintptr_t>(Locale::GetNumeric),
    reinterpret_cast<uintptr_t>(Locale::GetNumberingSystem),
    reinterpret_cast<uintptr_t>(Locale::GetLanguage),
    reinterpret_cast<uintptr_t>(Locale::GetScript),
    reinterpret_cast<uintptr_t>(Locale::GetRegion),
    reinterpret_cast<uintptr_t>(DateTimeFormat::DateTimeFormatConstructor),
    reinterpret_cast<uintptr_t>(DateTimeFormat::SupportedLocalesOf),
    reinterpret_cast<uintptr_t>(DateTimeFormat::Format),
    reinterpret_cast<uintptr_t>(DateTimeFormat::FormatToParts),
    reinterpret_cast<uintptr_t>(DateTimeFormat::ResolvedOptions),
    reinterpret_cast<uintptr_t>(DateTimeFormat::FormatRange),
    reinterpret_cast<uintptr_t>(DateTimeFormat::FormatRangeToParts),
    reinterpret_cast<uintptr_t>(NumberFormat::NumberFormatConstructor),
    reinterpret_cast<uintptr_t>(NumberFormat::SupportedLocalesOf),
    reinterpret_cast<uintptr_t>(NumberFormat::Format),
    reinterpret_cast<uintptr_t>(NumberFormat::FormatToParts),
    reinterpret_cast<uintptr_t>(NumberFormat::ResolvedOptions),
    reinterpret_cast<uintptr_t>(RelativeTimeFormat::RelativeTimeFormatConstructor),
    reinterpret_cast<uintptr_t>(RelativeTimeFormat::SupportedLocalesOf),
    reinterpret_cast<uintptr_t>(RelativeTimeFormat::Format),
    reinterpret_cast<uintptr_t>(RelativeTimeFormat::FormatToParts),
    reinterpret_cast<uintptr_t>(RelativeTimeFormat::ResolvedOptions),
    reinterpret_cast<uintptr_t>(Collator::CollatorConstructor),
    reinterpret_cast<uintptr_t>(Collator::SupportedLocalesOf),
    reinterpret_cast<uintptr_t>(Collator::Compare),
    reinterpret_cast<uintptr_t>(Collator::ResolvedOptions),
    reinterpret_cast<uintptr_t>(PluralRules::PluralRulesConstructor),
    reinterpret_cast<uintptr_t>(PluralRules::SupportedLocalesOf),
    reinterpret_cast<uintptr_t>(PluralRules::Select),
    reinterpret_cast<uintptr_t>(PluralRules::ResolvedOptions),
    reinterpret_cast<uintptr_t>(ListFormat::ListFormatConstructor),
    reinterpret_cast<uintptr_t>(ListFormat::SupportedLocalesOf),
    reinterpret_cast<uintptr_t>(ListFormat::Format),
    reinterpret_cast<uintptr_t>(ListFormat::FormatToParts),
    reinterpret_cast<uintptr_t>(ListFormat::ResolvedOptions),
    reinterpret_cast<uintptr_t>(Segmenter::SegmenterConstructor),
    reinterpret_cast<uintptr_t>(Segmenter::SupportedLocalesOf),
    reinterpret_cast<uintptr_t>(Segmenter::ResolvedOptions),
    reinterpret_cast<uintptr_t>(Segmenter::Segment),
    reinterpret_cast<uintptr_t>(Segments::Containing),
#endif

    // non ECMA standard jsapi containers.
    reinterpret_cast<uintptr_t>(ContainersPrivate::Load),
    reinterpret_cast<uintptr_t>(ArrayList::ArrayListConstructor),
    reinterpret_cast<uintptr_t>(ArrayList::Add),
    reinterpret_cast<uintptr_t>(ArrayList::Insert),
    reinterpret_cast<uintptr_t>(ArrayList::Clear),
    reinterpret_cast<uintptr_t>(ArrayList::Clone),
    reinterpret_cast<uintptr_t>(ArrayList::Has),
    reinterpret_cast<uintptr_t>(ArrayList::GetCapacity),
    reinterpret_cast<uintptr_t>(ArrayList::IncreaseCapacityTo),
    reinterpret_cast<uintptr_t>(ArrayList::TrimToCurrentLength),
    reinterpret_cast<uintptr_t>(ArrayList::GetIndexOf),
    reinterpret_cast<uintptr_t>(ArrayList::IsEmpty),
    reinterpret_cast<uintptr_t>(ArrayList::GetLastIndexOf),
    reinterpret_cast<uintptr_t>(ArrayList::RemoveByIndex),
    reinterpret_cast<uintptr_t>(ArrayList::Remove),
    reinterpret_cast<uintptr_t>(ArrayList::RemoveByRange),
    reinterpret_cast<uintptr_t>(ArrayList::ReplaceAllElements),
    reinterpret_cast<uintptr_t>(ArrayList::SubArrayList),
    reinterpret_cast<uintptr_t>(ArrayList::ConvertToArray),
    reinterpret_cast<uintptr_t>(ArrayList::ForEach),
    reinterpret_cast<uintptr_t>(ArrayList::GetIteratorObj),
    reinterpret_cast<uintptr_t>(ArrayList::Get),
    reinterpret_cast<uintptr_t>(ArrayList::Set),
    reinterpret_cast<uintptr_t>(ArrayList::GetSize),
    reinterpret_cast<uintptr_t>(JSAPIArrayListIterator::Next),
    reinterpret_cast<uintptr_t>(HashMap::HashMapConstructor),
    reinterpret_cast<uintptr_t>(HashMap::HasKey),
    reinterpret_cast<uintptr_t>(HashMap::HasValue),
    reinterpret_cast<uintptr_t>(HashMap::Replace),
    reinterpret_cast<uintptr_t>(HashMap::Keys),
    reinterpret_cast<uintptr_t>(HashMap::Values),
    reinterpret_cast<uintptr_t>(HashMap::Entries),
    reinterpret_cast<uintptr_t>(HashMap::ForEach),
    reinterpret_cast<uintptr_t>(HashMap::Set),
    reinterpret_cast<uintptr_t>(HashMap::SetAll),
    reinterpret_cast<uintptr_t>(HashMap::Remove),
    reinterpret_cast<uintptr_t>(HashMap::Get),
    reinterpret_cast<uintptr_t>(HashMap::Clear),
    reinterpret_cast<uintptr_t>(HashMap::GetLength),
    reinterpret_cast<uintptr_t>(HashMap::IsEmpty),
    reinterpret_cast<uintptr_t>(HashSet::HashSetConstructor),
    reinterpret_cast<uintptr_t>(HashSet::IsEmpty),
    reinterpret_cast<uintptr_t>(HashSet::Has),
    reinterpret_cast<uintptr_t>(HashSet::Add),
    reinterpret_cast<uintptr_t>(HashSet::Remove),
    reinterpret_cast<uintptr_t>(HashSet::Clear),
    reinterpret_cast<uintptr_t>(HashSet::GetLength),
    reinterpret_cast<uintptr_t>(HashSet::Values),
    reinterpret_cast<uintptr_t>(HashSet::Entries),
    reinterpret_cast<uintptr_t>(JSAPIHashMapIterator::Next),
    reinterpret_cast<uintptr_t>(JSAPIHashSetIterator::Next),
    reinterpret_cast<uintptr_t>(LightWeightMap::HasAll),
    reinterpret_cast<uintptr_t>(LightWeightMap::HasKey),
    reinterpret_cast<uintptr_t>(LightWeightMap::HasValue),
    reinterpret_cast<uintptr_t>(LightWeightMap::IncreaseCapacityTo),
    reinterpret_cast<uintptr_t>(LightWeightMap::Entries),
    reinterpret_cast<uintptr_t>(LightWeightMap::Get),
    reinterpret_cast<uintptr_t>(LightWeightMap::GetIndexOfKey),
    reinterpret_cast<uintptr_t>(LightWeightMap::GetIndexOfValue),
    reinterpret_cast<uintptr_t>(LightWeightMap::IsEmpty),
    reinterpret_cast<uintptr_t>(LightWeightMap::GetKeyAt),
    reinterpret_cast<uintptr_t>(LightWeightMap::Keys),
    reinterpret_cast<uintptr_t>(LightWeightMap::SetAll),
    reinterpret_cast<uintptr_t>(LightWeightMap::Set),
    reinterpret_cast<uintptr_t>(LightWeightMap::Remove),
    reinterpret_cast<uintptr_t>(LightWeightMap::RemoveAt),
    reinterpret_cast<uintptr_t>(LightWeightMap::Clear),
    reinterpret_cast<uintptr_t>(LightWeightMap::SetValueAt),
    reinterpret_cast<uintptr_t>(LightWeightMap::ForEach),
    reinterpret_cast<uintptr_t>(LightWeightMap::ToString),
    reinterpret_cast<uintptr_t>(LightWeightMap::GetValueAt),
    reinterpret_cast<uintptr_t>(LightWeightMap::Values),
    reinterpret_cast<uintptr_t>(JSAPILightWeightMapIterator::Next),
    reinterpret_cast<uintptr_t>(LightWeightSet::LightWeightSetConstructor),
    reinterpret_cast<uintptr_t>(LightWeightSet::Add),
    reinterpret_cast<uintptr_t>(LightWeightSet::AddAll),
    reinterpret_cast<uintptr_t>(LightWeightSet::IsEmpty),
    reinterpret_cast<uintptr_t>(LightWeightSet::GetValueAt),
    reinterpret_cast<uintptr_t>(LightWeightSet::HasAll),
    reinterpret_cast<uintptr_t>(LightWeightSet::Has),
    reinterpret_cast<uintptr_t>(LightWeightSet::HasHash),
    reinterpret_cast<uintptr_t>(LightWeightSet::Equal),
    reinterpret_cast<uintptr_t>(LightWeightSet::IncreaseCapacityTo),
    reinterpret_cast<uintptr_t>(LightWeightSet::GetIteratorObj),
    reinterpret_cast<uintptr_t>(LightWeightSet::Values),
    reinterpret_cast<uintptr_t>(LightWeightSet::Entries),
    reinterpret_cast<uintptr_t>(LightWeightSet::ForEach),
    reinterpret_cast<uintptr_t>(LightWeightSet::GetIndexOf),
    reinterpret_cast<uintptr_t>(LightWeightSet::Remove),
    reinterpret_cast<uintptr_t>(LightWeightSet::RemoveAt),
    reinterpret_cast<uintptr_t>(LightWeightSet::Clear),
    reinterpret_cast<uintptr_t>(LightWeightSet::ToString),
    reinterpret_cast<uintptr_t>(LightWeightSet::ToArray),
    reinterpret_cast<uintptr_t>(LightWeightSet::GetSize),
    reinterpret_cast<uintptr_t>(JSAPILightWeightSetIterator::Next),
    reinterpret_cast<uintptr_t>(TreeMap::TreeMapConstructor),
    reinterpret_cast<uintptr_t>(TreeMap::Set),
    reinterpret_cast<uintptr_t>(TreeMap::Get),
    reinterpret_cast<uintptr_t>(TreeMap::Remove),
    reinterpret_cast<uintptr_t>(TreeMap::GetFirstKey),
    reinterpret_cast<uintptr_t>(TreeMap::GetLastKey),
    reinterpret_cast<uintptr_t>(TreeMap::GetLowerKey),
    reinterpret_cast<uintptr_t>(TreeMap::GetHigherKey),
    reinterpret_cast<uintptr_t>(TreeMap::HasKey),
    reinterpret_cast<uintptr_t>(TreeMap::HasValue),
    reinterpret_cast<uintptr_t>(TreeMap::SetAll),
    reinterpret_cast<uintptr_t>(TreeMap::Replace),
    reinterpret_cast<uintptr_t>(TreeMap::Keys),
    reinterpret_cast<uintptr_t>(TreeMap::Values),
    reinterpret_cast<uintptr_t>(TreeMap::Entries),
    reinterpret_cast<uintptr_t>(TreeMap::ForEach),
    reinterpret_cast<uintptr_t>(TreeMap::Clear),
    reinterpret_cast<uintptr_t>(TreeMap::IsEmpty),
    reinterpret_cast<uintptr_t>(TreeMap::GetLength),
    reinterpret_cast<uintptr_t>(TreeSet::TreeSetConstructor),
    reinterpret_cast<uintptr_t>(TreeSet::Add),
    reinterpret_cast<uintptr_t>(TreeSet::Has),
    reinterpret_cast<uintptr_t>(TreeSet::Remove),
    reinterpret_cast<uintptr_t>(TreeSet::GetFirstValue),
    reinterpret_cast<uintptr_t>(TreeSet::GetLastValue),
    reinterpret_cast<uintptr_t>(TreeSet::GetLowerValue),
    reinterpret_cast<uintptr_t>(TreeSet::GetHigherValue),
    reinterpret_cast<uintptr_t>(TreeSet::PopFirst),
    reinterpret_cast<uintptr_t>(TreeSet::PopLast),
    reinterpret_cast<uintptr_t>(TreeSet::IsEmpty),
    reinterpret_cast<uintptr_t>(TreeSet::Values),
    reinterpret_cast<uintptr_t>(TreeSet::Entries),
    reinterpret_cast<uintptr_t>(TreeSet::ForEach),
    reinterpret_cast<uintptr_t>(TreeSet::Clear),
    reinterpret_cast<uintptr_t>(TreeSet::GetLength),
    reinterpret_cast<uintptr_t>(JSAPITreeMapIterator::Next),
    reinterpret_cast<uintptr_t>(JSAPITreeSetIterator::Next),
    reinterpret_cast<uintptr_t>(Deque::DequeConstructor),
    reinterpret_cast<uintptr_t>(Deque::InsertFront),
    reinterpret_cast<uintptr_t>(Deque::InsertEnd),
    reinterpret_cast<uintptr_t>(Deque::GetFirst),
    reinterpret_cast<uintptr_t>(Deque::GetLast),
    reinterpret_cast<uintptr_t>(Deque::Has),
    reinterpret_cast<uintptr_t>(Deque::PopFirst),
    reinterpret_cast<uintptr_t>(Deque::PopLast),
    reinterpret_cast<uintptr_t>(Deque::ForEach),
    reinterpret_cast<uintptr_t>(Deque::GetIteratorObj),
    reinterpret_cast<uintptr_t>(Deque::GetSize),
    reinterpret_cast<uintptr_t>(JSAPIDequeIterator::Next),
    reinterpret_cast<uintptr_t>(Vector::VectorConstructor),
    reinterpret_cast<uintptr_t>(Vector::Add),
    reinterpret_cast<uintptr_t>(Vector::Insert),
    reinterpret_cast<uintptr_t>(Vector::SetLength),
    reinterpret_cast<uintptr_t>(Vector::GetCapacity),
    reinterpret_cast<uintptr_t>(Vector::IncreaseCapacityTo),
    reinterpret_cast<uintptr_t>(Vector::Get),
    reinterpret_cast<uintptr_t>(Vector::GetIndexOf),
    reinterpret_cast<uintptr_t>(Vector::GetIndexFrom),
    reinterpret_cast<uintptr_t>(Vector::IsEmpty),
    reinterpret_cast<uintptr_t>(Vector::GetLastElement),
    reinterpret_cast<uintptr_t>(Vector::GetLastIndexOf),
    reinterpret_cast<uintptr_t>(Vector::GetLastIndexFrom),
    reinterpret_cast<uintptr_t>(Vector::Remove),
    reinterpret_cast<uintptr_t>(Vector::RemoveByIndex),
    reinterpret_cast<uintptr_t>(Vector::RemoveByRange),
    reinterpret_cast<uintptr_t>(Vector::Set),
    reinterpret_cast<uintptr_t>(Vector::SubVector),
    reinterpret_cast<uintptr_t>(Vector::ToString),
    reinterpret_cast<uintptr_t>(Vector::GetSize),
    reinterpret_cast<uintptr_t>(Vector::ForEach),
    reinterpret_cast<uintptr_t>(Vector::ReplaceAllElements),
    reinterpret_cast<uintptr_t>(Vector::TrimToCurrentLength),
    reinterpret_cast<uintptr_t>(Vector::Clear),
    reinterpret_cast<uintptr_t>(Vector::Clone),
    reinterpret_cast<uintptr_t>(Vector::Has),
    reinterpret_cast<uintptr_t>(Vector::GetFirstElement),
    reinterpret_cast<uintptr_t>(Vector::CopyToArray),
    reinterpret_cast<uintptr_t>(Vector::ConvertToArray),
    reinterpret_cast<uintptr_t>(Vector::Sort),
    reinterpret_cast<uintptr_t>(Vector::GetIteratorObj),
    reinterpret_cast<uintptr_t>(JSAPIVectorIterator::Next),
    reinterpret_cast<uintptr_t>(BitVector::BitVectorConstructor),
    reinterpret_cast<uintptr_t>(BitVector::Push),
    reinterpret_cast<uintptr_t>(BitVector::Pop),
    reinterpret_cast<uintptr_t>(BitVector::Has),
    reinterpret_cast<uintptr_t>(BitVector::SetBitsByRange),
    reinterpret_cast<uintptr_t>(BitVector::GetBitsByRange),
    reinterpret_cast<uintptr_t>(BitVector::Resize),
    reinterpret_cast<uintptr_t>(BitVector::SetAllBits),
    reinterpret_cast<uintptr_t>(BitVector::GetBitCountByRange),
    reinterpret_cast<uintptr_t>(BitVector::GetIndexOf),
    reinterpret_cast<uintptr_t>(BitVector::GetLastIndexOf),
    reinterpret_cast<uintptr_t>(BitVector::FlipBitByIndex),
    reinterpret_cast<uintptr_t>(BitVector::FlipBitsByRange),
    reinterpret_cast<uintptr_t>(BitVector::GetSize),
    reinterpret_cast<uintptr_t>(BitVector::GetIteratorObj),
    reinterpret_cast<uintptr_t>(JSAPIBitVectorIterator::Next),
    reinterpret_cast<uintptr_t>(Queue::QueueConstructor),
    reinterpret_cast<uintptr_t>(Queue::Add),
    reinterpret_cast<uintptr_t>(Queue::GetFirst),
    reinterpret_cast<uintptr_t>(Queue::Pop),
    reinterpret_cast<uintptr_t>(Queue::ForEach),
    reinterpret_cast<uintptr_t>(Queue::GetIteratorObj),
    reinterpret_cast<uintptr_t>(Queue::GetSize),
    reinterpret_cast<uintptr_t>(JSAPIQueueIterator::Next),
    reinterpret_cast<uintptr_t>(PlainArray::PlainArrayConstructor),
    reinterpret_cast<uintptr_t>(PlainArray::Add),
    reinterpret_cast<uintptr_t>(PlainArray::Clear),
    reinterpret_cast<uintptr_t>(PlainArray::Clone),
    reinterpret_cast<uintptr_t>(PlainArray::Has),
    reinterpret_cast<uintptr_t>(PlainArray::Get),
    reinterpret_cast<uintptr_t>(PlainArray::GetIteratorObj),
    reinterpret_cast<uintptr_t>(PlainArray::ForEach),
    reinterpret_cast<uintptr_t>(PlainArray::ToString),
    reinterpret_cast<uintptr_t>(PlainArray::GetIndexOfKey),
    reinterpret_cast<uintptr_t>(PlainArray::GetIndexOfValue),
    reinterpret_cast<uintptr_t>(PlainArray::IsEmpty),
    reinterpret_cast<uintptr_t>(PlainArray::GetKeyAt),
    reinterpret_cast<uintptr_t>(PlainArray::Remove),
    reinterpret_cast<uintptr_t>(PlainArray::RemoveAt),
    reinterpret_cast<uintptr_t>(PlainArray::RemoveRangeFrom),
    reinterpret_cast<uintptr_t>(PlainArray::SetValueAt),
    reinterpret_cast<uintptr_t>(PlainArray::GetValueAt),
    reinterpret_cast<uintptr_t>(PlainArray::GetSize),
    reinterpret_cast<uintptr_t>(JSAPIPlainArrayIterator::Next),
    reinterpret_cast<uintptr_t>(ContainerStack::StackConstructor),
    reinterpret_cast<uintptr_t>(ContainerStack::Iterator),
    reinterpret_cast<uintptr_t>(ContainerStack::IsEmpty),
    reinterpret_cast<uintptr_t>(ContainerStack::Push),
    reinterpret_cast<uintptr_t>(ContainerStack::Peek),
    reinterpret_cast<uintptr_t>(ContainerStack::Pop),
    reinterpret_cast<uintptr_t>(ContainerStack::Locate),
    reinterpret_cast<uintptr_t>(ContainerStack::ForEach),
    reinterpret_cast<uintptr_t>(ContainerStack::GetLength),
    reinterpret_cast<uintptr_t>(JSAPIStackIterator::Next),
    reinterpret_cast<uintptr_t>(List::ListConstructor),
    reinterpret_cast<uintptr_t>(List::Add),
    reinterpret_cast<uintptr_t>(List::GetFirst),
    reinterpret_cast<uintptr_t>(List::GetLast),
    reinterpret_cast<uintptr_t>(List::Insert),
    reinterpret_cast<uintptr_t>(List::Clear),
    reinterpret_cast<uintptr_t>(List::RemoveByIndex),
    reinterpret_cast<uintptr_t>(List::Remove),
    reinterpret_cast<uintptr_t>(List::Has),
    reinterpret_cast<uintptr_t>(List::IsEmpty),
    reinterpret_cast<uintptr_t>(List::Get),
    reinterpret_cast<uintptr_t>(List::GetIndexOf),
    reinterpret_cast<uintptr_t>(List::GetLastIndexOf),
    reinterpret_cast<uintptr_t>(List::Set),
    reinterpret_cast<uintptr_t>(List::ForEach),
    reinterpret_cast<uintptr_t>(List::ReplaceAllElements),
    reinterpret_cast<uintptr_t>(List::GetIteratorObj),
    reinterpret_cast<uintptr_t>(List::Equal),
    reinterpret_cast<uintptr_t>(List::Sort),
    reinterpret_cast<uintptr_t>(List::ConvertToArray),
    reinterpret_cast<uintptr_t>(List::GetSubList),
    reinterpret_cast<uintptr_t>(List::Length),
    reinterpret_cast<uintptr_t>(JSAPIListIterator::Next),
    reinterpret_cast<uintptr_t>(LinkedList::LinkedListConstructor),
    reinterpret_cast<uintptr_t>(LinkedList::Add),
    reinterpret_cast<uintptr_t>(LinkedList::GetFirst),
    reinterpret_cast<uintptr_t>(LinkedList::GetLast),
    reinterpret_cast<uintptr_t>(LinkedList::Insert),
    reinterpret_cast<uintptr_t>(LinkedList::AddFirst),
    reinterpret_cast<uintptr_t>(LinkedList::Clear),
    reinterpret_cast<uintptr_t>(LinkedList::Clone),
    reinterpret_cast<uintptr_t>(LinkedList::Has),
    reinterpret_cast<uintptr_t>(LinkedList::Get),
    reinterpret_cast<uintptr_t>(LinkedList::GetIndexOf),
    reinterpret_cast<uintptr_t>(LinkedList::GetLastIndexOf),
    reinterpret_cast<uintptr_t>(LinkedList::RemoveByIndex),
    reinterpret_cast<uintptr_t>(LinkedList::Remove),
    reinterpret_cast<uintptr_t>(LinkedList::RemoveFirst),
    reinterpret_cast<uintptr_t>(LinkedList::RemoveLast),
    reinterpret_cast<uintptr_t>(LinkedList::RemoveFirstFound),
    reinterpret_cast<uintptr_t>(LinkedList::RemoveLastFound),
    reinterpret_cast<uintptr_t>(LinkedList::Set),
    reinterpret_cast<uintptr_t>(LinkedList::ConvertToArray),
    reinterpret_cast<uintptr_t>(LinkedList::ForEach),
    reinterpret_cast<uintptr_t>(JSAPILinkedListIterator::Next),

    // not builtins method
    reinterpret_cast<uintptr_t>(JSFunction::PrototypeSetter),
    reinterpret_cast<uintptr_t>(JSFunction::PrototypeGetter),
    reinterpret_cast<uintptr_t>(JSFunction::NameGetter),
    reinterpret_cast<uintptr_t>(JSFunction::LengthGetter),
    reinterpret_cast<uintptr_t>(JSArray::LengthSetter),
    reinterpret_cast<uintptr_t>(JSArray::LengthGetter),
    reinterpret_cast<uintptr_t>(JSPandaFileManager::GetInstance)
};

void SnapshotProcessor::Initialize()
{
    auto heap = const_cast<Heap *>(vm_->GetHeap());
    size_t oldSpaceCapacity = heap->GetOldSpace()->GetInitialCapacity();
    oldLocalSpace_ = new LocalSpace(heap, oldSpaceCapacity, oldSpaceCapacity);
    size_t nonMovableCapacity = heap->GetNonMovableSpace()->GetInitialCapacity();
    nonMovableLocalSpace_ = new LocalSpace(heap, nonMovableCapacity, nonMovableCapacity);
    size_t machineCodeCapacity = heap->GetMachineCodeSpace()->GetInitialCapacity();
    machineCodeLocalSpace_ = new LocalSpace(heap, machineCodeCapacity, machineCodeCapacity);
    size_t snapshotSpaceCapacity = heap->GetSnapshotSpace()->GetMaximumCapacity();
    snapshotLocalSpace_ = new SnapshotSpace(heap, snapshotSpaceCapacity, snapshotSpaceCapacity);
    hugeObjectLocalSpace_ = new HugeObjectSpace(heap, heap->GetHeapRegionAllocator(),
                                                oldSpaceCapacity, oldSpaceCapacity);
}

SnapshotProcessor::~SnapshotProcessor()
{
    pandaMethod_.clear();
    stringVector_.clear();
    deserializeStringVector_.clear();
    regionIndexMap_.clear();
    if (oldLocalSpace_ != nullptr) {
        oldLocalSpace_->Reset();
        delete oldLocalSpace_;
        oldLocalSpace_ = nullptr;
    }
    if (nonMovableLocalSpace_ != nullptr) {
        nonMovableLocalSpace_->Reset();
        delete nonMovableLocalSpace_;
        nonMovableLocalSpace_ = nullptr;
    }
    if (machineCodeLocalSpace_ != nullptr) {
        machineCodeLocalSpace_->Reset();
        delete machineCodeLocalSpace_;
        machineCodeLocalSpace_ = nullptr;
    }
    if (snapshotLocalSpace_ != nullptr) {
        snapshotLocalSpace_->Destroy();
        delete snapshotLocalSpace_;
        snapshotLocalSpace_ = nullptr;
    }
    if (hugeObjectLocalSpace_ != nullptr) {
        hugeObjectLocalSpace_->Destroy();
        delete hugeObjectLocalSpace_;
        hugeObjectLocalSpace_ = nullptr;
    }
}

void SnapshotProcessor::StopAllocate()
{
    oldLocalSpace_->Stop();
    nonMovableLocalSpace_->Stop();
    machineCodeLocalSpace_->Stop();
    snapshotLocalSpace_->Stop();
}

void SnapshotProcessor::WriteObjectToFile(std::fstream &writer)
{
    WriteSpaceObjectToFile(oldLocalSpace_, writer);
    WriteSpaceObjectToFile(nonMovableLocalSpace_, writer);
    WriteSpaceObjectToFile(machineCodeLocalSpace_, writer);
    WriteSpaceObjectToFile(snapshotLocalSpace_, writer);
    WriteHugeObjectToFile(hugeObjectLocalSpace_, writer);
}

void SnapshotProcessor::WriteSpaceObjectToFile(Space* space, std::fstream &writer)
{
    size_t regionCount = space->GetRegionCount();
    if (regionCount > 0) {
        auto lastRegion = space->GetCurrentRegion();
        space->EnumerateRegions([this, &writer, lastRegion](Region *current) {
            if (current != lastRegion) {
                ASAN_UNPOISON_MEMORY_REGION(reinterpret_cast<void *>(ToUintPtr(current)), DEFAULT_REGION_SIZE);
                ResetRegionUnusedRange(current);
                SnapshotRegionHeadInfo info = GenerateRegionHeadInfo(current);
                // Firstly, serialize the region head information into the file;
                writer.write(reinterpret_cast<char *>(&info), SnapshotRegionHeadInfo::RegionHeadInfoSize());
                // Secondly, write the valid region memory (exclude region head and GC bit set).
                writer.write(reinterpret_cast<char *>(current->packedData_.begin_),
                             ToUintPtr(current) + DEFAULT_REGION_SIZE - current->packedData_.begin_);
                writer.flush();
            }
        });

        SnapshotRegionHeadInfo info = GenerateRegionHeadInfo(lastRegion);
        // Firstly, serialize the region object into the file;
        writer.write(reinterpret_cast<char *>(&info), SnapshotRegionHeadInfo::RegionHeadInfoSize());
        // Secondly, write the valid region memory (exclude region head and GC bit set).
        writer.write(reinterpret_cast<char *>(lastRegion->packedData_.begin_),
                     lastRegion->highWaterMark_ - lastRegion->packedData_.begin_);
        writer.flush();
    }
}

void SnapshotProcessor::WriteHugeObjectToFile(HugeObjectSpace* space, std::fstream &writer)
{
    space->EnumerateRegions([&writer](Region *region) {
        SnapshotRegionHeadInfo info;
        info.regionIndex_ = SnapshotHelper::GetHugeObjectRegionIndex(region->GetSnapshotData());
        size_t objSize = SnapshotHelper::GetHugeObjectSize(region->GetSnapshotData());
        info.aliveObjectSize_ = objSize;
        // Firstly, serialize the region head information into the file;
        writer.write(reinterpret_cast<char *>(&info), SnapshotRegionHeadInfo::RegionHeadInfoSize());
        // Secondly, write the valid region memory (exclude region head and GC bit set).
        writer.write(reinterpret_cast<char *>(region->packedData_.begin_), objSize);
        writer.flush();
    });
}

std::vector<uint32_t> SnapshotProcessor::StatisticsObjectSize()
{
    std::vector<uint32_t> objSizeVector;
    objSizeVector.emplace_back(StatisticsSpaceObjectSize(oldLocalSpace_));
    objSizeVector.emplace_back(StatisticsSpaceObjectSize(nonMovableLocalSpace_));
    objSizeVector.emplace_back(StatisticsSpaceObjectSize(machineCodeLocalSpace_));
    objSizeVector.emplace_back(StatisticsSpaceObjectSize(snapshotLocalSpace_));
    objSizeVector.emplace_back(StatisticsHugeObjectSize(hugeObjectLocalSpace_));
    return objSizeVector;
}

uint32_t SnapshotProcessor::StatisticsSpaceObjectSize(Space* space)
{
    size_t regionCount = space->GetRegionCount();
    size_t objSize = 0U;
    if (regionCount > 0) {
        auto lastRegion = space->GetCurrentRegion();
        size_t lastRegionSize = lastRegion->highWaterMark_ - lastRegion->packedData_.begin_;
        objSize = (regionCount - 1) * (SnapshotRegionHeadInfo::RegionHeadInfoSize() +
            Region::GetRegionAvailableSize()) + SnapshotRegionHeadInfo::RegionHeadInfoSize() + lastRegionSize;
    }
    ASSERT(objSize <= Constants::MAX_UINT_32);
    return static_cast<uint32_t>(objSize);
}

uint32_t SnapshotProcessor::StatisticsHugeObjectSize(HugeObjectSpace* space)
{
    size_t objSize = 0U;
    space->EnumerateRegions([&objSize](Region *region) {
        objSize += SnapshotRegionHeadInfo::RegionHeadInfoSize();
        uint64_t snapshotData = region->GetSnapshotData();
        // huge object size is storaged in region param snapshotData_ high 32 bits
        objSize += SnapshotHelper::GetHugeObjectSize(snapshotData);
    });
    return static_cast<uint32_t>(objSize);
}

void SnapshotProcessor::ProcessObjectQueue(CQueue<TaggedObject *> *queue,
                                           std::unordered_map<uint64_t, ObjectEncode> *data)
{
    while (!queue->empty()) {
        auto taggedObject = queue->front();
        if (taggedObject == nullptr) {
            break;
        }
        queue->pop();
        SerializeObject(taggedObject, queue, data);
    }

    StopAllocate();
}

uintptr_t SnapshotProcessor::AllocateObjectToLocalSpace(Space *space, size_t objectSize)
{
    uintptr_t newObj = 0;
    if (space->GetSpaceType() == MemSpaceType::HUGE_OBJECT_SPACE) {
        newObj = reinterpret_cast<HugeObjectSpace *>(space)->Allocate(objectSize, vm_->GetAssociatedJSThread());
    } else if (space->GetSpaceType() == MemSpaceType::SNAPSHOT_SPACE) {
        newObj = reinterpret_cast<SnapshotSpace *>(space)->Allocate(objectSize);
    } else {
        newObj = reinterpret_cast<LocalSpace *>(space)->Allocate(objectSize);
    }
    auto current = space->GetCurrentRegion();
    if (newObj == current->GetBegin()) {
        // region param snapshotData_ low 32 bits is reused to record regionIndex
        uint64_t snapshotData = regionIndex_;
        if (current->InHugeObjectSpace()) {
            // region param snapshotData_ high 32 bits is reused to record huge object size
            snapshotData += SnapshotHelper::EncodeHugeObjectSize(objectSize);
        }
        current->SetSnapshotData(snapshotData);
        regionIndex_++;
    }
    return newObj;
}

void SnapshotProcessor::SetObjectEncodeField(uintptr_t obj, size_t offset, uint64_t value)
{
    *reinterpret_cast<uint64_t *>(obj + offset) = value;
}

void SnapshotProcessor::DeserializeObjectExcludeString(uintptr_t oldSpaceBegin, size_t oldSpaceObjSize,
                                                       size_t nonMovableObjSize, size_t machineCodeObjSize,
                                                       size_t snapshotObjSize, size_t hugeSpaceObjSize)
{
    uintptr_t nonMovableBegin = oldSpaceBegin + oldSpaceObjSize;
    uintptr_t machineCodeBegin = nonMovableBegin + nonMovableObjSize;
    uintptr_t snapshotBegin = machineCodeBegin + machineCodeObjSize;
    uintptr_t hugeObjBegin = snapshotBegin + snapshotObjSize;
    auto heap = vm_->GetHeap();
    auto oldSpace = heap->GetOldSpace();
    auto nonMovableSpace = heap->GetNonMovableSpace();
    auto machineCodeSpace = heap->GetMachineCodeSpace();
    auto snapshotSpace = heap->GetSnapshotSpace();
    auto hugeObjectSpace = heap->GetHugeObjectSpace();

    DeserializeSpaceObject(oldSpaceBegin, oldSpace, oldSpaceObjSize);
    DeserializeSpaceObject(nonMovableBegin, nonMovableSpace, nonMovableObjSize);
    DeserializeSpaceObject(machineCodeBegin, machineCodeSpace, machineCodeObjSize);
    DeserializeSpaceObject(snapshotBegin, snapshotSpace, snapshotObjSize);
    DeserializeHugeSpaceObject(hugeObjBegin, hugeObjectSpace, hugeSpaceObjSize);
    snapshotSpace->ResetAllocator();
}

void SnapshotProcessor::DeserializeSpaceObject(uintptr_t beginAddr, Space* space, size_t spaceObjSize)
{
    size_t numberOfRegions = 0U;
    if (spaceObjSize != 0) {
        numberOfRegions = (spaceObjSize - 1) / (Region::GetRegionAvailableSize() +
            SnapshotRegionHeadInfo::RegionHeadInfoSize()) + 1;
    }
    for (size_t i = 0; i < numberOfRegions; i++) {
        Region *region = vm_->GetHeapRegionAllocator()->AllocateAlignedRegion(
            space, DEFAULT_REGION_SIZE, vm_->GetAssociatedJSThread(), const_cast<Heap *>(vm_->GetHeap()));
        auto info = ToNativePtr<SnapshotRegionHeadInfo>(beginAddr +
            i * (Region::GetRegionAvailableSize() + SnapshotRegionHeadInfo::RegionHeadInfoSize()));
        uintptr_t objectBeginAddr = ToUintPtr(info) + SnapshotRegionHeadInfo::RegionHeadInfoSize();
        size_t regionIndex = info->regionIndex_;
        size_t liveObjectSize = info->aliveObjectSize_;
        regionIndexMap_.emplace(regionIndex, region);

        ASAN_UNPOISON_MEMORY_REGION(reinterpret_cast<void *>(region->packedData_.begin_), liveObjectSize);
        if (errno_t ret = memcpy_s(ToVoidPtr(region->packedData_.begin_),
                                   liveObjectSize,
                                   ToVoidPtr(objectBeginAddr),
                                   liveObjectSize); ret != EOK) {
            LOG_FULL(FATAL) << "memcpy_s failed: " << ret;
            UNREACHABLE();
        }

        // Other information like aliveObject size, highWaterMark etc. in the region object to restore.
        region->aliveObject_ = liveObjectSize;
        region->highWaterMark_ = region->packedData_.begin_ + liveObjectSize;
        region->SetGCFlag(RegionGCFlags::NEED_RELOCATE);

        if (space->GetSpaceType() != MemSpaceType::SNAPSHOT_SPACE) {
            auto sparseSpace = reinterpret_cast<SparseSpace *>(space);
            sparseSpace->FreeLiveRange(region, region->GetHighWaterMark(), region->GetEnd(), true);
            sparseSpace->IncreaseLiveObjectSize(liveObjectSize);
            sparseSpace->IncreaseAllocatedSize(liveObjectSize);
            sparseSpace->AddRegion(region);
        } else {
            auto snapshotSpace = reinterpret_cast<SnapshotSpace *>(space);
            snapshotSpace->IncreaseLiveObjectSize(liveObjectSize);
            snapshotSpace->AddRegion(region);
        }
    }
}

void SnapshotProcessor::DeserializeHugeSpaceObject(uintptr_t beginAddr, HugeObjectSpace* space, size_t hugeSpaceObjSize)
{
    uintptr_t currentAddr = beginAddr;
    uintptr_t endAddr = beginAddr + hugeSpaceObjSize;
    while (currentAddr < endAddr) {
        auto info = ToNativePtr<SnapshotRegionHeadInfo>(currentAddr);
        // Retrieve the data beginning address based on the serialized data format.
        uintptr_t copyFrom = ToUintPtr(info) + SnapshotRegionHeadInfo::RegionHeadInfoSize();
        size_t objSize = info->aliveObjectSize_;
        size_t alignedRegionObjSize = AlignUp(sizeof(Region), static_cast<size_t>(MemAlignment::MEM_ALIGN_REGION));
        size_t hugeRegionHeadSize = AlignUp(alignedRegionObjSize + GCBitset::BYTE_PER_WORD,
                                            static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));

        ASSERT(objSize > MAX_REGULAR_HEAP_OBJECT_SIZE);
        size_t alignedHugeRegionSize = AlignUp(objSize + hugeRegionHeadSize, PANDA_POOL_ALIGNMENT_IN_BYTES);
        Region *region = vm_->GetHeapRegionAllocator()->AllocateAlignedRegion(
            space, alignedHugeRegionSize, vm_->GetAssociatedJSThread(), const_cast<Heap *>(vm_->GetHeap()));
        size_t regionIndex = info->regionIndex_;
        regionIndexMap_.emplace(regionIndex, region);

        ASAN_UNPOISON_MEMORY_REGION(reinterpret_cast<void *>(region->packedData_.begin_), objSize);
        if (memcpy_s(ToVoidPtr(region->packedData_.begin_),
                     objSize,
                     ToVoidPtr(copyFrom),
                     objSize) != EOK) {
            LOG_FULL(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }

        // Other information like aliveObject size, highWaterMark etc. in the region object to restore.
        region->aliveObject_ = objSize;
        region->highWaterMark_ = region->packedData_.begin_ + objSize;
        region->SetGCFlag(RegionGCFlags::NEED_RELOCATE);
        space->AddRegion(region);

        currentAddr += SnapshotRegionHeadInfo::RegionHeadInfoSize();
        currentAddr += objSize;
    }
}

void SnapshotProcessor::DeserializeString(uintptr_t stringBegin, uintptr_t stringEnd)
{
    EcmaStringTable *stringTable = vm_->GetEcmaStringTable();
    JSThread *thread = vm_->GetJSThread();
    ASSERT(deserializeStringVector_.empty());
    auto hugeSpace = sHeap_->GetHugeObjectSpace();
    auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
    auto lineStringClass = globalConst->GetLineStringClass();
    while (stringBegin < stringEnd) {
        // str is from snapshot file, which is in native heap.
        EcmaString *str = reinterpret_cast<EcmaString *>(stringBegin);
        str->SetClassWithoutBarrier(reinterpret_cast<JSHClass *>(lineStringClass.GetTaggedObject()));
        size_t strSize = EcmaStringAccessor(str).ObjectSize();
        strSize = AlignUp(strSize, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
        {
            auto hashcode = EcmaStringAccessor(str).GetHashcode();
#if ENABLE_NEXT_OPTIMIZATION
            EcmaString *strFromTable = stringTable->GetOrInternString(
                vm_, hashcode,
                [strSize, hugeSpace, thread, str, this]() {
                    uintptr_t newObj = 0;
                    if (UNLIKELY(strSize > MAX_REGULAR_HEAP_OBJECT_SIZE)) {
                        newObj = hugeSpace->Allocate(thread, strSize);
                    } else {
                        newObj = this->sHeap_->GetOldSpace()->TryAllocateAndExpand(thread, strSize, true);
                    }
                    if (newObj == 0) {
                        LOG_ECMA_MEM(FATAL) << "Snapshot Allocate OldSharedSpace OOM";
                        UNREACHABLE();
                    }
                    if (memcpy_s(ToVoidPtr(newObj), strSize, str, strSize) != EOK) {
                        LOG_FULL(FATAL) << "memcpy_s failed";
                        UNREACHABLE();
                    }

                    EcmaString *value = reinterpret_cast<EcmaString *>(newObj);
#ifdef USE_CMC_GC
                    ASSERT(value->IsInSharedHeap());
#else
                    ASSERT(Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(value))->InSharedHeap());
#endif
                    ASSERT(EcmaStringAccessor(value).NotTreeString());
                    JSHandle<EcmaString> stringHandle(thread, value);
                    return stringHandle;
                },
                [str](EcmaString *foundString) {
                    ASSERT(EcmaStringAccessor(str).NotTreeString());
                    return EcmaStringAccessor::StringsAreEqual(foundString, str);
                });
            ASSERT(strFromTable != nullptr);
            deserializeStringVector_.emplace_back(thread, strFromTable);
#else
            RuntimeLockHolder locker(thread,
                stringTable->stringTable_[EcmaStringTable::GetTableId(hashcode)].mutex_);
            auto strFromTable = stringTable->GetStringThreadUnsafe(str, hashcode);
            if (strFromTable) {
                deserializeStringVector_.emplace_back(thread, strFromTable);
            } else {
                uintptr_t newObj = 0;
                if (UNLIKELY(strSize > MAX_REGULAR_HEAP_OBJECT_SIZE)) {
                    newObj = hugeSpace->Allocate(thread, strSize);
                } else {
                    newObj = sHeap_->GetOldSpace()->TryAllocateAndExpand(thread, strSize, true);
                }
                if (newObj == 0) {
                    LOG_ECMA_MEM(FATAL) << "Snapshot Allocate OldSharedSpace OOM";
                    UNREACHABLE();
                }
                if (memcpy_s(ToVoidPtr(newObj), strSize, str, strSize) != EOK) {
                    LOG_FULL(FATAL) << "memcpy_s failed";
                    UNREACHABLE();
                }
                str = reinterpret_cast<EcmaString *>(newObj);
                stringTable->InsertStringToTableWithHashThreadUnsafe(str, hashcode);
                deserializeStringVector_.emplace_back(thread, str);
            }
#endif
        }
        stringBegin += strSize;
    }
}

void SnapshotProcessor::DeserializePandaMethod(uintptr_t begin, uintptr_t end, MethodLiteral *methods,
                                               size_t &methodNums, size_t &others)
{
    for (size_t i = 0; i < others; i++) {
        pandaMethod_.emplace_back(begin);
        auto method = reinterpret_cast<Method *>(begin);
        if (memcpy_s(methods + (--methodNums), Method::Size(), method, Method::Size()) != EOK) {
            LOG_FULL(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
        begin += Method::Size();
        if (begin >= end) {
            others = others - i - 1;
        }
    }
}

void SnapshotProcessor::HandleRootObject(SnapshotType type, uintptr_t rootObjectAddr,
                                         size_t objType, size_t &constSpecialIndex)
{
    switch (type) {
        case SnapshotType::VM_ROOT: {
            if (JSType(objType) == JSType::GLOBAL_ENV) {
                vm_->GetJSThread()->SetGlueGlobalEnv(
                    JSTaggedValue(reinterpret_cast<GlobalEnv *>(rootObjectAddr)));
            } else if (JSType(objType) == JSType::MICRO_JOB_QUEUE) {
                vm_->SetMicroJobQueue(reinterpret_cast<job::MicroJobQueue *>(rootObjectAddr));
            }
            break;
        }
        case SnapshotType::BUILTINS: {
            JSTaggedValue result(static_cast<JSTaggedType>(rootObjectAddr));
            auto constants = const_cast<GlobalEnvConstants *>(vm_->GetJSThread()->GlobalConstants());
            size_t constCount = constants->GetConstantCount();
            while (constants->IsSpecialOrUndefined(constSpecialIndex)) {
                constSpecialIndex++; // Skip special or undefined value
            }
            if (constSpecialIndex < constCount) {
                constants->SetConstant(ConstantIndex(constSpecialIndex), result);
            } else {
                vm_->GetJSThread()->SetGlueGlobalEnv(
                    JSTaggedValue(reinterpret_cast<GlobalEnv *>(rootObjectAddr)));
            }
            constSpecialIndex++;
            break;
        }
        case SnapshotType::AI: {
            JSTaggedValue item = JSTaggedValue(static_cast<JSTaggedType>(rootObjectAddr));
            if (!isRootObjRelocate_ && item.IsTaggedArray()) {
                root_ = item;
                isRootObjRelocate_ = true;
            }
            break;
        }
        default:
            break;
    }
}

void SnapshotProcessor::AddRootObjectToAOTFileManager(SnapshotType type, const CString &fileName)
{
    if (type == SnapshotType::AI) {
        ASSERT(!root_.IsHole());
        AOTFileManager *aotFileManager = vm_->GetAOTFileManager();
        aotFileManager->ParseDeserializedData(fileName, root_);
    }
}

SnapshotProcessor::SerializeObjectVisitor::SerializeObjectVisitor(SnapshotProcessor *processor, uintptr_t snapshotObj,
    CQueue<TaggedObject *> *queue, std::unordered_map<uint64_t, ObjectEncode> *data)
    : processor_(processor), snapshotObj_(snapshotObj), queue_(queue), data_(data) {}

void SnapshotProcessor::SerializeObjectVisitor::VisitObjectRangeImpl(BaseObject *root, uintptr_t start,
                                                                     uintptr_t endAddr, VisitObjectArea area)
{
    int index = 0;
    ObjectSlot end(endAddr);
    for (ObjectSlot slot(start); slot < end; slot++) {
        if (area == VisitObjectArea::NATIVE_POINTER) {
            auto nativePointer = *reinterpret_cast<void **>(slot.SlotAddress());
            processor_->SetObjectEncodeField(snapshotObj_, slot.SlotAddress() - ToUintPtr(root),
                                             processor_->NativePointerToEncodeBit(nativePointer).GetValue());
        } else {
            if (processor_->VisitObjectBodyWithRep(TaggedObject::Cast(root), slot, snapshotObj_, index, area)) {
                continue;
            }
            auto fieldAddr = reinterpret_cast<JSTaggedType *>(slot.SlotAddress());
            processor_->SetObjectEncodeField(snapshotObj_, slot.SlotAddress() - ToUintPtr(root),
                                             processor_->SerializeTaggedField(fieldAddr, queue_, data_));
        }
    }
}

void SnapshotProcessor::SerializeObject(TaggedObject *objectHeader, CQueue<TaggedObject *> *queue,
                                        std::unordered_map<uint64_t, ObjectEncode> *data)
{
    auto hclass = objectHeader->GetClass();
    JSType objectType = hclass->GetObjectType();
    uintptr_t snapshotObj = 0;
    if (UNLIKELY(data->find(ToUintPtr(objectHeader)) == data->end())) {
        LOG_FULL(FATAL) << "Data map can not find object";
        UNREACHABLE();
    } else {
        snapshotObj = data->find(ToUintPtr(objectHeader))->second.first;
    }

    // header
    EncodeBit encodeBit = SerializeObjectHeader(objectHeader, static_cast<size_t>(objectType), queue, data);
    SetObjectEncodeField(snapshotObj, 0, encodeBit.GetValue());

    SerializeObjectVisitor visitor(this, snapshotObj, queue, data);
    ObjectXRay::VisitObjectBody<VisitType::SNAPSHOT_VISIT>(objectHeader, objectHeader->GetClass(), visitor);
}

bool SnapshotProcessor::VisitObjectBodyWithRep(TaggedObject *root, ObjectSlot slot, uintptr_t obj, int index,
    VisitObjectArea area)
{
    if (area != VisitObjectArea::IN_OBJECT) {
        return false;
    }
    auto hclass = root->GetClass();
    ASSERT(!hclass->IsAllTaggedProp());
    auto layout = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
    auto attr = layout->GetAttr(index++);
    if (attr.GetRepresentation() == Representation::DOUBLE) {
        auto fieldAddr = reinterpret_cast<double *>(slot.SlotAddress());
        SetObjectEncodeField(obj, slot.SlotAddress() - ToUintPtr(root),
                             JSTaggedValue(*fieldAddr).GetRawData());
        return true;
    } else if (attr.GetRepresentation() == Representation::INT) {
        auto fieldAddr = reinterpret_cast<JSTaggedType *>(slot.SlotAddress());
        SetObjectEncodeField(obj, slot.SlotAddress() - ToUintPtr(root),
                             JSTaggedValue(static_cast<int32_t>(*fieldAddr)).GetRawData());
        return true;
    }
    return false;
}

void SnapshotProcessor::Relocate(SnapshotType type, const JSPandaFile *jsPandaFile, uint64_t rootObjSize)
{
    size_t methodNums = 0;
    MethodLiteral *methods = nullptr;
    if (jsPandaFile) {
        methodNums = jsPandaFile->GetNumMethods();
        methods = jsPandaFile->GetMethodLiterals();
    }

    auto heap = vm_->GetHeap();
    auto oldSpace = heap->GetOldSpace();
    auto nonMovableSpace = heap->GetNonMovableSpace();
    auto machineCodeSpace = heap->GetMachineCodeSpace();
    auto snapshotSpace = heap->GetSnapshotSpace();
    auto hugeObjectSpace = heap->GetHugeObjectSpace();

    RelocateSpaceObject(oldSpace, type, methods, methodNums, rootObjSize);
    RelocateSpaceObject(nonMovableSpace, type, methods, methodNums, rootObjSize);
    RelocateSpaceObject(machineCodeSpace, type, methods, methodNums, rootObjSize);
    RelocateSpaceObject(snapshotSpace, type, methods, methodNums, rootObjSize);
    RelocateSpaceObject(hugeObjectSpace, type, methods, methodNums, rootObjSize);
}

void SnapshotProcessor::RelocateSpaceObject(Space* space, SnapshotType type, MethodLiteral* methods,
                                            size_t methodNums, size_t rootObjSize)
{
    size_t others = 0;
    size_t objIndex = 0;
    size_t constSpecialIndex = 0;
    EcmaStringTable *stringTable = vm_->GetEcmaStringTable();
    space->EnumerateRegions([stringTable, &others, &objIndex, &rootObjSize, &constSpecialIndex,
                            &type, this, methods, &methodNums](Region *current) {
        if (!current->NeedRelocate()) {
            return;
        }
        current->ClearGCFlag(RegionGCFlags::NEED_RELOCATE);
        size_t allocated = current->GetAllocatedBytes();
        uintptr_t begin = current->GetBegin();
        uintptr_t end = begin + allocated;
        while (begin < end) {
            if (others != 0) {
                DeserializePandaMethod(begin, end, methods, methodNums, others);
                break;
            }
            EncodeBit encodeBit(*reinterpret_cast<uint64_t *>(begin));
            auto objType = encodeBit.GetObjectType();
            if (objType == Constants::MASK_METHOD_SPACE_BEGIN) {
                begin += sizeof(uint64_t);
                others = encodeBit.GetNativePointerOrObjectIndex();
                DeserializePandaMethod(begin, end, methods, methodNums, others);
                break;
            }
            TaggedObject *objectHeader = reinterpret_cast<TaggedObject *>(begin);
            DeserializeClassWord(objectHeader);
            DeserializeField(objectHeader);
            if (builtinsDeserialize_ &&
                (JSType(objType) >= JSType::STRING_FIRST && JSType(objType) <= JSType::STRING_LAST)) {
                EcmaString *str = reinterpret_cast<EcmaString *>(begin);
                EcmaStringAccessor(str).ClearInternString();
                stringTable->GetOrInternFlattenString(vm_, str);
            }
            if (objIndex < rootObjSize) {
                HandleRootObject(type, begin, objType, constSpecialIndex);
            }
            begin = begin + AlignUp(objectHeader->GetSize(),
                                    static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
            objIndex++;
        }
    });
}

EncodeBit SnapshotProcessor::SerializeObjectHeader(TaggedObject *objectHeader, size_t objectType,
                                                   CQueue<TaggedObject *> *queue,
                                                   std::unordered_map<uint64_t, ObjectEncode> *data)
{
    auto hclass = objectHeader->GetClass();
    ASSERT(hclass != nullptr);
    EncodeBit encodeBit(0);
    if (data->find(ToUintPtr(hclass)) == data->end()) {
        encodeBit = EncodeTaggedObject(hclass, queue, data);
    } else {
        ObjectEncode objectEncodePair = data->find(ToUintPtr(hclass))->second;
        encodeBit = objectEncodePair.second;
    }
    encodeBit.SetObjectType(objectType);
    return encodeBit;
}

uint64_t SnapshotProcessor::SerializeTaggedField(JSTaggedType *tagged, CQueue<TaggedObject *> *queue,
                                                 std::unordered_map<uint64_t, ObjectEncode> *data)
{
    JSTaggedValue taggedValue(*tagged);
    if (taggedValue.IsWeak()) {
        taggedValue.RemoveWeakTag();
        if (taggedValue.IsJSHClass()) {
            EncodeBit encodeBit = GetObjectEncode(taggedValue, queue, data);
            encodeBit.SetTSWeakObject();
            return encodeBit.GetValue();
        }
        EncodeBit special(JSTaggedValue::Undefined().GetRawData());
        special.SetObjectSpecial();
        return special.GetValue();
    }

    if (taggedValue.IsSpecial()) {
        EncodeBit special(taggedValue.GetRawData());
        special.SetObjectSpecial();
        return special.GetValue();  // special encode bit
    }

    if (!taggedValue.IsHeapObject()) {
        return taggedValue.GetRawData();  // not object
    }

    EncodeBit encodeBit = GetObjectEncode(taggedValue, queue, data);

    if (taggedValue.IsString()) {
        encodeBit.SetReferenceToString(true);
    }
    return encodeBit.GetValue();  // object
}

void SnapshotProcessor::DeserializeTaggedField(uint64_t *value, TaggedObject *root)
{
    EncodeBit encodeBit(*value);
    if (!builtinsDeserialize_ && encodeBit.IsReference() && encodeBit.IsGlobalConstOrBuiltins()) {
        size_t index = encodeBit.GetNativePointerOrObjectIndex();
        auto object = vm_->GetSnapshotEnv()->RelocateRootObjectAddr(index);
        *value = object;
        WriteBarrier<WriteBarrierType::DESERIALIZE>(vm_->GetJSThread(), reinterpret_cast<void *>(value), 0, object);
        return;
    }

    if (!encodeBit.IsReference()) {
        return;
    }

    if (encodeBit.IsReference() && !encodeBit.IsSpecial()) {
        Region *rootRegion = Region::ObjectAddressToRange(ToUintPtr(root));
        uintptr_t taggedObjectAddr = TaggedObjectEncodeBitToAddr(encodeBit);
        Region *valueRegion = Region::ObjectAddressToRange(taggedObjectAddr);
        if (rootRegion->InGeneralOldSpace() && valueRegion->InYoungSpace()) {
            // Should align with '8' in 64 and 32 bit platform
            ASSERT((ToUintPtr(value) % static_cast<uint8_t>(MemAlignment::MEM_ALIGN_OBJECT)) == 0);
            rootRegion->InsertOldToNewRSet((uintptr_t)value);
        }
        if (valueRegion->InSharedSweepableSpace()) {
            if (!rootRegion->InSharedHeap()) {
                rootRegion->InsertLocalToShareRSet((uintptr_t)value);
            }
            // In deserializing can not use barriers, only mark the shared value to prevent markingbit being lost
            if (vm_->GetJSThread()->IsSharedConcurrentMarkingOrFinished()) {
                ASSERT(DaemonThread::GetInstance()->IsConcurrentMarkingOrFinished());
                valueRegion->AtomicMark(reinterpret_cast<void*>(taggedObjectAddr));
            }
        }
        *value = taggedObjectAddr;
        return;
    }

    if (encodeBit.IsSpecial()) {
        encodeBit.ClearObjectSpecialFlag();
        *value = encodeBit.GetValue();
    }
}

void SnapshotProcessor::DeserializeClassWord(TaggedObject *object)
{
    // During AOT deserialization, setting the hclass on an object does not require atomic operations, but a write
    // barrier is still needed to track cross-generation references.
    EncodeBit encodeBit(*reinterpret_cast<uint64_t *>(object));
    if (!builtinsDeserialize_ && encodeBit.IsGlobalConstOrBuiltins()) {
        size_t hclassIndex = encodeBit.GetNativePointerOrObjectIndex();
        auto globalConst = const_cast<GlobalEnvConstants *>(vm_->GetJSThread()->GlobalConstants());
        JSTaggedValue hclassValue = globalConst->GetGlobalConstantObject(hclassIndex);
        ASSERT(hclassValue.IsJSHClass());
        object->SetClassWithoutBarrier(JSHClass::Cast(hclassValue.GetTaggedObject()));
        WriteBarrier<WriteBarrierType::AOT_DESERIALIZE>(vm_->GetJSThread(), object, JSHClass::HCLASS_OFFSET,
                                                        hclassValue.GetRawData());
        return;
    }
    uintptr_t hclassAddr = TaggedObjectEncodeBitToAddr(encodeBit);
    JSHClass *hclass = reinterpret_cast<JSHClass *>(hclassAddr);
    object->SetClassWithoutBarrier(hclass);
    WriteBarrier<WriteBarrierType::AOT_DESERIALIZE>(vm_->GetJSThread(), object, JSHClass::HCLASS_OFFSET,
                                                    JSTaggedValue(hclass).GetRawData());
}

SnapshotProcessor::DeserializeFieldVisitor::DeserializeFieldVisitor(SnapshotProcessor *processor)
    : processor_(processor) {}

void SnapshotProcessor::DeserializeFieldVisitor::VisitObjectRangeImpl(BaseObject *rootObject, uintptr_t startAddr,
                                                                      uintptr_t endAddr, VisitObjectArea area)
{
    ObjectSlot start(startAddr);
    ObjectSlot end(endAddr);
    auto root = TaggedObject::Cast(rootObject);
    for (ObjectSlot slot = start; slot < end; slot++) {
        auto encodeBitAddr = reinterpret_cast<uint64_t *>(slot.SlotAddress());
        if (area == VisitObjectArea::NATIVE_POINTER) {
            processor_->DeserializeNativePointer(encodeBitAddr);
        } else {
            processor_->DeserializeTaggedField(encodeBitAddr, root);
        }
    }
}

void SnapshotProcessor::DeserializeField(TaggedObject *objectHeader)
{
    DeserializeFieldVisitor visitor(this);
    ObjectXRay::VisitObjectBody<VisitType::SNAPSHOT_VISIT>(objectHeader, objectHeader->GetClass(), visitor);
}

EncodeBit SnapshotProcessor::NativePointerToEncodeBit(void *nativePointer)
{
    EncodeBit native(0);
    if (nativePointer != nullptr) {  // nativePointer
        size_t index = Constants::MAX_C_POINTER_INDEX;

        if (programSerialize_) {
            pandaMethod_.emplace_back(ToUintPtr(nativePointer));
            ASSERT(pandaMethod_.size() + GetNativeTableSize() <= Constants::MAX_UINT_16);
            ASSERT(pandaMethod_.size() + GetNativeTableSize() > 0);
            // NOLINTNEXTLINE(bugprone-narrowing-conversions, cppcoreguidelines-narrowing-conversions)
            index = pandaMethod_.size() + GetNativeTableSize() - 1;
        } else {
            index = SearchNativeMethodIndex(nativePointer);
        }

        LOG_ECMA_IF(index > Constants::MAX_C_POINTER_INDEX, FATAL) << "MAX_C_POINTER_INDEX: " << index;
        native.SetNativePointerOrObjectIndex(index);
    }
    return native;
}

size_t SnapshotProcessor::SearchNativeMethodIndex(void *nativePointer)
{
    for (size_t i = 0; i < GetNativeTableSize(); i++) {
        if (nativePointer == reinterpret_cast<void *>(g_nativeTable[i])) {
            return i;
        }
    }

    // not found
    LOG_FULL(FATAL) << "native method did not register in g_table, please register it first";
    UNREACHABLE();
}

uintptr_t SnapshotProcessor::TaggedObjectEncodeBitToAddr(EncodeBit taggedBit)
{
    ASSERT(taggedBit.IsReference());
    if (!builtinsDeserialize_ && taggedBit.IsReferenceToString()) {
        size_t stringIndex = taggedBit.GetNativePointerOrObjectIndex();
        return reinterpret_cast<uintptr_t>(*deserializeStringVector_.at(stringIndex));
    }
    size_t regionIndex = taggedBit.GetRegionIndex();
    if (UNLIKELY(regionIndexMap_.find(regionIndex) == regionIndexMap_.end())) {
        LOG_FULL(FATAL) << "Snapshot deserialize can not find region by index";
    }
    Region *region = regionIndexMap_.find(regionIndex)->second;
    size_t objectOffset = taggedBit.GetObjectOffsetInRegion();

    uintptr_t addr = ToUintPtr(region) + objectOffset;
    if (taggedBit.IsTSWeakObject()) {
        JSTaggedValue object(static_cast<JSTaggedType>(addr));
        object.CreateWeakRef();
        addr = object.GetRawData();
    }
    return addr;
}

void SnapshotProcessor::DeserializeNativePointer(uint64_t *value)
{
    EncodeBit native(*value);
    size_t index = native.GetNativePointerOrObjectIndex();
    uintptr_t addr = 0U;
    size_t nativeTableSize = GetNativeTableSize();
    if (index < nativeTableSize) {
        addr = g_nativeTable[index];
    } else {
        addr = pandaMethod_.at(index - nativeTableSize);
    }
    *value = addr;
}

void SnapshotProcessor::SerializePandaFileMethod()
{
    EncodeBit encodeBit(pandaMethod_.size());
    encodeBit.SetObjectType(Constants::MASK_METHOD_SPACE_BEGIN);

    ObjectFactory *factory = vm_->GetFactory();
    // panda method space begin
    uintptr_t snapshotObj = factory->NewSpaceBySnapshotAllocator(sizeof(uint64_t));
    if (snapshotObj == 0) {
        LOG_ECMA(ERROR) << "SnapshotAllocator OOM";
        return;
    }
    SetObjectEncodeField(snapshotObj, 0, encodeBit.GetValue());  // methods

    // panda methods
    for (auto &it : pandaMethod_) {
        // write method
        size_t methodObjSize = Method::Size();
        uintptr_t methodObj = factory->NewSpaceBySnapshotAllocator(methodObjSize);
        if (methodObj == 0) {
            LOG_ECMA(ERROR) << "SnapshotAllocator OOM";
            return;
        }
        if (memcpy_s(ToVoidPtr(methodObj), methodObjSize, ToVoidPtr(it), Method::Size()) != EOK) {
            LOG_FULL(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
    }
}

uintptr_t SnapshotProcessor::GetNewObj(size_t objectSize, TaggedObject *objectHeader)
{
    if (builtinsSerialize_) {
        return AllocateObjectToLocalSpace(snapshotLocalSpace_, objectSize);
    }
    auto region = Region::ObjectAddressToRange(objectHeader);
    if (region->InYoungOrOldSpace() || region->InSharedOldSpace()) {
        return AllocateObjectToLocalSpace(oldLocalSpace_, objectSize);
    }
    if (region->InMachineCodeSpace()) {
        return AllocateObjectToLocalSpace(machineCodeLocalSpace_, objectSize);
    }
    if (region->InNonMovableSpace() || region->InReadOnlySpace() ||
        region->InSharedNonMovableSpace() || region->InSharedReadOnlySpace()) {
        return AllocateObjectToLocalSpace(nonMovableLocalSpace_, objectSize);
    }
    if (region->InHugeObjectSpace() || region->InSharedHugeObjectSpace()) {
        return AllocateObjectToLocalSpace(hugeObjectLocalSpace_, objectSize);
    }
    return AllocateObjectToLocalSpace(snapshotLocalSpace_, objectSize);
}

EncodeBit SnapshotProcessor::EncodeTaggedObject(TaggedObject *objectHeader, CQueue<TaggedObject *> *queue,
                                                std::unordered_map<uint64_t, ObjectEncode> *data)
{
    if (!builtinsSerialize_) {
        // String duplicate
        if (objectHeader->GetClass()->IsString()) {
            ASSERT(stringVector_.size() < Constants::MAX_OBJECT_INDEX);
            EncodeBit encodeBit(stringVector_.size());
            if (EcmaStringAccessor(objectHeader).IsTreeString()) {
                data->emplace(ToUintPtr(objectHeader), std::make_pair(0U, encodeBit));
                objectHeader = EcmaStringAccessor::FlattenNoGCForSnapshot(vm_, EcmaString::Cast(objectHeader));
            }
            stringVector_.emplace_back(ToUintPtr(objectHeader));
            data->emplace(ToUintPtr(objectHeader), std::make_pair(0U, encodeBit));
            return encodeBit;
        }

        // builtins object reuse
        size_t index = vm_->GetSnapshotEnv()->FindEnvObjectIndex(ToUintPtr(objectHeader));
        if (index != SnapshotEnv::MAX_UINT_32) {
            EncodeBit encodeBit(index);
            encodeBit.SetGlobalConstOrBuiltins();
            data->emplace(ToUintPtr(objectHeader), std::make_pair(0U, encodeBit));
            return encodeBit;
        }
    }
    auto oldObjHeader = objectHeader;
    if (objectHeader->GetClass()->IsString()) {
        if (EcmaStringAccessor(objectHeader).IsTreeString()) {
            objectHeader = EcmaStringAccessor::FlattenNoGCForSnapshot(vm_, EcmaString::Cast(objectHeader));
        }
    }
    queue->emplace(objectHeader);
    size_t objectSize = objectHeader->GetSize();
    if (objectSize == 0) {
        LOG_ECMA_MEM(FATAL) << "It is a zero object. Not Support.";
    }
    uintptr_t newObj = GetNewObj(objectSize, objectHeader);
    if (newObj == 0) {
        LOG_ECMA_MEM(FATAL) << "Snapshot Allocate OOM";
    }
    if (memcpy_s(ToVoidPtr(newObj), objectSize, objectHeader, objectSize) != EOK) {
        LOG_FULL(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }
    auto currentRegion = Region::ObjectAddressToRange(newObj);
    // region snapshotData_ low 32 bits is used to record region index for snapshot
    uint64_t snapshotData = currentRegion->GetSnapshotData();
    size_t regionIndex = SnapshotHelper::GetHugeObjectRegionIndex(snapshotData);
    size_t objOffset = newObj - ToUintPtr(currentRegion);
    EncodeBit encodeBit(static_cast<uint64_t>(regionIndex));
    encodeBit.SetObjectOffsetInRegion(objOffset);
    if (oldObjHeader->GetClass()->IsString()) {
        if (EcmaStringAccessor(oldObjHeader).IsTreeString()) {
            data->emplace(ToUintPtr(oldObjHeader), std::make_pair(0U, encodeBit));
        }
    }
    data->emplace(ToUintPtr(objectHeader), std::make_pair(newObj, encodeBit));
    return encodeBit;
}

EncodeBit SnapshotProcessor::GetObjectEncode(JSTaggedValue object, CQueue<TaggedObject *> *queue,
                                             std::unordered_map<uint64_t, ObjectEncode> *data)
{
    JSTaggedType addr = object.GetRawData();
    EncodeBit encodeBit(0);

    if (data->find(addr) == data->end()) {
        encodeBit = EncodeTaggedObject(object.GetTaggedObject(), queue, data);
    } else {
        ObjectEncode objectEncodePair = data->find(object.GetRawData())->second;
        encodeBit = objectEncodePair.second;
    }
    return encodeBit;
}

void SnapshotProcessor::EncodeTaggedObjectRange(ObjectSlot start, ObjectSlot end, CQueue<TaggedObject *> *queue,
                                                std::unordered_map<uint64_t, ObjectEncode> *data)
{
    while (start < end) {
        JSTaggedValue object(start.GetTaggedType());
        start++;
        if (object.IsHeapObject()) {
            EncodeBit encodeBit(0);
            if (data->find(object.GetRawData()) == data->end()) {
                encodeBit = EncodeTaggedObject(object.GetTaggedObject(), queue, data);
            }
        }
    }
}

size_t SnapshotProcessor::GetNativeTableSize() const
{
    return sizeof(g_nativeTable) / sizeof(g_nativeTable[0]);
}

SnapshotRegionHeadInfo SnapshotProcessor::GenerateRegionHeadInfo(Region *region)
{
    // Record region head information for deserialize
    SnapshotRegionHeadInfo info;
    info.regionIndex_ = region->GetSnapshotData();
    if (region->InSnapshotSpace()) {
        info.aliveObjectSize_ = region->highWaterMark_ - region->packedData_.begin_;
    } else {
        info.aliveObjectSize_ = region->AliveObject();
    }
    return info;
}

void SnapshotProcessor::ResetRegionUnusedRange(Region *region)
{
    // memset unused memory to 0
    if (region->AliveObject() < region->end_ - region->packedData_.begin_) {
        uintptr_t unusedAddrBegin = region->packedData_.begin_ + region->AliveObject();
        size_t unusedSize = region->end_ - region->packedData_.begin_ - region->AliveObject();
        if (memset_s(reinterpret_cast<void *>(unusedAddrBegin), unusedSize, 0, unusedSize)) {
            LOG_FULL(FATAL) << "memset_s failed";
            UNREACHABLE();
        }
    }
}
}  // namespace panda::ecmascript
