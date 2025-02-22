
#include "iuliia.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <locale.h>

#include <sys/types.h>
#include <fcntl.h>
#if defined(_WIN32)
#include <io.h>
#endif

#if defined(_DEBUG) && defined(USE_STB_LEAKCHECK)
#define STB_LEAKCHECK_IMPLEMENTATION
#include "forks/stb/stb_leakcheck.h"
#endif

const wchar_t *scheme_names[] = {
		L"../test_schemes/wikipedia_cp1251_all.json",
		L"../test_schemes/wikipedia_cp1251_description.json",
		L"../test_schemes/wikipedia_cp1251_ending_mapping.json",
		L"../test_schemes/wikipedia_cp1251_ending_mapping_part.json",
		L"../test_schemes/wikipedia_cp1251_mapping.json",
		L"../test_schemes/wikipedia_cp1251_mapping_part.json",
		L"../test_schemes/wikipedia_cp1251_name.json",
		L"../test_schemes/wikipedia_cp1251_next_mapping.json",
		L"../test_schemes/wikipedia_cp1251_next_mapping_part.json",
		L"../test_schemes/wikipedia_cp1251_prev_mapping.json",
		L"../test_schemes/wikipedia_cp1251_prev_mapping_part.json",
		L"../test_schemes/wikipedia_cp1251_samples.json",
		L"../test_schemes/wikipedia_cp1251_samples_part.json",
		L"../test_schemes/wikipedia_cp1251_url.json"
	};

int main(void)
{
	size_t failed_schemes = 0, passed_tests = 0, missed_tests = 0, i;
	setlocale(LC_ALL, "");

#ifdef _MSC_VER
	_setmode(_fileno(stdin), _O_U16TEXT);
	_setmode(_fileno(stdout), _O_U16TEXT);
	_setmode(_fileno(stderr), _O_U16TEXT);
#endif

	for(i = 0; i < sizeof(scheme_names)/sizeof(wchar_t *); i++) {
		iuliia_scheme_t *scheme = 0;

		scheme = iuliiaLoadSchemeW(scheme_names[i]);
		if(!scheme) {
			failed_schemes++;
			passed_tests++;
			iuliiaFreeScheme(scheme);
		} else {
			wprintf(L"Unintentionally opened \"%ls\"\n", scheme_names[i]);

			missed_tests++;
		}
	}

	wprintf(L"Total failed to open schemes: %u\n", (unsigned int)failed_schemes);
	wprintf(L"Total passed tests: %u\n", (unsigned int)passed_tests);
	wprintf(L"Total missed tests: %u\n", (unsigned int)missed_tests);


#if defined(_DEBUG) && defined(USE_STB_LEAKCHECK)
#ifdef _MSC_VER
	_setmode(_fileno(stdin), _O_TEXT);
	_setmode(_fileno(stdout), _O_TEXT);
	_setmode(_fileno(stderr), _O_TEXT);
#endif
	stb_leakcheck_dumpmem();
#endif

	if(missed_tests)
		return EXIT_FAILURE;
	else
		return EXIT_SUCCESS;
}
