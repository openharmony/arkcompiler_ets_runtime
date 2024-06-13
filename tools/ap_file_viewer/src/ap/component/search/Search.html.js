/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
export const SearchHtml = `
        <style>
        :host{
        }
        .root{
            background-color: var(--dark-background5,#fff);
            border-radius: 40px;
            padding: 3px 20px;
            display: flex;
            justify-content: center;
            align-items: center;
            border: 1px solid var(--dark-border,#c5c5c5);
            width: 35vw;
            overflow: hidden;
            }
        .root input{
            outline: none;
            width: max-content;
            border: 0px;
            background-color: transparent;
            font-size: inherit;
            color: var(--dark-color,#666666);
            flex: 1;
            height: auto;
            vertical-align:middle;
            line-height:inherit;
            height:inherit;
            padding: 6px 6px 6px 6px;
            max-height: inherit;
            box-sizing: border-box;
        }
        ::placeholder {
          color: #b5b7ba;
          font-size: 1em;
        }
        .write::placeholder {
          color: #b5b7ba;
          font-size: 1em;
        }
        .readonly::placeholder {
          color: #4f7ab3;
          font-size: 1em;
        }
        .text-Roll::placeholder {
          font-weight: 700;
          color: #DB5860;
          font-size: 1em;
        }
        :host([show-search-info]) .search-info{
            display: inline-flex;
            higth:100%!important;
            justify-content: center;
            align-items: center;
        }
        :host(:not([show-search-info])) .search-info{
            display: none;
        }
        :host(:not([distributed])) #trace_select{
            display: none;
        }
        :host([distributed]) #trace_select{
            display: block;
        }
        .search-info span{
            color:#ABABAB;
        }
        .search-info lit-icon{
            font-weight: bold;
        }
        
        :host([textRoll]) input {
            position: relative;
            animation: textRoll 5s ease-in-out 0s backwards;
            white-space: nowrap;
            overflow: hidden;
            display: block;
            text-overflow: ellipsis;
        }
      
        @keyframes textRoll {
            0% {
                left: 0;
            }
            100% {
                left: 100%;
            }
        }
         .search-history {
            position: relative;
         }
        .search-history-list {
            list-style-type: none;
            margin: 0;
            padding: 0;
            position: absolute;
            width: 37vw;
            top: 100%;
            background-color: #FFFFFF;
            border: 1px solid #ddd;
            max-height: 200px;
            overflow-y: auto;
            display: none;
            border-radius: 0 0 20px 20px;
        }     
        .search-history-list:hover{
             cursor: pointer;
        }  
        .search-history-list-item {
            overflow: hidden;
            white-space: nowrap;
            text-overflow: ellipsis;
            cursor: pointer;
            width: 100%;
        }
        .search-list:hover {
            background-color: #e9e9e9;
        }
        .search-list {
            display: flex;
            justify-content: space-between;
            padding-right: 20px;
            padding-left: 45px;
        }
        #trace_selector{
            position: unset;
            width: 100px;
            margin-left: -10px;
            border:none;
        }
        input[name="retarge_index"]{
          width:100px!important;
        }
        </style>
        <div class="root" style="display: none">
            <div id="trace_select" style="border-right: 1px solid var(--dark-border,#c5c5c5)">
                <lit-select id="trace_selector" default-value="1" placement="bottom"></lit-select>
            </div>
            <lit-icon id="search-icon" name="search" size="22" color="#aaaaaa"></lit-icon>

            <input name="search" class="readonly" placeholder="Search" readonly/>
            <div class="search-info">
                <input name="retarge_index" placeholder="Go" oninput="value=value.replace(/^(0+)|[^0-9]/g,'')"/>
            </div>
        </div>
        <div id="prompt" style="display: none"></div>
        <div class="search-history">
              <ul class="search-history-list"></ul>
        </div>
        `;
