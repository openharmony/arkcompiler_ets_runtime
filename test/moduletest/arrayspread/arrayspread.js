/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

/*
 * @tc.name:async
 * @tc.desc:test array spread
 * @tc.type: FUNC
 * @tc.require: issues/I8J3HT
 */

var a = [, 2];
print([...a]);
print([...a].hasOwnProperty(0));
function foo0() { return arguments.hasOwnProperty(0); }
print(foo0(...a));
a.__proto__.push(2);
print([...a]);

//  ========================normal tests========================
print("\n=== Basic Spread Tests ===");
var arr1 = [1, 2, 3];
print([...arr1]);
print([...arr1].length);
print([...arr1][0]);
print([...arr1][2]);

// Spread with empty array
print("\n=== Empty Array Spread ===");
var empty = [];
print([...empty]);
print([...empty].length);

// Spread with single element
print("\n=== Single Element Spread ===");
var single = [42];
print([...single]);
print([...single][0]);

// Spread with undefined and null
print("\n=== Undefined and Null Spread ===");
var withUndefined = [1, undefined, 3];
print([...withUndefined]);
print([...withUndefined].length);
print([...withUndefined][1]);

var withNull = [1, null, 3];
print([...withNull]);
print([...withNull][1]);

// Spread with mixed types
print("\n=== Mixed Types Spread ===");
var mixed = [1, 'string', true, null, undefined, 3.14];
print([...mixed]);
print([...mixed].length);
print([...mixed][1]);
print([...mixed][2]);
print([...mixed][4]);

// Nested array spread
print("\n=== Nested Array Spread ===");
var nested = [[1, 2], [3, 4]];
print([...nested]);
print([...nested][0]);
print([...nested][1]);

var flat = [1, 2, 3];
var nested2 = [0, ...flat, 4];
print(nested2);
print(nested2.length);
print(nested2[0]);
print(nested2[3]);

// Multiple spread in one array
print("\n=== Multiple Spread ===");
var arr2 = [5, 6];
var arr3 = [7, 8];
var combined = [...arr1, ...arr2, ...arr3];
print(combined);
print(combined.length);
print(combined[4]);
print(combined[6]);

// Spread with literals
print("\n=== Spread with Literals ===");
var withLiterals = [0, ...arr1, 4, 5];
print(withLiterals);
print(withLiterals.length);

// Spread string
print("\n=== String Spread ===");
var str = "hello";
print([...str]);
print([...str].length);
print([...str][0]);
print([...str][4]);

var emptyStr = "";
print([...emptyStr]);
print([...emptyStr].length);

// Spread with array-like objects (arguments)
print("\n=== Array-like Spread ===");
function testArgs() {
    print([...arguments]);
    print([...arguments].length);
}
testArgs(1, 2, 3, 4);

// Spread in function calls
print("\n=== Spread in Function Calls ===");
function sum(a, b, c) {
    return a + b + c;
}
var nums = [1, 2, 3];
print(sum(...nums));
print(sum(...[10, 20, 30]));

function max() {
    var maxVal = arguments[0];
    for (var i = 1; i < arguments.length; i++) {
        if (arguments[i] > maxVal) {
            maxVal = arguments[i];
        }
    }
    return maxVal;
}
print(max(...[1, 5, 3, 9, 2]));
print(max(...[100, 50, 75]));

// Spread with Math methods
print("\n=== Spread with Math ===");
print(Math.max(...[1, 5, 3, 9, 2]));
print(Math.min(...[1, 5, 3, 9, 2]));

// Spread creating new array vs reference
print("\n=== Spread Creates New Array ===");
var original = [1, 2, 3];
var copied = [...original];
print(copied);
copied[0] = 99;
print(original);
print(copied);

// Spread with holes
print("\n=== Spread with Holes ===");
var withHoles = [1, , , 4];
print([...withHoles]);
print([...withHoles].length);
print([...withHoles].hasOwnProperty(1));
print([...withHoles].hasOwnProperty(2));

// Spread with push
print("\n=== Spread with Push ===");
var base = [1, 2];
var toAdd = [3, 4];
base.push(...toAdd);
print(base);
print(base.length);

// Spread with unshift
print("\n=== Spread with Unshift ===");
var base2 = [3, 4];
var toAdd2 = [1, 2];
base2.unshift(...toAdd2);
print(base2);
print(base2.length);

// Spread large arrays
print("\n=== Large Array Spread ===");
var large = [];
for (var i = 0; i < 100; i++) {
    large.push(i);
}
var spreadLarge = [...large];
print(spreadLarge.length);
print(spreadLarge[0]);
print(spreadLarge[50]);
print(spreadLarge[99]);

// Spread with array methods
print("\n=== Spread with Array Methods ===");
var mapArr = [1, 2, 3, 4];
var doubled = [...mapArr].map(x => x * 2);
print(doubled);
print(doubled.length);

var filtered = [...mapArr].filter(x => x > 2);
print(filtered);
print(filtered.length);

// Spread in array destructuring
print("\n=== Spread in Destructuring ===");
var source = [1, 2, 3, 4, 5];
var [first, second, ...rest] = source;
print(first);
print(second);
print(rest);
print(rest.length);

// Spread with concat-like behavior
print("\n=== Spread Concat ===");
var part1 = [1, 2];
var part2 = [3, 4];
var part3 = [5, 6];
var concatSpread = [...part1, ...part2, ...part3];
print(concatSpread);
print(concatSpread.length);

// Spread with boolean values
print("\n=== Boolean Spread ===");
var boolArr = [true, false, true];
print([...boolArr]);
print([...boolArr][0]);
print([...boolArr][1]);

// Spread with negative numbers
print("\n=== Negative Numbers Spread ===");
var negArr = [-1, -2, -3];
print([...negArr]);
print([...negArr][0]);

// Spread with zero
print("\n=== Zero Spread ===");
var zeroArr = [0, 0, 0];
print([...zeroArr]);
print([...zeroArr][0]);

// Spread with floating point
print("\n=== Float Spread ===");
var floatArr = [1.1, 2.2, 3.3];
print([...floatArr]);
print([...floatArr][0]);

// Spread with objects (if supported)
print("\n=== Object Spread ===");
var objArr = [{a: 1}, {b: 2}];
print([...objArr]);
print([...objArr][0]);

// Spread with regex
print("\n=== Regex Spread ===");
var regexArr = [/test/g, /pattern/i];
print([...regexArr]);
print([...regexArr][0]);

// Spread empty multiple times
print("\n=== Multiple Empty Spread ===");
var empty1 = [];
var empty2 = [];
var emptyCombined = [...empty1, ...empty2];
print(emptyCombined);
print(emptyCombined.length);

// Spread with array containing arrays
print("\n=== Array of Arrays Spread ===");
var arrayOfArrays = [[1], [2], [3]];
print([...arrayOfArrays]);
print([...arrayOfArrays][0]);
print([...arrayOfArrays][0][0]);

// Spread preserving non-enumerable properties test
print("\n=== Spread Properties ===");
var arrWithProps = [1, 2, 3];
arrWithProps.custom = 'property';
var spreadArr = [...arrWithProps];
print(spreadArr);
print(spreadArr.custom);

// Spread with very long string
print("\n=== Long String Spread ===");
var longStr = "abcdefghijklmnopqrstuvwxyz";
print([...longStr]);
print([...longStr].length);

// Spread with NaN and Infinity
print("\n=== Special Numbers Spread ===");
var specialNums = [NaN, Infinity, -Infinity, 0];
print([...specialNums]);
print([...specialNums][0] === NaN);
print([...specialNums][1]);

// Spread duplicate values
print("\n=== Duplicate Values Spread ===");
var dupArr = [1, 2, 2, 3, 3, 3];
print([...dupArr]);
print([...dupArr].length);

// Spread modifying original shouldn't affect spread result
print("\n=== Spread Independence ===");
var originalArr = [1, 2, 3];
var spreadResult = [...originalArr];
originalArr.push(4);
print(originalArr);
print(spreadResult);

// Spread in array constructor context
print("\n=== Spread in Array Context ===");
var baseArr = [1, 2];
var extended = [0, ...baseArr, 3, 4];
print(extended);
print(extended.length);

// Spread with undefined elements
print("\n=== Explicit Undefined Spread ===");
var undefExplicit = [undefined, undefined];
print([...undefExplicit]);
print([...undefExplicit].length);
print([...undefExplicit][0]);

// Spread with array containing only one undefined
print("\n=== Single Undefined Spread ===");
var singleUndef = [undefined];
print([...singleUndef]);
print([...singleUndef].length);

// Spread after modifications
print("\n=== Spread After Modification ===");
var modArr = [1];
modArr.push(2);
modArr.push(3);
print([...modArr]);
print([...modArr].length);

// Spread with sparse array
print("\n=== Sparse Array Spread ===");
var sparse = new Array(5);
sparse[0] = 1;
sparse[4] = 5;
print([...sparse]);
print([...sparse].length);
print([...sparse].hasOwnProperty(1));
print([...sparse].hasOwnProperty(2));

// Spread in return statement
print("\n=== Spread in Return ===");
function returnSpread() {
    var arr = [1, 2, 3];
    return [...arr];
}
print(returnSpread());

// Spread with different number types
print("\n=== Different Number Types ===");
var numTypes = [1, 1.5, -10, 0, 0.0, -0];
print([...numTypes]);
print([...numTypes].length);

// Spread with array containing whitespace string
print("\n=== Whitespace String Spread ===");
var wsArr = [' ', '\t', '\n'];
print([...wsArr]);
print([...wsArr].length);
print([...wsArr][0]);

// Spread concatenation behavior
print("\n=== Spread Concatenation ===");
var a1 = [1, 2];
var a2 = [3, 4];
var a3 = [...a1, ...a2];
print(a3);
print(a3[0] === 1);
print(a3[3] === 4);

// Spread order preservation
print("\n=== Order Preservation ===");
var ordered = [5, 4, 3, 2, 1];
print([...ordered]);
print([...ordered][0]);
print([...ordered][4]);

// Spread with array containing boolean and null
print("\n=== Boolean Null Spread ===");
var bnArr = [true, false, null, undefined];
print([...bnArr]);
print([...bnArr].length);
print([...bnArr][2]);

// Spread with trailing comma
print("\n=== Trailing Comma Spread ===");
var trailing = [1, 2, 3,];
print([...trailing]);
print([...trailing].length);

// Spread with array methods chain
print("\n=== Spread with Methods Chain ===");
var chainArr = [1, 2, 3, 4, 5];
var result1 = [...chainArr].map(x => x * 2).filter(x => x > 4);
print(result1);
print(result1.length);

// Spread in conditional context
print("\n=== Conditional Spread ===");
var cond = true;
var arrA = [1, 2];
var arrB = [3, 4];
var conditional = cond ? [...arrA] : [...arrB];
print(conditional);
print(conditional.length);

// Spread with array iteration
print("\n=== Spread Iteration ===");
var iterArr = [10, 20, 30];
var spreadIter = [...iterArr];
for (var i = 0; i < spreadIter.length; i++) {
    if (i === 0 || i === spreadIter.length - 1) {
        print(spreadIter[i]);
    }
}

// Spread with slice-like behavior
print("\n=== Spread Slice ===");
var fullArr = [1, 2, 3, 4, 5];
var firstTwo = [...fullArr.slice(0, 2)];
var lastTwo = [...fullArr.slice(3)];
print(firstTwo);
print(lastTwo);

// Spread type coercion
print("\n=== Type Coercion Spread ===");
var strNum = "123";
var numSpread = [...strNum];
print(numSpread);
print(typeof numSpread[0]);

// Spread with array containing expressions
print("\n=== Expression Spread ===");
var x = 10;
var y = 20;
var exprArr = [x + y, x * y, x - y];
print([...exprArr]);
print([...exprArr][0]);

// Spread with empty-like values
print("\n=== Empty-like Spread ===");
var emptyLike = [0, "", false, null, undefined];
print([...emptyLike]);
print([...emptyLike].length);

// Spread cloning depth
print("\n=== Shallow Clone ===");
var originalDeep = [[1], [2], [3]];
var cloned = [...originalDeep];
cloned[0][0] = 99;
print(originalDeep[0][0]);
print(cloned[0][0]);

// Spread with array length property
print("\n=== Spread Length ===");
var lenTest = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
print([...lenTest].length);
print([...lenTest, 11].length);

// Spread in various positions
print("\n=== Spread Positions ===");
var start = [1, 2];
var middle = [3, 4];
var end = [5, 6];
var allPositions = [...start, ...middle, ...end];
print(allPositions);
print(allPositions.length);

// Spread with array containing only one element repeated
print("\n=== Repeated Element Spread ===");
var repeated = [1, 1, 1, 1];
print([...repeated]);
print([...repeated].length);

// Spread with reversed array
print("\n=== Reversed Spread ===");
var normal = [1, 2, 3];
var reversed = normal.reverse();
print([...reversed]);

// Spread with sorted array
print("\n=== Sorted Spread ===");
var unsorted = [3, 1, 2];
unsorted.sort();
print([...unsorted]);

// Spread with join behavior
print("\n=== Spread Join ===");
var toJoin = ['a', 'b', 'c'];
var spreadJoin = [...toJoin].join('-');
print(spreadJoin);

// Spread with indexed access
print("\n=== Indexed Access ===");
var idxArr = [100, 200, 300];
var spreadIdx = [...idxArr];
print(spreadIdx[0]);
print(spreadIdx[1]);
print(spreadIdx[2]);

// Spread with array containing truthy/falsy values
print("\n=== Truthy/Falsy Spread ===");
var tfArr = [1, 0, "hello", "", {}, []];
print([...tfArr]);
print([...tfArr].length);

// Spread prototype chain
print("\n=== Prototype Chain Spread ===");
var protoArr = [1, 2, 3];
Array.prototype.customProp = 'custom';
var spreadProto = [...protoArr];
print(spreadProto.hasOwnProperty('customProp'));

// Spread with constructor
print("\n=== Constructor Spread ===");
var arr = [1, 2, 3];
print(arr.constructor === Array);
print([...arr].constructor === Array);

// Spread with toString
print("\n=== Spread ToString ===");
var toStrArr = [1, 2, 3];
print([...toStrArr].toString());

// Spread with valueOf
print("\n=== Spread ValueOf ===");
var valArr = [1, 2, 3];
print([...valArr].valueOf());

// Spread with isArray
print("\n=== IsArray Spread ===");
var isArrTest = [1, 2, 3];
print(Array.isArray([...isArrTest]));

// Spread with instanceof
print("\n=== Instanceof Spread ===");
var instArr = [1, 2, 3];
print([...instArr] instanceof Array);

// Spread final tests
print("\n=== Final Spread Tests ===");
var finalArr = [1, 2, 3];
var finalSpread = [...finalArr, 4, 5, 6];
print(finalSpread);
print(finalSpread.length);
print(finalSpread[finalSpread.length - 1]);