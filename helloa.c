
#include "iuliia.h"

#include <stdio.h>
#include <locale.h>

#if defined(_DEBUG) && defined(USE_STB_LEAKCHECK)
#define STB_LEAKCHECK_IMPLEMENTATION
#include "forks/stb/stb_leakcheck.h"
#endif

int main(void)
{
	iuliia_scheme_t *scheme = 0;
	char *new_s;
#ifdef _WIN32
#ifdef __WATCOMC__
	const char *s = "\x8F\xE0\xA8\xA2\xA5\xE2, \xAC\xA8\xE0!"; // CP866
#else
	const char *s = "\xCF\xF0\xE8\xE2\xE5\xF2, \xEC\xE8\xF0!"; // CP1251
#endif
#else
	const char *s = "Привет, мир!";
#endif

	setlocale(LC_ALL, "");

	scheme = iuliiaLoadSchemeA("../../../forks/iuliia/wikipedia.json");
	if(!scheme) {
		printf("Scheme not loaded\n");

		return 0;
	}

	if(scheme->name)
		printf("Scheme name: %ls\n", scheme->name);
	if(scheme->description)
		printf("Scheme description: %ls\n", scheme->description);
	if(scheme->url)
		printf("Scheme url: %ls\n", scheme->url);

	new_s = iuliiaTranslateA(s, scheme);

	if(new_s) {
		printf("Before: %s\n", s);
		printf("After: %s\n", new_s);
		iuliiaFreeString(new_s);
	} else
		printf("Error translating\n");

	iuliiaFreeScheme(scheme);

#if defined(_DEBUG) && defined(USE_STB_LEAKCHECK)
	stb_leakcheck_dumpmem();
#endif

	return 0;
}
