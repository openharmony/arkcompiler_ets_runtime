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
//region block: polyfills
if (typeof ArrayBuffer.isView === 'undefined') {
  ArrayBuffer.isView = function (a) {
    return a != null && a.__proto__ != null && a.__proto__.__proto__ === Int8Array.prototype.__proto__;
  };
}
if (typeof Math.imul === 'undefined') {
  Math.imul = function imul(a, b) {
    return (a & 4.29490176E9) * (b & 65535) + (a & 65535) * (b | 0) | 0;
  };
}
//endregion
 var kntr = function (_) {
  'use strict';
  //region block: imports
  var imul = Math.imul;
  var isView = ArrayBuffer.isView;
  //endregion
  //region block: pre-declaration
  class CharSequence {}
  class Number_0 {
    constructor() {
    }
  }
  class Unit {
    constructor() {
    }
    toString() {
      return 'kotlin.Unit';
    }
  }
  class AbstractCollection {
    static new_kotlin_collections_AbstractCollection_ddhn3c_k$() {
      var $this = createThis(this);
      return $this;
    }
    isEmpty_y1axqb_k$() {
      return this.get_size_woubt6_k$() === 0;
    }
    toString() {
      return joinToString_0(this, ', ', '[', ']', VOID, VOID, AbstractCollection$toString$lambda(this));
    }
    toArray() {
      return collectionToArray(this);
    }
  }
  class AbstractMutableCollection extends AbstractCollection {
    static new_kotlin_collections_AbstractMutableCollection_jgoj1k_k$() {
      var $this = this.new_kotlin_collections_AbstractCollection_ddhn3c_k$();
      return $this;
    }
    toJSON() {
      return this.toArray();
    }
    checkIsMutable_h5js84_k$() {
    }
  }
  class IteratorImpl {
    constructor($outer) {
      this.$this_1 = $outer;
      this.index_1 = 0;
      this.last_1 = -1;
    }
    hasNext_bitz1p_k$() {
      return this.index_1 < this.$this_1.get_size_woubt6_k$();
    }
    next_20eer_k$() {
      if (!this.hasNext_bitz1p_k$())
        throw NoSuchElementException.new_kotlin_NoSuchElementException_wy3d4q_k$();
      var tmp = this;
      var tmp0_this = this;
      var tmp1 = tmp0_this.index_1;
      tmp0_this.index_1 = tmp1 + 1 | 0;
      tmp.last_1 = tmp1;
      return this.$this_1.get_c1px32_k$(this.last_1);
    }
  }
  class AbstractMutableList extends AbstractMutableCollection {
    static new_kotlin_collections_AbstractMutableList_ddn594_k$() {
      var $this = this.new_kotlin_collections_AbstractMutableCollection_jgoj1k_k$();
      $this.modCount_1 = 0;
      return $this;
    }
    add_utx5q5_k$(element) {
      this.checkIsMutable_h5js84_k$();
      this.add_dl6gt3_k$(this.get_size_woubt6_k$(), element);
      return true;
    }
    iterator_jk1svi_k$() {
      return new IteratorImpl(this);
    }
  }
  class Companion {
    constructor() {
      Companion_instance = this;
      var tmp = this;
      var tmp$ret$0;
      // Inline function 'kotlin.also' call
      var this_0 = ArrayList.new_kotlin_collections_ArrayList_tdd6ob_k$(0);
      // Inline function 'kotlin.contracts.contract' call
      // Inline function 'kotlin.collections.Companion.Empty.<anonymous>' call
      var it = this_0;
      it.isReadOnly_1 = true;
      tmp$ret$0 = this_0;
      tmp.Empty_1 = tmp$ret$0;
    }
  }
  class ArrayList extends AbstractMutableList {
    static new_kotlin_collections_ArrayList_ny19rj_k$(array) {
      Companion_getInstance();
      var $this = this.new_kotlin_collections_AbstractMutableList_ddn594_k$();
      $this.array_1 = array;
      $this.isReadOnly_1 = false;
      return $this;
    }
    static new_kotlin_collections_ArrayList_ony0vx_k$() {
      Companion_getInstance();
      var tmp$ret$0;
      // Inline function 'kotlin.emptyArray' call
      tmp$ret$0 = [];
      var $this = this.new_kotlin_collections_ArrayList_ny19rj_k$(tmp$ret$0);
      return $this;
    }
    static new_kotlin_collections_ArrayList_tdd6ob_k$(initialCapacity) {
      Companion_getInstance();
      var tmp$ret$0;
      // Inline function 'kotlin.emptyArray' call
      tmp$ret$0 = [];
      var $this = this.new_kotlin_collections_ArrayList_ny19rj_k$(tmp$ret$0);
      // Inline function 'kotlin.require' call
      var value = initialCapacity >= 0;
      // Inline function 'kotlin.contracts.contract' call
      if (!value) {
        var tmp$ret$1;
        // Inline function 'kotlin.collections.ArrayList.<init>.<anonymous>' call
        tmp$ret$1 = 'Negative initial capacity: ' + initialCapacity;
        var message = tmp$ret$1;
        throw IllegalArgumentException.new_kotlin_IllegalArgumentException_sfqr8_k$(toString_1(message));
      }
      return $this;
    }
    static new_kotlin_collections_ArrayList_603as_k$(elements) {
      Companion_getInstance();
      var tmp$ret$0;
      // Inline function 'kotlin.collections.toTypedArray' call
      var this_0 = elements;
      tmp$ret$0 = copyToArray(this_0);
      var $this = this.new_kotlin_collections_ArrayList_ny19rj_k$(tmp$ret$0);
      return $this;
    }
    get_size_woubt6_k$() {
      return this.array_1.length;
    }
    get_c1px32_k$(index) {
      var tmp = this.array_1[rangeCheck(this, index)];
      return (tmp == null ? true : !(tmp == null)) ? tmp : THROW_CCE();
    }
    add_utx5q5_k$(element) {
      this.checkIsMutable_h5js84_k$();
      var tmp$ret$0;
      // Inline function 'kotlin.js.asDynamic' call
      var this_0 = this.array_1;
      tmp$ret$0 = this_0;
      tmp$ret$0.push(element);
      var tmp0_this = this;
      var tmp1 = tmp0_this.modCount_1;
      tmp0_this.modCount_1 = tmp1 + 1 | 0;
      Unit_instance;
      return true;
    }
    add_dl6gt3_k$(index, element) {
      this.checkIsMutable_h5js84_k$();
      var tmp$ret$0;
      // Inline function 'kotlin.js.asDynamic' call
      var this_0 = this.array_1;
      tmp$ret$0 = this_0;
      tmp$ret$0.splice(insertionRangeCheck(this, index), 0, element);
      var tmp0_this = this;
      var tmp1 = tmp0_this.modCount_1;
      tmp0_this.modCount_1 = tmp1 + 1 | 0;
      Unit_instance;
    }
    toString() {
      return arrayToString(this.array_1);
    }
    toArray_jjyjqa_k$() {
      return [].slice.call(this.array_1);
    }
    toArray() {
      return this.toArray_jjyjqa_k$();
    }
    checkIsMutable_h5js84_k$() {
      if (this.isReadOnly_1)
        throw UnsupportedOperationException.new_kotlin_UnsupportedOperationException_cv3bvm_k$();
    }
  }
  class BaseOutput {
    constructor() {
    }
    println_uvj9r3_k$() {
      this.print_o1pwgy_k$('\n');
    }
    println_ghnc0w_k$(message) {
      this.print_o1pwgy_k$(message);
      this.println_uvj9r3_k$();
    }
  }
  class NodeJsOutput extends BaseOutput {
    constructor(outputStream) {
      super();
      this.outputStream_1 = outputStream;
    }
    print_o1pwgy_k$(message) {
      var tmp$ret$0;
      // Inline function 'kotlin.io.String' call
      var value = message;
      tmp$ret$0 = String(value);
      var messageString = tmp$ret$0;
      this.outputStream_1.write(messageString);
    }
  }
  class BufferedOutput extends BaseOutput {
    constructor() {
      super();
      this.buffer_1 = '';
    }
    print_o1pwgy_k$(message) {
      var tmp0_this = this;
      var tmp = tmp0_this;
      var tmp_0 = tmp0_this.buffer_1;
      var tmp$ret$0;
      // Inline function 'kotlin.io.String' call
      var value = message;
      tmp$ret$0 = String(value);
      tmp.buffer_1 = tmp_0 + tmp$ret$0;
    }
  }
  class BufferedOutputToConsoleLog extends BufferedOutput {
    constructor() {
      super();
    }
    print_o1pwgy_k$(message) {
      var tmp$ret$0;
      // Inline function 'kotlin.io.String' call
      var value = message;
      tmp$ret$0 = String(value);
      var s = tmp$ret$0;
      var tmp$ret$2;
      // Inline function 'kotlin.text.nativeLastIndexOf' call
      var this_0 = s;
      var str = '\n';
      var fromIndex = 0;
      var tmp$ret$1;
      // Inline function 'kotlin.js.asDynamic' call
      var this_1 = this_0;
      tmp$ret$1 = this_1;
      tmp$ret$2 = tmp$ret$1.lastIndexOf(str, fromIndex);
      var i = tmp$ret$2;
      if (i >= 0) {
        var tmp0_this = this;
        var tmp = tmp0_this;
        var tmp_0 = tmp0_this.buffer_1;
        var tmp$ret$4;
        // Inline function 'kotlin.text.substring' call
        var this_2 = s;
        var startIndex = 0;
        var endIndex = i;
        var tmp$ret$3;
        // Inline function 'kotlin.js.asDynamic' call
        var this_3 = this_2;
        tmp$ret$3 = this_3;
        tmp$ret$4 = tmp$ret$3.substring(startIndex, endIndex);
        tmp.buffer_1 = tmp_0 + tmp$ret$4;
        this.flush_shahbo_k$();
        var tmp$ret$6;
        // Inline function 'kotlin.text.substring' call
        var this_4 = s;
        var startIndex_0 = i + 1 | 0;
        var tmp$ret$5;
        // Inline function 'kotlin.js.asDynamic' call
        var this_5 = this_4;
        tmp$ret$5 = this_5;
        tmp$ret$6 = tmp$ret$5.substring(startIndex_0);
        s = tmp$ret$6;
      }
      var tmp1_this = this;
      tmp1_this.buffer_1 = tmp1_this.buffer_1 + s;
    }
    flush_shahbo_k$() {
      console.log(this.buffer_1);
      this.buffer_1 = '';
    }
  }
  class StringBuilder {
    static new_kotlin_text_StringBuilder_7at1nh_k$(content) {
      var $this = createThis(this);
      $this.string_1 = !(content === undefined) ? content : '';
      return $this;
    }
    static new_kotlin_text_StringBuilder_u46mrb_k$() {
      var $this = this.new_kotlin_text_StringBuilder_7at1nh_k$('');
      return $this;
    }
    append_t84oo1_k$(value) {
      var tmp0_this = this;
      tmp0_this.string_1 = tmp0_this.string_1 + toString(value);
      return this;
    }
    append_jgojdo_k$(value) {
      var tmp0_this = this;
      tmp0_this.string_1 = tmp0_this.string_1 + toString_0(value);
      return this;
    }
    toString() {
      return this.string_1;
    }
  }
  class Char {}
  class arrayIterator$1 {
    constructor($array) {
      this.$array_1 = $array;
      this.index_1 = 0;
    }
    hasNext_bitz1p_k$() {
      return !(this.index_1 === this.$array_1.length);
    }
    next_20eer_k$() {
      var tmp;
      if (!(this.index_1 === this.$array_1.length)) {
        var tmp0_this = this;
        var tmp1 = tmp0_this.index_1;
        tmp0_this.index_1 = tmp1 + 1 | 0;
        tmp = this.$array_1[tmp1];
      } else {
        throw NoSuchElementException.new_kotlin_NoSuchElementException_eborbh_k$('' + this.index_1);
      }
      return tmp;
    }
  }
  class Companion_0 {
    constructor() {
      Companion_instance_0 = this;
      this.MIN_VALUE_1 = new Long(0, -2147483648);
      this.MAX_VALUE_1 = new Long(-1, 2147483647);
      this.SIZE_BYTES_1 = 8;
      this.SIZE_BITS_1 = 64;
    }
  }
  class Long extends Number_0 {
    constructor(low, high) {
      Companion_getInstance_0();
      super();
      this.low_1 = low;
      this.high_1 = high;
    }
    plus_eydew_k$(other) {
      return add(this, other);
    }
    div_et91rl_k$(other) {
      return divide(this, other);
    }
    unaryMinus_6uz0qp_k$() {
      return this.inv_28kx_k$().plus_eydew_k$(new Long(1, 0));
    }
    inv_28kx_k$() {
      return new Long(~this.low_1, ~this.high_1);
    }
    toInt_1tsl84_k$() {
      return this.low_1;
    }
    toDouble_ygsx0s_k$() {
      return toNumber(this);
    }
    valueOf() {
      return this.toDouble_ygsx0s_k$();
    }
    toString() {
      return toStringImpl(this, 10);
    }
  }
  class Exception extends Error {
    static new_kotlin_Exception_f32mds_k$() {
      var $this = createThis(this);
      init_kotlin_Exception($this);
      Unit_instance;
      setPropertiesToThrowableInstance($this);
      return $this;
    }
    static new_kotlin_Exception_hsqbop_k$(message) {
      var $this = createThis(this);
      init_kotlin_Exception($this);
      Unit_instance;
      setPropertiesToThrowableInstance($this, message);
      return $this;
    }
  }
  class RuntimeException extends Exception {
    static new_kotlin_RuntimeException_29f9zq_k$() {
      var $this = this.new_kotlin_Exception_f32mds_k$();
      init_kotlin_RuntimeException($this);
      return $this;
    }
    static new_kotlin_RuntimeException_xu1s8h_k$(message) {
      var $this = this.new_kotlin_Exception_hsqbop_k$(message);
      init_kotlin_RuntimeException($this);
      return $this;
    }
  }
  class IllegalArgumentException extends RuntimeException {
    static new_kotlin_IllegalArgumentException_pv5o3f_k$() {
      var $this = this.new_kotlin_RuntimeException_29f9zq_k$();
      init_kotlin_IllegalArgumentException($this);
      return $this;
    }
    static new_kotlin_IllegalArgumentException_sfqr8_k$(message) {
      var $this = this.new_kotlin_RuntimeException_xu1s8h_k$(message);
      init_kotlin_IllegalArgumentException($this);
      return $this;
    }
  }
  class IndexOutOfBoundsException extends RuntimeException {
    static new_kotlin_IndexOutOfBoundsException_cc7xqw_k$() {
      var $this = this.new_kotlin_RuntimeException_29f9zq_k$();
      init_kotlin_IndexOutOfBoundsException($this);
      return $this;
    }
    static new_kotlin_IndexOutOfBoundsException_ddr8db_k$(message) {
      var $this = this.new_kotlin_RuntimeException_xu1s8h_k$(message);
      init_kotlin_IndexOutOfBoundsException($this);
      return $this;
    }
  }
  class IllegalStateException extends RuntimeException {
    static new_kotlin_IllegalStateException_1wtnp1_k$() {
      var $this = this.new_kotlin_RuntimeException_29f9zq_k$();
      init_kotlin_IllegalStateException($this);
      return $this;
    }
    static new_kotlin_IllegalStateException_w47ei6_k$(message) {
      var $this = this.new_kotlin_RuntimeException_xu1s8h_k$(message);
      init_kotlin_IllegalStateException($this);
      return $this;
    }
  }
  class UnsupportedOperationException extends RuntimeException {
    static new_kotlin_UnsupportedOperationException_cv3bvm_k$() {
      var $this = this.new_kotlin_RuntimeException_29f9zq_k$();
      init_kotlin_UnsupportedOperationException($this);
      return $this;
    }
  }
  class NoSuchElementException extends RuntimeException {
    static new_kotlin_NoSuchElementException_wy3d4q_k$() {
      var $this = this.new_kotlin_RuntimeException_29f9zq_k$();
      init_kotlin_NoSuchElementException($this);
      return $this;
    }
    static new_kotlin_NoSuchElementException_eborbh_k$(message) {
      var $this = this.new_kotlin_RuntimeException_xu1s8h_k$(message);
      init_kotlin_NoSuchElementException($this);
      return $this;
    }
  }
  class ClassCastException extends RuntimeException {
    static new_kotlin_ClassCastException_zhuhe1_k$() {
      var $this = this.new_kotlin_RuntimeException_29f9zq_k$();
      init_kotlin_ClassCastException($this);
      return $this;
    }
  }
  class Companion_1 {
    constructor() {
      this.maxArraySize_1 = 2147483639;
    }
    checkElementIndex_r3t1bm_k$(index, size) {
      if (index < 0 ? true : index >= size) {
        throw IndexOutOfBoundsException.new_kotlin_IndexOutOfBoundsException_ddr8db_k$('index: ' + index + ', size: ' + size);
      }
    }
    checkPositionIndex_xiph8v_k$(index, size) {
      if (index < 0 ? true : index > size) {
        throw IndexOutOfBoundsException.new_kotlin_IndexOutOfBoundsException_ddr8db_k$('index: ' + index + ', size: ' + size);
      }
    }
  }
  class ArrayAsCollection {
    constructor(values, isVarargs) {
      this.values_1 = values;
      this.isVarargs_1 = isVarargs;
    }
    get_size_woubt6_k$() {
      return this.values_1.length;
    }
    isEmpty_y1axqb_k$() {
      var tmp$ret$0;
      // Inline function 'kotlin.collections.isEmpty' call
      var this_0 = this.values_1;
      tmp$ret$0 = this_0.length === 0;
      return tmp$ret$0;
    }
    iterator_jk1svi_k$() {
      return arrayIterator(this.values_1);
    }
  }
  class EmptyList {
    constructor() {
      EmptyList_instance = this;
      this.serialVersionUID_1 = new Long(-1478467534, -1720727600);
    }
    toString() {
      return '[]';
    }
    get_size_woubt6_k$() {
      return 0;
    }
    isEmpty_y1axqb_k$() {
      return true;
    }
    iterator_jk1svi_k$() {
      return EmptyIterator_instance;
    }
  }
  class EmptyIterator {
    constructor() {
    }
    hasNext_bitz1p_k$() {
      return false;
    }
    next_20eer_k$() {
      throw NoSuchElementException.new_kotlin_NoSuchElementException_wy3d4q_k$();
    }
  }
  class Color {
    constructor(colorString, colorInt) {
      colorString = colorString === VOID ? null : colorString;
      colorInt = colorInt === VOID ? 0 : colorInt;
      this.colorString_1 = colorString;
      this.colorInt_1 = colorInt;
    }
  }
  class Apple {
    constructor(field1, field2, field3, field4, field5, field6, field7, field8, field9, field10, additionalField1, additionalField2, additionalField3, additionalField4, additionalField5, additionalField6, additionalField7, additionalField8, additionalField9, additionalField10) {
      this.field1__1 = field1;
      this.field2__1 = field2;
      this.field3__1 = field3;
      this.field4__1 = field4;
      this.field5__1 = field5;
      this.field6__1 = field6;
      this.field7__1 = field7;
      this.field8__1 = field8;
      this.field9__1 = field9;
      this.field10__1 = field10;
      this.additionalField1__1 = additionalField1;
      this.additionalField2__1 = additionalField2;
      this.additionalField3__1 = additionalField3;
      this.additionalField4__1 = additionalField4;
      this.additionalField5__1 = additionalField5;
      this.additionalField6__1 = additionalField6;
      this.additionalField7__1 = additionalField7;
      this.additionalField8__1 = additionalField8;
      this.additionalField9__1 = additionalField9;
      this.additionalField10__1 = additionalField10;
    }
    get_field2_dbuq9t_k$() {
      return this.field2__1;
    }
  }
  class Orange {
    constructor(field1, field2, field3, field4, field5, field6, field7, field8, field9, field10, additionalField1, additionalField2, additionalField3, additionalField4, additionalField5, additionalField6, additionalField7, additionalField8, additionalField9, additionalField10) {
      this.field1__1 = field1;
      this.field2__1 = field2;
      this.field3__1 = field3;
      this.field4__1 = field4;
      this.field5__1 = field5;
      this.field6__1 = field6;
      this.field7__1 = field7;
      this.field8__1 = field8;
      this.field9__1 = field9;
      this.field10__1 = field10;
      this.additionalField1__1 = additionalField1;
      this.additionalField2__1 = additionalField2;
      this.additionalField3__1 = additionalField3;
      this.additionalField4__1 = additionalField4;
      this.additionalField5__1 = additionalField5;
      this.additionalField6__1 = additionalField6;
      this.additionalField7__1 = additionalField7;
      this.additionalField8__1 = additionalField8;
      this.additionalField9__1 = additionalField9;
      this.additionalField10__1 = additionalField10;
    }
    get_field2_dbuq9t_k$() {
      return this.field2__1;
    }
  }
  class Grape {
    constructor(field1, field2, field3, field4, field5, field6, field7, field8, field9, field10, additionalField1, additionalField2, additionalField3, additionalField4, additionalField5, additionalField6, additionalField7, additionalField8, additionalField9, additionalField10) {
      this.field1__1 = field1;
      this.field2__1 = field2;
      this.field3__1 = field3;
      this.field4__1 = field4;
      this.field5__1 = field5;
      this.field6__1 = field6;
      this.field7__1 = field7;
      this.field8__1 = field8;
      this.field9__1 = field9;
      this.field10__1 = field10;
      this.additionalField1__1 = additionalField1;
      this.additionalField2__1 = additionalField2;
      this.additionalField3__1 = additionalField3;
      this.additionalField4__1 = additionalField4;
      this.additionalField5__1 = additionalField5;
      this.additionalField6__1 = additionalField6;
      this.additionalField7__1 = additionalField7;
      this.additionalField8__1 = additionalField8;
      this.additionalField9__1 = additionalField9;
      this.additionalField10__1 = additionalField10;
    }
    get_field2_dbuq9t_k$() {
      return this.field2__1;
    }
  }
  class Banana {
    constructor(field1, field2, field3, field4, field5, field6, field7, field8, field9, field10, additionalField1, additionalField2, additionalField3, additionalField4, additionalField5, additionalField6, additionalField7, additionalField8, additionalField9, additionalField10) {
      this.field1__1 = field1;
      this.field2__1 = field2;
      this.field3__1 = field3;
      this.field4__1 = field4;
      this.field5__1 = field5;
      this.field6__1 = field6;
      this.field7__1 = field7;
      this.field8__1 = field8;
      this.field9__1 = field9;
      this.field10__1 = field10;
      this.additionalField1__1 = additionalField1;
      this.additionalField2__1 = additionalField2;
      this.additionalField3__1 = additionalField3;
      this.additionalField4__1 = additionalField4;
      this.additionalField5__1 = additionalField5;
      this.additionalField6__1 = additionalField6;
      this.additionalField7__1 = additionalField7;
      this.additionalField8__1 = additionalField8;
      this.additionalField9__1 = additionalField9;
      this.additionalField10__1 = additionalField10;
    }
    get_field2_dbuq9t_k$() {
      return this.field2__1;
    }
  }
  //endregion
  var Unit_instance;
  function Unit_getInstance() {
    return Unit_instance;
  }
  function isNaN_0(_this__u8e3s4) {
    return !(_this__u8e3s4 === _this__u8e3s4);
  }
  function collectionToArray(collection) {
    return collectionToArrayCommonImpl(collection);
  }
  function copyToArray(collection) {
    var tmp;
    var tmp$ret$0;
    // Inline function 'kotlin.js.asDynamic' call
    var this_0 = collection;
    tmp$ret$0 = this_0;
    if (tmp$ret$0.toArray !== undefined) {
      var tmp$ret$2;
      // Inline function 'kotlin.js.unsafeCast' call
      var tmp$ret$1;
      // Inline function 'kotlin.js.asDynamic' call
      var this_1 = collection;
      tmp$ret$1 = this_1;
      var this_2 = tmp$ret$1.toArray();
      tmp$ret$2 = this_2;
      tmp = tmp$ret$2;
    } else {
      var tmp$ret$4;
      // Inline function 'kotlin.js.unsafeCast' call
      var this_3 = collectionToArray(collection);
      var tmp$ret$3;
      // Inline function 'kotlin.js.asDynamic' call
      var this_4 = this_3;
      tmp$ret$3 = this_4;
      tmp$ret$4 = tmp$ret$3;
      tmp = tmp$ret$4;
    }
    return tmp;
  }
  function listOf(element) {
    return arrayListOf([element]);
  }
  var Companion_instance;
  function Companion_getInstance() {
    if (Companion_instance === VOID)
      new Companion();
    return Companion_instance;
  }
  function rangeCheck($this, index) {
    var tmp$ret$0;
    // Inline function 'kotlin.apply' call
    var this_0 = index;
    // Inline function 'kotlin.contracts.contract' call
    // Inline function 'kotlin.collections.ArrayList.rangeCheck.<anonymous>' call
    var $this$apply = this_0;
    Companion_instance_1.checkElementIndex_r3t1bm_k$(index, $this.get_size_woubt6_k$());
    Unit_instance;
    tmp$ret$0 = this_0;
    return tmp$ret$0;
  }
  function insertionRangeCheck($this, index) {
    var tmp$ret$0;
    // Inline function 'kotlin.apply' call
    var this_0 = index;
    // Inline function 'kotlin.contracts.contract' call
    // Inline function 'kotlin.collections.ArrayList.insertionRangeCheck.<anonymous>' call
    var $this$apply = this_0;
    Companion_instance_1.checkPositionIndex_xiph8v_k$(index, $this.get_size_woubt6_k$());
    Unit_instance;
    tmp$ret$0 = this_0;
    return tmp$ret$0;
  }
  function get_output() {
    _init_properties_console_kt__rfg7jv();
    return output;
  }
  var output;
  function println(message) {
    _init_properties_console_kt__rfg7jv();
    get_output().println_ghnc0w_k$(message);
  }
  var properties_initialized_console_kt_gll9dl;
  function _init_properties_console_kt__rfg7jv() {
    if (properties_initialized_console_kt_gll9dl) {
    } else {
      properties_initialized_console_kt_gll9dl = true;
      var tmp$ret$1;
      // Inline function 'kotlin.run' call
      // Inline function 'kotlin.contracts.contract' call
      var tmp$ret$0;
      // Inline function 'kotlin.io.output.<anonymous>' call
      var isNode = typeof process !== 'undefined' && process.versions && !!process.versions.node;
      tmp$ret$0 = isNode ? new NodeJsOutput(process.stdout) : new BufferedOutputToConsoleLog();
      tmp$ret$1 = tmp$ret$0;
      output = tmp$ret$1;
    }
  }
  function joinToString(_this__u8e3s4, separator, prefix, postfix, limit, truncated, transform) {
    separator = separator === VOID ? ', ' : separator;
    prefix = prefix === VOID ? '' : prefix;
    postfix = postfix === VOID ? '' : postfix;
    limit = limit === VOID ? -1 : limit;
    truncated = truncated === VOID ? '...' : truncated;
    transform = transform === VOID ? null : transform;
    return joinTo(_this__u8e3s4, StringBuilder.new_kotlin_text_StringBuilder_u46mrb_k$(), separator, prefix, postfix, limit, truncated, transform).toString();
  }
  function joinTo(_this__u8e3s4, buffer, separator, prefix, postfix, limit, truncated, transform) {
    separator = separator === VOID ? ', ' : separator;
    prefix = prefix === VOID ? '' : prefix;
    postfix = postfix === VOID ? '' : postfix;
    limit = limit === VOID ? -1 : limit;
    truncated = truncated === VOID ? '...' : truncated;
    transform = transform === VOID ? null : transform;
    buffer.append_jgojdo_k$(prefix);
    Unit_instance;
    var count = 0;
    var indexedObject = _this__u8e3s4;
    var inductionVariable = 0;
    var last = indexedObject.length;
    $l$loop: while (inductionVariable < last) {
      var element = indexedObject[inductionVariable];
      inductionVariable = inductionVariable + 1 | 0;
      count = count + 1 | 0;
      if (count > 1) {
        buffer.append_jgojdo_k$(separator);
        Unit_instance;
      }
      if (limit < 0 ? true : count <= limit) {
        appendElement(buffer, element, transform);
        Unit_instance;
      } else
        break $l$loop;
    }
    if (limit >= 0 ? count > limit : false) {
      buffer.append_jgojdo_k$(truncated);
      Unit_instance;
    }
    buffer.append_jgojdo_k$(postfix);
    Unit_instance;
    return buffer;
  }
  function joinToString_0(_this__u8e3s4, separator, prefix, postfix, limit, truncated, transform) {
    separator = separator === VOID ? ', ' : separator;
    prefix = prefix === VOID ? '' : prefix;
    postfix = postfix === VOID ? '' : postfix;
    limit = limit === VOID ? -1 : limit;
    truncated = truncated === VOID ? '...' : truncated;
    transform = transform === VOID ? null : transform;
    return joinTo_0(_this__u8e3s4, StringBuilder.new_kotlin_text_StringBuilder_u46mrb_k$(), separator, prefix, postfix, limit, truncated, transform).toString();
  }
  function joinTo_0(_this__u8e3s4, buffer, separator, prefix, postfix, limit, truncated, transform) {
    separator = separator === VOID ? ', ' : separator;
    prefix = prefix === VOID ? '' : prefix;
    postfix = postfix === VOID ? '' : postfix;
    limit = limit === VOID ? -1 : limit;
    truncated = truncated === VOID ? '...' : truncated;
    transform = transform === VOID ? null : transform;
    buffer.append_jgojdo_k$(prefix);
    Unit_instance;
    var count = 0;
    var tmp0_iterator = _this__u8e3s4.iterator_jk1svi_k$();
    $l$loop: while (tmp0_iterator.hasNext_bitz1p_k$()) {
      var element = tmp0_iterator.next_20eer_k$();
      count = count + 1 | 0;
      if (count > 1) {
        buffer.append_jgojdo_k$(separator);
        Unit_instance;
      }
      if (limit < 0 ? true : count <= limit) {
        appendElement(buffer, element, transform);
        Unit_instance;
      } else
        break $l$loop;
    }
    if (limit >= 0 ? count > limit : false) {
      buffer.append_jgojdo_k$(truncated);
      Unit_instance;
    }
    buffer.append_jgojdo_k$(postfix);
    Unit_instance;
    return buffer;
  }
  function _get_value__a43j40($this) {
    return $this;
  }
  function toString($this) {
    var tmp$ret$0;
    // Inline function 'kotlin.js.unsafeCast' call
    var this_0 = String.fromCharCode(_get_value__a43j40($this));
    tmp$ret$0 = this_0;
    return tmp$ret$0;
  }
  function toString_0(_this__u8e3s4) {
    var tmp0_safe_receiver = _this__u8e3s4;
    var tmp1_elvis_lhs = tmp0_safe_receiver == null ? null : toString_1(tmp0_safe_receiver);
    return tmp1_elvis_lhs == null ? 'null' : tmp1_elvis_lhs;
  }
  function implement(interfaces) {
    var maxSize = 1;
    var masks = [];
    var indexedObject = interfaces;
    var inductionVariable = 0;
    var last = indexedObject.length;
    while (inductionVariable < last) {
      var i = indexedObject[inductionVariable];
      inductionVariable = inductionVariable + 1 | 0;
      var currentSize = maxSize;
      var tmp1_elvis_lhs = i.prototype.$imask$;
      var imask = tmp1_elvis_lhs == null ? i.$imask$ : tmp1_elvis_lhs;
      if (!(imask == null)) {
        masks.push(imask);
        currentSize = imask.length;
      }
      var iid = i.$metadata$.iid;
      var tmp2_safe_receiver = iid;
      var tmp;
      if (tmp2_safe_receiver == null) {
        tmp = null;
      } else {
        var tmp$ret$1;
        // Inline function 'kotlin.let' call
        var this_0 = tmp2_safe_receiver;
        // Inline function 'kotlin.contracts.contract' call
        var tmp$ret$0;
        // Inline function 'kotlin.js.implement.<anonymous>' call
        var it = this_0;
        tmp$ret$0 = bitMaskWith(it);
        tmp$ret$1 = tmp$ret$0;
        tmp = tmp$ret$1;
      }
      var iidImask = tmp;
      if (!(iidImask == null)) {
        masks.push(iidImask);
        currentSize = Math.max(currentSize, iidImask.length);
      }
      if (currentSize > maxSize) {
        maxSize = currentSize;
      }
    }
    return compositeBitMask(maxSize, masks);
  }
  function bitMaskWith(activeBit) {
    var numberIndex = activeBit >> 5;
    var intArray = new Int32Array(numberIndex + 1 | 0);
    var positionInNumber = activeBit & 31;
    var numberWithSettledBit = 1 << positionInNumber;
    intArray[numberIndex] = intArray[numberIndex] | numberWithSettledBit;
    return intArray;
  }
  function compositeBitMask(capacity, masks) {
    var tmp = 0;
    var tmp_0 = capacity;
    var tmp_1 = new Int32Array(tmp_0);
    while (tmp < tmp_0) {
      var tmp_2 = tmp;
      var tmp$ret$0;
      var result = 0;
      var indexedObject = masks;
      var inductionVariable = 0;
      var last = indexedObject.length;
      while (inductionVariable < last) {
        var mask = indexedObject[inductionVariable];
        inductionVariable = inductionVariable + 1 | 0;
        if (tmp_2 < mask.length) {
          result = result | mask[tmp_2];
        }
      }
      tmp$ret$0 = result;
      tmp_1[tmp_2] = tmp$ret$0;
      tmp = tmp + 1 | 0;
    }
    return tmp_1;
  }
  function isBitSet(_this__u8e3s4, possibleActiveBit) {
    var numberIndex = possibleActiveBit >> 5;
    if (numberIndex > _this__u8e3s4.length)
      return false;
    var positionInNumber = possibleActiveBit & 31;
    var numberWithSettledBit = 1 << positionInNumber;
    return !((_this__u8e3s4[numberIndex] & numberWithSettledBit) === 0);
  }
  function fillArrayVal(array, initValue) {
    var inductionVariable = 0;
    var last = array.length - 1 | 0;
    if (inductionVariable <= last)
      do {
        var i = inductionVariable;
        inductionVariable = inductionVariable + 1 | 0;
        array[i] = initValue;
        Unit_instance;
      }
       while (!(i === last));
    return array;
  }
  function arrayIterator(array) {
    return new arrayIterator$1(array);
  }
  function arrayToString(array) {
    return joinToString(array, ', ', '[', ']', VOID, VOID, arrayToString$lambda);
  }
  function arrayToString$lambda(it) {
    return toString_1(it);
  }
  function toString_1(o) {
    var tmp;
    if (o == null) {
      tmp = 'null';
    } else if (isArrayish(o)) {
      tmp = '[...]';
    } else if (!(typeof o.toString === 'function')) {
      tmp = anyToString(o);
    } else {
      var tmp$ret$0;
      // Inline function 'kotlin.js.unsafeCast' call
      var this_0 = o.toString();
      tmp$ret$0 = this_0;
      tmp = tmp$ret$0;
    }
    return tmp;
  }
  function anyToString(o) {
    return Object.prototype.toString.call(o);
  }
  function unboxIntrinsic(x) {
    var message = 'Should be lowered';
    throw IllegalStateException.new_kotlin_IllegalStateException_w47ei6_k$(toString_1(message));
  }
  function captureStack(instance, constructorFunction) {
    if (Error.captureStackTrace != null) {
      Error.captureStackTrace(instance, constructorFunction);
    } else {
      var tmp$ret$0;
      // Inline function 'kotlin.js.asDynamic' call
      var this_0 = instance;
      tmp$ret$0 = this_0;
      tmp$ret$0.stack = (new Error()).stack;
    }
  }
  function protoOf(constructor) {
    return constructor.prototype;
  }
  function defineProp(obj, name, getter, setter) {
    return Object.defineProperty(obj, name, {configurable: true, get: getter, set: setter});
  }
  function createThis(ctor, box) {
    var self_0 = Object.create(ctor.prototype);
    boxApply(self_0, box);
    return self_0;
  }
  function boxApply(self_0, box) {
    if (box !== VOID)
      Object.assign(self_0, box);
  }
  function setPropertiesToThrowableInstance(this_, message, cause) {
    var errorInfo = calculateErrorInfo(Object.getPrototypeOf(this_));
    if ((errorInfo & 1) === 0) {
      var tmp;
      if (message == null) {
        var tmp_0;
        if (!(message === null)) {
          var tmp0_safe_receiver = cause;
          var tmp1_elvis_lhs = tmp0_safe_receiver == null ? null : tmp0_safe_receiver.toString();
          tmp_0 = tmp1_elvis_lhs == null ? VOID : tmp1_elvis_lhs;
        } else {
          tmp_0 = VOID;
        }
        tmp = tmp_0;
      } else {
        tmp = message;
      }
      this_.message = tmp;
    }
    if ((errorInfo & 2) === 0) {
      this_.cause = cause;
    }
    this_.name = Object.getPrototypeOf(this_).constructor.name;
  }
  function THROW_CCE() {
    throw ClassCastException.new_kotlin_ClassCastException_zhuhe1_k$();
  }
  var Companion_instance_0;
  function Companion_getInstance_0() {
    if (Companion_instance_0 === VOID)
      new Companion_0();
    return Companion_instance_0;
  }
  function get_ZERO() {
    _init_properties_longjs_kt__tqrzid();
    return ZERO;
  }
  var ZERO;
  function get_ONE() {
    _init_properties_longjs_kt__tqrzid();
    return ONE;
  }
  var ONE;
  function get_NEG_ONE() {
    _init_properties_longjs_kt__tqrzid();
    return NEG_ONE;
  }
  var NEG_ONE;
  function get_MAX_VALUE() {
    _init_properties_longjs_kt__tqrzid();
    return MAX_VALUE;
  }
  var MAX_VALUE;
  function get_MIN_VALUE() {
    _init_properties_longjs_kt__tqrzid();
    return MIN_VALUE;
  }
  var MIN_VALUE;
  function get_TWO_PWR_24_() {
    _init_properties_longjs_kt__tqrzid();
    return TWO_PWR_24_;
  }
  var TWO_PWR_24_;
  function compare(_this__u8e3s4, other) {
    _init_properties_longjs_kt__tqrzid();
    if (equalsLong(_this__u8e3s4, other)) {
      return 0;
    }
    var thisNeg = isNegative(_this__u8e3s4);
    var otherNeg = isNegative(other);
    return (thisNeg ? !otherNeg : false) ? -1 : (!thisNeg ? otherNeg : false) ? 1 : isNegative(subtract(_this__u8e3s4, other)) ? -1 : 1;
  }
  function add(_this__u8e3s4, other) {
    _init_properties_longjs_kt__tqrzid();
    var a48 = _this__u8e3s4.high_1 >>> 16 | 0;
    var a32 = _this__u8e3s4.high_1 & 65535;
    var a16 = _this__u8e3s4.low_1 >>> 16 | 0;
    var a00 = _this__u8e3s4.low_1 & 65535;
    var b48 = other.high_1 >>> 16 | 0;
    var b32 = other.high_1 & 65535;
    var b16 = other.low_1 >>> 16 | 0;
    var b00 = other.low_1 & 65535;
    var c48 = 0;
    var c32 = 0;
    var c16 = 0;
    var c00 = 0;
    c00 = c00 + (a00 + b00 | 0) | 0;
    c16 = c16 + (c00 >>> 16 | 0) | 0;
    c00 = c00 & 65535;
    c16 = c16 + (a16 + b16 | 0) | 0;
    c32 = c32 + (c16 >>> 16 | 0) | 0;
    c16 = c16 & 65535;
    c32 = c32 + (a32 + b32 | 0) | 0;
    c48 = c48 + (c32 >>> 16 | 0) | 0;
    c32 = c32 & 65535;
    c48 = c48 + (a48 + b48 | 0) | 0;
    c48 = c48 & 65535;
    return new Long(c16 << 16 | c00, c48 << 16 | c32);
  }
  function subtract(_this__u8e3s4, other) {
    _init_properties_longjs_kt__tqrzid();
    return add(_this__u8e3s4, other.unaryMinus_6uz0qp_k$());
  }
  function multiply(_this__u8e3s4, other) {
    _init_properties_longjs_kt__tqrzid();
    if (isZero(_this__u8e3s4)) {
      return get_ZERO();
    } else if (isZero(other)) {
      return get_ZERO();
    }
    if (equalsLong(_this__u8e3s4, get_MIN_VALUE())) {
      return isOdd(other) ? get_MIN_VALUE() : get_ZERO();
    } else if (equalsLong(other, get_MIN_VALUE())) {
      return isOdd(_this__u8e3s4) ? get_MIN_VALUE() : get_ZERO();
    }
    if (isNegative(_this__u8e3s4)) {
      var tmp;
      if (isNegative(other)) {
        tmp = multiply(negate(_this__u8e3s4), negate(other));
      } else {
        tmp = negate(multiply(negate(_this__u8e3s4), other));
      }
      return tmp;
    } else if (isNegative(other)) {
      return negate(multiply(_this__u8e3s4, negate(other)));
    }
    if (lessThan(_this__u8e3s4, get_TWO_PWR_24_()) ? lessThan(other, get_TWO_PWR_24_()) : false) {
      return fromNumber(toNumber(_this__u8e3s4) * toNumber(other));
    }
    var a48 = _this__u8e3s4.high_1 >>> 16 | 0;
    var a32 = _this__u8e3s4.high_1 & 65535;
    var a16 = _this__u8e3s4.low_1 >>> 16 | 0;
    var a00 = _this__u8e3s4.low_1 & 65535;
    var b48 = other.high_1 >>> 16 | 0;
    var b32 = other.high_1 & 65535;
    var b16 = other.low_1 >>> 16 | 0;
    var b00 = other.low_1 & 65535;
    var c48 = 0;
    var c32 = 0;
    var c16 = 0;
    var c00 = 0;
    c00 = c00 + imul(a00, b00) | 0;
    c16 = c16 + (c00 >>> 16 | 0) | 0;
    c00 = c00 & 65535;
    c16 = c16 + imul(a16, b00) | 0;
    c32 = c32 + (c16 >>> 16 | 0) | 0;
    c16 = c16 & 65535;
    c16 = c16 + imul(a00, b16) | 0;
    c32 = c32 + (c16 >>> 16 | 0) | 0;
    c16 = c16 & 65535;
    c32 = c32 + imul(a32, b00) | 0;
    c48 = c48 + (c32 >>> 16 | 0) | 0;
    c32 = c32 & 65535;
    c32 = c32 + imul(a16, b16) | 0;
    c48 = c48 + (c32 >>> 16 | 0) | 0;
    c32 = c32 & 65535;
    c32 = c32 + imul(a00, b32) | 0;
    c48 = c48 + (c32 >>> 16 | 0) | 0;
    c32 = c32 & 65535;
    c48 = c48 + (((imul(a48, b00) + imul(a32, b16) | 0) + imul(a16, b32) | 0) + imul(a00, b48) | 0) | 0;
    c48 = c48 & 65535;
    return new Long(c16 << 16 | c00, c48 << 16 | c32);
  }
  function divide(_this__u8e3s4, other) {
    _init_properties_longjs_kt__tqrzid();
    if (isZero(other)) {
      throw Exception.new_kotlin_Exception_hsqbop_k$('division by zero');
    } else if (isZero(_this__u8e3s4)) {
      return get_ZERO();
    }
    if (equalsLong(_this__u8e3s4, get_MIN_VALUE())) {
      if (equalsLong(other, get_ONE()) ? true : equalsLong(other, get_NEG_ONE())) {
        return get_MIN_VALUE();
      } else if (equalsLong(other, get_MIN_VALUE())) {
        return get_ONE();
      } else {
        var halfThis = shiftRight(_this__u8e3s4, 1);
        var approx = shiftLeft(halfThis.div_et91rl_k$(other), 1);
        if (equalsLong(approx, get_ZERO())) {
          return isNegative(other) ? get_ONE() : get_NEG_ONE();
        } else {
          var rem = subtract(_this__u8e3s4, multiply(other, approx));
          return add(approx, rem.div_et91rl_k$(other));
        }
      }
    } else if (equalsLong(other, get_MIN_VALUE())) {
      return get_ZERO();
    }
    if (isNegative(_this__u8e3s4)) {
      var tmp;
      if (isNegative(other)) {
        tmp = negate(_this__u8e3s4).div_et91rl_k$(negate(other));
      } else {
        tmp = negate(negate(_this__u8e3s4).div_et91rl_k$(other));
      }
      return tmp;
    } else if (isNegative(other)) {
      return negate(_this__u8e3s4.div_et91rl_k$(negate(other)));
    }
    var res = get_ZERO();
    var rem_0 = _this__u8e3s4;
    while (greaterThanOrEqual(rem_0, other)) {
      var approxDouble = toNumber(rem_0) / toNumber(other);
      var approx2 = Math.max(1.0, Math.floor(approxDouble));
      var log2 = Math.ceil(Math.log(approx2) / Math.LN2);
      var delta = log2 <= 48.0 ? 1.0 : Math.pow(2.0, log2 - 48);
      var approxRes = fromNumber(approx2);
      var approxRem = multiply(approxRes, other);
      while (isNegative(approxRem) ? true : greaterThan(approxRem, rem_0)) {
        approx2 = approx2 - delta;
        approxRes = fromNumber(approx2);
        approxRem = multiply(approxRes, other);
      }
      if (isZero(approxRes)) {
        approxRes = get_ONE();
      }
      res = add(res, approxRes);
      rem_0 = subtract(rem_0, approxRem);
    }
    return res;
  }
  function shiftLeft(_this__u8e3s4, numBits) {
    _init_properties_longjs_kt__tqrzid();
    var numBits_0 = numBits & 63;
    if (numBits_0 === 0) {
      return _this__u8e3s4;
    } else {
      if (numBits_0 < 32) {
        return new Long(_this__u8e3s4.low_1 << numBits_0, _this__u8e3s4.high_1 << numBits_0 | (_this__u8e3s4.low_1 >>> (32 - numBits_0 | 0) | 0));
      } else {
        return new Long(0, _this__u8e3s4.low_1 << (numBits_0 - 32 | 0));
      }
    }
  }
  function shiftRight(_this__u8e3s4, numBits) {
    _init_properties_longjs_kt__tqrzid();
    var numBits_0 = numBits & 63;
    if (numBits_0 === 0) {
      return _this__u8e3s4;
    } else {
      if (numBits_0 < 32) {
        return new Long(_this__u8e3s4.low_1 >>> numBits_0 | 0 | _this__u8e3s4.high_1 << (32 - numBits_0 | 0), _this__u8e3s4.high_1 >> numBits_0);
      } else {
        return new Long(_this__u8e3s4.high_1 >> (numBits_0 - 32 | 0), _this__u8e3s4.high_1 >= 0 ? 0 : -1);
      }
    }
  }
  function toNumber(_this__u8e3s4) {
    _init_properties_longjs_kt__tqrzid();
    return _this__u8e3s4.high_1 * 4.294967296E9 + getLowBitsUnsigned(_this__u8e3s4);
  }
  function equalsLong(_this__u8e3s4, other) {
    _init_properties_longjs_kt__tqrzid();
    return _this__u8e3s4.high_1 === other.high_1 ? _this__u8e3s4.low_1 === other.low_1 : false;
  }
  function toStringImpl(_this__u8e3s4, radix) {
    _init_properties_longjs_kt__tqrzid();
    if (radix < 2 ? true : 36 < radix) {
      throw Exception.new_kotlin_Exception_hsqbop_k$('radix out of range: ' + radix);
    }
    if (isZero(_this__u8e3s4)) {
      return '0';
    }
    if (isNegative(_this__u8e3s4)) {
      if (equalsLong(_this__u8e3s4, get_MIN_VALUE())) {
        var radixLong = fromInt(radix);
        var div = _this__u8e3s4.div_et91rl_k$(radixLong);
        var rem = subtract(multiply(div, radixLong), _this__u8e3s4).toInt_1tsl84_k$();
        var tmp = toStringImpl(div, radix);
        var tmp$ret$1;
        // Inline function 'kotlin.js.unsafeCast' call
        var tmp$ret$0;
        // Inline function 'kotlin.js.asDynamic' call
        var this_0 = rem;
        tmp$ret$0 = this_0;
        var this_1 = tmp$ret$0.toString(radix);
        tmp$ret$1 = this_1;
        return tmp + tmp$ret$1;
      } else {
        return '-' + toStringImpl(negate(_this__u8e3s4), radix);
      }
    }
    var digitsPerTime = radix === 2 ? 31 : radix <= 10 ? 9 : radix <= 21 ? 7 : radix <= 35 ? 6 : 5;
    var radixToPower = fromNumber(Math.pow(radix, digitsPerTime));
    var rem_0 = _this__u8e3s4;
    var result = '';
    while (true) {
      var remDiv = rem_0.div_et91rl_k$(radixToPower);
      var intval = subtract(rem_0, multiply(remDiv, radixToPower)).toInt_1tsl84_k$();
      var tmp$ret$3;
      // Inline function 'kotlin.js.unsafeCast' call
      var tmp$ret$2;
      // Inline function 'kotlin.js.asDynamic' call
      var this_2 = intval;
      tmp$ret$2 = this_2;
      var this_3 = tmp$ret$2.toString(radix);
      tmp$ret$3 = this_3;
      var digits = tmp$ret$3;
      rem_0 = remDiv;
      if (isZero(rem_0)) {
        return digits + result;
      } else {
        while (digits.length < digitsPerTime) {
          digits = '0' + digits;
        }
        result = digits + result;
      }
    }
  }
  function fromInt(value) {
    _init_properties_longjs_kt__tqrzid();
    return new Long(value, value < 0 ? -1 : 0);
  }
  function isNegative(_this__u8e3s4) {
    _init_properties_longjs_kt__tqrzid();
    return _this__u8e3s4.high_1 < 0;
  }
  function isZero(_this__u8e3s4) {
    _init_properties_longjs_kt__tqrzid();
    return _this__u8e3s4.high_1 === 0 ? _this__u8e3s4.low_1 === 0 : false;
  }
  function isOdd(_this__u8e3s4) {
    _init_properties_longjs_kt__tqrzid();
    return (_this__u8e3s4.low_1 & 1) === 1;
  }
  function negate(_this__u8e3s4) {
    _init_properties_longjs_kt__tqrzid();
    return _this__u8e3s4.unaryMinus_6uz0qp_k$();
  }
  function lessThan(_this__u8e3s4, other) {
    _init_properties_longjs_kt__tqrzid();
    return compare(_this__u8e3s4, other) < 0;
  }
  function fromNumber(value) {
    _init_properties_longjs_kt__tqrzid();
    if (isNaN_0(value)) {
      return get_ZERO();
    } else if (value <= -9.223372036854776E18) {
      return get_MIN_VALUE();
    } else if (value + 1 >= 9.223372036854776E18) {
      return get_MAX_VALUE();
    } else if (value < 0.0) {
      return negate(fromNumber(-value));
    } else {
      var twoPwr32 = 4.294967296E9;
      var tmp$ret$0;
      // Inline function 'kotlin.js.jsBitwiseOr' call
      var lhs = value % twoPwr32;
      var rhs = 0;
      tmp$ret$0 = lhs | rhs;
      var tmp = tmp$ret$0;
      var tmp$ret$1;
      // Inline function 'kotlin.js.jsBitwiseOr' call
      var lhs_0 = value / twoPwr32;
      var rhs_0 = 0;
      tmp$ret$1 = lhs_0 | rhs_0;
      return new Long(tmp, tmp$ret$1);
    }
  }
  function greaterThan(_this__u8e3s4, other) {
    _init_properties_longjs_kt__tqrzid();
    return compare(_this__u8e3s4, other) > 0;
  }
  function greaterThanOrEqual(_this__u8e3s4, other) {
    _init_properties_longjs_kt__tqrzid();
    return compare(_this__u8e3s4, other) >= 0;
  }
  function getLowBitsUnsigned(_this__u8e3s4) {
    _init_properties_longjs_kt__tqrzid();
    return _this__u8e3s4.low_1 >= 0 ? _this__u8e3s4.low_1 : 4.294967296E9 + _this__u8e3s4.low_1;
  }
  var properties_initialized_longjs_kt_5aju7t;
  function _init_properties_longjs_kt__tqrzid() {
    if (properties_initialized_longjs_kt_5aju7t) {
    } else {
      properties_initialized_longjs_kt_5aju7t = true;
      ZERO = fromInt(0);
      ONE = fromInt(1);
      NEG_ONE = fromInt(-1);
      MAX_VALUE = new Long(-1, 2147483647);
      MIN_VALUE = new Long(0, -2147483648);
      TWO_PWR_24_ = fromInt(16777216);
    }
  }
  function classMeta(name, defaultConstructor, associatedObjectKey, associatedObjects, suspendArity) {
    return createMetadata('class', name, defaultConstructor, associatedObjectKey, associatedObjects, suspendArity, null);
  }
  function createMetadata(kind, name, defaultConstructor, associatedObjectKey, associatedObjects, suspendArity, iid) {
    var undef = VOID;
    return {kind: kind, simpleName: name, associatedObjectKey: associatedObjectKey, associatedObjects: associatedObjects, suspendArity: suspendArity, $kClass$: undef, defaultConstructor: defaultConstructor, iid: iid};
  }
  function setMetadataFor(ctor, name, metadataConstructor, parent, interfaces, defaultConstructor, associatedObjectKey, associatedObjects, suspendArity) {
    if (!(parent == null)) {
      ctor.prototype = Object.create(parent.prototype);
      ctor.prototype.constructor = ctor;
    }
    var tmp0_elvis_lhs = suspendArity;
    var metadata = metadataConstructor(name, defaultConstructor, associatedObjectKey, associatedObjects, tmp0_elvis_lhs == null ? [] : tmp0_elvis_lhs);
    ctor.$metadata$ = metadata;
    if (!(interfaces == null)) {
      var receiver = !(metadata.iid == null) ? ctor : ctor.prototype;
      receiver.$imask$ = implement(interfaces);
    }
  }
  function interfaceMeta(name, defaultConstructor, associatedObjectKey, associatedObjects, suspendArity) {
    return createMetadata('interface', name, defaultConstructor, associatedObjectKey, associatedObjects, suspendArity, generateInterfaceId());
  }
  function generateInterfaceId() {
    if (iid === VOID) {
      iid = 0;
    }
    var tmp$ret$0;
    // Inline function 'kotlin.js.unsafeCast' call
    var this_0 = iid;
    tmp$ret$0 = this_0;
    iid = tmp$ret$0 + 1 | 0;
    var tmp$ret$1;
    // Inline function 'kotlin.js.unsafeCast' call
    var this_1 = iid;
    tmp$ret$1 = this_1;
    return tmp$ret$1;
  }
  var iid;
  function objectMeta(name, defaultConstructor, associatedObjectKey, associatedObjects, suspendArity) {
    return createMetadata('object', name, defaultConstructor, associatedObjectKey, associatedObjects, suspendArity, null);
  }
  function isArrayish(o) {
    return isJsArray(o) ? true : isView(o);
  }
  function isJsArray(obj) {
    var tmp$ret$0;
    // Inline function 'kotlin.js.unsafeCast' call
    var this_0 = Array.isArray(obj);
    tmp$ret$0 = this_0;
    return tmp$ret$0;
  }
  function isInterface(obj, iface) {
    return isInterfaceImpl(obj, iface.$metadata$.iid);
  }
  function isInterfaceImpl(obj, iface) {
    var tmp$ret$0;
    // Inline function 'kotlin.js.unsafeCast' call
    var this_0 = obj.$imask$;
    tmp$ret$0 = this_0;
    var tmp0_elvis_lhs = tmp$ret$0;
    var tmp;
    if (tmp0_elvis_lhs == null) {
      return false;
    } else {
      tmp = tmp0_elvis_lhs;
    }
    var mask = tmp;
    return isBitSet(mask, iface);
  }
  function isCharSequence(value) {
    return typeof value === 'string' ? true : isInterface(value, CharSequence);
  }
  function calculateErrorInfo(proto) {
    var tmp0_safe_receiver = proto.constructor;
    var metadata = tmp0_safe_receiver == null ? null : tmp0_safe_receiver.$metadata$;
    var tmp1_safe_receiver = metadata;
    var tmp2_safe_receiver = tmp1_safe_receiver == null ? null : tmp1_safe_receiver.errorInfo;
    if (tmp2_safe_receiver == null)
      null;
    else {
      var tmp$ret$0;
      // Inline function 'kotlin.let' call
      var this_0 = tmp2_safe_receiver;
      // Inline function 'kotlin.contracts.contract' call
      var it = this_0;
      return it;
    }
    Unit_instance;
    var result = 0;
    if (hasProp(proto, 'message'))
      result = result | 1;
    if (hasProp(proto, 'cause'))
      result = result | 2;
    if (!(result === 3)) {
      var parentProto = getPrototypeOf(proto);
      if (parentProto != Error.prototype) {
        result = result | calculateErrorInfo(parentProto);
      }
    }
    if (!(metadata == null)) {
      metadata.errorInfo = result;
      Unit_instance;
    }
    return result;
  }
  function hasProp(proto, propName) {
    return proto.hasOwnProperty(propName);
  }
  function getPrototypeOf(obj) {
    return Object.getPrototypeOf(obj);
  }
  function get_VOID() {
    _init_properties_void_kt__3zg9as();
    return VOID;
  }
  var VOID;
  var properties_initialized_void_kt_e4ret2;
  function _init_properties_void_kt__3zg9as() {
    if (properties_initialized_void_kt_e4ret2) {
    } else {
      properties_initialized_void_kt_e4ret2 = true;
      VOID = void 0;
    }
  }
  function asList(_this__u8e3s4) {
    var tmp$ret$1;
    // Inline function 'kotlin.js.unsafeCast' call
    var this_0 = _this__u8e3s4;
    var tmp$ret$0;
    // Inline function 'kotlin.js.asDynamic' call
    var this_1 = this_0;
    tmp$ret$0 = this_1;
    tmp$ret$1 = tmp$ret$0;
    return ArrayList.new_kotlin_collections_ArrayList_ny19rj_k$(tmp$ret$1);
  }
  function init_kotlin_Exception(_this__u8e3s4) {
    captureStack(_this__u8e3s4, _this__u8e3s4.$throwableCtor_1);
  }
  function init_kotlin_IllegalArgumentException(_this__u8e3s4) {
    captureStack(_this__u8e3s4, _this__u8e3s4.$throwableCtor_3);
  }
  function init_kotlin_IndexOutOfBoundsException(_this__u8e3s4) {
    captureStack(_this__u8e3s4, _this__u8e3s4.$throwableCtor_3);
  }
  function init_kotlin_IllegalStateException(_this__u8e3s4) {
    captureStack(_this__u8e3s4, _this__u8e3s4.$throwableCtor_3);
  }
  function init_kotlin_UnsupportedOperationException(_this__u8e3s4) {
    captureStack(_this__u8e3s4, _this__u8e3s4.$throwableCtor_3);
  }
  function init_kotlin_RuntimeException(_this__u8e3s4) {
    captureStack(_this__u8e3s4, _this__u8e3s4.$throwableCtor_2);
  }
  function init_kotlin_NoSuchElementException(_this__u8e3s4) {
    captureStack(_this__u8e3s4, _this__u8e3s4.$throwableCtor_3);
  }
  function init_kotlin_ClassCastException(_this__u8e3s4) {
    captureStack(_this__u8e3s4, _this__u8e3s4.$throwableCtor_3);
  }
  function AbstractCollection$toString$lambda(this$0) {
    return function (it) {
      return it === this$0 ? '(this Collection)' : toString_0(it);
    };
  }
  var Companion_instance_1;
  function Companion_getInstance_1() {
    return Companion_instance_1;
  }
  function collectionToArrayCommonImpl(collection) {
    if (collection.isEmpty_y1axqb_k$()) {
      var tmp$ret$0;
      // Inline function 'kotlin.emptyArray' call
      tmp$ret$0 = [];
      return tmp$ret$0;
    }
    var tmp$ret$1;
    // Inline function 'kotlin.arrayOfNulls' call
    var size = collection.get_size_woubt6_k$();
    tmp$ret$1 = fillArrayVal(Array(size), null);
    var destination = tmp$ret$1;
    var iterator = collection.iterator_jk1svi_k$();
    var index = 0;
    while (iterator.hasNext_bitz1p_k$()) {
      var tmp0 = index;
      index = tmp0 + 1 | 0;
      destination[tmp0] = iterator.next_20eer_k$();
      Unit_instance;
    }
    return destination;
  }
  function arrayListOf(elements) {
    return elements.length === 0 ? ArrayList.new_kotlin_collections_ArrayList_ony0vx_k$() : ArrayList.new_kotlin_collections_ArrayList_603as_k$(new ArrayAsCollection(elements, true));
  }
  function listOf_0(elements) {
    return elements.length > 0 ? asList(elements) : emptyList();
  }
  function emptyList() {
    return EmptyList_getInstance();
  }
  var EmptyList_instance;
  function EmptyList_getInstance() {
    if (EmptyList_instance === VOID)
      new EmptyList();
    return EmptyList_instance;
  }
  var EmptyIterator_instance;
  function EmptyIterator_getInstance() {
    return EmptyIterator_instance;
  }
  function appendElement(_this__u8e3s4, element, transform) {
    if (!(transform == null)) {
      _this__u8e3s4.append_jgojdo_k$(transform(element));
      Unit_instance;
    } else {
      if (element == null ? true : isCharSequence(element)) {
        _this__u8e3s4.append_jgojdo_k$(element);
        Unit_instance;
      } else {
        if (element instanceof Char) {
          _this__u8e3s4.append_t84oo1_k$(element.value_1);
          Unit_instance;
        } else {
          _this__u8e3s4.append_jgojdo_k$(toString_0(element));
          Unit_instance;
        }
      }
    }
  }
  function get_fruitArray() {
    _init_properties_dummy_kt__osk9ey();
    return fruitArray;
  }
  var fruitArray;
  function operate() {
    _init_properties_dummy_kt__osk9ey();
    var inductionVariable = 0;
    if (inductionVariable < 20)
      do {
        var i = inductionVariable;
        inductionVariable = inductionVariable + 1 | 0;
        var item = new Apple('Field1-' + i, i + 1 | 0, (i % 2 | 0) === 0, listOf('Field4-' + i), new Color('ColorString-' + i, i), new Date(), 'Field7-' + i, (i % 3 | 0) === 0 ? null : 'Field8-' + i, listOf_0([i, imul(i, 2), imul(i, 3)]), (i % 4 | 0) === 0, 'AdditionalField1-' + i, i + 11 | 0, (i % 5 | 0) === 0, listOf('AdditionalField4-' + i), new Color('AdditionalColorString-' + i, i + 5 | 0), new Date(), 'AdditionalField7-' + i, (i % 2 | 0) === 0 ? null : 'AdditionalField8-' + i, listOf_0([i + 1 | 0, imul(i + 1 | 0, 2), imul(i + 1 | 0, 3)]), (i % 3 | 0) === 0);
        get_fruitArray().add_utx5q5_k$(item);
        Unit_instance;
      }
       while (inductionVariable < 20);
    var inductionVariable_0 = 0;
    if (inductionVariable_0 < 20)
      do {
        var i_0 = inductionVariable_0;
        inductionVariable_0 = inductionVariable_0 + 1 | 0;
        var item_0 = new Orange('Field1-' + i_0, i_0 + 1 | 0, (i_0 % 2 | 0) === 0, listOf('Field4-' + i_0), new Color('ColorString-' + i_0, i_0), new Date(), 'Field7-' + i_0, (i_0 % 3 | 0) === 0 ? null : 'Field8-' + i_0, listOf_0([i_0, imul(i_0, 2), imul(i_0, 3)]), (i_0 % 4 | 0) === 0, 'AdditionalField1-' + i_0, i_0 + 11 | 0, (i_0 % 5 | 0) === 0, listOf('AdditionalField4-' + i_0), new Color('AdditionalColorString-' + i_0, i_0 + 5 | 0), new Date(), 'AdditionalField7-' + i_0, (i_0 % 2 | 0) === 0 ? null : 'AdditionalField8-' + i_0, listOf_0([i_0 + 1 | 0, imul(i_0 + 1 | 0, 2), imul(i_0 + 1 | 0, 3)]), (i_0 % 3 | 0) === 0);
        get_fruitArray().add_utx5q5_k$(item_0);
        Unit_instance;
      }
       while (inductionVariable_0 < 20);
    var inductionVariable_1 = 0;
    if (inductionVariable_1 < 20)
      do {
        var i_1 = inductionVariable_1;
        inductionVariable_1 = inductionVariable_1 + 1 | 0;
        var item_1 = new Grape('Field1-' + i_1, i_1 + 1 | 0, (i_1 % 2 | 0) === 0, listOf('Field4-' + i_1), new Color('ColorString-' + i_1, i_1), new Date(), 'Field7-' + i_1, (i_1 % 3 | 0) === 0 ? null : 'Field8-' + i_1, listOf_0([i_1, imul(i_1, 2), imul(i_1, 3)]), (i_1 % 4 | 0) === 0, 'AdditionalField1-' + i_1, i_1 + 11 | 0, (i_1 % 5 | 0) === 0, listOf('AdditionalField4-' + i_1), new Color('AdditionalColorString-' + i_1, i_1 + 5 | 0), new Date(), 'AdditionalField7-' + i_1, (i_1 % 2 | 0) === 0 ? null : 'AdditionalField8-' + i_1, listOf_0([i_1 + 1 | 0, imul(i_1 + 1 | 0, 2), imul(i_1 + 1 | 0, 3)]), (i_1 % 3 | 0) === 0);
        get_fruitArray().add_utx5q5_k$(item_1);
        Unit_instance;
      }
       while (inductionVariable_1 < 20);
    var inductionVariable_2 = 0;
    if (inductionVariable_2 < 20)
      do {
        var i_2 = inductionVariable_2;
        inductionVariable_2 = inductionVariable_2 + 1 | 0;
        var item_2 = new Banana('Field1-' + i_2, i_2 + 1 | 0, (i_2 % 2 | 0) === 0, listOf('Field4-' + i_2), new Color('ColorString-' + i_2, i_2), new Date(), 'Field7-' + i_2, (i_2 % 3 | 0) === 0 ? null : 'Field8-' + i_2, listOf_0([i_2, imul(i_2, 2), imul(i_2, 3)]), (i_2 % 4 | 0) === 0, 'AdditionalField1-' + i_2, i_2 + 11 | 0, (i_2 % 5 | 0) === 0, listOf('AdditionalField4-' + i_2), new Color('AdditionalColorString-' + i_2, i_2 + 5 | 0), new Date(), 'AdditionalField7-' + i_2, (i_2 % 2 | 0) === 0 ? null : 'AdditionalField8-' + i_2, listOf_0([i_2 + 1 | 0, imul(i_2 + 1 | 0, 2), imul(i_2 + 1 | 0, 3)]), (i_2 % 3 | 0) === 0);
        get_fruitArray().add_utx5q5_k$(item_2);
        Unit_instance;
      }
       while (inductionVariable_2 < 20);
  }
  function kotlinTest() {
    _init_properties_dummy_kt__osk9ey();
    var startTime = Date.now();
    // Inline function 'kotlin.repeat' call
    var times = 300;
    // Inline function 'kotlin.contracts.contract' call
    var inductionVariable = 0;
    if (inductionVariable < times)
      do {
        var index = inductionVariable;
        inductionVariable = inductionVariable + 1 | 0;
        // Inline function 'kotlinTest.<anonymous>' call
        var it = index;
        operate();
        Unit_instance;
      }
       while (inductionVariable < times);
    var tmp$ret$1;
    // Inline function 'kotlin.collections.sumOf' call
    var this_0 = get_fruitArray();
    var sum = 0;
    var tmp0_iterator = this_0.iterator_jk1svi_k$();
    while (tmp0_iterator.hasNext_bitz1p_k$()) {
      var element = tmp0_iterator.next_20eer_k$();
      var tmp = sum;
      var tmp$ret$0;
      // Inline function 'kotlinTest.<anonymous>' call
      var it_0 = element;
      tmp$ret$0 = it_0.get_field2_dbuq9t_k$();
      sum = tmp + tmp$ret$0 | 0;
    }
    tmp$ret$1 = sum;
    var result = tmp$ret$1;
    var tmp$ret$3;
    // Inline function 'kotlin.collections.sumOf' call
    var this_1 = get_fruitArray();
    var sum_0 = 0;
    var tmp0_iterator_0 = this_1.iterator_jk1svi_k$();
    while (tmp0_iterator_0.hasNext_bitz1p_k$()) {
      var element_0 = tmp0_iterator_0.next_20eer_k$();
      var tmp_0 = sum_0;
      var tmp$ret$2;
      // Inline function 'kotlinTest.<anonymous>' call
      var it_1 = element_0;
      var tmp0_subject = it_1;
      var tmp_1;
      if (tmp0_subject instanceof Banana) {
        tmp_1 = it_1.additionalField2__1;
      } else {
        if (tmp0_subject instanceof Grape) {
          tmp_1 = it_1.additionalField2__1;
        } else {
          if (tmp0_subject instanceof Orange) {
            tmp_1 = it_1.additionalField2__1;
          } else {
            tmp_1 = 0;
          }
        }
      }
      tmp$ret$2 = tmp_1;
      sum_0 = tmp_0 + tmp$ret$2 | 0;
    }
    tmp$ret$3 = sum_0;
    var result2 = tmp$ret$3;
    var tmp$ret$5;
    // Inline function 'kotlin.collections.sumOf' call
    var this_2 = get_fruitArray();
    var sum_1 = 0;
    var tmp0_iterator_1 = this_2.iterator_jk1svi_k$();
    while (tmp0_iterator_1.hasNext_bitz1p_k$()) {
      var element_1 = tmp0_iterator_1.next_20eer_k$();
      var tmp_2 = sum_1;
      var tmp$ret$4;
      // Inline function 'kotlinTest.<anonymous>' call
      var it_2 = element_1;
      var tmp0_subject_0 = it_2;
      var tmp_3;
      if (tmp0_subject_0 instanceof Banana) {
        var tmp1_safe_receiver = it_2.additionalField5__1;
        var tmp2_elvis_lhs = tmp1_safe_receiver == null ? null : tmp1_safe_receiver.colorInt_1;
        tmp_3 = tmp2_elvis_lhs == null ? 0 : tmp2_elvis_lhs;
      } else {
        if (tmp0_subject_0 instanceof Grape) {
          var tmp3_safe_receiver = it_2.additionalField5__1;
          var tmp4_elvis_lhs = tmp3_safe_receiver == null ? null : tmp3_safe_receiver.colorInt_1;
          tmp_3 = tmp4_elvis_lhs == null ? 0 : tmp4_elvis_lhs;
        } else {
          if (tmp0_subject_0 instanceof Apple) {
            var tmp5_safe_receiver = it_2.additionalField5__1;
            var tmp6_elvis_lhs = tmp5_safe_receiver == null ? null : tmp5_safe_receiver.colorInt_1;
            tmp_3 = tmp6_elvis_lhs == null ? 0 : tmp6_elvis_lhs;
          } else {
            tmp_3 = 0;
          }
        }
      }
      tmp$ret$4 = tmp_3;
      sum_1 = tmp_2 + tmp$ret$4 | 0;
    }
    tmp$ret$5 = sum_1;
    var result3 = tmp$ret$5;
    var endTime = Date.now();
    print(`bili_kntr: ms = ${endTime-startTime}`)
//    println('KotlinJs cost time: ' + (endTime - startTime) + 'ms, results: ' + result + ' ' + result2 + ' ' + result3);
  }
  var properties_initialized_dummy_kt_eeoni0;
  function _init_properties_dummy_kt__osk9ey() {
    if (properties_initialized_dummy_kt_eeoni0) {
    } else {
      properties_initialized_dummy_kt_eeoni0 = true;
      var tmp$ret$0;
      // Inline function 'kotlin.collections.mutableListOf' call
      tmp$ret$0 = ArrayList.new_kotlin_collections_ArrayList_ony0vx_k$();
      fruitArray = tmp$ret$0;
    }
  }
  //region block: post-declaration
  setMetadataFor(CharSequence, 'CharSequence', interfaceMeta);
  setMetadataFor(Number_0, 'Number', classMeta);
  setMetadataFor(Unit, 'Unit', objectMeta);
  setMetadataFor(AbstractCollection, 'AbstractCollection', classMeta);
  setMetadataFor(AbstractMutableCollection, 'AbstractMutableCollection', classMeta);
  setMetadataFor(IteratorImpl, 'IteratorImpl', classMeta);
  setMetadataFor(AbstractMutableList, 'AbstractMutableList', classMeta);
  setMetadataFor(Companion, 'Companion', objectMeta);
  setMetadataFor(ArrayList, 'ArrayList', classMeta, VOID, VOID, ArrayList.new_kotlin_collections_ArrayList_ony0vx_k$);
  setMetadataFor(BaseOutput, 'BaseOutput', classMeta);
  setMetadataFor(NodeJsOutput, 'NodeJsOutput', classMeta);
  setMetadataFor(BufferedOutput, 'BufferedOutput', classMeta, VOID, VOID, BufferedOutput);
  setMetadataFor(BufferedOutputToConsoleLog, 'BufferedOutputToConsoleLog', classMeta, VOID, VOID, BufferedOutputToConsoleLog);
  setMetadataFor(StringBuilder, 'StringBuilder', classMeta, VOID, [CharSequence], StringBuilder.new_kotlin_text_StringBuilder_u46mrb_k$);
  setMetadataFor(Char, 'Char', classMeta);
  setMetadataFor(arrayIterator$1, VOID, classMeta);
  setMetadataFor(Companion_0, 'Companion', objectMeta);
  setMetadataFor(Long, 'Long', classMeta);
  setMetadataFor(Exception, 'Exception', classMeta, VOID, VOID, Exception.new_kotlin_Exception_f32mds_k$);
  setMetadataFor(RuntimeException, 'RuntimeException', classMeta, VOID, VOID, RuntimeException.new_kotlin_RuntimeException_29f9zq_k$);
  setMetadataFor(IllegalArgumentException, 'IllegalArgumentException', classMeta, VOID, VOID, IllegalArgumentException.new_kotlin_IllegalArgumentException_pv5o3f_k$);
  setMetadataFor(IndexOutOfBoundsException, 'IndexOutOfBoundsException', classMeta, VOID, VOID, IndexOutOfBoundsException.new_kotlin_IndexOutOfBoundsException_cc7xqw_k$);
  setMetadataFor(IllegalStateException, 'IllegalStateException', classMeta, VOID, VOID, IllegalStateException.new_kotlin_IllegalStateException_1wtnp1_k$);
  setMetadataFor(UnsupportedOperationException, 'UnsupportedOperationException', classMeta, VOID, VOID, UnsupportedOperationException.new_kotlin_UnsupportedOperationException_cv3bvm_k$);
  setMetadataFor(NoSuchElementException, 'NoSuchElementException', classMeta, VOID, VOID, NoSuchElementException.new_kotlin_NoSuchElementException_wy3d4q_k$);
  setMetadataFor(ClassCastException, 'ClassCastException', classMeta, VOID, VOID, ClassCastException.new_kotlin_ClassCastException_zhuhe1_k$);
  setMetadataFor(Companion_1, 'Companion', objectMeta);
  setMetadataFor(ArrayAsCollection, 'ArrayAsCollection', classMeta);
  setMetadataFor(EmptyList, 'EmptyList', objectMeta);
  setMetadataFor(EmptyIterator, 'EmptyIterator', objectMeta);
  setMetadataFor(Color, 'Color', classMeta, VOID, VOID, Color);
  setMetadataFor(Apple, 'Apple', classMeta);
  setMetadataFor(Orange, 'Orange', classMeta);
  setMetadataFor(Grape, 'Grape', classMeta);
  setMetadataFor(Banana, 'Banana', classMeta);
  //endregion
  //region block: init
  Unit_instance = new Unit();
  Companion_instance_1 = new Companion_1();
  EmptyIterator_instance = new EmptyIterator();
  //endregion
  //region block: exports
  function $jsExportAll$(_) {
    _.kotlinTest = kotlinTest;
  }
  $jsExportAll$(_);
  //endregion
  return _;
}(typeof kntr === 'undefined' ? {} : kntr);


let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  kntr.kotlinTest()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

//# sourceMappingURL=kntr.js.map
kntr.kotlinTest()