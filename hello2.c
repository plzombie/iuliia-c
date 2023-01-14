
#include "iuliia.h"

#include <stdio.h>
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

int main(void)
{
	iuliia_scheme_t* scheme = 0;
	wchar_t* new_s;
	const wchar_t* s = L"Hello \x263b";
	size_t i;

	setlocale(LC_ALL, "");

#ifdef _MSC_VER
	_setmode(_fileno(stdin), _O_U16TEXT);
	_setmode(_fileno(stdout), _O_U16TEXT);
	_setmode(_fileno(stderr), _O_U16TEXT);
#endif

	scheme = iuliiaLoadSchemeW(L"../../../my_schemes/smiles.json");
	if(!scheme) {
		wprintf(L"Scheme not loaded\n");

		return 0;
	}

	if(scheme->name)
		wprintf(L"Scheme name: %ls\n", scheme->name);
	if(scheme->description)
		wprintf(L"Scheme description: %ls\n", scheme->description);
	if(scheme->url)
		wprintf(L"Scheme url: %ls\n", scheme->url);

	new_s = iuliiaTranslateW(s, scheme);

	if(new_s) {
		wprintf(L"Before: %ls\n", s);
		wprintf(L"After: %ls\n", new_s);
		iuliiaFreeString(new_s);
	}
	else
		wprintf(L"Error translating\n");

	for(i = 0; i < scheme->nof_samples; i++) {
		wprintf(L"Sample %u\n", (unsigned int)i);
		wprintf(L"Before: %ls\n", scheme->samples[i].in);
		new_s = iuliiaTranslateW(scheme->samples[i].in, scheme);
		if(new_s) {
			wprintf(L"After: %ls\n", new_s);
			iuliiaFreeString(new_s);
		} else
			wprintf(L"Error translating\n");
		wprintf(L"Should be: %ls\n", scheme->samples[i].out);
	}

	iuliiaFreeScheme(scheme);

#if defined(_DEBUG)
	stb_leakcheck_dumpmem();
#endif

	return 0;
}