/*
MIT License

Copyright (c) 2022 Mikhail Morozov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "iuliia.h"
#include "forks/json.h/json.h"

#include <stdio.h>
#include <stdlib.h>

iuliia_scheme_t *iuliiaLoadSchemeFromMemory(char *json, size_t json_length)
{
	iuliia_scheme_t *scheme;
	struct json_value_s *root = json_parse(json, json_length);

	if(root->type != json_type_object) {
		free(root);

		return 0;
	}

	scheme = malloc(sizeof(iuliia_scheme_t));
	if(!scheme) {
		free(root);

		return 0;
	}

	free(root);

	return scheme;
}

void iuliiaFreeScheme(iuliia_scheme_t *scheme)
{
	if(!scheme) return;

	free(scheme);
}

iuliia_scheme_t *iuliiaLoadSchemeW(wchar_t *filename)
{
	FILE *f;
	char *json;
	size_t json_length;
	long long f_size;
	iuliia_scheme_t *scheme;

	f = _wfopen(filename, L"rb");
	if(!f) return 0;

	if(fseek(f, 0, SEEK_END)) {
		fclose(f);

		return 0;
	}

	f_size = _ftelli64(f);
	if(f_size > SIZE_MAX) {
		fclose(f);

		return 0;
	}

	json_length = (size_t)f_size;

	if(fseek(f, 0, SEEK_SET)) {
		fclose(f);

		return 0;
	}

	json = malloc(json_length+1);
	if(!json) {
		fclose(f);

		return 0;
	}
	json[json_length] = 0;

	if(fread(json, 1, json_length, f) != json_length) {
		free(json);
		fclose(f);

		return 0;
	}

	fclose(f);

	scheme = iuliiaLoadSchemeFromMemory(json, json_length);

	free(json);

	return scheme;
}

size_t iuliiaU32len(const uint32_t *s)
{
	size_t size = 0;
	
	while(*(s++) != 0) size++;
	
	return size;
}

wchar_t *iuliiaU32toW(const uint32_t *s)
{
	size_t s_len;
	wchar_t *new_s;

	s_len = iuliiaU32len(s);
	new_s = malloc((s_len+1)*(sizeof(uint32_t)/sizeof(wchar_t)));
	if(!new_s) return 0;
	
	if(sizeof(uint32_t) == sizeof(wchar_t)) {
		memcpy(new_s, s, (s_len+1)*sizeof(wchar_t));
	} else {
		wchar_t *p;
		
		p = new_s;
		while(1) {
			if(*s < 65536)
				*p = *s;
			else {
				*p = (*s-0x10000) >> 10 | 0xd800;
				p++;
				*p = (*s-0x10000) & 0x3ff | 0xdc00;
			}

			if(*s == 0) break;

			p++;
			s++;
		}
	}
	
	return new_s;
}

uint32_t *iuliiaWtoU32(const wchar_t *s)
{
	size_t s_len;
	uint32_t *new_s;

	s_len = wcslen(s);
	new_s = malloc((s_len+1)*sizeof(uint32_t));
	if(!new_s) return 0;

	if(sizeof(uint32_t) == sizeof(wchar_t)) {
		memcpy(new_s, s, (s_len+1)*sizeof(wchar_t));
	} else {
		uint32_t *p;
		
		p = new_s;
		while(1) {
			if(*s < 0xd800 || *s > 0xdfff)
				*p = *s;
			else {
				if(*s >= 0xdc00) {
					free(new_s);
					
					return 0;
				}
				
				*p = ((*s) & 0x3ff) << 10;
				s++;
				
				if(*s < 0xdc00 || *s > 0xdfff) {
					free(new_s);
					
					return 0;
				}
				
				*p = *p + ((*s) & 0x3ff) + 0x10000;
			}

			if(*s == 0) break;

			p++;
			s++;
		}
	}
	
	return new_s;
}

uint32_t *iuliiaTranslateU32(const uint32_t *s, const iuliia_scheme_t *scheme)
{
	uint32_t *new_s, *p;

	new_s = malloc((iuliiaU32len(s)+1)*sizeof(uint32_t));
	if(!new_s) return 0;
	
	p = new_s;
	while(*s) {
		*(p++) = *(s++);
	}
	*p = 0;
	
	return new_s;
}

uint32_t *iuliiaTranslateWtoU32(const wchar_t *s, const iuliia_scheme_t *scheme)
{
	if(sizeof(uint32_t) == sizeof(wchar_t))
		return iuliiaTranslateU32((const uint32_t *)s, scheme);
	else {
		uint32_t *su32, *new_su32;
		
		su32 = iuliiaWtoU32(s);
		if(!su32) return 0;
		
		new_su32 = iuliiaTranslateU32(su32, scheme);
		
		iuliiaFreeString(su32);
		
		return new_su32;
	}
}

wchar_t *iuliiaTranslateW(const wchar_t *s, const iuliia_scheme_t *scheme)
{
	if(sizeof(uint32_t) == sizeof(wchar_t))
		return (wchar_t *)iuliiaTranslateU32((const uint32_t*)s, scheme);
	else {
		wchar_t *new_s;
		uint32_t *new_su32;
		
		new_su32 = iuliiaTranslateWtoU32(s, scheme);
		if(!new_su32) return 0;
		
		new_s = iuliiaU32toW(new_su32);
		if(!new_s) {
			iuliiaFreeString(new_su32);
			
			return 0;
		}
		
		return new_s;
	}
}

uint32_t *iuliiaTranslateAtoU32(const char *s, const iuliia_scheme_t *scheme)
{
	return 0;
}

char *iuliiaTranslateA(const char *s, const iuliia_scheme_t *scheme)
{
	return 0;
}

void iuliiaFreeString(void *s)
{
	if(s) free(s);
}
