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
#include <stdbool.h>

#include <Windows.h>

static bool iuliiaIntJsonLoadStringW(struct json_value_s *value, wchar_t **str)
{
	struct json_string_s* val;

	val = json_value_as_string(value);
	if (!val) return false;
	
	*str = malloc((val->string_size + 1) * sizeof(wchar_t));
	if(!(*str)) return false;
	(*str)[val->string_size] = 0;

	MultiByteToWideChar(CP_UTF8, 0, val->string, (int)(val->string_size+1), *str, (int)(val->string_size+1));

	return true;
}

static bool iuliiaIntJsonReadMapping1char(struct json_object_s *obj, iuliia_mapping_1char_t **map)
{
	size_t i;
	struct json_object_element_s *el;
	iuliia_mapping_1char_t *new_map;

	new_map = malloc(obj->length*sizeof(iuliia_mapping_1char_t));
	if(!new_map) return false;
	memset(new_map, 0, obj->length*sizeof(iuliia_mapping_1char_t));

	el = obj->start;
	for(i = 0; i < obj->length; i++) {
		const uint8_t *new_c;
		struct json_string_s *str;

		new_c = iuliiaCharU8toU32(el->name->string, &(new_map[i].c));
		if(!new_c) return false;

		str = json_value_as_string(el->value);
		if(!str) return false;

		new_map[i].repl = iuliiaU8toU32(str->string);

		el = el->next;
	}

	*map = new_map;

	return true;
}

static bool iuliiaIntJsonReadSamples(struct json_array_s *arr, iuliia_samples_t **samples)
{
	iuliia_samples_t *new_samples;
	struct json_array_element_s * arr_el;
	size_t i = 0;

	new_samples = malloc(arr->length*sizeof(iuliia_samples_t));
	if(!new_samples) return false;
	memset(new_samples, 0, arr->length*sizeof(iuliia_samples_t));

	arr_el = arr->start;
	while(arr_el) {
		struct json_array_s *sample;
		struct json_array_element_s *sample_el;
		
		sample = json_value_as_array(arr_el->value);
		if(!sample) goto IULIIA_ERROR;

		if(sample->length != 2) goto IULIIA_ERROR;

		sample_el = sample->start;
		if(!iuliiaIntJsonLoadStringW(sample_el->value, &(new_samples[i].in))) goto IULIIA_ERROR;
		sample_el = sample_el->next;
		if(!iuliiaIntJsonLoadStringW(sample_el->value, &(new_samples[i].out))) goto IULIIA_ERROR;

		arr_el = arr_el->next;
		i++;
	}

	*samples = new_samples;

	return true;

IULIIA_ERROR:

	for(i = 0; i < arr->length; i++) {
		if(new_samples[i].in) free(new_samples[i].in);
		if(new_samples[i].out) free(new_samples[i].out);
	}

	free(new_samples);

	return false;
}

iuliia_scheme_t *iuliiaLoadSchemeFromMemory(char *json, size_t json_length)
{
	iuliia_scheme_t *scheme;
	struct json_value_s *root = json_parse(json, json_length);
	struct json_object_s *object;
	struct json_object_element_s *el;

	object = json_value_as_object(root);
	if(!json) {
		free(root);

		return 0;
	}

	scheme = malloc(sizeof(iuliia_scheme_t));
	if(!scheme) {
		free(root);

		return 0;
	}
	memset(scheme, 0, sizeof(iuliia_scheme_t));

	el = object->start;
	while(el) {
		if(!strncmp(el->name->string, "name", el->name->string_size)) {
			if(!iuliiaIntJsonLoadStringW(el->value, &(scheme->name))) goto IULIIA_ERROR;
		} else if(!strncmp(el->name->string, "description", el->name->string_size)) {
			if(!iuliiaIntJsonLoadStringW(el->value, &(scheme->description))) goto IULIIA_ERROR;
		} else if(!strncmp(el->name->string, "url", el->name->string_size)) {
			if(!iuliiaIntJsonLoadStringW(el->value, &(scheme->url))) goto IULIIA_ERROR;
		} else if(!strncmp(el->name->string, "mapping", el->name->string_size)) {
			struct json_object_s* obj;

			obj = json_value_as_object(el->value);
			if(!obj) goto IULIIA_ERROR;
			if(!iuliiaIntJsonReadMapping1char(obj, &(scheme->mapping))) goto IULIIA_ERROR;
			scheme->nof_mapping = obj->length;
		} else if(!strncmp(el->name->string, "prev_mapping", el->name->string_size)) {
		} else if(!strncmp(el->name->string, "next_mapping", el->name->string_size)) {
		} else if(!strncmp(el->name->string, "ending_mapping", el->name->string_size)) {
		} else if(!strncmp(el->name->string, "samples", el->name->string_size)) {
			struct json_array_s *arr;

			arr = json_value_as_array(el->value);
			if(!arr) goto IULIIA_ERROR;
			if(!iuliiaIntJsonReadSamples(arr, &(scheme->samples))) goto IULIIA_ERROR;
			scheme->nof_samples = arr->length;
		}

		el = el->next;
	}

	free(root);

	return scheme;

IULIIA_ERROR:

	iuliiaFreeScheme(scheme);

	return 0;
}

void iuliiaFreeScheme(iuliia_scheme_t *scheme)
{
	if(!scheme) return;

	if(scheme->name) free(scheme->name);
	if(scheme->description) free(scheme->description);
	if(scheme->url) free(scheme->url);

	if(scheme->nof_mapping) {
		size_t i;

		for(i = 0; i < scheme->nof_mapping; i++) {
			if(scheme->mapping[i].repl) free(scheme->mapping[i].repl);
		}

		free(scheme->mapping);
	}

	if(scheme->nof_samples) {
		size_t i;

		for(i = 0; i < scheme->nof_samples; i++) {
			free(scheme->samples[i].in);
			free(scheme->samples[i].out);
		}

		free(scheme->samples);
	}

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

const uint8_t *iuliiaCharU8toU32(const uint8_t *u8, uint32_t *u32)
{
	uint32_t new_u32 = 0;
	const uint8_t *pu8;
	
	pu8 = u8;

	if(*pu8 < 0x80) {
		new_u32 = *pu8;
		pu8++;
	} else {
		if((*pu8 & 0xe0) == 0xc0) {
			new_u32 = (*pu8 & 0x1f) << 6;
			pu8++;
			if(!(*pu8)) return 0;
			if((*pu8 & 0xc0) == 0x80) {
				new_u32 += *pu8 & 0x3f;
				pu8++;
			} else return 0;
		} else if((*pu8 & 0xf0) == 0xe0) {
			new_u32 = (*pu8 & 0xf) << 12;
			pu8++;

			if(!(*pu8)) return 0;
			if((*pu8 & 0xc0) == 0x80) {
				new_u32 += (*pu8 & 0x3f) << 6;
				pu8++;
			} else return 0;

			if (!(*pu8)) return 0;
			if((*pu8 & 0xc0) == 0x80) {
				new_u32 += *pu8 & 0x3f;
				pu8++;
			} else return 0;
		} else if((*pu8 & 0xf8) == 0xf0) {
			new_u32 = (*pu8 & 0xf) << 18;
			pu8++;

			if(!(*pu8)) return 0;
			if((*pu8 & 0xc0) == 0x80) {
				new_u32 += (*pu8 & 0x3f) << 12;
				pu8++;
			} else return 0;

			if(!(*pu8)) return 0;
			if((*pu8 & 0xc0) == 0x80) {
				new_u32 += (*pu8 & 0x3f) << 6;
				pu8++;
			} else return 0;

			if (!(*pu8)) return 0;
			if((*pu8 & 0xc0) == 0x80) {
				new_u32 += *pu8 & 0x3f;
				pu8++;
			} else return 0;
		} else // TODO: Fix this
			return 0;
	} 

	*u32 = new_u32;

	return pu8;
}

uint32_t *iuliiaU8toU32(const uint8_t *u8)
{
	const uint8_t *pu8;
	uint32_t *u32, *pu32;
	size_t str_size;

	str_size = strlen(u8);

	u32 = malloc((str_size+1)*sizeof(uint32_t));

	pu8 = u8;
	pu32 = u32;
	while(1) {
		bool need_break = false;

		if (*pu8 == 0) need_break = true;

		pu8 = iuliiaCharU8toU32(pu8, pu32);
		if(!pu8) {
			free(u32);

			return 0;
		}

		if(need_break) break;

		pu32++;
	}

	return u32;
}

uint32_t iuliiaU32ToLower(uint32_t c)
{
	if(c < 65536)
		return towlower(c);
	else
		return c;
}

uint32_t iuliiaU32ToUpper(uint32_t c)
{
	if(c < 65536)
		return towupper(c);
	else
		return c;
}

int iuliiaU32IsUpper(uint32_t c)
{
	if(c < 65536)
		return iswupper(c);
	else
		return 0;
}

uint32_t *iuliiaTranslateU32(const uint32_t *s, const iuliia_scheme_t *scheme)
{
	uint32_t *new_s;
	size_t new_len, new_offset = 0;

	new_len = iuliiaU32len(s);
	new_s = malloc((new_len+1)*sizeof(uint32_t));
	if(!new_s) return 0;
	
	while(*s) {
		uint32_t *repl = 0;
		size_t i;
		if(new_offset == new_len) {
			uint32_t *_new_s;

			_new_s = realloc(new_s, (new_len+5)*sizeof(uint32_t));
			if(!_new_s) {
				free(new_s);
				return 0;
			}
			new_s = _new_s;
			new_len += 4;
		}

		for(i = 0; i < scheme->nof_mapping; i++) {
			if(scheme->mapping[i].c == iuliiaU32ToLower(*s)) {
				repl = scheme->mapping[i].repl;
				break;
			}
		}

		if(repl) {
			bool first_char = true;

			while(*repl) {
				if(new_offset == new_len) {
					uint32_t* _new_s;

					_new_s = realloc(new_s, (new_len+5)*sizeof(uint32_t));
					if(!_new_s) {
						free(new_s);
						return 0;
					}
					new_s = _new_s;
					new_len += 4;
				}
				if(iuliiaU32IsUpper(*s) && first_char)
					new_s[new_offset] = iuliiaU32ToUpper(*repl);
				else
					new_s[new_offset] = *repl;
				new_offset++;
				repl++;
				first_char = false;
			}
		} else {
			new_s[new_offset] = *s;
			new_offset++;
		}

		s++;
	}
	new_s[new_offset] = 0;
	
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
