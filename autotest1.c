
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

#if defined(_DEBUG)
#include "forks/stb/stb_leakcheck.h"
#endif

bool TestScheme(const wchar_t *scheme_name, size_t *passed, size_t *missed);

const wchar_t *scheme_names[] = {
		L"../../../forks/iuliia/ala_lc.json",
		L"../../../forks/iuliia/ala_lc_alt.json",
		L"../../../forks/iuliia/bgn_pcgn.json",
		L"../../../forks/iuliia/bgn_pcgn_alt.json",
		L"../../../forks/iuliia/bs_2979.json",
		L"../../../forks/iuliia/bs_2979_alt.json",
		L"../../../forks/iuliia/gost_16876.json",
		L"../../../forks/iuliia/gost_16876_alt.json",
		L"../../../forks/iuliia/gost_52290.json",
		L"../../../forks/iuliia/gost_52535.json",
		L"../../../forks/iuliia/gost_7034.json",
		L"../../../forks/iuliia/gost_779.json",
		L"../../../forks/iuliia/gost_779_alt.json",
		L"../../../forks/iuliia/icao_doc_9303.json",
		L"../../../forks/iuliia/iso_9_1954.json",
		L"../../../forks/iuliia/iso_9_1968.json",
		L"../../../forks/iuliia/iso_9_1968_alt.json",
		L"../../../forks/iuliia/mosmetro.json",
		L"../../../forks/iuliia/mvd_310.json",
		L"../../../forks/iuliia/mvd_310_fr.json",
		L"../../../forks/iuliia/mvd_782.json",
		L"../../../forks/iuliia/scientific.json",
		L"../../../forks/iuliia/telegram.json",
		L"../../../forks/iuliia/ungegn_1987.json",
		L"../../../forks/iuliia/wikipedia.json",
		L"../../../forks/iuliia/yandex_maps.json",
		L"../../../forks/iuliia/yandex_money.json"
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
		size_t current_passed = 0, current_missed = 0;

		if(!TestScheme(scheme_names[i], &current_passed, &current_missed)) failed_schemes++;

		passed_tests += current_passed;
		missed_tests += current_missed;
	}

	wprintf(L"Total failed to open schemes: %u\n", (unsigned int)failed_schemes);
	wprintf(L"Total passed tests: %u\n", (unsigned int)passed_tests);
	wprintf(L"Total missed tests: %u\n", (unsigned int)missed_tests);


#if defined(_DEBUG)
	stb_leakcheck_dumpmem();
#endif

	if(failed_schemes || missed_tests)
		return EXIT_FAILURE;
	else
		return EXIT_SUCCESS;
}

bool TestScheme(const wchar_t *scheme_name, size_t *passed, size_t *missed)
{
	size_t current_passed = 0, current_missed = 0, i;
	iuliia_scheme_t *scheme;

	*passed = 0;
	*missed = 0;

	scheme = iuliiaLoadSchemeW(scheme_name);
	if(!scheme) return false;

	for(i = 0; i < scheme->nof_samples; i++) {
		wchar_t *new_s;

		new_s = iuliiaTranslateW(scheme->samples[i].in, scheme);
		if(new_s) {
			if(!wcscmp(new_s, scheme->samples[i].out))
				current_passed += 1;
			else {
				current_missed += 1;
				wprintf(L"Scheme: %ls\n", scheme_name);
				wprintf(L"Sample %u\n", (unsigned int)i);
				wprintf(L"Before: %ls\n", scheme->samples[i].in);
				wprintf(L"After: %ls\n", new_s);
				wprintf(L"Should be: %ls\n", scheme->samples[i].out);
			}
			iuliiaFreeString(new_s);
		} else
			current_missed++;
	}

	iuliiaFreeScheme(scheme);

	*passed = current_passed;
	*missed = current_missed;

	return true;
}
