/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_CONTAINERS_CONTAINERS_PRIVATE_H
#define ECMASCRIPT_CONTAINERS_CONTAINERS_PRIVATE_H

#include "ecmascript/base/builtins_base.h"

#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_function.h"
namespace panda::ecmascript::containers {
using InitializeFunction = JSHandle<JSTaggedValue> (*)(JSThread *);
enum FuncLength : uint8_t { ZERO = 0, ONE, TWO, THREE, FOUR, FIVE };
enum ContainerTag : uint8_t {
    ArrayList = 0,
    Queue,
    Deque,
    Stack,
    Vector,
    List,
    LinkedList,
    TreeMap,
    TreeSet,
    HashMap,
    HashSet,
    LightWeightMap,
    LightWeightSet,
    PlainArray,
    BitVector,
    FastBuffer,
    END
};

// Using Lazy-loading container, including ArrayList, Queue, Stack, Vector, List, LinkedList, Deque,
// TreeMap, TreeSet, HashMap, HashSet, LightWeightMap, LightWeightSet, PlainArray.
// Use through ArkPrivate.Load([ContainerTag]) in js, ContainTag was declaerd in ArkPrivate like ArkPrivate::ArrayList.
class ContainersPrivate : public base::BuiltinsBase {
public:
    static JSTaggedValue Load(EcmaRuntimeCallInfo *msg);

    static void InitializeContainer(const JSHandle<GlobalEnv> &env);

private:
    static JSHandle<JSTaggedValue> InitializeTreeSet(const JSHandle<GlobalEnv> &env);
    static JSHandle<JSTaggedValue> InitializeArrayList(const JSHandle<GlobalEnv> &env);
    static JSHandle<JSTaggedValue> InitializeHashMap(const JSHandle<GlobalEnv> &env);
    static JSHandle<JSTaggedValue> InitializeHashSet(const JSHandle<GlobalEnv> &env);
    static JSHandle<JSTaggedValue> InitializeList(const JSHandle<GlobalEnv> &env);
    static JSHandle<JSTaggedValue> InitializeLinkedList(const JSHandle<GlobalEnv> &env);

    static JSHandle<JSFunction> NewContainerConstructor(const JSHandle<GlobalEnv> &env,
                                                        const JSHandle<JSObject> &prototype,
                                                        EcmaEntrypoint ctorFunc, const char *name, int length);
    static JSHandle<JSFunction> NewFunction(const JSHandle<GlobalEnv> &env,
                                            const JSHandle<JSTaggedValue> &key, EcmaEntrypoint func,
                                            int length, kungfu::BuiltinsStubCSigns::ID builtinId =
                                            BUILTINS_STUB_ID(INVALID));
    static void SetFrozenFunction(const JSHandle<GlobalEnv> &env,
                                  const JSHandle<JSObject> &obj, const char *key, EcmaEntrypoint func,
                                  int length, kungfu::BuiltinsStubCSigns::ID builtinId =
                                  BUILTINS_STUB_ID(INVALID));
    static void SetFrozenConstructor(JSThread *thread, const JSHandle<JSObject> &obj, const char *keyChar,
                                     JSHandle<JSTaggedValue> &value);
    static JSHandle<JSTaggedValue> CreateGetter(const JSHandle<GlobalEnv> &env,
                                                EcmaEntrypoint func, const char *name, int length);
    static void SetGetter(JSThread *thread, const JSHandle<JSObject> &obj,
                          const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &getter);
    static void SetFunctionAtSymbol(const JSHandle<GlobalEnv> &env,
                                    const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &symbol,
                                    const char *name, EcmaEntrypoint func, int length);
    static void SetStringTagSymbol(const JSHandle<GlobalEnv> &env,
                                   const JSHandle<JSObject> &obj, const char *key);

    static JSTaggedValue InitializeContainer(JSThread *thread, const JSHandle<JSObject> &obj, InitializeFunction func,
                                             const char *name);
    static void InitializeArrayListIterator(const JSHandle<GlobalEnv> &env);
    static void InitializeTreeSetIterator(const JSHandle<GlobalEnv> &env);
    static void InitializeHashMapIterator(const JSHandle<GlobalEnv> &env);
    static void InitializeHashSetIterator(const JSHandle<GlobalEnv> &env);
    static JSHandle<JSTaggedValue> InitializeLightWeightMap(JSThread *thread);
    static void InitializeLightWeightMapIterator(JSThread *thread);
    static JSHandle<JSTaggedValue> InitializeLightWeightSet(JSThread *thread);
    static void InitializeLightWeightSetIterator(JSThread *thread);
    static JSHandle<JSTaggedValue> InitializeTreeMap(JSThread *thread);
    static void InitializeTreeMapIterator(JSThread *thread);
    static JSHandle<JSTaggedValue> InitializePlainArray(JSThread *thread);
    static void InitializePlainArrayIterator(JSThread *thread);
    static JSHandle<JSTaggedValue> InitializeVector(JSThread *thread);
    static void InitializeVectorIterator(JSThread *thread, const JSHandle<GlobalEnv> &env);
    static JSHandle<JSTaggedValue> InitializeBitVector(JSThread *thread);
    static void InitializeBitVectorIterator(JSThread *thread, const JSHandle<GlobalEnv> &env);
    static JSHandle<JSTaggedValue> InitializeQueue(JSThread *thread);
    static void InitializeQueueIterator(JSThread *thread, const JSHandle<GlobalEnv> &env);
    static JSHandle<JSTaggedValue> InitializeDeque(JSThread *thread);
    static void InitializeDequeIterator(JSThread *thread,  const JSHandle<GlobalEnv> &env);
    static JSHandle<JSTaggedValue> InitializeStack(JSThread *thread);
    static void InitializeStackIterator(JSThread *thread);
    static void InitializeLinkedListIterator(const JSHandle<GlobalEnv> &env);
    static void InitializeListIterator(const JSHandle<GlobalEnv> &env);
    static JSHandle<JSTaggedValue> InitializeBuffer(JSThread *thread);
};
}  // namespace panda::ecmascript::containers

#endif  // ECMASCRIPT_CONTAINERS_CONTAINERS_PRIVATE_H
