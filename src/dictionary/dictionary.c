#define _GNU_SOURCE
#include "dictionary.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h> //to nie wiem czy wyrzucic

#define NULL_MARKER L'#'
#define ALPHABET_SIZE 100
#define HINTS_SIZE (1000 * 1000)


struct dictionary
{
	wchar_t key;
	struct dictionary **children;
	int children_size;
};


static struct dictionary * create_node(wchar_t key)
{
	struct dictionary *node =
		(struct dictionary *) malloc(sizeof(struct dictionary));
	node->key = key;
	node->children = NULL;
	node->children_size = 0;
	return node;
}

struct dictionary * dictionary_new()
{
	struct dictionary *dict = create_node(NULL_MARKER);
	assert( dict != NULL);
	return dict;
}

static void dictionary_free(struct dictionary *dict)
{
	for(int i = 0; i < dict->children_size; i++)
	{
		dictionary_free(*(dict->children + i));
	}
	free(dict->children);
	free(dict);
}


/* Wstawia dziecko do wezla dict, w kolejnosci leksykograficznej po key,
	uwaga: w dict->children nie moze byc wezla o key == child->key */
static void put_child(struct dictionary *dict, struct dictionary *child)
{
	if (child == NULL)
		return; // DODANE po napisaniu deserialize i na jego potrzeby;
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
		//assert(wcscmp(&(*dict->children + i)->key, &child->key) != 0);
		*(dict->children + i) = child;
		dict->children_size++;
	}
}

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

	/* Jak takie slowo juz istnieje to false */
//	if (dictionary_find(dict, word))
//		return 0;
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

/* @return <0 jeśli operacja się nie powiedzie, 0 w p.p.*/
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

void dictionary_done(struct dictionary *dict)
{
	dictionary_free(dict);
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

static void delete_child(struct dictionary *prev, struct dictionary *child)
{
	if (prev == NULL)
		return;
	assert(child->children_size == 0);
	if (prev->children_size == 1)
	{
		free(*prev->children);
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
	//free(child->children);
	free(child);
	free(prev->children);
	prev->children = new_children;
	prev->children_size--;
}

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

/* moze ulepszyc zeby obylo sie bez dict find */
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


static void delete_at(wchar_t *src, int pos, int len)
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

static void insert_at(const wchar_t *src, const wchar_t *c, int pos,
					  int len, wchar_t **dst)
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

static void replace_at(wchar_t *src, const wchar_t c, int pos, int len)
{
	if (pos < 0)
		return;
	src += pos;
	*src = c;
}



static void alphabet_helper(const struct dictionary *dict, wchar_t *ptra)
{
	if (dict == NULL)
	{
		ptra = NULL;
		return;
	}
	if (dict->key != NULL_MARKER && wcschr(ptra, dict->key) == NULL)
	{
		wcscat(ptra, &dict->key);
	}
	for (int i = 0; i < dict->children_size; i++)
		alphabet_helper(*(dict->children + i), ptra);
}

static const wchar_t * create_alphabet(const struct dictionary *dict)
{
	static wchar_t alphabet[ALPHABET_SIZE];
	wchar_t *ptra = alphabet;
	alphabet_helper(dict, ptra);
	return ptra;
}

static int compare(const void *arg1, const void *arg2)
{
	return wcscoll(*(const wchar_t**)arg1, *(const wchar_t**)arg2);
}

static void possible_hints(const struct dictionary *dict, const wchar_t *word,
						   int **hints_size, wchar_t ***output)
{
	const wchar_t * alphabet = create_alphabet(dict);
	wchar_t *hints[HINTS_SIZE];
	int size = 0;
	for (int i = 0; i < (wcslen(word) + 1); i++)
	{
		wchar_t *h1 = malloc(wcslen(word) * sizeof(wchar_t *));
		wcscpy(h1, word);
		delete_at(h1, i, wcslen(word));
		hints[size] = h1;
		size++;
	}
	for (int i = 0; i < wcslen(word); i++)
		for (int j = 0; alphabet[j]; j++)
		{
			wchar_t *h2 = malloc((wcslen(word) + 1) * sizeof(wchar_t *));
			wcscpy(h2, word);
			replace_at(h2, alphabet[j], i, wcslen(word));
			hints[size] = h2;
			size++;
		}

	for (int i = 0; i <= wcslen(word); i++)
		for (int j = 0; alphabet[j]; j++)
		{
			wchar_t *h3 = malloc((wcslen(word) + 2) * sizeof(wchar_t *));
			h3[0] = L'\0';
			wchar_t c[2];
			c[0] = alphabet[j];
			c[1] = L'\0';
			//replace_at(c, alphabet[j], 0, 1);
			insert_at(word, c, i, wcslen(word), &h3);
			hints[size] = h3;
			size++;
		}
	*hints_size = &size;
	qsort(hints, size, sizeof(wchar_t *), compare);
	*output = hints;
}


static bool word_list_find(const struct word_list *list, const wchar_t* word)
{
	const wchar_t * const * a = word_list_get(list);
	for (size_t i = 0; i < word_list_size(list); i++)
		if (!wcscmp(a[i], word))
			return true;
	return false;
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
