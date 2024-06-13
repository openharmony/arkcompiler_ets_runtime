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
export const applicationHtml = `
        <style>
        :host{

        }
        .root{
            display: grid;
            grid-template-rows: min-content 1fr;
            grid-template-columns: min-content 1fr;
            grid-template-areas: 'm s'
                                 'm b';
            height: 100vh;
            width: 100vw;
        }
        .filedrag::after {
             content: 'Drop the trace file to open it';
             position: fixed;
             z-index: 2001;
             top: 0;
             left: 0;
             right: 0;
             bottom: 0;
             border: 5px dashed var(--dark-color1,#404854);
             text-align: center;
             font-size: 3rem;
             line-height: 100vh;
             background: rgba(255, 255, 255, 0.5);
        }
        .menu{
            grid-area: m;
            box-shadow: 4px 0px 20px rgba(0,0,0,0.05);
            z-index: 2000;
        }
        .search-vessel{
            z-index: 999;
            position: relative;
            cursor: default;
        }
        .progress{
            bottom: 0;
            position: absolute;
            height: 1px;
            left: 0;
            right: 0;
        }

        :host(:not([search])) .search-vessel  {
           display: none;
        }
        :host(:not([search])) .search-vessel .search  {
            background-color: var(--dark-background5,#F6F6F6);
        }
        .search{
            grid-area: s;
            background-color: var(--dark-background,#FFFFFF);
            height: 48px;
            display: flex;
            justify-content: center;
            align-items: center;

        }
        .search .search-bg{
            background-color: var(--dark-background5,#fff);
            border-radius: 40px;
            padding: 3px 20px;
            display: flex;
            justify-content: center;
            align-items: center;
            border: 1px solid var(--dark-border,#c5c5c5);
        }
        lit-search input{
            outline: none;
            border: 0px;
            background-color: transparent;
            font-size: inherit;
            color: var(--dark-color,#666666);
            width: 30vw;
            height: auto;
            vertical-align:middle;
            line-height:inherit;
            height:inherit;
            padding: 6px 6px 6px 6px};
            max-height: inherit;
            box-sizing: border-box;

        }
        ::placeholder { /* CSS 3 標準 */
          color: #b5b7ba;
          font-size: 1em;
        }
        lit-search input::placeholder {
          color: #b5b7ba;
          font-size: 1em;
        }
        .content{
            grid-area: b;
            background-color: #ffffff;
            height: 100%;
            overflow: auto;
            position:relative;
        }
        .sheet{

        }
        .sidebar-button{
            position: absolute;
            top: 0;
            left: 0;
            background-color: var(--dark-background1,#FFFFFF);
            height: 100%;
            border-radius: 0 5px 5px 0;
            width: 48px;
            display: flex;
            align-content: center;
            justify-content: center;
            cursor: pointer;
        }
        :host{
            font-size: inherit;
            display: inline-block;
            transition: .3s;
         }
         :host([spin]){
            animation: rotate 1.75s linear infinite;
         }
         @keyframes rotate {
            to{
                transform: rotate(360deg);
            }
         }
         .icon{
            display: block;
            width: 1em;
            height: 1em;
            margin: auto;
            fill: currentColor;
            overflow: hidden;
            font-size: 20px;
            color: var(--dark-color1,#47A7E0);
         }
        .content-center-option {
          justify-content: center;
          width: -webkit-fill-available;
          margin-right: 5.2em;
          align-items: center;
          width: auto;
        }
        </style>
        <div class="root" style="position: relative;">
            <lit-main-menu id="main-menu" class="menu" data=''></lit-main-menu>
            <div class="search-vessel">
                <div class="search" style="position: relative;">
                    <div class="sidebar-button" style="width: 0">
                        <svg class="icon" id="icon" aria-hidden="true" viewBox="0 0 1024 1024">
                             <use id="use" xlink:href="./base-ui/icon.svg#icon-menu"></use>
                        </svg>
                    </div>
                    <lit-search id="lit-search"></lit-search>
                </div>
                <lit-progress-bar class="progress"></lit-progress-bar>
            </div>
            <div id="app-content" class="content">
                <ap-welcome style="visibility:visible;top:0px;left:0px;position:absolute;z-index: 100" id="ap-welcome">
                </ap-welcome>
                <div style="height:100%;">
                      <tab-ap-summary id="tabpane-summary" mode="hidden" tree ondragstart="return false">
                      </tab-ap-summary>
                </div> 
            </div>
        </div>
        `;
