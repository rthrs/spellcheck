#include "dictionary.h"
#include <string.h>
#include <locale.h>
#include <wctype.h>
#include <stdio.h>
#include <stdlib.h>

/** Maksymalna długość słowa.
	Maksymalna długość słowa bez kończącego znaku '\0'
  */
#define MAX_WORD_LENGTH 63


/** Makro służące do otrzymania stałej w formie napisu */
#define str(x)          # x
/** Makro służące do otrzymania stałej w formie napisu */
#define xstr(x)         str(x)



/** Zamienia słowo na złożone z małych liter.
  @param[in,out] word Modyfikowane słowo.
  @return 0, jeśli słowo nie jest złożone z samych liter, 1 w p.p.
 */
int make_lowercase(wchar_t *word)
{
	for (wchar_t *w = word; *w; ++w)
		if (!iswalpha(*w))
			return 0;
		else
			*w = towlower(*w);
	return 1;
}


void write_hints(struct dictionary *dict, int w, int z, wchar_t *word,
				 wchar_t *word_lower_case)
{
	struct word_list list;
	dictionary_hints(dict, word_lower_case, &list);
	const wchar_t * const *a = word_list_get(&list);
	fprintf(stderr, "%d,%d %ls: ", w, z, word);
	for (size_t i = 0; i < word_list_size(&list); ++i)
	{
		if (i)
			fprintf(stderr, " ");
		fprintf(stderr, "%ls", a[i]);
	}
	fprintf(stderr, "\n");
	word_list_done(&list);
}


int read(struct dictionary *dict, int v, int *w, int *z)
{
	wchar_t c[2] = L"";
	if (fgetws (c, 2, stdin) == NULL)
		return 0;
	(*z)++;
	if (c[0] == L'\n')
	{
		(*w)++;
		*z = 0;
	}
	if (!iswalpha(c[0])) {
		printf("%ls", c);
	}
	else
	{
		wchar_t word[MAX_WORD_LENGTH + 1] = L"";
		wcscat(word, c);
		while (iswalpha(c[0]))
		{
			if (fgetws (c, 2, stdin) == NULL)
				return 0;
			if (iswalpha(c[0]))
				wcscat(word, c);
		}
		wchar_t word_lower_case[MAX_WORD_LENGTH + 1];
		wcscpy(word_lower_case, word);
		word_lower_case[wcslen(word) + 1] = L'\0';
		make_lowercase(word_lower_case);
		if (dictionary_find(dict, word_lower_case))
		{
			printf("%ls", word);
		}
		else
		{
			printf("#%ls", word);
			if (v)
				write_hints(dict, *w, *z, word, word_lower_case);
		}
		if (c[0] == L'\n')
		{
			(*w)++;
			*z = 0;
		}
		else
		{
			*z += wcslen(word);
		}
		printf("%ls", c);
	}
	return 1;
}

/*
 * Zakladam ze jedyne poprawne wywolania programu to:
 * ./dict-check dict lub ./dict-check -v dict
 */
int main(int argc, char *argv[]){
	char *filename;
	int v;
	if (argc == 2)
	{
		filename = argv[1];
		v = 0;
	}
	else if (argc == 3 && strcmp(argv[1], "-v") == 0)
	{
		filename = argv[2];
		v = 1;
	}
	else
	{
		printf("usage: %s filename OR %s -v filename\n", argv[0], argv[0]);
		return 0;
	}
	setlocale(LC_ALL, "pl_PL.UTF-8");
	FILE *f = fopen(filename, "r");
	struct dictionary *dict;
	if (!f || !(dict = dictionary_load(f)))
	{
		fprintf(stderr, "Failed to load dictionary\n");
		exit(1); //czy to tu zadziala ?
	}
	fclose(f);
	int w = 1;
	int z = 0;
	do {} while (read(dict, v, &w, &z));
	dictionary_done(dict);
	return 0;
}
