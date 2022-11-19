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

typedef struct {
	void *p;
} iuliia_scheme_t;

extern iuliia_scheme_t *iuliiaLoadSchemeFromMemory(char* json, size_t json_length);
extern void iuliiaFreeScheme(iuliia_scheme_t* scheme);

extern iuliia_scheme_t *iuliiaLoadSchemeW(wchar_t *filename);

extern size_t iuliiaU32len(const uint32_t *s);
extern wchar_t *iuliiaU32toW(const uint32_t *s);
extern uint32_t *iuliiaWtoU32(const wchar_t *s);

extern uint32_t *iuliiaTranslateU32(const uint32_t *s, const iuliia_scheme_t *scheme);

extern uint32_t *iuliiaTranslateWtoU32(const wchar_t *s, const iuliia_scheme_t *scheme);
extern wchar_t *iuliiaTranslateW(const wchar_t *s, const iuliia_scheme_t *scheme);

extern uint32_t *iuliiaTranslateAtoU32(const char *s, const iuliia_scheme_t *scheme);
extern char *iuliiaTranslateA(const char *s, const iuliia_scheme_t *scheme);

extern void iuliiaFreeString(void *s);

#endif
