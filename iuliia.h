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

#ifndef IULIIA_H
#define IULIIA_H

#include <wchar.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint32_t c;
	uint32_t *repl;
} iuliia_mapping_1char_t;

typedef struct {
	uint32_t c; // Main characher
	uint32_t cor_c; // Corresponding characher
	uint32_t *repl;
} iuliia_mapping_2char_t;

typedef struct {
	wchar_t *in;
	wchar_t *out;
} iuliia_samples_t;

typedef struct {
	wchar_t *name;
	wchar_t *description;
	wchar_t *url;
	iuliia_mapping_1char_t *mapping;
	size_t nof_mapping;
	iuliia_mapping_2char_t* prev_mapping;
	size_t nof_prev_mapping;
	iuliia_mapping_2char_t *next_mapping;
	size_t nof_next_mapping;
	iuliia_mapping_2char_t *ending_mapping;
	size_t nof_ending_mapping;
	iuliia_samples_t *samples;
	size_t nof_samples;
} iuliia_scheme_t;

extern iuliia_scheme_t *iuliiaLoadSchemeFromMemory(char* json, size_t json_length);
extern void iuliiaFreeScheme(iuliia_scheme_t* scheme);

extern iuliia_scheme_t *iuliiaLoadSchemeFromFile(FILE *f);
extern iuliia_scheme_t *iuliiaLoadSchemeW(const wchar_t *filename);
extern iuliia_scheme_t *iuliiaLoadSchemeA(const char *filename);

extern size_t iuliiaU32len(const uint32_t *s);
extern wchar_t *iuliiaU32toW(const uint32_t *s);
extern uint32_t *iuliiaWtoU32(const wchar_t *s);
extern const uint8_t *iuliiaCharU8toU32(const uint8_t *u8, uint32_t *u32);
extern uint32_t *iuliiaU8toU32(const uint8_t *u8);

extern uint32_t iuliiaU32ToLower(uint32_t c);
extern uint32_t iuliiaU32ToUpper(uint32_t c);
extern int iuliiaU32IsUpper(uint32_t c);
extern int iuliiaU32IsBlank(uint32_t c);

extern uint32_t *iuliiaTranslateU32(const uint32_t *s, const iuliia_scheme_t *scheme);

extern uint32_t *iuliiaTranslateWtoU32(const wchar_t *s, const iuliia_scheme_t *scheme);
extern wchar_t *iuliiaTranslateW(const wchar_t *s, const iuliia_scheme_t *scheme);

extern uint32_t *iuliiaTranslateAtoU32(const char *s, const iuliia_scheme_t *scheme);
extern wchar_t* iuliiaTranslateAtoW(const char *s, const iuliia_scheme_t *scheme);
extern char *iuliiaTranslateA(const char *s, const iuliia_scheme_t *scheme);

extern void iuliiaFreeString(void *s);

#ifdef __cplusplus
}
#endif

#endif
