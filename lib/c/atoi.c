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

int hexval(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
		
	return 0;
}

int atoi(const char *num)
{
	int value = 0;
	if (num[0] == '0' && num[1] == 'x') {
		// hex
		num += 2;
		while (*num && ishexdigit(*num))
			value = value * 16 + hexval(*num++);
	} else {
		// decimal
		while (*num && isdigit(*num))
			value = value * 10 + *num++  - '0';
	}

	return value;
}
