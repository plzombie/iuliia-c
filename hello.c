
#include "iuliia.h"

#include <stdio.h>
#include <locale.h>

#include <sys/types.h>
#include <fcntl.h>
#include <io.h>

int main(void)
{
	iuliia_scheme_t *scheme = 0;
	wchar_t *new_s;
	const wchar_t *s = L"Привет, мир!";

	setlocale(LC_ALL, "");

#ifdef _MSC_VER
	_setmode(_fileno(stdin), _O_U16TEXT);
	_setmode(_fileno(stdout), _O_U16TEXT);
	_setmode(_fileno(stderr), _O_U16TEXT);
#endif

	scheme = iuliiaLoadSchemeW(L"../../../forks/iuliia/wikipedia.json");
	if(!scheme) {
		wprintf(L"Scheme not loaded\n");

		return 0;
	}

	new_s = iuliiaTranslateW(s, scheme);

	if(new_s) {
		wprintf(L"Before: %ls\n", s);
		wprintf(L"After: %ls\n", new_s);
	} else
		wprintf(L"Error translating from W to W\n");

	iuliiaFreeScheme(scheme);

	return 0;
}