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

// === tests/cases/conformance/types/objectTypeLiteral/propertySignatures/propertyNamesOfReservedWords.ts ===
declare function AssertType(value:any, type:string):void;
class C {
    abstract;
    as;
    boolean;
    break;
    byte;
    case;
    catch;
    char;
    class;
    continue;
    const;
    debugger;
    default;
    delete;
    do;
    double;
    else;
    enum;
    export;
    extends;
    false;
    final;
    finally;
    float;
    for;
    function;
    goto;
    if;
    implements;
    import;
    in;
    instanceof;
    int;
    interface;
    is;
    long;
    namespace;
    native;
    new;
    null;
    package;
    private;
    protected;
    public;
    return;
    short;
    static;
    super;
    switch;
    synchronized;
    this;
    throw;
    throws;
    transient;
    true;
    try;
    typeof;
    use;
    let;
    void;
    volatile;
    while;
    with;
}
let c: C;
AssertType(c, "C");

let r1 = c.abstract;
AssertType(r1, "any");
AssertType(c.abstract, "any");

let r2 = c.as;
AssertType(r2, "any");
AssertType(c.as, "any");

interface I {
    abstract;
    as;
    boolean;
    break;
    byte;
    case;
    catch;
    char;
    class;
    continue;
    const;
    debugger;
    default;
    delete;
    do;
    double;
    else;
    enum;
    export;
    extends;
    false;
    final;
    finally;
    float;
    for;
    function;
    goto;
    if;
    implements;
    import;
    in;
    instanceof;
    int;
    interface;
    is;
    long;
    namespace;
    native;
    new;
    null;
    package;
    private;
    protected;
    public;
    return;
    short;
    static;
    super;
    switch;
    synchronized;
    this;
    throw;
    throws;
    transient;
    true;
    try;
    typeof;
    use;
    let;
    void;
    volatile;
    while;
    with;
}

let i: I;
AssertType(i, "I");

let r3 = i.abstract;
AssertType(r3, "any");
AssertType(i.abstract, "any");

let r4 = i.as;
AssertType(r4, "any");
AssertType(i.as, "any");

let a: {
AssertType(a, "{ abstract: any; as: any; boolean: any; break: any; byte: any; case: any; catch: any; char: any; class: any; continue: any; const: any; debugger: any; default: any; delete: any; do: any; double: any; else: any; enum: any; export: any; extends: any; false: any; final: any; finally: any; float: any; for: any; function: any; goto: any; if: any; implements: any; import: any; in: any; instanceof: any; int: any; interface: any; is: any; long: any; namespace: any; native: any; new: any; null: any; package: any; private: any; protected: any; public: any; return: any; short: any; static: any; super: any; switch: any; synchronized: any; this: any; throw: any; throws: any; transient: any; true: any; try: any; typeof: any; use: any; let: any; void: any; volatile: any; while: any; with: any; }");

    abstract;
AssertType(abstract, "any");

    as;
AssertType(as, "any");

    boolean;
AssertType(boolean, "any");

    break;
AssertType(break, "any");

    byte;
AssertType(byte, "any");

    case;
AssertType(case, "any");

    catch;
AssertType(catch, "any");

    char;
AssertType(char, "any");

    class;
AssertType(class, "any");

    continue;
AssertType(continue, "any");

    const;
AssertType(const, "any");

    debugger;
AssertType(debugger, "any");

    default;
AssertType(default, "any");

    delete;
AssertType(delete, "any");

    do;
AssertType(do, "any");

    double;
AssertType(double, "any");

    else;
AssertType(else, "any");

    enum;
AssertType(enum, "any");

    export;
AssertType(export, "any");

    extends;
AssertType(extends, "any");

    false;
AssertType(false, "any");

    final;
AssertType(final, "any");

    finally;
AssertType(finally, "any");

    float;
AssertType(float, "any");

    for;
AssertType(for, "any");

    function;
AssertType(function, "any");

    goto;
AssertType(goto, "any");

    if;
AssertType(if, "any");

    implements;
AssertType(implements, "any");

    import;
AssertType(import, "any");

    in;
AssertType(in, "any");

    instanceof;
AssertType(instanceof, "any");

    int;
AssertType(int, "any");

    interface;
AssertType(interface, "any");

    is;
AssertType(is, "any");

    long;
AssertType(long, "any");

    namespace;
AssertType(namespace, "any");

    native;
AssertType(native, "any");

    new;
    null;
AssertType(null, "any");

    package;
AssertType(package, "any");

    private;
AssertType(private, "any");

    protected;
AssertType(protected, "any");

    public;
AssertType(public, "any");

AssertType(return, "any");
    return;

    short;
AssertType(short, "any");

    static;
AssertType(static, "any");

    super;
AssertType(super, "any");

    switch;
AssertType(switch, "any");

    synchronized;
AssertType(synchronized, "any");

    this;
AssertType(this, "any");

    throw;
AssertType(throw, "any");

    throws;
AssertType(throws, "any");

    transient;
AssertType(transient, "any");

    true;
AssertType(true, "any");

    try;
AssertType(try, "any");

    typeof;
AssertType(typeof, "any");

    use;
AssertType(use, "any");

    let;
AssertType(let, "any");

    void;
AssertType(void, "any");

    volatile;
AssertType(volatile, "any");

    while;
AssertType(while, "any");

    with;
AssertType(with, "any");
}

let r5 = a.abstract;
AssertType(r5, "any");
AssertType(a.abstract, "any");

let r6 = a.as;
AssertType(r6, "any");
AssertType(a.as, "any");

enum E {
    abstract,
    as,
    boolean,
    break,
    byte,
    case,
    catch,
    char,
    class,
    continue,
    const,
    debugger,
    default,
    delete,
    do,
    double,
    else,
    enum,
    export,
    extends,
    false,
    final,
    finally,
    float,
    for,
    function,
    goto,
    if,
    implements,
    import,
    in,
    instanceof,
    int,
    interface,
    is,
    long,
    namespace,
    native,
    new,
    null,
    package,
    private,
    protected,
    public,
    return,
    short,
    static,
    super,
    switch,
    synchronized,
    this,
    throw,
    throws,
    transient,
    true,
    try,
    typeof,
    use,
    let,
    void,
    volatile,
    while,
    with,
}

let r7 = E.abstract;
AssertType(r7, "E");
AssertType(E.abstract, "E.abstract");

let r8 = E.as;
AssertType(r8, "E");
AssertType(E.as, "E.as");


