// 
// Copyright 1998-2012 Jeff Bush
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// 

#include <ctype.h>

enum {
	kNumHex = (kTypeNum | kTypeHexDigit | kTypeGraphic),
	kAlphaHex = (kTypeHexDigit | kTypeGraphic),
	kAlphaHexLower = (kAlphaHex | kTypeLower),
	kAlphaGraph = (kTypeAlpha | kTypeGraphic),
	kAlphaGraphLower = (kAlphaGraph | kTypeLower)
};

unsigned char __ctype_table[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	kTypeSpace,	/* LF */
	0,
	kTypeSpace,	/* tab */
	kTypeSpace,	/* CR */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0,
	
	kTypeSpace, 	/* space (32) */
	
	kTypeGraphic, kTypeGraphic, kTypeGraphic, kTypeGraphic, kTypeGraphic, kTypeGraphic,
	kTypeGraphic, kTypeGraphic, kTypeGraphic, kTypeGraphic, kTypeGraphic, kTypeGraphic,
	kTypeGraphic, kTypeGraphic, kTypeGraphic, /* !"#$%&'()*+,-./ */
	
	kNumHex, kNumHex, kNumHex, kNumHex, kNumHex, kNumHex, kNumHex, kNumHex, kNumHex,
	kNumHex, 	/* 0-9 */
	
	0, 0, 0, 0, 0, 0, 0,	/* :;<=>?@ */
	
	kAlphaHex, kAlphaHex, kAlphaHex, kAlphaHex, kAlphaHex, kAlphaHex, /* A-F */

	kAlphaGraph, kAlphaGraph, kAlphaGraph, kAlphaGraph, kAlphaGraph, kAlphaGraph,
	kAlphaGraph, kAlphaGraph, kAlphaGraph, kAlphaGraph, kAlphaGraph, kAlphaGraph,
	kAlphaGraph, kAlphaGraph, kAlphaGraph, kAlphaGraph, kAlphaGraph, kAlphaGraph,
	kAlphaGraph, kAlphaGraph,  /* G-Z */
	
	kTypeGraphic, kTypeGraphic, kTypeGraphic, kTypeGraphic, kTypeGraphic, kTypeGraphic,
		 /* [\]^_` */
		
	kAlphaHexLower, kAlphaHexLower, kAlphaHexLower, kAlphaHexLower, kAlphaHexLower,
	kAlphaHexLower, /* a-f */

	kAlphaGraphLower, kAlphaGraphLower, kAlphaGraphLower, kAlphaGraphLower,
	kAlphaGraphLower, kAlphaGraphLower, kAlphaGraphLower, kAlphaGraphLower,
	kAlphaGraphLower, kAlphaGraphLower, kAlphaGraphLower, kAlphaGraphLower,
	kAlphaGraphLower, kAlphaGraphLower, kAlphaGraphLower, kAlphaGraphLower,
	kAlphaGraphLower, kAlphaGraphLower, kAlphaGraphLower, kAlphaGraphLower,  /* g-z */
	
	kTypeGraphic, kTypeGraphic, kTypeGraphic, kTypeGraphic,	/*  {|}~ */
	
	0, /* DEL */

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
