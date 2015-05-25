/** @file
  Implementacja słownika na drzewie TRIE.
  @ingroup dictionary
  @author Artur Myszkowski <am347189@students.mimuw.edu.pl>
  @date 2015-05-24
 */

#include "dictionary.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define _GNU_SOURCE
/**
  Marker klucza, oznaczający root'a lub to , iż dany ciąg kluczy węzłów
  powyżej zamarkowanego węzła tworzy słowo dodane do słownika.
 */

#define NULL_MARKER L'#'
/**
  Maksymalna wielkość alfabetu, zawierającego tylko te litery,
  które zawierają słowa występujące w słowniku.
 */

#define ALPHABET_SIZE 100
/**
  Maksymalna ilość wszystkich możliwych
  modyfikacji słowa dla dictionary_hints.
*/

#define HINTS_SIZE (1000 * 1000)

/**
  Struktura przechowująca słownik.
  Implementacja na drzewie TRIE.
 */
struct dictionary
{
	wchar_t key; ///< Klucz.
	struct dictionary **children; ///< Tablica wskaźników na dzieci.
	int children_size; ///< Ilość dzieci.
};

/** @name Funkcje pomocnicze
  @{
 */

/**
 * Tworzy węzeł o podanym kluczu 'key'.
 * @param[in] key Klucz.
 * @return Wskaźnik na utworzony węzeł.
 */
static struct dictionary * create_node(wchar_t key)
{
	struct dictionary *node =
		(struct dictionary *) malloc(sizeof(struct dictionary));
	node->key = key;
	node->children = NULL;
	node->children_size = 0;
	return node;
}

/**
  Czyszczenie pamięci słownika.
  @param[in,out] dict Słownik.
 */
static void dictionary_free(struct dictionary *dict)
{
	for(int i = 0; i < dict->children_size; i++)
	{
		dictionary_free(*(dict->children + i));
	}
	free(dict->children);
	free(dict);
}

/**
 * Zwraca czy słowo 'word' znajduje się w liście słów
 * @param[in] list Lista słów
 * @param[in] word Słowo.
 * @return True jeśli słowo znajduje się w liście słów, false w p.p.
 */
static bool word_list_find(const struct word_list *list, const wchar_t* word)
{
	const wchar_t * const * a = word_list_get(list);
	for (size_t i = 0; i < word_list_size(list); i++)
		if (!wcscmp(a[i], word))
			return true;
	return false;
}

/**
 * Wstawia dziecko do węzła 'dict', w kolejnosci leksykograficznej po kluczu
 * 'key' dziecka.
 * Uwaga: w 'dict'->children nie może byc węzła o 'key' == 'child'->key.
 * @param[in,out] dict Węzeł słownika.
 * @param[in] child Wstawiany węzeł.
 */
static void put_child(struct dictionary *dict, struct dictionary *child)
{
	if (child == NULL)
		return;
	int children_size = dict->children_size;
	if (dict->children_size == 0)
	{
		dict->children = malloc(sizeof(struct dictionary *));
		*dict->children = child;
		dict->children_size++;
	}
	else
	{
		dict->children = realloc(dict->children,
			(children_size + 1) * sizeof(struct dictionary *));
		assert(dict->children != NULL);
		int i = children_size;
		/* co jak == ? */
		while (i > 0 &&
			   wcscmp(&(*(dict->children + i - 1))->key, &child->key) > 0)
		{
			*(dict->children + i) = *(dict->children + i - 1);
			i--;
		}
		*(dict->children + i) = child;
		dict->children_size++;
	}
}

/**
 * Zwraca czy w tablicy dzieci węzła 'dict', znajduje się dziecko o danym
 * kluczu 'key'.
 * @param[in] dict Węzeł słownika.
 * @param[in,out] found Wskażnik na dziecko.
 * @param[in] key Klucz.
 * @return True jeśli taki węzeł znajduje się, a na 'found' zapisywany
 * jest wskażnik na to dziecko. false i 'found' = NULL w p.p.
 */
static bool find_child(const struct dictionary *dict, struct dictionary **found,
					  const wchar_t key)
{
	struct dictionary **children = dict->children;
	if (NULL == children) {
		*found = NULL;
		return false;
	}
	int l = 0;
	int r = dict->children_size - 1;
	while (l < r)
	{
		int s = (l + r) / 2;
		if (key > (*(children + s))->key)
			l = s + 1;
		else
			r = s;
	}
	*found = *(children + l);
	if ((*found)->key == key)
	{
		return true;
	}
	else
	{
		*found = NULL;
		return false;
	}
}

/**
 * Usuwa dziecko 'child' z tablicy węzłów 'prev'.
 * @param[in,out] prev Węzeł z, którego usuwane jest dziecko.
 * @param[in] child Usuwany węzeł.
 */
static void delete_child(struct dictionary *prev, struct dictionary *child)
{
	if (prev == NULL)
		return;
	assert(child->children_size == 0);
	if (prev->children_size == 1)
	{
		free(child);
		free(prev->children);
		prev->children = NULL;
		prev->children_size--;
		return;
	}
	struct dictionary **new_children =
		malloc((prev->children_size - 1) * sizeof(struct dictionary *));
	int j = 0;
	for (int i = 0; i < prev->children_size; i++)
	{
		if (*(prev->children + i) != child)
		{
			*(new_children + j) = *(prev->children + i);
			j++;
		}
	}
	free(child);
	free(prev->children);
	prev->children = new_children;
	prev->children_size--;
}

/**
 * Funkcja pomocnicza dictionary_delete.
 * Usuwa węzły reprezentujące slowo 'word' w słowniku 'dict'.
 * @param[in,out] dict Słownik.
 * @param[in] prev Wskaźnik na ojca. Jeśli 'dict' to root należy wstawić NULL.
 * @param[in] word Usuwane słowo.
 * @return 0 jeśli 'prev' po usunięciu węzła nie posiada już dzieci, 1 w p.p.
 */
static int delete_helper(struct dictionary *dict, struct dictionary *prev,
					  const wchar_t *word)
{
	if (dict)
	{
		if (*word == L'\0')
		{
			if (dict->key == NULL_MARKER)
			{
				delete_child(prev, dict);
				if (prev->children_size == 0)
					return 1;
				return 0;
			}
		}
		else
		{
			struct dictionary *found = NULL;
			word++;
			if (*word == L'\0')
				find_child(dict, &found, NULL_MARKER);
			else
				find_child(dict, &found, *word);
			if (delete_helper(found, dict, word))
			{
				delete_child(prev, dict);
				return (prev && prev->children_size == 0);
			}
		}
	}
	return 0;
}

/**
 * Funkcja pomocnicza dictionary_load.
 * Zwraca na 'dict' wskaznik do powstałego słownika, utworzonego na
 * podstawie pliku 'stream'.
 * @param[in,out] dict Słownik.
 * @param[in] stream Plik.
 * @return <0 jeśli operacja się nie powiedzie, 0 w p.p
 */
static int deserialize(struct dictionary **dict, FILE* stream)
{
	int valid = 0;
	wchar_t key;
	if (fscanf(stream, "%1ls", &key) != EOF)
	{
		int size;
		if (fscanf(stream, "%d", &size) != EOF)
		{
			*dict = create_node(key);
			for (int i = 0; i < size; i++)
			{
				struct dictionary *child = NULL;
				valid += deserialize(&child, stream);
				put_child(*dict, child);
			}
		}
	}
	if (ferror(stream))
		valid += -1;
	return valid;
}

/**
 * Usuwa ze słowa 'src' znak na pozycji 'pos'.
 * @param[in,out] src Słowo.
 * @param[in] pos Pozycja usunięcia.
 * @param[in] len Długość słowa.
 */
static void delete_at(wchar_t *src, size_t pos, size_t len)
{
	wchar_t *dst;
	if (pos < 0)
		return;
	src += pos;
	dst = src;
	src++;
	for (int i = pos + 1; i < len && *src != L'\0'; i++)
		*dst++ = *src++;
	*dst = L'\0';
}

/**
 * Wstawia do słowa 'src' znak 'c' na pozycji 'pos'.
 * @param[in] src Słowo
 * @param[in] c Znak.
 * @param[in] pos Pozycja wstawiania.
 * @param[in] len Długość słowa.
 * @param[in,out] dst Wskaźnik na nowe słowo.
 */
static void insert_at(const wchar_t *src, const wchar_t *c, size_t pos,
					  size_t len, wchar_t **dst)
{
	wchar_t fst[pos + 1];
	fst[0] = L'\0';
	wchar_t snd[len - pos + 1];
	snd[0] = L'\0';
	wcsncpy(fst, src, pos);
	fst[pos] = L'\0';
	wcsncpy(snd, &src[pos], len - pos);
	snd[len - pos] = L'\0';
	wcscat(*dst, fst);
	wcscat(*dst, c);
	wcscat(*dst, snd);
}

/**
 * Zamienia w słowie 'src' znak na pozycji 'pos' na znak 'c'.
 * @param[in,out] src Słowo.
 * @param[in] c Znak.
 * @param[in] pos Pozycja zamiany.
 * @param[in] len Długość słowa.
 */
static void replace_at(wchar_t *src, const wchar_t c, size_t pos, size_t len)
{
	if (pos < 0)
		return;
	src += pos;
	*src = c;
}

/**
 * Funkcja pomocnicza create_alphabet.
 * Konkatenuje do *ptra kolejne klucze węzłów słownika,
 * jeśli jeszcze sie w nim nie pojawiły.
 * @param[in] dict Słownik.
 * @param[in,out] ptra Wskażnik na "wide string" alfabetu.
 */
static void alphabet_helper(const struct dictionary *dict, wchar_t *ptra)
{
	if (dict == NULL)
	{
		ptra = NULL;
		return;
	}
	wchar_t k[2];
	k[0] = dict->key;
	k[1] = L'\0';
	if (dict->key != NULL_MARKER && wcschr(ptra, k[0]) == NULL)
	{
		wcscat(ptra, k);
	}
	for (int i = 0; i < dict->children_size; i++)
		alphabet_helper(*(dict->children + i), ptra);
}

/**
 * Tworzy alfabet, zawierający tylko te litery, które zawierają
 * słowa występujące w słowniku.
 * @param[in] dict Słownik.
 * @return "Wide string" alfabetu.
 */
static const wchar_t * create_alphabet(const struct dictionary *dict)
{
	static wchar_t alphabet[ALPHABET_SIZE];
	alphabet_helper(dict, alphabet);
	return alphabet;
}

/**
 * Porównuje leksykograficznie dwa słowa.
 * Komparator otoczka dla qsort, wykorzystująca do porównań wcscoll.
 * @param[in] arg1 Słowo pierwsze.
 * @param[in] arg2 Słowo drugie.
 * @return 0 jeśli słowa są identyczne, <0 jeśli słowo pierwsze jest
 * leksykograficznie mniejsze od słowa pierwszego, >0 jeśli słowo drugie
 * jest leksykograficznie większe od słowa pierwszego.
 */
static int compare(const void *arg1, const void *arg2)
{
	return wcscoll(*(const wchar_t**)arg1, *(const wchar_t**)arg2);
}

/**
 * Tworzy wszytskie możliwe modyfikacje słowa 'word' według zasad
 * ustalonych dla dictionary_hints.
 * @param[in] dict Słownik.
 * @param[in] word Słowo.
 * @param[in,out] hints_size Ilość podpowiedzi.
 * @param[in,out] output Wskaźnik na tablicę podpowiedzi.
 */
static void possible_hints(const struct dictionary *dict, const wchar_t *word,
						   int **hints_size, wchar_t ***output)
{
	const wchar_t * alphabet = create_alphabet(dict);
	wchar_t *hints[HINTS_SIZE];
	int size = 0;
	for (size_t i = 0; i < (wcslen(word) + 1); i++)
	{
		wchar_t *h1 = malloc(wcslen(word) * sizeof(wchar_t *));
		wcscpy(h1, word);
		delete_at(h1, i, wcslen(word));
		hints[size] = h1;
		size++;
	}
	for (size_t i = 0; i < wcslen(word); i++)
		for (int j = 0; alphabet[j]; j++)
		{
			wchar_t *h2 = malloc((wcslen(word) + 1) * sizeof(wchar_t *));
			wcscpy(h2, word);
			replace_at(h2, alphabet[j], i, wcslen(word));
			hints[size] = h2;
			size++;
		}

	for (size_t i = 0; i <= wcslen(word); i++)
		for (int j = 0; alphabet[j]; j++)
		{
			wchar_t *h3 = malloc((wcslen(word) + 2) * sizeof(wchar_t *));
			h3[0] = L'\0';
			wchar_t c[2];
			c[0] = alphabet[j];
			c[1] = L'\0';
			insert_at(word, c, i, wcslen(word), &h3);
			hints[size] = h3;
			size++;
		}
	*hints_size = &size;
	qsort(hints, size, sizeof(wchar_t *), compare);
	*output = hints;
}

/**@}*/
/** @name Elementy interfejsu
  @{
 */

struct dictionary * dictionary_new()
{
	struct dictionary *dict = create_node(NULL_MARKER);
	assert( dict != NULL);
	return dict;
}


void dictionary_done(struct dictionary *dict)
{
	dictionary_free(dict);
}


int dictionary_insert(struct dictionary *dict, const wchar_t *word)
{
	assert(dict != NULL);
	struct dictionary *node = NULL;

	/* Pierwsze slowo */
	if (dict->children_size == 0)
	{
		for(node = dict; *word; node = *node->children)
		{
			put_child(node, create_node(*word));
			word++;
		}
		put_child(node, create_node(NULL_MARKER));
		return 1;
	}
	node = dict;
	struct dictionary *found = NULL;
	while (find_child(node, &found, *word) && *word)
	{
		node = found;
		word++;
	}
	if (*word == L'\0')
	{
		if (find_child(node, &found, NULL_MARKER))
			return 0;
		put_child(node, create_node(NULL_MARKER));
	}
	else
	{
		struct dictionary *tmp = create_node(*word);
		put_child(node, tmp);
		dictionary_insert(tmp, ++word);
	}
	return 1;
}


bool dictionary_find(const struct dictionary *dict, const wchar_t *word)
{
	if (dict == NULL)
		return false;
	struct dictionary *found = NULL;
	if (*word == L'\0')
	{
		return find_child(dict, &found, NULL_MARKER);
	}
	else
	{
		find_child(dict, &found, *word);
		word++;
		return dictionary_find(found, word);
	}
}


int dictionary_delete(struct dictionary *dict, const wchar_t *word)
{
	if (dict == NULL || word == NULL)
		return 0;
	if (dictionary_find(dict, word))
	{
		struct dictionary *found = NULL;
		find_child(dict, &found, *word);
		delete_helper(found, NULL, word);
		return 1;
	}
	return 0;
}


int dictionary_save(const struct dictionary *dict, FILE* stream)
{
	int valid = 0;
	wchar_t key[2];
	key[0] = dict->key;
	key[1] = L'\0';
	if (fprintf(stream, "%ls%d", key, dict->children_size) < 0)
		return -1;
	for (int i = 0; i < dict->children_size; i++)
		valid += dictionary_save(*(dict->children + i), stream);
	return valid;
}


struct dictionary * dictionary_load(FILE* stream)
{
	struct dictionary *dict = NULL;
	if (deserialize(&dict, stream))
	{
		dictionary_done(dict);
		dict = NULL;
	}
	return dict;
}


void dictionary_hints(const struct dictionary *dict, const wchar_t* word,
        struct word_list *list)
{
	word_list_init(list);
	int *n;
	wchar_t **hints;
	possible_hints(dict, word, &n, &hints);
	for (int i = 0; i < *n; i++)
		if (dictionary_find(dict, hints[i]) && !word_list_find(list, hints[i]))
			word_list_add(list, hints[i]);
	for (int i = 0; i < *n; i++)
		free(hints[i]);

}
/**@}*/
