/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

declare function print(arg: any): string;
declare const ArkTools: {
    jitCompileAsync(args: any): boolean;
    waitJitCompileFinish(args: any): boolean;
};

// ==================== Optional Chaining ====================

// Basic optional chaining
function optionalChainBasic(): string {
    const obj: any = { a: { b: { c: 42 } } };
    const val1 = obj?.a?.b?.c;
    const val2 = obj?.x?.y?.z;
    return `${val1},${val2}`;
}

// Optional chaining with null
function optionalChainNull(): string {
    const obj: any = { a: null };
    const val = obj?.a?.b?.c;
    return `${val}`;
}

// Optional method call
function optionalChainMethod(): string {
    const obj: any = {
        greet: () => "Hello",
        nested: { fn: (x: number) => x * 2 }
    };
    const r1 = obj?.greet?.();
    const r2 = obj?.missing?.();
    const r3 = obj?.nested?.fn?.(5);
    return `${r1},${r2},${r3}`;
}

// Optional array access
function optionalChainArray(): string {
    const arr: any = [[1, 2], [3, 4], null];
    const v1 = arr?.[0]?.[1];
    const v2 = arr?.[2]?.[0];
    const v3 = arr?.[10]?.[0];
    return `${v1},${v2},${v3}`;
}

// Optional chaining in assignment
function optionalChainAssign(): string {
    const obj: any = { data: { value: 10 } };
    const result = obj?.data?.value ?? 0;
    const missing = obj?.other?.value ?? 100;
    return `${result},${missing}`;
}

// ==================== Nullish Coalescing ====================

// Basic nullish coalescing
function nullishBasic(): string {
    const a = null ?? "default";
    const b = undefined ?? "default";
    const c = 0 ?? "default";
    const d = "" ?? "default";
    const e = false ?? "default";
    return `${a},${b},${c},${d},${e}`;
}

// Nullish coalescing with optional chaining
function nullishWithOptional(): string {
    const obj: any = { value: null };
    const r1 = obj?.value ?? "fallback";
    const r2 = obj?.missing ?? "fallback";
    return `${r1},${r2}`;
}

// Nullish assignment
function nullishAssignment(): string {
    let a: any = null;
    let b: any = undefined;
    let c: any = 0;
    a ??= "assigned";
    b ??= "assigned";
    c ??= "assigned";
    return `${a},${b},${c}`;
}

// Nullish vs OR
function nullishVsOr(): string {
    const val1 = 0 || "or-default";
    const val2 = 0 ?? "nullish-default";
    const val3 = "" || "or-default";
    const val4 = "" ?? "nullish-default";
    return `${val1},${val2},${val3},${val4}`;
}

// ==================== Template Literals ====================

// Basic template literal
function templateBasic(): string {
    const name = "World";
    const greeting = `Hello, ${name}!`;
    return greeting;
}

// Multi-line template
function templateMultiLine(): string {
    const text = `Line 1
Line 2
Line 3`;
    return `${text.split('\n').length}`;
}

// Expression in template
function templateExpression(): string {
    const a = 10;
    const b = 20;
    return `Sum: ${a + b}, Product: ${a * b}`;
}

// Nested template
function templateNested(): string {
    const items = [1, 2, 3];
    return `Items: ${items.map(i => `[${i}]`).join(', ')}`;
}

// Tagged template
function tag(strings: TemplateStringsArray, ...values: any[]): string {
    return strings.reduce((acc, str, i) => {
        return acc + str + (values[i] !== undefined ? `<${values[i]}>` : '');
    }, '');
}

function templateTagged(): string {
    const a = 1;
    const b = 2;
    return tag`Value a is ${a} and b is ${b}`;
}

// Template with function call
function templateWithCall(): string {
    const upper = (s: string) => s.toUpperCase();
    const name = "test";
    return `Result: ${upper(name)}`;
}

// ==================== Computed Property ====================

// Basic computed property
function computedBasic(): string {
    const key = "dynamicKey";
    const obj = { [key]: "value", ["key" + 2]: "value2" };
    return `${obj.dynamicKey},${obj.key2}`;
}

// Computed with expression
function computedExpression(): string {
    const prefix = "prop";
    const obj = {
        [prefix + "_a"]: 1,
        [prefix + "_b"]: 2,
        [`${prefix}_c`]: 3
    };
    return `${obj.prop_a},${obj.prop_b},${obj.prop_c}`;
}

// Computed method names
function computedMethods(): string {
    const methodName = "calculate";
    const obj = {
        [methodName](a: number, b: number): number {
            return a + b;
        },
        ["get" + "Value"](): number {
            return 42;
        }
    };
    return `${obj.calculate(1, 2)},${obj.getValue()}`;
}

// Computed with Symbol
function computedSymbol(): string {
    const sym = Symbol("mySymbol");
    const obj = { [sym]: "symbolValue", regular: "normalValue" };
    return `${obj[sym]},${obj.regular}`;
}

// ==================== Symbol ====================

// Symbol creation
function symbolCreation(): string {
    const s1 = Symbol("desc1");
    const s2 = Symbol("desc1");
    const s3 = Symbol.for("shared");
    const s4 = Symbol.for("shared");
    return `${s1 !== s2},${s3 === s4},${Symbol.keyFor(s3)}`;
}

// Symbol as property key
function symbolProperty(): string {
    const sym = Symbol("private");
    const obj: any = { [sym]: "hidden", public: "visible" };
    return `${obj[sym]},${Object.keys(obj).length}`;
}

// Well-known symbols
function symbolWellKnown(): string {
    const obj = {
        [Symbol.toStringTag]: "CustomObject"
    };
    return Object.prototype.toString.call(obj);
}

// Symbol.iterator
function symbolIterator(): string {
    const range = {
        start: 1,
        end: 5,
        [Symbol.iterator]() {
            let current = this.start;
            const end = this.end;
            return {
                next() {
                    if (current <= end) {
                        return { value: current++, done: false };
                    }
                    return { value: undefined, done: true };
                }
            };
        }
    };
    return [...range].join(',');
}

// Symbol description
function symbolDescription(): string {
    const s1 = Symbol("test");
    const s2 = Symbol();
    return `${s1.description},${s2.description}`;
}

// ==================== Short-circuit Evaluation ====================

// Logical AND assignment
function logicalAndAssign(): string {
    let a: any = 1;
    let b: any = 0;
    a &&= 10;
    b &&= 10;
    return `${a},${b}`;
}

// Logical OR assignment
function logicalOrAssign(): string {
    let a: any = 0;
    let b: any = 1;
    a ||= 10;
    b ||= 10;
    return `${a},${b}`;
}

// Combined patterns
function combinedPatterns(): string {
    const obj: any = { a: { b: null } };
    const result = obj?.a?.b ?? obj?.a?.c ?? "default";
    return result;
}

// Warmup
for (let i = 0; i < 20; i++) {
    optionalChainBasic();
    optionalChainNull();
    optionalChainMethod();
    optionalChainArray();
    optionalChainAssign();
    nullishBasic();
    nullishWithOptional();
    nullishAssignment();
    nullishVsOr();
    templateBasic();
    templateMultiLine();
    templateExpression();
    templateNested();
    templateTagged();
    templateWithCall();
    computedBasic();
    computedExpression();
    computedMethods();
    computedSymbol();
    symbolCreation();
    symbolProperty();
    symbolWellKnown();
    symbolIterator();
    symbolDescription();
    logicalAndAssign();
    logicalOrAssign();
    combinedPatterns();
}

// JIT compile
ArkTools.jitCompileAsync(optionalChainBasic);
ArkTools.jitCompileAsync(optionalChainNull);
ArkTools.jitCompileAsync(optionalChainMethod);
ArkTools.jitCompileAsync(optionalChainArray);
ArkTools.jitCompileAsync(optionalChainAssign);
ArkTools.jitCompileAsync(nullishBasic);
ArkTools.jitCompileAsync(nullishWithOptional);
ArkTools.jitCompileAsync(nullishAssignment);
ArkTools.jitCompileAsync(nullishVsOr);
ArkTools.jitCompileAsync(templateBasic);
ArkTools.jitCompileAsync(templateMultiLine);
ArkTools.jitCompileAsync(templateExpression);
ArkTools.jitCompileAsync(templateNested);
ArkTools.jitCompileAsync(templateTagged);
ArkTools.jitCompileAsync(templateWithCall);
ArkTools.jitCompileAsync(computedBasic);
ArkTools.jitCompileAsync(computedExpression);
ArkTools.jitCompileAsync(computedMethods);
ArkTools.jitCompileAsync(computedSymbol);
ArkTools.jitCompileAsync(symbolCreation);
ArkTools.jitCompileAsync(symbolProperty);
ArkTools.jitCompileAsync(symbolWellKnown);
ArkTools.jitCompileAsync(symbolIterator);
ArkTools.jitCompileAsync(symbolDescription);
ArkTools.jitCompileAsync(logicalAndAssign);
ArkTools.jitCompileAsync(logicalOrAssign);
ArkTools.jitCompileAsync(combinedPatterns);

print("compile optionalChainBasic: " + ArkTools.waitJitCompileFinish(optionalChainBasic));
print("compile optionalChainNull: " + ArkTools.waitJitCompileFinish(optionalChainNull));
print("compile optionalChainMethod: " + ArkTools.waitJitCompileFinish(optionalChainMethod));
print("compile optionalChainArray: " + ArkTools.waitJitCompileFinish(optionalChainArray));
print("compile optionalChainAssign: " + ArkTools.waitJitCompileFinish(optionalChainAssign));
print("compile nullishBasic: " + ArkTools.waitJitCompileFinish(nullishBasic));
print("compile nullishWithOptional: " + ArkTools.waitJitCompileFinish(nullishWithOptional));
print("compile nullishAssignment: " + ArkTools.waitJitCompileFinish(nullishAssignment));
print("compile nullishVsOr: " + ArkTools.waitJitCompileFinish(nullishVsOr));
print("compile templateBasic: " + ArkTools.waitJitCompileFinish(templateBasic));
print("compile templateMultiLine: " + ArkTools.waitJitCompileFinish(templateMultiLine));
print("compile templateExpression: " + ArkTools.waitJitCompileFinish(templateExpression));
print("compile templateNested: " + ArkTools.waitJitCompileFinish(templateNested));
print("compile templateTagged: " + ArkTools.waitJitCompileFinish(templateTagged));
print("compile templateWithCall: " + ArkTools.waitJitCompileFinish(templateWithCall));
print("compile computedBasic: " + ArkTools.waitJitCompileFinish(computedBasic));
print("compile computedExpression: " + ArkTools.waitJitCompileFinish(computedExpression));
print("compile computedMethods: " + ArkTools.waitJitCompileFinish(computedMethods));
print("compile computedSymbol: " + ArkTools.waitJitCompileFinish(computedSymbol));
print("compile symbolCreation: " + ArkTools.waitJitCompileFinish(symbolCreation));
print("compile symbolProperty: " + ArkTools.waitJitCompileFinish(symbolProperty));
print("compile symbolWellKnown: " + ArkTools.waitJitCompileFinish(symbolWellKnown));
print("compile symbolIterator: " + ArkTools.waitJitCompileFinish(symbolIterator));
print("compile symbolDescription: " + ArkTools.waitJitCompileFinish(symbolDescription));
print("compile logicalAndAssign: " + ArkTools.waitJitCompileFinish(logicalAndAssign));
print("compile logicalOrAssign: " + ArkTools.waitJitCompileFinish(logicalOrAssign));
print("compile combinedPatterns: " + ArkTools.waitJitCompileFinish(combinedPatterns));

// Verify
print("optionalChainBasic: " + optionalChainBasic());
print("optionalChainNull: " + optionalChainNull());
print("optionalChainMethod: " + optionalChainMethod());
print("optionalChainArray: " + optionalChainArray());
print("optionalChainAssign: " + optionalChainAssign());
print("nullishBasic: " + nullishBasic());
print("nullishWithOptional: " + nullishWithOptional());
print("nullishAssignment: " + nullishAssignment());
print("nullishVsOr: " + nullishVsOr());
print("templateBasic: " + templateBasic());
print("templateMultiLine: " + templateMultiLine());
print("templateExpression: " + templateExpression());
print("templateNested: " + templateNested());
print("templateTagged: " + templateTagged());
print("templateWithCall: " + templateWithCall());
print("computedBasic: " + computedBasic());
print("computedExpression: " + computedExpression());
print("computedMethods: " + computedMethods());
print("computedSymbol: " + computedSymbol());
print("symbolCreation: " + symbolCreation());
print("symbolProperty: " + symbolProperty());
print("symbolWellKnown: " + symbolWellKnown());
print("symbolIterator: " + symbolIterator());
print("symbolDescription: " + symbolDescription());
print("logicalAndAssign: " + logicalAndAssign());
print("logicalOrAssign: " + logicalOrAssign());
print("combinedPatterns: " + combinedPatterns());
