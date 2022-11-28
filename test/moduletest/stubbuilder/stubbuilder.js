/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
 * @tc.name:stubbuilder
 * @tc.desc:test stubbuilder
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */

/*
*    ZExtInt8ToPtr(); IntPtr(); Int8(); PtrMul(); Int8LSR(); Int8And(); Store(); Load();
*    ReadInst4_0(); ReadInst4_1(); GetVregValue(); SetVregValue();
*/
                      /**************HandleMovV4V4****************/

{
    var fun = function(parameter) {
        return parameter;
    }
    
    var call = fun(100);
    print(call);
}

/*
*   ZExtInt16ToPtr(); Int16(); ZExtInt8ToInt16();  Int16LSL(); Int16Add();
*   
*   ReadInst16_0(); ReadInst16_2(); GetVregValue(); SetVregValue(); ReadInst8_0(); ReadInst8_1(); ReadInst8_2(); ReadInst8_3();
*/
                      /**************HandleMovV16V16****************/

{
    var fun = function(pmt1, pmt2, pmt3, pmt4, pmt5, pmt6, pmt7, pmt8, pmt9, pmt10) {
        let sum = pmt1 + pmt2 + pmt3 + pmt4 + pmt5 + pmt6 + pmt7 + pmt8 + pmt9 + pmt10;
        return sum;
    }
    
    var call = fun(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    print(call);
}

/*
*    IntToTaggedPtr(); ZExtInt8ToInt32(); Int32LSL(); Int32(); Int32Add();
*    ReadInst32_0();
*/
                      /**************HandleLdaiImm32****************/
{
    var fun = function() {
        let a = 20;
        return a;
    }
    var iter = fun();
    print(iter);
}

/*
*    SExtInt32ToPtr(); SExtInt1ToInt32();
*    ReadInstSigned8_0();
*/
                      /**************HandleJmpImm8****************/
{
    var fun = function() {
        try {
            print("foo");
          } finally {
            print("bar");
        }
    }
    var iter = fun();
}

/*
*    CastInt64ToFloat64(); DoubleToTaggedDoublePtr(); ZExtInt8ToInt64(); Int64(); Int64LSL(); Int64Add();
*    CastDoubleToInt64(); Int64ToTaggedPtr();
*    ReadInst64_0(); ReadInst8_7(); ReadInst8_6(); ReadInst8_5(); ReadInst8_4(); ReadInst8_3(); ReadInst8_2(); ReadInst8_1();
*    ReadInst8_0();
*/
                      /**************HandleFldaiImm64****************/
{
    {
        var fun = function() {
            let a = 2.55;
            return a;
        }
        var iter = fun();
        print(iter);
    }
}

/*
*    TaggedIsInt(); TaggedCastToInt32(); Int32Equal(); TaggedIsDouble(); TaggedCastToDouble(); Double(); DoubleAdd();
*    CallRuntime(); Int64And(); Int64Equal(); TaggedCastToInt64(); ChangeInt64ToInt32(); BoolNot(); TaggedIsNumber();
*    BoolAnd(); ChangeTaggedPointerToInt64(); Int64Sub(); SavePcIfNeeded(); TaggedIsObject(); Argument(); PtrSub();
*/
                      /**************HandleIncImm8****************/
{
    var fun = function() {
        let a = 10;
        return ++a;
    }
    var iter = fun();
    print(iter);
}

/*
*    TaggedIsException(); TaggedIsUndefined(); Int32Not(); TruncPtrToInt32(); IntPtrEqual(); CallNGCRuntime(); Equal();
*    TruncInt64ToInt32();
*/
                      /**************HandleSuspendgeneratorV8****************/
{
    var fun = function *() {
        yield 888;
    }

    let iter = fun();
    print(iter.next().value);
}

/*
*    ZExtInt16ToInt32(); Int16Sub(); TaggedIsHeapObject(); IsBase(); IsJSFunction(); IsConstructor(); IsJSHClass(); GetObjectType();
*    JSCallDispatch(); ChangeInt32ToIntPtr(); Int16ToTaggedInt(); HasPendingException(); GetMethodFromJSFunction();
*    Int32LessThanOrEqual(); Int32LSR(); Int32And(); Int32GreaterThanOrEqual(); LoadHClass(); Int32NotEqual(); ZExtInt32ToInt64();
*    SExtInt16ToInt64(); Int64Or(); Int64NotEqual(); IsJSFunctionBase();
*    
*/
                      /**************HandleNewobjrangeImm8Imm8V8****************/
{
    class Parent {
        constructor(name, age) {
            this.name = name;
            this.age = age;
        }
        sayName() {
            print(this.name);
            print(this.age);
        }
    };
    const child = new Parent('echo', 26);
    child.sayName(); //echo
}

/*
*    Int32Sub(); DoubleSub();
*    
*/
                      /**************HandleDecImm8****************/
{
    var fun = function() {
        let a = 10;
        return --a;
    }
    var iter = fun();
    print(iter);
}

/*
*    FastStrictEqual(); ChangeInt32ToFloat64(); DoubleIsNAN(); BoolOr(); DoubleEqual(); BothAreString(); IsUtf16String();
*    ZExtInt1ToInt32(); GetLengthFromString(); GetHashcodeFromString(); CallNGCRuntime(); TaggedIsBigInt();
*    BothAreString();
*/
                      /**************HandleStrictnoteqImm8V8****************/
{
    var fun = function(parameter1, parameter2) {
        if (parameter1 === parameter2) {
            return parameter2;
        }
        return parameter1;
    }
    var iter = fun(10n, 5);
    print(iter);
}

/*
*   DoubleToInt(); Int32Xor(); ChangeFloat64ToInt32(); Int64LSR(); Int32LessThan();
*    
*/
                      /**************HandleXor2Imm8V8****************/
{
    var fun = function(parameter1, parameter2) {
        if (parameter1 != parameter2) {
            return parameter1^parameter2;
        }
        return parameter2;
    }
    var iter = fun(10, 5);
    print(iter);
}

/*
*   Int32ASR(); 
*    
*/
                      /**************HandleAshr2Imm8V8****************/
{
    var fun = function(parameter1, parameter2) {
        if (parameter1 != parameter2) {
            return parameter1 >> parameter2;
        }
        return parameter2;
    }
    var iter = fun(10, 1);
    print(iter);
}

/*
*  TaggedIsNumeric(); 
*    
*/
                      /**************HandleTonumericImm8****************/
{
    var fun = function() {
        let parameter = 5n;
        return parameter++;
    }
    var iter = fun();
    print(iter);
}

/*
*  FastMul(); TaggedIsHole(); FastAddSubAndMul(); Int64GreaterThan(); Int64LessThan(); FastBinaryOp();
*    
*/
                      /**************HandleMul2Imm8V8****************/
{
    var fun = function() {
        let parameter1 = 5.5;
        let parameter2 = 2;
        return parameter1 * parameter2;
    }
    var iter = fun();
    print(iter);
}

/*
*  GetPropertyByIndex(); IntToTaggedInt(); TaggedFalse(); Undefined(); IsSpecialIndexedObj(); IsFastTypeArray();
*  IsSpecialContainer(); GetContainerProperty(); GetElementsArray(); IsDictionaryElement(); GetLengthOfTaggedArray();
*  Int32UnsignedLessThan(); GetValueFromTaggedArray(); TaggedIsNotHole(); FindElementFromNumberDictionary();
*  CallGetterHelper(); GetPrototypeFromHClass(); SExtInt32ToInt64(); Int32GreaterThan(); IsAccessor(); JSAPIContainerGet();
*/
                      /**************HandleLdobjbyindexImm8Imm16****************/
{
    var fun = function() {
        let array = new Array(2);
        array[0] = 12;
        array[1] = 18;
        return array[1];
    }
    var iter = fun();
    print(iter);
}

/*
*  FastEqual(); TaggedIsUndefinedOrNull(); TaggedIsBoolean(); TaggedIsSpecial();
*    
*/
                      /**************HandleEqImm8V8****************/
{
    var fun = function(parameter1, parameter2) {
        if (parameter1 == parameter2) {
            return parameter1;
        }
        return parameter2;
    }
    var iter = fun(10, 10);
    print(iter);
}

/*
*  FastMod(); Int32Mod(); DoubleIsINF();
*    
*/
                      /**************HandleMod2Imm8V8****************/
{
    var fun = function(parameter1, parameter2) {
        let result = parameter1 % parameter2;
        return result;
    }
    var iter = fun(55, 10);
    print(iter);
}

/*
*  FastTypeOf(); GetGlobalConstantString(); TaggedIsTrue(); TaggedIsFalse(); TaggedIsNull(); IsString(); IsSymbol(); IsCallable();
*  TaggedObjectIsBigInt();  Int32Mul(); Int64Mul();
*/
                      /**************HandleTypeofImm8****************/
{
    var fun = function(parameter1, parameter2) {
        let result = parameter1 % parameter2;
        return typeof(result);
    }
    var iter = fun(55, 10);
    print(iter);
}

/*
*  TaggedGetInt(); DoubleLessThan(); TaggedTrue(); 
*    
*/
                      /**************HandleLessImm8V8****************/
{
    var fun = function(parameter1, parameter2) {
        if (parameter1 < parameter2) {
            return parameter1;
        };
        return parameter2;
    }
    var iter = fun(55, 11);
    print(iter);
}

/*
*  DoubleLessThanOrEqual();
*    
*/
                      /**************HandleLesseqImm8V8****************/
{
    var fun = function(parameter1, parameter2) {
        if (parameter1 <= parameter2) {
            return parameter1;
        };
        return parameter2;
    }
    var iter = fun(5, 11);
    print(iter);
}

/*
*  DoubleGreaterThan();
*    
*/
                      /**************HandleGreaterImm8V8****************/
{
    var fun = function(parameter1, parameter2) {
        if (parameter1 > parameter2) {
            return parameter1;
        };
        return parameter2;
    }
    var iter = fun(5, 11);
    print(iter);
}

/*
*  DoubleGreaterThanOrEqual();
*    
*/
                      /**************HandleGreatereqImm8V8****************/
{
    var fun = function(parameter1, parameter2) {
        if (parameter1 >= parameter2) {
            return parameter1;
        };
        return parameter2;
    }
    var iter = fun(5, 9);
    print(iter);
}

/*
*  Int32UnsignedGreaterThan(); ChangeUInt32ToFloat64();
*    
*/
                      /**************HandleShr2Imm8V8****************/
{
    var fun = function(parameter1, parameter2) {
        if (parameter1 >> parameter2) {
            return parameter1;
        };
        return parameter2;
    }
    var iter = fun(5, 1);
    print(iter);
}

/*
*  SetPropertyByIndex(); LoopBegin(); Hole(); SetValueToTaggedArray(); IsExtensible(); Exception();
*    
*/
                      /**************HandleStobjbyindexImm8V8Imm16****************/
{
    var fun = function() {
	
        var myarray=new Array();
        return myarray[0]=66;
    }
    var iter = fun();
    print(iter);
}

/*
*  FastDiv(); Int64Xor(); DoubleDiv();
*    
*/
                      /**************HandleDiv2Imm8V8****************/
{
    var fun = function(parameter1, parameter2) {
        let result = parameter1 / parameter2;
        return result;
    }
    var iter = fun(55, 10);
    print(iter);
}

/*
*  IsCallableFromBitField(); GetCallFieldFromMethod(); DispatchBuiltins(); IsClassConstructorFromBitField(); ConstructorCheck();
*  SaveJumpSizeIfNeeded(); TaggedObjectIsEcmaObject();
*/
                      /**************HandleNewobjrangeImm8Imm8V8****************/
{
    var fun = function() {
        let func = new Object();
        func.name = "don";
        return func.name;
    }
    var iter = fun();
    print(iter);
}

/*
*  FastAdd();
*    
*/
                      /**************HandleAdd2Imm8V8****************/
{
    var fun = function(parameter1, parameter2) {

        return parameter1 + parameter2;
    }
    var iter = fun(10, 10);
    print(iter);
}

/*
*  FastSub();
*    
*/
                      /**************HandleSub2Imm8V8****************/
{
    var fun = function(parameter1, parameter2) {

        return parameter1 - parameter2;
    }
    var iter = fun(10, 10);
    print(iter);
}

/*
*  FastToBoolean();
*    
*/
                      /**************HandleIsfalse****************/
{
    var fun = function(parameter1, parameter2) {

        if ((parameter1 == 0) || (parameter2 > 0)) {
            return parameter1;
        }
        return parameter2;
    }
    var iter = fun(1, 10);
    print(iter);
}

/*
*  Null();
*    
*/
                      /**************HandleLdnull****************/
{
    var fun = function() {

        let par = null;
        return undefined;
    }
    var iter = fun();
    print(iter);
}

/*
*  LoopEnd(); GetPropertiesFromLexicalEnv();
*    
*/
                      /**************HandleLdlexvarImm4Imm4****************/
{
    function Module() {
        function f(a) {
          a = a | 0;
          a = funTable[a & 0](a | 0) | 0;
          return a | 0;
        }
        function g() {
          return 2.3;
        }
        var funTable = [ g ];
        return { f:f };
      }
    print(Module().f());
}

/*
*  Int32Or();
*    
*/
                      /**************HandleOr2Imm8V8****************/
{
    function Module() {
        function f(a) {
          a = a | 0;
          var b = 0;
          b = (a + 23) | 0;
          return b;
        }
        return { f:f };
      }
      print(Module().f());
}

/*
*    PtrAdd(); IntPtrSize();
*    DispatchWithId();
*/
                      /**************HandleThrow****************/
{
    var fun = function() {
        print("boom");
        throw "boom";
    }
    var a = fun();
}