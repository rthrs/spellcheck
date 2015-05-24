/** @file
  Implementacja listy słów.

  @ingroup dictionary
  @author Jakub Pawlewicz <pan@mimuw.edu.pl>
  @copyright Uniwerstet Warszawski
  @date 2015-05-10
 */

#include "word_list.h"
#include <stdlib.h>

/** @name Elementy interfejsu 
   @{
 */

void word_list_init(struct word_list *list)
{
    list->size = 0;
	list->buffer_size = STD_BUFFER_SIZE;
	list->array = malloc(STD_BUFFER_SIZE * sizeof(wchar_t *));
}

void word_list_done(struct word_list *list)
{
	for (int i = 0; i < list->size; i++)
		free(list->array[i]);
	free(list->array);
}

int word_list_add(struct word_list *list, const wchar_t *word)
{
	if (list->size >= list->buffer_size)
	{
		list->array = realloc (list->array,
			(list->buffer_size + STD_BUFFER_SIZE) * sizeof(wchar_t *));
		if (list->array == NULL)
			return 0;
	}
	size_t len = wcslen(word) + 1;
	wchar_t *word_array = malloc(len * sizeof(wchar_t *));
	wcscpy(word_array, word);
	list->array[list->size] = word_array;
	list->size++;
    return 1;
}

/**@}*/
