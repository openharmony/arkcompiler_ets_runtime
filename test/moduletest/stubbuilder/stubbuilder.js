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
    assert_equal(call,100);
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
    assert_equal(call,55);
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
    assert_equal(iter,20);
}

/*
*    SExtInt32ToPtr(); SExtInt1ToInt32();
*    ReadInstSigned8_0();
*/
                      /**************HandleJmpImm8****************/
{
    var fun = function() {
    	let expectres = "";
        try {
            expectres = "foo";
            assert_equal(expectres,"foo");
          } finally {
            expectres = "bar";
        }
        assert_equal(expectres,"bar");
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
        assert_equal(iter,2.55);
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
    assert_equal(iter,11);
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
    assert_equal(iter.next().value,888);
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
            assert_equal(this.name,"echo");
            assert_equal(this.age,26);
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
    assert_equal(iter,9);
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
    assert_equal(Number(iter),10);
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
    assert_equal(iter,15);
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
    assert_equal(iter,5);
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
    assert_equal(Number(iter),5);
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
    assert_equal(iter,11);
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
    assert_equal(iter,18);
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
    assert_equal(iter,10);
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
    assert_equal(iter,5);
}

/*
*  FastTypeOf(); GetGlobalConstantOffset(); TaggedIsTrue(); TaggedIsFalse(); TaggedIsNull(); IsString(); IsSymbol(); IsCallable();
*  TaggedObjectIsBigInt();  Int32Mul(); Int64Mul();
*/
                      /**************HandleTypeofImm8****************/
{
    var fun = function(parameter1, parameter2) {
        let result = parameter1 % parameter2;
        return typeof(result);
    }
    var iter = fun(55, 10);
    assert_equal(iter,"number");
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
    assert_equal(iter,11);
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
    assert_equal(iter,5);
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
    assert_equal(iter,11);
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
    assert_equal(iter,9);
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
    assert_equal(iter,5);
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
    assert_equal(iter,66);
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
    assert_equal(iter,5.5);
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
    assert_equal(iter,"don");
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
    assert_equal(iter,20);
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
    assert_equal(iter,0);
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
    assert_equal(iter,1);
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
    assert_equal(iter,undefined);
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
      assert_equal(Module().f(),2);
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
      assert_equal(Module().f(),23);
}

/*
*  GetPropertyByValue();  
*/
                      /**************HandleLdobjbyvalueImm16V8****************/
{
    var obj = {name1:"bom", 2:"arm"};
    var v1 = obj.name1;
    var v3 = obj[2];
    var key = "name1";
    var v4 = obj.key;
    var v5 = obj[key];
    assert_equal(v1,"bom");
    assert_equal(v3,"arm");
    assert_equal(v4,undefined);
    assert_equal(v5,"bom");
}

/*
*  Int8ToTaggedInt(); SExtInt8ToInt64();  
*/
                      /**************HandleThrowIfsupernotcorrectcallPrefImm16****************/
{
    class A {
        constructor() {
        }
    }

    class B extends A {
        constructor(name, age) {
          super(); 
          this.name = name;
          this.age = age;
        }
        sayName() {
            assert_equal(this.name,"echo");
            assert_equal(this.age,19);
        }
    }
    new A();
    const people = new B("echo", 19);
    people.sayName();
}

/*
*  SetPropertyByName(); SetTypeArrayPropertyByName(); CallSetterHelper(); ShouldCallSetter(); IsWritable(); UpdateValueInDict();
*  AddPropertyByName(); SetHasConstructorCondition(); SetHasConstructorToHClass(); SetPropertyInlinedProps(); SetOffsetFieldInPropAttr();
*  SetIsInlinePropsFieldInPropAttr(); JSHClassAddProperty(); IsDictionaryMode(); SetPropertiesArray(); SetDictionaryOrderFieldInPropAttr();
*  ComputeNonInlinedFastPropsCapacity;
*/
                      /**************HandleStobjbynameImm8Id16V8****************/
{
    var obj1 = {"qq":10};
    var obj2={"mm":2,"nn":3};
    var obj3={"xx":1,"yy":5};
    obj1.field1 = obj2;
    obj1.field2 = obj3;
    var assertObj = {
    	"qq": 10,
    	"field1": {
    		"mm": 2,
    		"nn": 3
    		},
    	"field2": {
    		"xx": 1,
    		"yy": 5
    		}
    	}
    assert_equal(JSON.stringify(obj1),JSON.stringify(assertObj));

}

/**************HandleGetnextpropnameV8****************/
{
    var fun = function() {
        let array = [1, 8, 6, 9], arr;
        let expectedValues = [1, 8, 6, 9]; // 预期的输出值
        let index = 0;
        for (arr in array) {
            assert_equal(expectedValues[index],array[arr]);
            index++;
        };
        return undefined;
    }
    var iter = fun();
    assert_equal(iter,undefined);
}

/**************HandleDeprecatedTonumberPrefV8****************/
{
    function Module(stdlib, foreign, heap) {
        var x = +stdlib.x;
    }
    assert_equal(Module(this, { x:0 }),undefined);
}

/**************HandleIsinImm8V8****************/
{
    function Person() {
    };
    Person.prototype.name = "hello";
    Person.prototype.age = "12";
    Person.prototype.sayName = function(){alert(this.name);};
     
    function hasPrototypeProperty(object, name){
        return !object.hasOwnProperty(name) && (name in object);
    };
    var p1 = new Person();
    assert_equal(hasPrototypeProperty(p1,"name"),true);
    p1.name = "nn";
    assert_equal(hasPrototypeProperty(p1,"name"),false);
}

/**************HandleInstanceofImm8V8****************/
{
    function MyInstanceOf(left, right) {
        let proto = Object.getPrototypeOf(left);
        let prototype = right.prototype;
        while (true) {
            if (!proto) return false;
            if (proto === prototype) {
                return true;
            }
            proto = Object.getPrototypeOf(proto)
        }
    }

    class Person {
    }

    class Student extends Person{ 
    }

    let p1 = new Person();
    let stu1 = new Student();
    assert_equal(p1 instanceof Person,true);
    assert_equal(MyInstanceOf(p1, Person),true);
    assert_equal(stu1 instanceof Person,true);
    assert_equal(MyInstanceOf(stu1, Person),true);
    assert_equal(stu1 instanceof Student,true);
    assert_equal(MyInstanceOf(stu1, Student),true);
    assert_equal(p1 instanceof Student,false);
    assert_equal(MyInstanceOf(p1, Student),false);
}

/**************HandleThrowUndefinedifholePrefV8V8****************/
{
    var array = [11, 8, 19];

    function distinct(arr) {
        const obj = {}
        const result = arr.reduce((pre, cur) => {
        if (!obj[cur.stuId]) {
            obj[cur.stuId] = true;
            return [...pre, cur];
        }
        return pre;
    }, [])
        assert_equal(result[0],11);
        return result;
    }
    distinct(array);
}

/**************HandleThrowIfnotobjectPrefV8****************/
{
    var array = [168, 168, 19];

    function process(arr) {
        const cache = [];
        for (const t of arr) {
            if (cache.find(c => c.maxDeptCode === t.maxDeptCode && c.minDeptCode === t.minDeptCode)) {
                continue;
            }
            cache.push(t);
        }
 
        return cache;
    }
    assert_equal(process(array)[0],168);
}

/**************HandleCreateemptyobject****************/
{
    let str = 'baobfabobodabfodao';
    let num = str.split("").reduce((pre, cur) => {
        if (cur in pre) {
            pre[cur]++;
        } else {
            pre[cur] = 1;
        }
        return pre;
    }, {})
}

/**************HandleCreateemptyarrayImm8****************/
{
    let array1 = [
        { id: "50", active: "a", value: 12 },
        { id: "51", active: "a", value: 15 }
    ];
    let array2 = [
        { id: "50", active: "a", value: 10 },
        { id: "51", active: "a", value: 11 },
        { id: "52", active: "a", value: 13 }
    ];
    
    let arr = [];
    array1.map(id =>
        arr.push({
            id: id.id,
            newValue: array2.find(o => o.id === id.id).value + 2
        })
    );
    let expectVar4 = "2 "+JSON.stringify(arr);
    assert_equal(expectVar4, "2 [{\"id\":\"50\",\"newValue\":12},{\"id\":\"51\",\"newValue\":13}]");
}

/**************HandleGetnextpropnameV8****************/
{
    let obj1 = {
        name: "bob",
        age: 18,
        sex: "man",
      } 
    let obj2 = {}
    
    for (let key in obj1) {
      obj2[key] = obj1[key];
    }
    obj1.name = "Tom" 
    assert_equal(obj1.name,"Tom");
}

/**************HandleGetunmappedargs****************/
{
    function abs() {
        if (arguments.length === 0) {
            return 0;
        }
        var x = arguments[0];
        return x >= 0 ? x : -x;
    }
     
    assert_equal(abs(),0);
    assert_equal(abs(10),10);
    assert_equal(abs(-9),9); 
}

/**************HandleCopyrestargsImm8****************/
{
    function add(...theArgs) {
        theArgs.push(31);
        var result = 0; 
        for (let i = 0; i < theArgs.length; i++) {
            result = result + theArgs[i];
        }
        var lastItem = theArgs.pop();
        assert_equal(lastItem,31);
        return result;
    }

    add();
}

/**************HandleStarrayspreadV8V8****************/
{
    class Parent {
        static myMethod(msg) {
            let expectVar5 = "static 1"
            assert_equal(expectVar5,'static '+ msg);
        }
        myMethod(msg) {
            let expectVar5 = "instance 2"
            assert_equal(expectVar5,'instance '+ msg);
        }
    }

    class Child extends Parent {
        static myMethod(msg) {
            super.myMethod(msg);
        }
        myMethod(msg) {
            super.myMethod(msg);
        }
    }
    
    Child.myMethod(1);
    var child = new Child();
    child.myMethod(2);
}

/**************HandleSupercallspreadImm8V8****************/
{
    class Demo { 
        constructor(x) {
          this.x = x;
        }
   }
   
   class Demo2 extends Demo{}

   let d = new Demo2('hello');
   assert_equal(d.x,"hello");
}

/**************HandleDeprecatedLdsuperbynamePrefId32V8****************/
{
    class Demo {
        constructor(x,y) {
            this.x = x;
            this.y = y;
        }
        customSplit(){
            return [...this.y]
        }
    }

    class Demo2 extends Demo {
        constructor(x, y) {
            super(x, y);
        }
        customSplit(){
            return [...this.x]
        }
        task1(){
            return super.customSplit();
        }
        task2(){
            return this.customSplit();
        }
    }

    let d = new Demo2('hello','world');
    assert_equal(d.task1().toString(),"w,o,r,l,d");
    assert_equal(d.task2().toString(),"h,e,l,l,o");
}

/**************HandleCreateregexpwithliteralImm8Id16Imm8****************/
{
    function filterParams(obj) {
        let _newPar = {};
        for (let key in obj) {
            if ((obj[key] === 0 || obj[key]) && obj[key].toString().replace(/(^\s*)|(\s*$)/g, '') !== '') {
                _newPar[key] = obj[key];
            }
        }
        return _newPar;
    }
  var func = filterParams({a:"", b:null, c:"010", d:123});
  assert_equal(func.toString(),"[object Object]");
}

/**************HandleTryldglobalbynameImm8Id16****************/
{
    function randomNumber(n1, n2) {
        if (arguments.length === 2) {
            return Math.round(n1 + Math.random() * (n2 - n1));
        } else if (arguments.length === 1) {
            return Math.round(Math.random() * n1)
        } else {
            return Math.round(Math.random() * 255)
        }
    }

    var func = randomNumber(5, 10);
    let expectVar6 = "succ";
    assert_equal(expectVar6,"succ");
}

/**************HandleLdaStrId16****************/
{
    let promise = new Promise((resolve, reject) => {
        let expectVar7 = "Promise";
        assert_equal(expectVar7,'Promise');
        resolve();
    });  
    promise.then(function() {
    });  
}

/**************HandleDeprecatedCreatearraywithbufferPrefImm16****************/
{
    function Change() {
        var newArr = [];
        for (var i = arguments.length - 1; i >= 0; i--) {
            newArr[newArr.length] = arguments[i]
        }
        return newArr;
    }

    Change(1, 2345, 6, 8, 89, 5, 9);
    var arr = [1, 2345, 6, 8, 89, 5, 9];
    assert_equal(arr.reverse().toString(),"9,5,89,8,6,2345,1");
}

/**************HandleDeprecatedNegPrefV8****************/
{
    var arr = ['red', 'blue', 'red', 'green', 'pink', 'red', 'red'];
    var index = arr.indexOf('red');

    var number = 0;
    var indexArray = []; 
    while (index != -1) {
        indexArray.push(index);
        var index = arr.indexOf('red', index + 1);
        number++;
    }
    assert_equal(JSON.stringify(indexArray),JSON.stringify([0, 2, 5, 6]) );
    assert_equal(number, 4);
}

/**************HandleJeqzImm8****************/
{
    var str5 = 'abaaudffgggghhhhjkkkgfddduuuuuu23444343434';
        do {
            var index = str5.indexOf('s');
            str5 = str5.replace('s', 'u');
        } while (index !== -1)

    assert_equal(str5,"abaaudffgggghhhhjkkkgfddduuuuuu23444343434");
}

/**************HandleStobjbyvalueImm8V8V8****************/
{
    var str = "abcoefoxyozzopp";
    var o = {};
    for (var i = 0; i < str.length; i++) {
        var chars = str.charAt(i);
        if (o[chars]) {
            o[chars]++;
        } else {
            o[chars] = 1;
        }
    }
    var assertO = {
    	"a": 1,
    	"b": 1,
    	"c": 1,
    	"o": 4,
    	"e": 1,
    	"f": 1,
    	"x": 1,
    	"y": 1,
    	"z": 2,
    	"p": 2
        }
    assert_equal(JSON.stringify(o),JSON.stringify(assertO));
}

/**************HandleNoteqImm8V8****************/
{
    var arr = [2, 0, 6, 1, 77, 0, 52, 0, 25, 7];
    var newArr = [];
    for (var i = 0; i < arr.length; i++) {
        if (arr[i] != 0) {
            newArr[newArr.length] = arr[i];
        }
    }
    assert_equal(JSON.stringify(newArr),JSON.stringify([2,6,1,77,52,25,7]));
}

/**************HandleStricteqImm8V8****************/
{
    function unique(arr) {
        var newArr = []
        for (var i = 0; i < arr.length; i++) {
          if (newArr.indexOf(arr[i]) === -1) {
            newArr.push(arr[i])
          }
        }
        return newArr;
    }
    var demo = unique(['blue', 'green', 'blue']);
    assert_equal(demo.toString(),"blue,green");
}

/**************HandleDeprecatedCallthisrangePrefImm16V8****************/
{
   
    function countDown(time) {
        var nowTime = +new Date();
        var inputTime = +new Date(time);
        var times = (inputTime - nowTime) / 1000;
        var d = parseInt(times / 60 / 60 / 24);
        d = d < 10 ? '0' + d : d;
        var h = parseInt((times / 60 / 60) % 24);
        h = h < 10 ? '0' + h : h;
        var m = parseInt((times / 60) % 60);
        m = m < 10 ? '0' + m : m;
        var s = parseInt(times % 60);
        s = s < 10 ? '0' + s : s;
        return d + '天' + h + '时' + m + '分' + s + '秒';
    }
    (countDown('2022-10-10 18:00:00'));
    var date = new Date();
    var date1 = +new Date();
    assert_equal("blue","blue");
}

/**************HandleStownbyindexImm8V8Imm16****************/
{
    function doMath(data) {
        let min = data[0];
        let max = data[0];
        let sum = 0;
        data.sort(function(a, b) {
                return a - b;
        })
        min = data[0];
        max = data[data.length - 1];
        for (let i = 0; i < data.length; i++) {
            sum += data[i];
        }
        return [min, max, sum];
    }
    assert_equal(JSON.stringify(doMath([23, 54, 34, 2, 7, 8, 1, 77, 43])),JSON.stringify([1,77,249]));
}

/**************HandleWideNewobjrangePrefImm16V8****************/
{
    function Person(name, age) {
        this.name = name;
        this.age = age;
        this.say = function () {
            return "name : " + this.name + " age: " + this.age;
        };
    }

    var o = new Object();
    Person.call(o, "zhangsan", 20);
    assert_equal(o.say(),"name : zhangsan age: 20");
}

/**************HandleWideLdobjbyindexPrefImm32****************/
{
    function maxValue() {
        var max = arguments[0];
        for (var i = 0; i < arguments.length; i++) {
           if (max < arguments[i]) {
            max = arguments[i];
           }
        }
        return max;
    }
    assert_equal(maxValue(2, 4, 5, 9),9);
    assert_equal(maxValue(12, 4, 9),12);
}

/**************HandleDeprecatedLdobjbynamePrefId32V8****************/
{
    var obj = {};
    Object.defineProperty(obj, "x", {
    value : 1,
    writable : true,
    enumerable : true,
    configurable : true
    });
    assert_equal(obj.x,1);
}

/*
*    PtrAdd(); IntPtrSize();
*    DispatchWithId();
*/
                      /**************HandleThrow****************/
{
    var fun = function() {
        throw "boom";
    }
    try {
        var a = fun();
    } catch (e) {}
}

test_end();