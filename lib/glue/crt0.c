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

extern void main();
extern void __syslib_init();

typedef void (*ctor_dtor_hook)();
extern ctor_dtor_hook __CTORS__[];
extern ctor_dtor_hook __DTORS__[];

void _start()
{
	ctor_dtor_hook *hook;

	__syslib_init();

	for (hook = __CTORS__; *hook; hook++)
		 (*hook)();

	main();

	for (hook = __DTORS__; *hook; hook++)
		 (*hook)();
}
