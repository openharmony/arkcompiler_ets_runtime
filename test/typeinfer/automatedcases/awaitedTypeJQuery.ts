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

// === tests/cases/compiler/awaitedTypeJQuery.ts ===
declare function AssertType(value:any, type:string):void;

/// <reference lib="dom" />

interface Thenable<T> extends PromiseLike<T> { }

// JQuery's Promise type
interface PromiseBase<TR, TJ, TN,
    UR, UJ, UN,
    VR, VJ, VN,
    SR, SJ, SN> {
    then<ARD = never, AJD = never, AND = never,
        BRD = never, BJD = never, BND = never,
        CRD = never, CJD = never, CND = never,
        RRD = never, RJD = never, RND = never,
        ARF = never, AJF = never, ANF = never,
        BRF = never, BJF = never, BNF = never,
        CRF = never, CJF = never, CNF = never,
        RRF = never, RJF = never, RNF = never,
        ARP = never, AJP = never, ANP = never,
        BRP = never, BJP = never, BNP = never,
        CRP = never, CJP = never, CNP = never,
        RRP = never, RJP = never, RNP = never>(
            doneFilter: (t: TR, u: UR, v: VR, ...s: SR[]) => PromiseBase<ARD, AJD, AND,
                BRD, BJD, BND,
                CRD, CJD, CND,
                RRD, RJD, RND> | Thenable<ARD> | ARD,
            failFilter: (t: TJ, u: UJ, v: VJ, ...s: SJ[]) => PromiseBase<ARF, AJF, ANF,
                BRF, BJF, BNF,
                CRF, CJF, CNF,
                RRF, RJF, RNF> | Thenable<ARF> | ARF,
            progressFilter: (t: TN, u: UN, v: VN, ...s: SN[]) => PromiseBase<ARP, AJP, ANP,
                BRP, BJP, BNP,
                CRP, CJP, CNP,
                RRP, RJP, RNP> | Thenable<ANP> | ANP): PromiseBase<ARD | ARF | ARP, AJD | AJF | AJP, AND | ANF | ANP,
        BRD | BRF | BRP, BJD | BJF | BJP, BND | BNF | BNP,
        CRD | CRF | CRP, CJD | CJF | CJP, CND | CNF | CNP,
        RRD | RRF | RRP, RJD | RJF | RJP, RND | RNF | RNP>;
    then<ARF = never, AJF = never, ANF = never,
        BRF = never, BJF = never, BNF = never,
        CRF = never, CJF = never, CNF = never,
        RRF = never, RJF = never, RNF = never,
        ARP = never, AJP = never, ANP = never,
        BRP = never, BJP = never, BNP = never,
        CRP = never, CJP = never, CNP = never,
        RRP = never, RJP = never, RNP = never>(
            doneFilter: null,
            failFilter: (t: TJ, u: UJ, v: VJ, ...s: SJ[]) => PromiseBase<ARF, AJF, ANF,
                BRF, BJF, BNF,
                CRF, CJF, CNF,
                RRF, RJF, RNF> | Thenable<ARF> | ARF,
            progressFilter: (t: TN, u: UN, v: VN, ...s: SN[]) => PromiseBase<ARP, AJP, ANP,
                BRP, BJP, BNP,
                CRP, CJP, CNP,
                RRP, RJP, RNP> | Thenable<ANP> | ANP): PromiseBase<ARF | ARP, AJF | AJP, ANF | ANP,
        BRF | BRP, BJF | BJP, BNF | BNP,
        CRF | CRP, CJF | CJP, CNF | CNP,
        RRF | RRP, RJF | RJP, RNF | RNP>;
    then<ARP = never, AJP = never, ANP = never,
        BRP = never, BJP = never, BNP = never,
        CRP = never, CJP = never, CNP = never,
        RRP = never, RJP = never, RNP = never>(
            doneFilter: null,
            failFilter: null,
            progressFilter?: (t: TN, u: UN, v: VN, ...s: SN[]) => PromiseBase<ARP, AJP, ANP,
                BRP, BJP, BNP,
                CRP, CJP, CNP,
                RRP, RJP, RNP> | Thenable<ANP> | ANP): PromiseBase<ARP, AJP, ANP,
        BRP, BJP, BNP,
        CRP, CJP, CNP,
        RRP, RJP, RNP>;
    then<ARD = never, AJD = never, AND = never,
        BRD = never, BJD = never, BND = never,
        CRD = never, CJD = never, CND = never,
        RRD = never, RJD = never, RND = never,
        ARF = never, AJF = never, ANF = never,
        BRF = never, BJF = never, BNF = never,
        CRF = never, CJF = never, CNF = never,
        RRF = never, RJF = never, RNF = never>(
            doneFilter: (t: TR, u: UR, v: VR, ...s: SR[]) => PromiseBase<ARD, AJD, AND,
                BRD, BJD, BND,
                CRD, CJD, CND,
                RRD, RJD, RND> | Thenable<ARD> | ARD,
            failFilter: (t: TJ, u: UJ, v: VJ, ...s: SJ[]) => PromiseBase<ARF, AJF, ANF,
                BRF, BJF, BNF,
                CRF, CJF, CNF,
                RRF, RJF, RNF> | Thenable<ARF> | ARF,
            progressFilter?: null): PromiseBase<ARD | ARF, AJD | AJF, AND | ANF,
        BRD | BRF, BJD | BJF, BND | BNF,
        CRD | CRF, CJD | CJF, CND | CNF,
        RRD | RRF, RJD | RJF, RND | RNF>;
    then<ARF = never, AJF = never, ANF = never,
        BRF = never, BJF = never, BNF = never,
        CRF = never, CJF = never, CNF = never,
        RRF = never, RJF = never, RNF = never>(
            doneFilter: null,
            failFilter: (t: TJ, u: UJ, v: VJ, ...s: SJ[]) => PromiseBase<ARF, AJF, ANF,
                BRF, BJF, BNF,
                CRF, CJF, CNF,
                RRF, RJF, RNF> | Thenable<ARF> | ARF,
            progressFilter?: null): PromiseBase<ARF, AJF, ANF,
        BRF, BJF, BNF,
        CRF, CJF, CNF,
        RRF, RJF, RNF>;
    then<ARD = never, AJD = never, AND = never,
        BRD = never, BJD = never, BND = never,
        CRD = never, CJD = never, CND = never,
        RRD = never, RJD = never, RND = never>(
            doneFilter: (t: TR, u: UR, v: VR, ...s: SR[]) => PromiseBase<ARD, AJD, AND,
                BRD, BJD, BND,
                CRD, CJD, CND,
                RRD, RJD, RND> | Thenable<ARD> | ARD,
            failFilter?: null,
            progressFilter?: null): PromiseBase<ARD, AJD, AND,
        BRD, BJD, BND,
        CRD, CJD, CND,
        RRD, RJD, RND>;
}

interface Promise3<TR, TJ, TN,
    UR, UJ, UN,
    VR, VJ, VN> extends PromiseBase<TR, TJ, TN,
    UR, UJ, UN,
    VR, VJ, VN,
    never, never, never> { }

type T = Awaited<Promise3<string, Error, number, {}, string, boolean, any, Element, never>>; // string

