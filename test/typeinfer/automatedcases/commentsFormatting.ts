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

// === tests/cases/compiler/commentsFormatting.ts ===
declare function AssertType(value:any, type:string):void;

module m {
    /** this is first line - aligned to class declaration
* this is 4 spaces left aligned
 * this is 3 spaces left aligned
  * this is 2 spaces left aligned
   * this is 1 spaces left aligned
    * this is at same level as first line
     * this is 1 spaces right aligned
      * this is 2 spaces right aligned
       * this is 3 spaces right aligned
        * this is 4 spaces right aligned
         * this is 5 spaces right aligned
          * this is 6 spaces right aligned
           * this is 7 spaces right aligned
            * this is 8 spaces right aligned */
    export class c {
    }

        /** this is first line - 4 spaces right aligned to class but in js file should be aligned to class declaration
* this is 8 spaces left aligned
 * this is 7 spaces left aligned
  * this is 6 spaces left aligned
   * this is 5 spaces left aligned
    * this is 4 spaces left aligned
     * this is 3 spaces left aligned
      * this is 2 spaces left aligned
       * this is 1 spaces left aligned
        * this is at same level as first line
         * this is 1 spaces right aligned
          * this is 2 spaces right aligned
           * this is 3 spaces right aligned
            * this is 4 spaces right aligned
             * this is 5 spaces right aligned
              * this is 6 spaces right aligned
               * this is 7 spaces right aligned
                * this is 8 spaces right aligned */
    export class c2 {
    }

    /** this is comment with new lines in between

this is 4 spaces left aligned but above line is empty

 this is 3 spaces left aligned but above line is empty

  this is 2 spaces left aligned but above line is empty

    this is 1 spaces left aligned but above line is empty

    this is at same level as first line but above line is empty 

     this is 1 spaces right aligned but above line is empty

      this is 2 spaces right aligned but above line is empty

       this is 3 spaces right aligned but above line is empty

        this is 4 spaces right aligned but above line is empty
    
    
    Above 2 lines are empty
    
    
    
    above 3 lines are empty*/
    export class c3 {
    }

    /** this is first line - aligned to class declaration
	*              this is 0 space + tab
 	*              this is 1 space + tab
  	*              this is 2 spaces + tab
   	*              this is 3 spaces + tab
    	*          this is 4 spaces + tab
     	*          this is 5 spaces + tab
      	*          this is 6 spaces + tab
       	*          this is 7 spaces + tab
        	*      this is 8 spaces + tab
         	*      this is 9 spaces + tab
          	*      this is 10 spaces + tab
           	*      this is 11 spaces + tab
            	*  this is 12 spaces + tab */
    export class c4 {
    }
}

