/** @file
    Interfejs listy słów.

    @ingroup dictionary
    @author Jakub Pawlewicz <pan@mimuw.edu.pl>
    @copyright Uniwerstet Warszawski
    @date 2015-05-10
    @todo Poszerzyć implementację, aby można było trzymać dowolną
      liczbę słów.
 */

#ifndef __WORD_LIST_H__
#define __WORD_LIST_H__

#include <wchar.h>

/**
  Początkowy rozmiar tablicy słów.
  */
# define STD_BUFFER_SIZE 50

/**
  Struktura przechowująca listę słów.
  */
struct word_list
{
    /// Liczba słów.
    size_t size;
	/// Aktualny rozmiar tablicy dynamicznej.
    size_t buffer_size;
    /// Tablica słów.
	wchar_t **array;
};

/**
  Inicjuje listę słów.
  @param[in,out] list Lista słów.
  */
void word_list_init(struct word_list *list);

/**
  Destrukcja listy słów.
  @param[in,out] list Lista słów.
  */
void word_list_done(struct word_list *list);

/**
  Dodaje słowo do listy.
  @param[in,out] list Lista słów.
  @param[in] word Dodawane słowo.
  @return 1 jeśli się udało, 0 w p.p.
  */
int word_list_add(struct word_list *list, const wchar_t *word);

/**
  Zwraca liczę słów w liście.
  @param[in] list Lista słów.
  @return Liczba słów w liście.
  */
static inline
size_t word_list_size(const struct word_list *list)
{
    return list->size;
}

/**
  Zwraca tablicę słów w liście.
  @param[in] list Lista słów.
  @return Tablica słów.
  */
static inline
const wchar_t * const * word_list_get(const struct word_list *list)
{
	return (const wchar_t * const *) list->array;
}

#endif /* __WORD_LIST_H__ */
