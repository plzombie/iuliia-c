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

#if defined(_DEBUG) && defined(USE_STB_LEAKCHECK)
#include "forks/stb/stb_leakcheck.h"
#endif

#if defined(_WIN32)
#include <Windows.h>
#else
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include "iuliia.h"
#include "forks/json.h/json.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <wctype.h>

#include <errno.h>

static bool iuliiaIntJsonLoadStringW(struct json_value_s *value, wchar_t **str)
{
	struct json_string_s *val;

	val = json_value_as_string(value);
	if(!val) return false;

	if(SIZE_MAX/sizeof(wchar_t) <= val->string_size) return false;
	
	*str = malloc((val->string_size + 1) * sizeof(wchar_t));
	if(!(*str)) return false;
	(*str)[val->string_size] = 0;

#if defined(_WIN32)
	MultiByteToWideChar(CP_UTF8, 0, val->string, (int)(val->string_size+1), *str, (int)(val->string_size+1));
#else
	mbstowcs(*str, val->string, val->string_size);
#endif

	return true;
}

static bool iuliiaIntJsonReadMapping1char(struct json_object_s *obj, iuliia_mapping_1char_t **map)
{
	size_t i;
	struct json_object_element_s *el;
	iuliia_mapping_1char_t *new_map;

	if(SIZE_MAX/sizeof(iuliia_mapping_1char_t) < obj->length) return false;

	new_map = malloc(obj->length*sizeof(iuliia_mapping_1char_t));
	if(!new_map) return false;
	memset(new_map, 0, obj->length*sizeof(iuliia_mapping_1char_t));

	el = obj->start;
	for(i = 0; i < obj->length; i++) {
		const uint8_t *new_c;
		struct json_string_s *str;

		new_c = iuliiaCharU8toU32((const uint8_t *)el->name->string, &(new_map[i].c));
		if(!new_c) return false;

		str = json_value_as_string(el->value);
		if(!str) return false;

		new_map[i].repl = iuliiaU8toU32((const uint8_t *)str->string);

		el = el->next;
	}

	*map = new_map;

	return true;
}

static bool iuliiaIntJsonReadMapping2char(struct json_object_s *obj, iuliia_mapping_2char_t **map, bool cor_first)
{
	size_t i;
	struct json_object_element_s *el;
	iuliia_mapping_2char_t *new_map;

	if(SIZE_MAX/sizeof(iuliia_mapping_2char_t) < obj->length) return false;

	new_map = malloc(obj->length*sizeof(iuliia_mapping_2char_t));
	if(!new_map) return false;
	memset(new_map, 0, obj->length*sizeof(iuliia_mapping_2char_t));

	el = obj->start;
	for(i = 0; i < obj->length; i++) {
		uint32_t *in_str;
		size_t in_str_len;
		struct json_string_s *str;

		in_str = iuliiaU8toU32((const uint8_t *)el->name->string);
		if(!in_str) {
			free(new_map);

			return false;
		}
		in_str_len = iuliiaU32len(in_str);
		if(in_str_len == 1)
			new_map[i].c = in_str[0];
		else if(in_str_len == 2) {
			if(cor_first) {
				new_map[i].cor_c = in_str[0];
				new_map[i].c = in_str[1];
			} else {
				new_map[i].c = in_str[0];
				new_map[i].cor_c = in_str[1];
			}
		} else {
			free(in_str);
			return false;
		}
		free(in_str);

		str = json_value_as_string(el->value);
		if(!str) {
			free(new_map);

			return false;
		}

		new_map[i].repl = iuliiaU8toU32((const uint8_t *)str->string);

		el = el->next;
	}

	*map = new_map;

	return true;
}

static bool iuliiaIntJsonReadSamples(struct json_array_s *arr, iuliia_samples_t **samples)
{
	iuliia_samples_t *new_samples;
	struct json_array_element_s *arr_el;
	size_t i = 0;

	if(SIZE_MAX/sizeof(iuliia_samples_t) < arr->length) return false;

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

	if(!root) return 0;

	object = json_value_as_object(root);
	if(!object) {
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
			if(!json_value_is_null(el->value)) {
				struct json_object_s *obj;

				obj = json_value_as_object(el->value);
				if(!obj) goto IULIIA_ERROR;
				if(!iuliiaIntJsonReadMapping1char(obj, &(scheme->mapping))) goto IULIIA_ERROR;
				scheme->nof_mapping = obj->length;
			}
		} else if(!strncmp(el->name->string, "prev_mapping", el->name->string_size)) {
			if(!json_value_is_null(el->value)) {
				struct json_object_s *obj;

				obj = json_value_as_object(el->value);
				if(!obj) goto IULIIA_ERROR;
				if(!iuliiaIntJsonReadMapping2char(obj, &(scheme->prev_mapping), true)) goto IULIIA_ERROR;
				scheme->nof_prev_mapping = obj->length;
			}
		} else if(!strncmp(el->name->string, "next_mapping", el->name->string_size)) {
			if(!json_value_is_null(el->value)) {
				struct json_object_s *obj;

				obj = json_value_as_object(el->value);
				if(!obj) goto IULIIA_ERROR;
				if(!iuliiaIntJsonReadMapping2char(obj, &(scheme->next_mapping), false)) goto IULIIA_ERROR;
				scheme->nof_next_mapping = obj->length;
			}
		} else if(!strncmp(el->name->string, "ending_mapping", el->name->string_size)) {
			if(!json_value_is_null(el->value)) {
				struct json_object_s *obj;

				obj = json_value_as_object(el->value);
				if(!obj) goto IULIIA_ERROR;
				if(!iuliiaIntJsonReadMapping2char(obj, &(scheme->ending_mapping), false)) goto IULIIA_ERROR;
				scheme->nof_ending_mapping = obj->length;
			}
		} else if(!strncmp(el->name->string, "samples", el->name->string_size)) {
			if(!json_value_is_null(el->value)) {
				struct json_array_s *arr;

				arr = json_value_as_array(el->value);
				if(!arr) goto IULIIA_ERROR;
				if(!iuliiaIntJsonReadSamples(arr, &(scheme->samples))) goto IULIIA_ERROR;
				scheme->nof_samples = arr->length;
			}
		}

		el = el->next;
	}

	free(root);

	iuliiaPrepareScheme(scheme);

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

	if(scheme->nof_prev_mapping) {
		size_t i;

		for(i = 0; i < scheme->nof_prev_mapping; i++) {
			if(scheme->prev_mapping[i].repl) free(scheme->prev_mapping[i].repl);
		}

		free(scheme->prev_mapping);
	}

	if(scheme->nof_next_mapping) {
		size_t i;

		for(i = 0; i < scheme->nof_next_mapping; i++) {
			if(scheme->next_mapping[i].repl) free(scheme->next_mapping[i].repl);
		}

		free(scheme->next_mapping);
	}

	if(scheme->nof_ending_mapping) {
		size_t i;

		for(i = 0; i < scheme->nof_ending_mapping; i++) {
			if(scheme->ending_mapping[i].repl) free(scheme->ending_mapping[i].repl);
		}

		free(scheme->ending_mapping);
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

static int iuliiaCompare1char(const iuliia_mapping_1char_t *a, const iuliia_mapping_1char_t *b)
{
	return a->c - b->c;
}

static int iuliiaCompare2char(const iuliia_mapping_2char_t *a, const iuliia_mapping_2char_t *b)
{
	if(a->c != b->c)
		return a->c - b->c;
	else
		return a->cor_c - b->cor_c;
}

typedef int (* iuliia_comparator_t)(const void*, const void*);

void iuliiaPrepareScheme(iuliia_scheme_t *scheme)
{
	qsort(scheme->mapping, scheme->nof_mapping, sizeof(iuliia_mapping_1char_t), (iuliia_comparator_t)iuliiaCompare1char);
	qsort(scheme->prev_mapping, scheme->nof_prev_mapping, sizeof(iuliia_mapping_2char_t), (iuliia_comparator_t)iuliiaCompare2char);
	qsort(scheme->next_mapping, scheme->nof_next_mapping, sizeof(iuliia_mapping_2char_t), (iuliia_comparator_t)iuliiaCompare2char);
	qsort(scheme->ending_mapping, scheme->nof_ending_mapping, sizeof(iuliia_mapping_2char_t), (iuliia_comparator_t)iuliiaCompare2char);
}

iuliia_scheme_t *iuliiaLoadSchemeFromFile(FILE *f)
{
	char *json;
	size_t json_length;
	long long f_size;
	iuliia_scheme_t *scheme;

	if(!f) return 0;

	if(fseek(f, 0, SEEK_END)) return 0;

#if defined(_WIN32)
	f_size = _ftelli64(f);
#else
	f_size = ftello64(f);
#endif
	if(f_size >= SIZE_MAX) return 0;

	json_length = (size_t)f_size;

	if(fseek(f, 0, SEEK_SET)) return 0;

	json = malloc(json_length+1);
	if(!json) return 0;
	json[json_length] = 0;

	if(fread(json, 1, json_length, f) != json_length) {
		free(json);

		return 0;
	}

	scheme = iuliiaLoadSchemeFromMemory(json, json_length);

	free(json);

	return scheme;
}

#if !defined(_WIN32)
static FILE *_wfopen(const wchar_t *filename, const wchar_t *mode)
{
	size_t filename_len, mode_len;
	char *cfilename = 0, *cmode = 0;
	FILE *f = 0;

	filename_len = wcslen(filename);
	mode_len = wcslen(mode);

	if(SIZE_MAX/MB_CUR_MAX <= filename_len || SIZE_MAX/MB_CUR_MAX <= mode_len) {
		errno = ENAMETOOLONG;
		return 0;
	}

	cfilename = malloc(filename_len * MB_CUR_MAX + 1);
	cmode = malloc(mode_len * MB_CUR_MAX + 1);
	if(!cfilename || !cmode) {
		errno = ENOMEM;
		goto FINAL;
	}

	if(wcstombs(cfilename, filename, filename_len * MB_CUR_MAX + 1) == (size_t)(-1)) {
		errno = EINVAL;
		goto FINAL;
	}

	if(wcstombs(cmode, mode, mode_len * MB_CUR_MAX + 1) == (size_t)(-1)) {
		errno = EINVAL;
		goto FINAL;
	}

	f = fopen(cfilename, cmode);

FINAL:
	if(cfilename) free(cfilename);
	if(cmode) free(cmode);

	return f;
}
#endif

iuliia_scheme_t *iuliiaLoadSchemeW(const wchar_t *filename)
{
	FILE *f;
	iuliia_scheme_t *scheme;

	f = _wfopen(filename, L"rb");
	if(!f) return 0;

	scheme = iuliiaLoadSchemeFromFile(f);

	fclose(f);

	return scheme;
}

iuliia_scheme_t *iuliiaLoadSchemeA(const char *filename)
{
	FILE *f;
	iuliia_scheme_t *scheme;

	f = fopen(filename, "rb");
	if(!f) return 0;

	scheme = iuliiaLoadSchemeFromFile(f);

	fclose(f);

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
	new_s = malloc((s_len+1)*sizeof(uint32_t));
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
				*p = ((*s-0x10000) & 0x3ff) | 0xdc00;
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
	if(SIZE_MAX/sizeof(uint32_t) <= s_len) return 0;
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

	str_size = strlen((const char *)u8);

	if(SIZE_MAX/sizeof(uint32_t) <= str_size) return 0;

	u32 = malloc((str_size+1)*sizeof(uint32_t));
	if(!u32) return 0;

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
	if(c < 128) {
		if(c >= 'A' && c <= 'Z')
			return c + 32;
		else
			return c;
	} else if(c == 0x401) return 0x451;
	else if(c >= 0x410 && c <= 0x44F) {
		if(c <= 0x42F)
			return c + 32;
		else
			return c;
	} if(c < 65536)
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
	if(c >= 'A' && c <= 'Z')
		return 1;
	else if(c >= 0x410 && c <= 0x42F)
		return 1;
	else if(c < 65536)
		return iswupper(c);
	else
		return 0;
}

int iuliiaU32IsAlpha(uint32_t c) {
	if(c < 65536)
		return iswalpha(c);
	else
		return 0;
}

static uint32_t *iuliiaBsearch1char(uint32_t c, const iuliia_mapping_1char_t *mapping, size_t size)
{
	size_t start, end;

	if(!size) return 0;
	if(SIZE_MAX/2 < size) return 0;

	start = 0;
	end = size - 1;

	while(start <= end) {
		size_t mid;

		mid = (end + start) / 2;
		if(mapping[mid].c == c)
			return mapping[mid].repl;
		else if(mapping[mid].c < c)
			start = mid + 1;
		else if(mid > 0) // mapping[mid].c > c
			end = mid - 1;
		else
			break;
	}

	return 0;
}

static uint32_t *iuliiaBsearch2char(uint32_t c, uint32_t cor_c, const iuliia_mapping_2char_t *mapping, size_t size)
{
	size_t start, end;

	if(!size) return 0;
	if(SIZE_MAX/2 < size) return 0;

	start = 0;
	end = size - 1;

	while (start <= end) {
		size_t mid;

		mid = (end + start) / 2;
		if(mapping[mid].c == c) {
			if(mapping[mid].cor_c == cor_c)
				return mapping[mid].repl;
			else if(mapping[mid].cor_c < cor_c)
				start = mid + 1; 
			else if(mid > 0) // mapping[mid].cor_c > cor_c
				end = mid - 1;
			else
				break;
		} else if(mapping[mid].c < c)
			start = mid + 1; 
		else if(mid > 0) // mapping[mid].c > c
			end = mid - 1;
		else
			break;
	}

	return 0;
}

uint32_t *iuliiaTranslateU32(const uint32_t *s, const iuliia_scheme_t *scheme)
{
	uint32_t *new_s, prev_s = 0;
	size_t new_len, new_offset = 0;

	new_len = iuliiaU32len(s);
	new_s = malloc((new_len+1)*sizeof(uint32_t));
	if(!new_s) return 0;
	
	while(*s) {
		uint32_t *repl = 0, next_s, cur_s;
		if(new_offset == new_len) {
			uint32_t *_new_s = 0;

			if(SIZE_MAX/sizeof(uint32_t)-5 >= new_len) _new_s = realloc(new_s, (new_len+5)*sizeof(uint32_t));
			if(!_new_s) {
				free(new_s);
				return 0;
			}
			new_s = _new_s;
			new_len += 4;
		}
		
		next_s = iuliiaU32ToLower(*(s+1));
		cur_s = iuliiaU32ToLower(*s);

		// Check word ending
		if(iuliiaU32IsAlpha(*s) && iuliiaU32IsAlpha(next_s)
			&& next_s != 0) {

			if(*(s+2) == 0 || !iuliiaU32IsAlpha(*(s+2))) {
				repl = iuliiaBsearch2char(cur_s, next_s, scheme->ending_mapping, scheme->nof_ending_mapping);
				if(repl) s++;
			}
		}

		// Check previous mapping
		if(!repl) {
			repl = iuliiaBsearch2char(cur_s, prev_s, scheme->prev_mapping, scheme->nof_prev_mapping);
		}

		// Check next mapping
		if(!repl) {
			repl = iuliiaBsearch2char(cur_s, next_s, scheme->next_mapping, scheme->nof_next_mapping);
		}

		// Check direct mapping
		if(!repl) {
			repl = iuliiaBsearch1char(cur_s, scheme->mapping, scheme->nof_mapping);
		}

		if(repl) {
			bool first_char = true;

			while(*repl) {
				if(new_offset == new_len) {
					uint32_t* _new_s = 0;

					if(SIZE_MAX/sizeof(uint32_t)-5 >= new_len) _new_s = realloc(new_s, (new_len+5)*sizeof(uint32_t));
					if(!_new_s) {
						free(new_s);
						return 0;
					}
					new_s = _new_s;
					new_len += 4;
				}
				if(first_char && iuliiaU32IsUpper(*s))
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

		prev_s = cur_s;
		if(!iuliiaU32IsAlpha(prev_s)) prev_s = 0;
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
		iuliiaFreeString(new_su32);
		if(!new_s) return 0;
		
		return new_s;
	}
}

uint32_t *iuliiaTranslateAtoU32(const char *s, const iuliia_scheme_t *scheme)
{
	wchar_t *sw;
	uint32_t *new_su32;
	size_t s_len;

	s_len = strlen(s);
	if(SIZE_MAX/sizeof(wchar_t) <= s_len) return 0;
	sw = malloc((s_len+1)*sizeof(wchar_t));
	if(!sw) return 0;

	mbstowcs(sw, s, s_len);
	sw[s_len] = 0;

	new_su32 = iuliiaTranslateWtoU32(sw, scheme);
	free(sw);

	return new_su32;
}

wchar_t *iuliiaTranslateAtoW(const char *s, const iuliia_scheme_t *scheme)
{
	wchar_t *sw, *new_sw;
	size_t s_len;

	s_len = strlen(s);
	if(SIZE_MAX/sizeof(wchar_t) <= s_len) return 0;
	sw = malloc((s_len+1)*sizeof(wchar_t));
	if(!sw) return 0;

	mbstowcs(sw, s, s_len);
	sw[s_len] = 0;

	new_sw = iuliiaTranslateW(sw, scheme);
	free(sw);
	
	return new_sw;
}

char *iuliiaTranslateA(const char *s, const iuliia_scheme_t *scheme)
{
	char *new_s;
	wchar_t *new_sw;
	size_t new_sw_len, new_s_len;

	new_sw = iuliiaTranslateAtoW(s, scheme);
	if(!new_sw) return 0;

	new_sw_len = wcslen(new_sw);
	if(SIZE_MAX/MB_CUR_MAX <= new_sw_len) {
		free(new_sw);

		return 0;
	}
	new_s_len = MB_CUR_MAX*new_sw_len;
	new_s = malloc(new_s_len+1);
	if(!new_s) {
		free(new_sw);

		return 0;
	}

	wcstombs(new_s, new_sw, new_s_len);
	new_s[new_s_len] = 0;
	free(new_sw);

	return new_s;
}

void iuliiaFreeString(void *s)
{
	if(s) free(s);
}
