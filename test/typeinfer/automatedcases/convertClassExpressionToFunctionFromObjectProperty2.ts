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

// === tests/cases/compiler/convertClassExpressionToFunctionFromObjectProperty2.ts ===
declare function AssertType(value:any, type:string):void;
({
AssertType({    x: class {        constructor() { }    },    y: class {        constructor() { }    },    // keywords    break: class {        constructor() { }    },    case: class {        constructor() { }    },    catch: class {        constructor() { }    },    class: class {        constructor() { }    },    const: class {        constructor() { }    },    continue: class {        constructor() { }    },    debugger: class {        constructor() { }    },    default: class {        constructor() { }    },    delete: class {        constructor() { }    },    do: class {        constructor() { }    },    else: class {        constructor() { }    },    enum: class {        constructor() { }    },    export: class {        constructor() { }    },    extends: class {        constructor() { }    },    false: class {        constructor() { }    },    finally: class {        constructor() { }    },    for: class {        constructor() { }    },    function: class {        constructor() { }    },    if: class {        constructor() { }    },    import: class {        constructor() { }    },    in: class {        constructor() { }    },    instanceof: class {        constructor() { }    },    new: class {        constructor() { }    },    null: class {        constructor() { }    },    return: class {        constructor() { }    },    super: class {        constructor() { }    },    switch: class {        constructor() { }    },    this: class {        constructor() { }    },    throw: class {        constructor() { }    },    true: class {        constructor() { }    },    try: class {        constructor() { }    },    typeof: class {        constructor() { }    },    let: class {        constructor() { }    },    void: class {        constructor() { }    },    while: class {        constructor() { }    },    with: class {        constructor() { }    },    implements: class {        constructor() { }    },    interface: class {        constructor() { }    },    let: class {        constructor() { }    },    package: class {        constructor() { }    },    private: class {        constructor() { }    },    protected: class {        constructor() { }    },    public: class {        constructor() { }    },    static: class {        constructor() { }    },    yield: class {        constructor() { }    },    abstract: class {        constructor() { }    },    as: class {        constructor() { }    },    asserts: class {        constructor() { }    },    any: class {        constructor() { }    },    async: class {        constructor() { }    },    await: class {        constructor() { }    },    boolean: class {        constructor() { }    },    constructor: class {        constructor() { }    },    declare: class {        constructor() { }    },    get: class {        constructor() { }    },    infer: class {        constructor() { }    },    is: class {        constructor() { }    },    keyof: class {        constructor() { }    },    module: class {        constructor() { }    },    namespace: class {        constructor() { }    },    never: class {        constructor() { }    },    readonly: class {        constructor() { }    },    require: class {        constructor() { }    },    number: class {        constructor() { }    },    object: class {        constructor() { }    },    set: class {        constructor() { }    },    string: class {        constructor() { }    },    symbol: class {        constructor() { }    },    type: class {        constructor() { }    },    undefined: class {        constructor() { }    },    unique: class {        constructor() { }    },    unknown: class {        constructor() { }    },    from: class {        constructor() { }    },    global: class {        constructor() { }    },    bigint: class {        constructor() { }    },    of: class {        constructor() { }    }}, "{ x: typeof x; y: typeof y; break: typeof break; case: typeof case; catch: typeof catch; class: typeof class; const: typeof const; continue: typeof continue; debugger: typeof debugger; default: typeof default; delete: typeof delete; do: typeof do; else: typeof else; enum: typeof enum; export: typeof export; extends: typeof extends; false: typeof false; finally: typeof finally; for: typeof for; function: typeof function; if: typeof if; import: typeof import; in: typeof in; instanceof: typeof instanceof; new: typeof new; null: typeof null; return: typeof return; super: typeof super; switch: typeof switch; this: typeof this; throw: typeof throw; true: typeof true; try: typeof try; typeof: typeof typeof; let: typeof let; void: typeof void; while: typeof while; with: typeof with; implements: typeof implements; interface: typeof interface; let: typeof let; package: typeof package; private: typeof private; protected: typeof protected; public: typeof public; static: typeof static; yield: typeof yield; abstract: typeof abstract; as: typeof as; asserts: typeof asserts; any: typeof any; async: typeof async; await: typeof await; boolean: typeof boolean; constructor: typeof constructor; declare: typeof declare; get: typeof get; infer: typeof infer; is: typeof is; keyof: typeof keyof; module: typeof module; namespace: typeof namespace; never: typeof never; readonly: typeof readonly; require: typeof require; number: typeof number; object: typeof object; set: typeof set; string: typeof string; symbol: typeof symbol; type: typeof type; undefined: typeof undefined; unique: typeof unique; unknown: typeof unknown; from: typeof from; global: typeof global; bigint: typeof bigint; of: typeof of; }");
AssertType(({    x: class {        constructor() { }    },    y: class {        constructor() { }    },    // keywords    break: class {        constructor() { }    },    case: class {        constructor() { }    },    catch: class {        constructor() { }    },    class: class {        constructor() { }    },    const: class {        constructor() { }    },    continue: class {        constructor() { }    },    debugger: class {        constructor() { }    },    default: class {        constructor() { }    },    delete: class {        constructor() { }    },    do: class {        constructor() { }    },    else: class {        constructor() { }    },    enum: class {        constructor() { }    },    export: class {        constructor() { }    },    extends: class {        constructor() { }    },    false: class {        constructor() { }    },    finally: class {        constructor() { }    },    for: class {        constructor() { }    },    function: class {        constructor() { }    },    if: class {        constructor() { }    },    import: class {        constructor() { }    },    in: class {        constructor() { }    },    instanceof: class {        constructor() { }    },    new: class {        constructor() { }    },    null: class {        constructor() { }    },    return: class {        constructor() { }    },    super: class {        constructor() { }    },    switch: class {        constructor() { }    },    this: class {        constructor() { }    },    throw: class {        constructor() { }    },    true: class {        constructor() { }    },    try: class {        constructor() { }    },    typeof: class {        constructor() { }    },    let: class {        constructor() { }    },    void: class {        constructor() { }    },    while: class {        constructor() { }    },    with: class {        constructor() { }    },    implements: class {        constructor() { }    },    interface: class {        constructor() { }    },    let: class {        constructor() { }    },    package: class {        constructor() { }    },    private: class {        constructor() { }    },    protected: class {        constructor() { }    },    public: class {        constructor() { }    },    static: class {        constructor() { }    },    yield: class {        constructor() { }    },    abstract: class {        constructor() { }    },    as: class {        constructor() { }    },    asserts: class {        constructor() { }    },    any: class {        constructor() { }    },    async: class {        constructor() { }    },    await: class {        constructor() { }    },    boolean: class {        constructor() { }    },    constructor: class {        constructor() { }    },    declare: class {        constructor() { }    },    get: class {        constructor() { }    },    infer: class {        constructor() { }    },    is: class {        constructor() { }    },    keyof: class {        constructor() { }    },    module: class {        constructor() { }    },    namespace: class {        constructor() { }    },    never: class {        constructor() { }    },    readonly: class {        constructor() { }    },    require: class {        constructor() { }    },    number: class {        constructor() { }    },    object: class {        constructor() { }    },    set: class {        constructor() { }    },    string: class {        constructor() { }    },    symbol: class {        constructor() { }    },    type: class {        constructor() { }    },    undefined: class {        constructor() { }    },    unique: class {        constructor() { }    },    unknown: class {        constructor() { }    },    from: class {        constructor() { }    },    global: class {        constructor() { }    },    bigint: class {        constructor() { }    },    of: class {        constructor() { }    }}), "{ x: typeof x; y: typeof y; break: typeof break; case: typeof case; catch: typeof catch; class: typeof class; const: typeof const; continue: typeof continue; debugger: typeof debugger; default: typeof default; delete: typeof delete; do: typeof do; else: typeof else; enum: typeof enum; export: typeof export; extends: typeof extends; false: typeof false; finally: typeof finally; for: typeof for; function: typeof function; if: typeof if; import: typeof import; in: typeof in; instanceof: typeof instanceof; new: typeof new; null: typeof null; return: typeof return; super: typeof super; switch: typeof switch; this: typeof this; throw: typeof throw; true: typeof true; try: typeof try; typeof: typeof typeof; let: typeof let; void: typeof void; while: typeof while; with: typeof with; implements: typeof implements; interface: typeof interface; let: typeof let; package: typeof package; private: typeof private; protected: typeof protected; public: typeof public; static: typeof static; yield: typeof yield; abstract: typeof abstract; as: typeof as; asserts: typeof asserts; any: typeof any; async: typeof async; await: typeof await; boolean: typeof boolean; constructor: typeof constructor; declare: typeof declare; get: typeof get; infer: typeof infer; is: typeof is; keyof: typeof keyof; module: typeof module; namespace: typeof namespace; never: typeof never; readonly: typeof readonly; require: typeof require; number: typeof number; object: typeof object; set: typeof set; string: typeof string; symbol: typeof symbol; type: typeof type; undefined: typeof undefined; unique: typeof unique; unknown: typeof unknown; from: typeof from; global: typeof global; bigint: typeof bigint; of: typeof of; }");

    x: class {
AssertType(x, "typeof x");
AssertType(class {        constructor() { }    }, "typeof x");

        constructor() { }
    },
    y: class {
AssertType(y, "typeof y");
AssertType(class {        constructor() { }    }, "typeof y");

        constructor() { }
    },

    // keywords
    break: class {
AssertType(break, "typeof break");
AssertType(class {        constructor() { }    }, "typeof break");

        constructor() { }
    },
    case: class {
AssertType(case, "typeof case");
AssertType(class {        constructor() { }    }, "typeof case");

        constructor() { }
    },
    catch: class {
AssertType(catch, "typeof catch");
AssertType(class {        constructor() { }    }, "typeof catch");

        constructor() { }
    },
    class: class {
AssertType(class, "typeof class");
AssertType(class {        constructor() { }    }, "typeof class");

        constructor() { }
    },
    const: class {
AssertType(const, "typeof const");
AssertType(class {        constructor() { }    }, "typeof const");

        constructor() { }
    },
    continue: class {
AssertType(continue, "typeof continue");
AssertType(class {        constructor() { }    }, "typeof continue");

        constructor() { }
    },
    debugger: class {
AssertType(debugger, "typeof debugger");
AssertType(class {        constructor() { }    }, "typeof debugger");

        constructor() { }
    },
    default: class {
AssertType(default, "typeof default");
AssertType(class {        constructor() { }    }, "typeof default");

        constructor() { }
    },
    delete: class {
AssertType(delete, "typeof delete");
AssertType(class {        constructor() { }    }, "typeof delete");

        constructor() { }
    },
    do: class {
AssertType(do, "typeof do");
AssertType(class {        constructor() { }    }, "typeof do");

        constructor() { }
    },
    else: class {
AssertType(else, "typeof else");
AssertType(class {        constructor() { }    }, "typeof else");

        constructor() { }
    },
    enum: class {
AssertType(enum, "typeof enum");
AssertType(class {        constructor() { }    }, "typeof enum");

        constructor() { }
    },
    export: class {
AssertType(export, "typeof export");
AssertType(class {        constructor() { }    }, "typeof export");

        constructor() { }
    },
    extends: class {
AssertType(extends, "typeof extends");
AssertType(class {        constructor() { }    }, "typeof extends");

        constructor() { }
    },
    false: class {
AssertType(false, "typeof false");
AssertType(class {        constructor() { }    }, "typeof false");

        constructor() { }
    },
    finally: class {
AssertType(finally, "typeof finally");
AssertType(class {        constructor() { }    }, "typeof finally");

        constructor() { }
    },
    for: class {
AssertType(for, "typeof for");
AssertType(class {        constructor() { }    }, "typeof for");

        constructor() { }
    },
    function: class {
AssertType(function, "typeof function");
AssertType(class {        constructor() { }    }, "typeof function");

        constructor() { }
    },
    if: class {
AssertType(if, "typeof if");
AssertType(class {        constructor() { }    }, "typeof if");

        constructor() { }
    },
    import: class {
AssertType(import, "typeof import");
AssertType(class {        constructor() { }    }, "typeof import");

        constructor() { }
    },
    in: class {
AssertType(in, "typeof in");
AssertType(class {        constructor() { }    }, "typeof in");

        constructor() { }
    },
    instanceof: class {
AssertType(instanceof, "typeof instanceof");
AssertType(class {        constructor() { }    }, "typeof instanceof");

        constructor() { }
    },
    new: class {
AssertType(class {        constructor() { }    }, "typeof new");

        constructor() { }
    },
    null: class {
AssertType(null, "typeof null");
AssertType(class {        constructor() { }    }, "typeof null");

        constructor() { }
    },
AssertType(return, "typeof return");
AssertType(class {        constructor() { }    }, "typeof return");
    return: class {

        constructor() { }
    },
    super: class {
AssertType(super, "typeof super");
AssertType(class {        constructor() { }    }, "typeof super");

        constructor() { }
    },
    switch: class {
AssertType(switch, "typeof switch");
AssertType(class {        constructor() { }    }, "typeof switch");

        constructor() { }
    },
    this: class {
AssertType(this, "typeof this");
AssertType(class {        constructor() { }    }, "typeof this");

        constructor() { }
    },
    throw: class {
AssertType(throw, "typeof throw");
AssertType(class {        constructor() { }    }, "typeof throw");

        constructor() { }
    },
    true: class {
AssertType(true, "typeof true");
AssertType(class {        constructor() { }    }, "typeof true");

        constructor() { }
    },
    try: class {
AssertType(try, "typeof try");
AssertType(class {        constructor() { }    }, "typeof try");

        constructor() { }
    },
    typeof: class {
AssertType(typeof, "typeof typeof");
AssertType(class {        constructor() { }    }, "typeof typeof");

        constructor() { }
    },
    let: class {
AssertType(let, "typeof let");
AssertType(class {        constructor() { }    }, "typeof let");

        constructor() { }
    },
    void: class {
AssertType(void, "typeof void");
AssertType(class {        constructor() { }    }, "typeof void");

        constructor() { }
    },
    while: class {
AssertType(while, "typeof while");
AssertType(class {        constructor() { }    }, "typeof while");

        constructor() { }
    },
    with: class {
AssertType(with, "typeof with");
AssertType(class {        constructor() { }    }, "typeof with");

        constructor() { }
    },
    implements: class {
AssertType(implements, "typeof implements");
AssertType(class {        constructor() { }    }, "typeof implements");

        constructor() { }
    },
    interface: class {
AssertType(interface, "typeof interface");
AssertType(class {        constructor() { }    }, "typeof interface");

        constructor() { }
    },
    let: class {
AssertType(let, "typeof let");
AssertType(class {        constructor() { }    }, "typeof let");

        constructor() { }
    },
    package: class {
AssertType(package, "typeof package");
AssertType(class {        constructor() { }    }, "typeof package");

        constructor() { }
    },
    private: class {
AssertType(private, "typeof private");
AssertType(class {        constructor() { }    }, "typeof private");

        constructor() { }
    },
    protected: class {
AssertType(protected, "typeof protected");
AssertType(class {        constructor() { }    }, "typeof protected");

        constructor() { }
    },
    public: class {
AssertType(public, "typeof public");
AssertType(class {        constructor() { }    }, "typeof public");

        constructor() { }
    },
    static: class {
AssertType(static, "typeof static");
AssertType(class {        constructor() { }    }, "typeof static");

        constructor() { }
    },
    yield: class {
AssertType(yield, "typeof yield");
AssertType(class {        constructor() { }    }, "typeof yield");

        constructor() { }
    },
    abstract: class {
AssertType(abstract, "typeof abstract");
AssertType(class {        constructor() { }    }, "typeof abstract");

        constructor() { }
    },
    as: class {
AssertType(as, "typeof as");
AssertType(class {        constructor() { }    }, "typeof as");

        constructor() { }
    },
    asserts: class {
AssertType(asserts, "typeof asserts");
AssertType(class {        constructor() { }    }, "typeof asserts");

        constructor() { }
    },
    any: class {
AssertType(any, "typeof any");
AssertType(class {        constructor() { }    }, "typeof any");

        constructor() { }
    },
    async: class {
AssertType(async, "typeof async");
AssertType(class {        constructor() { }    }, "typeof async");

        constructor() { }
    },
    await: class {
AssertType(await, "typeof await");
AssertType(class {        constructor() { }    }, "typeof await");

        constructor() { }
    },
    boolean: class {
AssertType(boolean, "typeof boolean");
AssertType(class {        constructor() { }    }, "typeof boolean");

        constructor() { }
    },
    constructor: class {
AssertType(constructor, "typeof constructor");
AssertType(class {        constructor() { }    }, "typeof constructor");

        constructor() { }
    },
    declare: class {
AssertType(declare, "typeof declare");
AssertType(class {        constructor() { }    }, "typeof declare");

        constructor() { }
    },
    get: class {
AssertType(get, "typeof get");
AssertType(class {        constructor() { }    }, "typeof get");

        constructor() { }
    },
    infer: class {
AssertType(infer, "typeof infer");
AssertType(class {        constructor() { }    }, "typeof infer");

        constructor() { }
    },
    is: class {
AssertType(is, "typeof is");
AssertType(class {        constructor() { }    }, "typeof is");

        constructor() { }
    },
    keyof: class {
AssertType(keyof, "typeof keyof");
AssertType(class {        constructor() { }    }, "typeof keyof");

        constructor() { }
    },
    module: class {
AssertType(module, "typeof module");
AssertType(class {        constructor() { }    }, "typeof module");

        constructor() { }
    },
    namespace: class {
AssertType(namespace, "typeof namespace");
AssertType(class {        constructor() { }    }, "typeof namespace");

        constructor() { }
    },
    never: class {
AssertType(never, "typeof never");
AssertType(class {        constructor() { }    }, "typeof never");

        constructor() { }
    },
    readonly: class {
AssertType(readonly, "typeof readonly");
AssertType(class {        constructor() { }    }, "typeof readonly");

        constructor() { }
    },
    require: class {
AssertType(require, "typeof require");
AssertType(class {        constructor() { }    }, "typeof require");

        constructor() { }
    },
    number: class {
AssertType(number, "typeof number");
AssertType(class {        constructor() { }    }, "typeof number");

        constructor() { }
    },
    object: class {
AssertType(object, "typeof object");
AssertType(class {        constructor() { }    }, "typeof object");

        constructor() { }
    },
    set: class {
AssertType(set, "typeof set");
AssertType(class {        constructor() { }    }, "typeof set");

        constructor() { }
    },
    string: class {
AssertType(string, "typeof string");
AssertType(class {        constructor() { }    }, "typeof string");

        constructor() { }
    },
    symbol: class {
AssertType(symbol, "typeof symbol");
AssertType(class {        constructor() { }    }, "typeof symbol");

        constructor() { }
    },
    type: class {
AssertType(type, "typeof type");
AssertType(class {        constructor() { }    }, "typeof type");

        constructor() { }
    },
    undefined: class {
AssertType(undefined, "typeof undefined");
AssertType(class {        constructor() { }    }, "typeof undefined");

        constructor() { }
    },
    unique: class {
AssertType(unique, "typeof unique");
AssertType(class {        constructor() { }    }, "typeof unique");

        constructor() { }
    },
    unknown: class {
AssertType(unknown, "typeof unknown");
AssertType(class {        constructor() { }    }, "typeof unknown");

        constructor() { }
    },
    from: class {
AssertType(from, "typeof from");
AssertType(class {        constructor() { }    }, "typeof from");

        constructor() { }
    },
    global: class {
AssertType(global, "typeof global");
AssertType(class {        constructor() { }    }, "typeof global");

        constructor() { }
    },
    bigint: class {
AssertType(bigint, "typeof bigint");
AssertType(class {        constructor() { }    }, "typeof bigint");

        constructor() { }
    },
    of: class {
AssertType(of, "typeof of");
AssertType(class {        constructor() { }    }, "typeof of");

        constructor() { }
    }
})


