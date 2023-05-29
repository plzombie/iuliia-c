/*
MIT License

Copyright (c) 2023 Mikhail Morozov

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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <locale.h>
#include <limits.h>

#include <sys/types.h>
#include <fcntl.h>
#if defined(_WIN32)
#include <io.h>
#endif

#if defined(_DEBUG) && defined(USE_STB_LEAKCHECK)
#define STB_LEAKCHECK_IMPLEMENTATION
#include "forks/stb/stb_leakcheck.h"
#endif

#if defined(_WIN32)
#define IULIIALOADSCHEME(f) iuliiaLoadSchemeW(f)
#define IULIIATRANSLATE(t, s) iuliiaTranslateW(t, s)
#define PRINTF(t) wprintf(L##t);
#define STDERR_PRINTF(t) fwprintf(stderr, L##t)
#define FPUTS(t, f) fputws(t, f)
#define FGETS(t, f) fgetws(t, 1024, f)
#define FOPEN(f, a) _wfopen(f, L##a L", ccs=UTF-8")
#define CHAR wchar_t
#else
#define IULIIALOADSCHEME(f) iuliiaLoadSchemeA(f)
#define IULIIATRANSLATE(t, s) iuliiaTranslateA(t, s)
#define PRINTF(t) printf(t)
#define STDERR_PRINTF(t) fprintf(stderr, t)
#define FPUTS(t, f) fputs(t, f)
#define FGETS(t, f) fgets(t, 1024, f)
#define FOPEN(f, a) fopen(f, a)
#define CHAR char
#define _ftelli64 ftello64
#endif

#if defined(_WIN32)
int wmain(int argc, wchar_t **argv)
#else
int main(int argc, char** argv)
#endif
{
	CHAR *scheme_filename = 0, *input_filename = 0, *output_filename = 0;
	CHAR original_text[1024], *translated_text = 0;
	iuliia_scheme_t *scheme = 0;
	FILE *f_input = 0, *f_output = 0;

	if(argc < 2) {
		PRINTF("iuliia-c scheme_filename [input_filename] [output_filename]\n");

		return EXIT_SUCCESS;
	}

	setlocale(LC_ALL, "");

#ifdef _MSC_VER
	_setmode(_fileno(stdin), _O_U16TEXT);
	_setmode(_fileno(stdout), _O_U16TEXT);
	_setmode(_fileno(stderr), _O_U16TEXT);
#endif

	scheme_filename = argv[1];
	if(argc > 2) input_filename = argv[2];
	if(argc > 3) output_filename = argv[3];

	scheme = IULIIALOADSCHEME(scheme_filename);
	if(!scheme) {
		STDERR_PRINTF("Scheme not loaded\n");

		return EXIT_FAILURE;
	}

	if(input_filename) {
		f_input = FOPEN(input_filename, "r");

		if(!f_input) {
			iuliiaFreeScheme(scheme);

			return EXIT_FAILURE;
		}
	} else
		f_input = stdin;

	if(output_filename) {
		f_output = FOPEN(output_filename, "w");

		if(!f_output) {
			iuliiaFreeScheme(scheme);
			if(input_filename) fclose(f_input);

			return EXIT_FAILURE;
		}
	} else
		f_output = stdout;

	while(!feof(f_input)) {
		if(!FGETS(original_text, f_input)) break;

		translated_text = IULIIATRANSLATE(original_text, scheme);
		if(!translated_text) {
			iuliiaFreeScheme(scheme);
			if(input_filename) fclose(f_input);
			if(output_filename) fclose(f_output);

			return EXIT_FAILURE;
		}
	
		FPUTS(translated_text, f_output);
		iuliiaFreeString(translated_text);
	}

	iuliiaFreeScheme(scheme);
	if(input_filename) fclose(f_input);
	if(output_filename) fclose(f_output);

#if defined(_DEBUG) && defined(USE_STB_LEAKCHECK)
	stb_leakcheck_dumpmem();
#endif

	return EXIT_SUCCESS;
}
