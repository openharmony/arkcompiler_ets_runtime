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

// ==================== Error Basic ====================

function testErrorBasic(): string {
    const err = new Error('test message');
    return `${err.name},${err.message}`;
}

function testErrorWithCause(): string {
    const cause = new Error('original error');
    const err = new Error('wrapper error', { cause });
    return `${err.message},${(err.cause as Error).message}`;
}

function testErrorStack(): string {
    const err = new Error('stack test');
    return `${typeof err.stack === 'string'},${err.stack!.includes('Error')}`;
}

function testErrorToString(): string {
    const err = new Error('my message');
    return `${err.toString()}`;
}

// ==================== Error Types ====================

function testTypeError(): string {
    const err = new TypeError('type error message');
    return `${err.name},${err.message}`;
}

function testRangeError(): string {
    const err = new RangeError('range error message');
    return `${err.name},${err.message}`;
}

function testReferenceError(): string {
    const err = new ReferenceError('reference error message');
    return `${err.name},${err.message}`;
}

function testSyntaxError(): string {
    const err = new SyntaxError('syntax error message');
    return `${err.name},${err.message}`;
}

function testURIError(): string {
    const err = new URIError('uri error message');
    return `${err.name},${err.message}`;
}

function testEvalError(): string {
    const err = new EvalError('eval error message');
    return `${err.name},${err.message}`;
}

// ==================== Try-Catch ====================

function testTryCatch(): string {
    let caught = false;
    try {
        throw new Error('thrown error');
    } catch (e) {
        caught = true;
    }
    return `${caught}`;
}

function testTryCatchFinally(): string {
    const results: string[] = [];
    try {
        results.push('try');
        throw new Error('error');
    } catch (e) {
        results.push('catch');
    } finally {
        results.push('finally');
    }
    return results.join(',');
}

function testTryFinallyNoError(): string {
    const results: string[] = [];
    try {
        results.push('try');
    } finally {
        results.push('finally');
    }
    return results.join(',');
}

function testNestedTryCatch(): string {
    const results: string[] = [];
    try {
        try {
            throw new Error('inner');
        } catch (e) {
            results.push('inner-catch');
            throw new Error('rethrow');
        }
    } catch (e) {
        results.push('outer-catch');
    }
    return results.join(',');
}

function testCatchErrorType(): string {
    let errorType = '';
    try {
        throw new TypeError('type error');
    } catch (e) {
        if (e instanceof TypeError) {
            errorType = 'TypeError';
        } else if (e instanceof Error) {
            errorType = 'Error';
        }
    }
    return errorType;
}

function testCatchNonError(): string {
    let caught = '';
    try {
        throw 'string error';
    } catch (e) {
        caught = typeof e;
    }
    return caught;
}

function testCatchPrimitive(): string {
    const results: string[] = [];
    try {
        throw 42;
    } catch (e) {
        results.push(String(e));
    }
    try {
        throw true;
    } catch (e) {
        results.push(String(e));
    }
    try {
        throw null;
    } catch (e) {
        results.push(String(e));
    }
    return results.join(',');
}

// ==================== Error Rethrowing ====================

function testRethrow(): string {
    let caught = 0;
    try {
        try {
            throw new Error('original');
        } catch (e) {
            caught++;
            throw e;
        }
    } catch (e) {
        caught++;
    }
    return `${caught}`;
}

function testConditionalRethrow(): string {
    const results: string[] = [];
    function process(shouldRethrow: boolean) {
        try {
            throw new Error('test');
        } catch (e) {
            results.push('caught');
            if (shouldRethrow) {
                throw e;
            }
        }
    }
    process(false);
    try {
        process(true);
    } catch (e) {
        results.push('rethrown');
    }
    return results.join(',');
}

// ==================== Error in Functions ====================

function testErrorInNestedFunction(): string {
    function inner() {
        throw new Error('inner error');
    }
    function outer() {
        inner();
    }
    let caught = false;
    try {
        outer();
    } catch (e) {
        caught = true;
    }
    return `${caught}`;
}

function testErrorInCallback(): string {
    let caught = false;
    const arr = [1, 2, 3];
    try {
        arr.forEach((x) => {
            if (x === 2) throw new Error('callback error');
        });
    } catch (e) {
        caught = true;
    }
    return `${caught}`;
}

function testErrorInMapCallback(): string {
    let caught = false;
    try {
        [1, 2, 3].map((x) => {
            if (x === 2) throw new Error('map error');
            return x;
        });
    } catch (e) {
        caught = true;
    }
    return `${caught}`;
}

// ==================== Custom Errors ====================

class CustomError extends Error {
    code: number;
    constructor(message: string, code: number) {
        super(message);
        this.name = 'CustomError';
        this.code = code;
    }
}

function testCustomError(): string {
    const err = new CustomError('custom message', 500);
    return `${err.name},${err.message},${err.code}`;
}

function testCustomErrorInstanceof(): string {
    const err = new CustomError('test', 404);
    return `${err instanceof CustomError},${err instanceof Error}`;
}

class ValidationError extends Error {
    field: string;
    constructor(field: string, message: string) {
        super(message);
        this.name = 'ValidationError';
        this.field = field;
    }
}

function testValidationError(): string {
    const err = new ValidationError('email', 'invalid email format');
    return `${err.name},${err.field},${err.message}`;
}

// ==================== Error Handling Patterns ====================

function testErrorWithDefault(): string {
    function safeParse(json: string): any {
        try {
            return JSON.parse(json);
        } catch (e) {
            return null;
        }
    }
    const valid = safeParse('{"a": 1}');
    const invalid = safeParse('invalid');
    return `${valid?.a},${invalid}`;
}

function testErrorChain(): string {
    function level3() {
        throw new Error('level3 error');
    }
    function level2() {
        try {
            level3();
        } catch (e) {
            throw new Error('level2 error', { cause: e as Error });
        }
    }
    function level1() {
        try {
            level2();
        } catch (e) {
            const err = e as Error;
            const cause = err.cause as Error;
            return `${err.message},${cause.message}`;
        }
        return '';
    }
    return level1();
}

function testMultipleErrorTypes(): string {
    function handleError(fn: () => void): string {
        try {
            fn();
            return 'success';
        } catch (e) {
            if (e instanceof TypeError) return 'type';
            if (e instanceof RangeError) return 'range';
            if (e instanceof Error) return 'error';
            return 'unknown';
        }
    }
    const r1 = handleError(() => { throw new TypeError('t'); });
    const r2 = handleError(() => { throw new RangeError('r'); });
    const r3 = handleError(() => { throw new Error('e'); });
    return `${r1},${r2},${r3}`;
}

// ==================== Finally Behavior ====================

function testFinallyReturn(): string {
    function test(): string {
        try {
            return 'try';
        } finally {
            return 'finally';
        }
    }
    return test();
}

function testFinallyWithThrow(): string {
    const results: string[] = [];
    try {
        try {
            throw new Error('error');
        } finally {
            results.push('finally');
        }
    } catch (e) {
        results.push('caught');
    }
    return results.join(',');
}

function testFinallyOverridesReturn(): string {
    function test(): number {
        try {
            return 1;
        } finally {
            return 2;
        }
    }
    return `${test()}`;
}

// ==================== Error Properties ====================

function testErrorName(): string {
    const err = new Error('test');
    err.name = 'CustomName';
    return `${err.name},${err.toString()}`;
}

function testErrorMessage(): string {
    const err = new Error();
    return `${err.message === ''},${err.name}`;
}

function testErrorInheritance(): string {
    const te = new TypeError('test');
    const re = new RangeError('test');
    return `${te instanceof Error},${re instanceof Error}`;
}

// Warmup
for (let i = 0; i < 20; i++) {
    testErrorBasic();
    testErrorWithCause();
    testErrorStack();
    testErrorToString();
    testTypeError();
    testRangeError();
    testReferenceError();
    testSyntaxError();
    testURIError();
    testEvalError();
    testTryCatch();
    testTryCatchFinally();
    testTryFinallyNoError();
    testNestedTryCatch();
    testCatchErrorType();
    testCatchNonError();
    testCatchPrimitive();
    testRethrow();
    testConditionalRethrow();
    testErrorInNestedFunction();
    testErrorInCallback();
    testErrorInMapCallback();
    testCustomError();
    testCustomErrorInstanceof();
    testValidationError();
    testErrorWithDefault();
    testErrorChain();
    testMultipleErrorTypes();
    testFinallyReturn();
    testFinallyWithThrow();
    testFinallyOverridesReturn();
    testErrorName();
    testErrorMessage();
    testErrorInheritance();
}

// JIT compile
ArkTools.jitCompileAsync(testErrorBasic);
ArkTools.jitCompileAsync(testErrorWithCause);
ArkTools.jitCompileAsync(testErrorStack);
ArkTools.jitCompileAsync(testErrorToString);
ArkTools.jitCompileAsync(testTypeError);
ArkTools.jitCompileAsync(testRangeError);
ArkTools.jitCompileAsync(testReferenceError);
ArkTools.jitCompileAsync(testSyntaxError);
ArkTools.jitCompileAsync(testURIError);
ArkTools.jitCompileAsync(testEvalError);
ArkTools.jitCompileAsync(testTryCatch);
ArkTools.jitCompileAsync(testTryCatchFinally);
ArkTools.jitCompileAsync(testTryFinallyNoError);
ArkTools.jitCompileAsync(testNestedTryCatch);
ArkTools.jitCompileAsync(testCatchErrorType);
ArkTools.jitCompileAsync(testCatchNonError);
ArkTools.jitCompileAsync(testCatchPrimitive);
ArkTools.jitCompileAsync(testRethrow);
ArkTools.jitCompileAsync(testConditionalRethrow);
ArkTools.jitCompileAsync(testErrorInNestedFunction);
ArkTools.jitCompileAsync(testErrorInCallback);
ArkTools.jitCompileAsync(testErrorInMapCallback);
ArkTools.jitCompileAsync(testCustomError);
ArkTools.jitCompileAsync(testCustomErrorInstanceof);
ArkTools.jitCompileAsync(testValidationError);
ArkTools.jitCompileAsync(testErrorWithDefault);
ArkTools.jitCompileAsync(testErrorChain);
ArkTools.jitCompileAsync(testMultipleErrorTypes);
ArkTools.jitCompileAsync(testFinallyReturn);
ArkTools.jitCompileAsync(testFinallyWithThrow);
ArkTools.jitCompileAsync(testFinallyOverridesReturn);
ArkTools.jitCompileAsync(testErrorName);
ArkTools.jitCompileAsync(testErrorMessage);
ArkTools.jitCompileAsync(testErrorInheritance);

print("compile testErrorBasic: " + ArkTools.waitJitCompileFinish(testErrorBasic));
print("compile testErrorWithCause: " + ArkTools.waitJitCompileFinish(testErrorWithCause));
print("compile testErrorStack: " + ArkTools.waitJitCompileFinish(testErrorStack));
print("compile testErrorToString: " + ArkTools.waitJitCompileFinish(testErrorToString));
print("compile testTypeError: " + ArkTools.waitJitCompileFinish(testTypeError));
print("compile testRangeError: " + ArkTools.waitJitCompileFinish(testRangeError));
print("compile testReferenceError: " + ArkTools.waitJitCompileFinish(testReferenceError));
print("compile testSyntaxError: " + ArkTools.waitJitCompileFinish(testSyntaxError));
print("compile testURIError: " + ArkTools.waitJitCompileFinish(testURIError));
print("compile testEvalError: " + ArkTools.waitJitCompileFinish(testEvalError));
print("compile testTryCatch: " + ArkTools.waitJitCompileFinish(testTryCatch));
print("compile testTryCatchFinally: " + ArkTools.waitJitCompileFinish(testTryCatchFinally));
print("compile testTryFinallyNoError: " + ArkTools.waitJitCompileFinish(testTryFinallyNoError));
print("compile testNestedTryCatch: " + ArkTools.waitJitCompileFinish(testNestedTryCatch));
print("compile testCatchErrorType: " + ArkTools.waitJitCompileFinish(testCatchErrorType));
print("compile testCatchNonError: " + ArkTools.waitJitCompileFinish(testCatchNonError));
print("compile testCatchPrimitive: " + ArkTools.waitJitCompileFinish(testCatchPrimitive));
print("compile testRethrow: " + ArkTools.waitJitCompileFinish(testRethrow));
print("compile testConditionalRethrow: " + ArkTools.waitJitCompileFinish(testConditionalRethrow));
print("compile testErrorInNestedFunction: " + ArkTools.waitJitCompileFinish(testErrorInNestedFunction));
print("compile testErrorInCallback: " + ArkTools.waitJitCompileFinish(testErrorInCallback));
print("compile testErrorInMapCallback: " + ArkTools.waitJitCompileFinish(testErrorInMapCallback));
print("compile testCustomError: " + ArkTools.waitJitCompileFinish(testCustomError));
print("compile testCustomErrorInstanceof: " + ArkTools.waitJitCompileFinish(testCustomErrorInstanceof));
print("compile testValidationError: " + ArkTools.waitJitCompileFinish(testValidationError));
print("compile testErrorWithDefault: " + ArkTools.waitJitCompileFinish(testErrorWithDefault));
print("compile testErrorChain: " + ArkTools.waitJitCompileFinish(testErrorChain));
print("compile testMultipleErrorTypes: " + ArkTools.waitJitCompileFinish(testMultipleErrorTypes));
print("compile testFinallyReturn: " + ArkTools.waitJitCompileFinish(testFinallyReturn));
print("compile testFinallyWithThrow: " + ArkTools.waitJitCompileFinish(testFinallyWithThrow));
print("compile testFinallyOverridesReturn: " + ArkTools.waitJitCompileFinish(testFinallyOverridesReturn));
print("compile testErrorName: " + ArkTools.waitJitCompileFinish(testErrorName));
print("compile testErrorMessage: " + ArkTools.waitJitCompileFinish(testErrorMessage));
print("compile testErrorInheritance: " + ArkTools.waitJitCompileFinish(testErrorInheritance));

// Verify
print("testErrorBasic: " + testErrorBasic());
print("testErrorWithCause: " + testErrorWithCause());
print("testErrorStack: " + testErrorStack());
print("testErrorToString: " + testErrorToString());
print("testTypeError: " + testTypeError());
print("testRangeError: " + testRangeError());
print("testReferenceError: " + testReferenceError());
print("testSyntaxError: " + testSyntaxError());
print("testURIError: " + testURIError());
print("testEvalError: " + testEvalError());
print("testTryCatch: " + testTryCatch());
print("testTryCatchFinally: " + testTryCatchFinally());
print("testTryFinallyNoError: " + testTryFinallyNoError());
print("testNestedTryCatch: " + testNestedTryCatch());
print("testCatchErrorType: " + testCatchErrorType());
print("testCatchNonError: " + testCatchNonError());
print("testCatchPrimitive: " + testCatchPrimitive());
print("testRethrow: " + testRethrow());
print("testConditionalRethrow: " + testConditionalRethrow());
print("testErrorInNestedFunction: " + testErrorInNestedFunction());
print("testErrorInCallback: " + testErrorInCallback());
print("testErrorInMapCallback: " + testErrorInMapCallback());
print("testCustomError: " + testCustomError());
print("testCustomErrorInstanceof: " + testCustomErrorInstanceof());
print("testValidationError: " + testValidationError());
print("testErrorWithDefault: " + testErrorWithDefault());
print("testErrorChain: " + testErrorChain());
print("testMultipleErrorTypes: " + testMultipleErrorTypes());
print("testFinallyReturn: " + testFinallyReturn());
print("testFinallyWithThrow: " + testFinallyWithThrow());
print("testFinallyOverridesReturn: " + testFinallyOverridesReturn());
print("testErrorName: " + testErrorName());
print("testErrorMessage: " + testErrorMessage());
print("testErrorInheritance: " + testErrorInheritance());
