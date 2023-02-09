/*
* Copyright (c) Microsoft Corporation. All rights reserved.
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
*
* This file has been modified by Huawei to verify type inference by adding verification statements.
*/

// === tests/cases/compiler/recursiveConditionalCrash3.ts ===
declare function AssertType(value:any, type:string):void;
// #43529

export {}
/**
 *
 * Some helper Types and Interfaces..
 *
 */

export type CanBeExpanded<T extends object = object, D = string> = {
    value: T
    default: D
}

interface Base {

}

interface User extends Base {
    id: string,
    role: CanBeExpanded<Role>,
    note: string,
}

interface Role extends Base {
    id: string,
    user: CanBeExpanded<User>,
    x: string
}

// This interface will be expanded in circular way.
interface X extends Base {
    id: string,
    name: string,
    user: CanBeExpanded<User>,
    role: CanBeExpanded<Role>
    roles: CanBeExpanded<Role[]>
}

type Join<K, P> = K extends string | number ?
    P extends string | number ?
        `${K}${"" extends P ? "" : "."}${P}`
        : never : never;

type PrefixWith<P, S, C = '.'> = P extends '' ? `${string & S}` : `${string & P}${string & C}${string & S}`

type SplitWithAllPossibleCombinations<S extends string, D extends string> =
    string extends S ? string :
        S extends '' ? '' :
            S extends `${infer T}${D}${infer U}` ?
                T | Join<T, SplitWithAllPossibleCombinations<U, D>>
                : S;


/**
 * This function will 
return all possibile keys that can be expanded on T, only to the N deep level
 */
type KeysCanBeExpanded_<T, N extends number, Depth extends number[]> = N extends Depth['length'] ? never :
    T extends CanBeExpanded ?
        KeysCanBeExpanded_<T['value'], N, Depth> :
        T extends Array<infer U> ? KeysCanBeExpanded_<U, N, Depth> :

            T extends object ?
                {
                    [K in keyof T ] :
                    T[K] extends object ?
                        K extends string | number
                            ? `${K}` | Join<`${K}`, KeysCanBeExpanded_<T[K], N, [1, ...Depth]>>
                            : never
                        : never

                }[keyof T]
                :
                never

export type KeysCanBeExpanded<T, N extends number = 4> = KeysCanBeExpanded_<T, N, []>

/**
 * Expand keys on `O` based on `Keys` parameter.
 */
type Expand__<O, Keys, P extends string, N extends number , Depth extends unknown[] > =
    N extends Depth['length'] ?
        O extends CanBeExpanded ?
            O['default'] :
            O :
        O extends CanBeExpanded ?
            Expand__<O[P extends Keys ? 'value' : 'default'], Keys, P, N, Depth> :
            O extends Array<infer U> ?
                Expand__<U, Keys, P, N, Depth>[]
                : O extends object ?
                {
                    [K in keyof O]-?: Expand__<O[K], Keys, PrefixWith<P, K>, N, [1, ...Depth]>
                }
                : O



type SplitAC<K> = SplitWithAllPossibleCombinations<K extends string ? K : '', '.'> extends infer Ko ? Ko : ''
type Expand_<T, K, N extends number = 4> = Expand__<T, SplitAC<K>, '', N, []>
type AllKeys<T, N extends number = 4> = KeysCanBeExpanded<T, N> extends infer R ? R : never



/**
 * If I open the popup, (pointing with the mouse on the Expand), the compiler shows the type Expand, expanded as expected.
 *
 * It's fast and it doesn't use additional memory
 *
 */
export type Expand<T extends object, K extends AllKeys<T, N> = never, N extends number = 4> = Expand_<T, K, N>

/**
 * These two functions work as charm, also they are superfast and as expected they don't use additional Memory
 */
let y1: Expand<X>
AssertType(y1, "{ id: string; name: string; user: string; role: string; roles: string; }");

let y2: Expand<X, 'user.role.user.role'>
AssertType(y2, "{ id: string; name: string; user: { id: string; role: { id: string; user: { id: string; role: string; note: string; }; x: string; }; note: string; }; role: string; roles: string; }");


/**
 *
 * ... nevertheless when I need to use the Expand in other Types, as the following examples, the popup show "loading..." and without show any information and
 * the Memory Heap grows to 1.2gb (in my case) every time... You can see it opening the Chrome DevTools and check the memory Tab.
 *
 * *******
 * I think this is causing "FATAL ERROR: Ineffective mark-compacts near heap limit Allocation failed - JavaScript heap out of memory"
 * on my project during the `yarn start`.
 * *******
 *
 */

type UseQueryOptions<T extends Base, K extends AllKeys<T, 4> > = Expand<T, K>

type UseQueryOptions2<T , K  > = Expand_<T, K>
type UseQueryOptions3<T , K  > = Expand_<T, K> extends infer O ? O : never

type ExpandResult<T,K> = Expand_<T, K> extends infer O ? O : never
type UseQueryOptions4<T , K  > = ExpandResult<T,K>


/**
 * but as you can see here, the expansion of Interface X it's still working.
 *
 * If a memory is still high, it may need some seconds to show popup.
 *
 */
let t: UseQueryOptions<X, 'role.user.role'>
AssertType(t, "{ id: string; name: string; user: string; role: { id: string; user: { id: string; role: { id: string; user: string; x: string; }; note: string; }; x: string; }; roles: string; }");


