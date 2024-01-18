/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*                                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By:                                            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created:   by                                     #+#    #+#             */
/*   Updated:   by                                    ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef BUFFER_SIZE
# define BUFFER_SIZE 4096
#endif

/*
* begin and *end define the data[] range to be processed
* when read fd to data[], begin set to data[], end set to data[] + read_length
* when data[] processed, begin set to next unprocessed data[], end unchanged
* (begin == end) => no data[] to be processed
*/
typedef struct s_buffer_list
{
	struct s_buffer_list	*next;
	char					*begin;
	char					*end;
	int						fd;
	char					data[BUFFER_SIZE];
}	t_buffer_list;

static bool	find_or_malloc_buffer(t_buffer_list **list, int fd);
static bool	read_fd_to_buffer(t_buffer_list *buffer);
static bool	append_line_from_buffer(
				char **line_str, size_t *line_len, t_buffer_list *buffer);
static void	free_buffer(t_buffer_list **list);

/* 
* flow: 1) create buffer, 2) fd to buffer, 3) buffer to line, 4) return line
* buffer is a linked list, with head being the buffer of current fd
* buffer is allocated once per fd and never resized
* line_str is the return string which may resize multiple times until \n ended
* line_len = strlen(line_str)
* EOF will return buffer regardless \n
* malloc() and read() failures will free(buffer) and return NULL
*/
char	*get_next_line(int fd)
{
	static t_buffer_list	*buffer = NULL;
	char					*line_str;
	size_t					line_len;

	line_len = 0;
	line_str = NULL;
	if (find_or_malloc_buffer(&buffer, fd) == false)
		return (NULL);
	while (read_fd_to_buffer(buffer))
	{
		if (buffer->begin == buffer->end)
		{
			if (line_str == NULL)
				free_buffer(&buffer);
			return (line_str);
		}
		if (append_line_from_buffer(&line_str, &line_len, buffer) == false)
			break ;
		if (line_str[line_len - 1] == '\n')
			return (line_str);
	}
	free_buffer(&buffer);
	free(line_str);
	return (NULL);
}

/* 
* get buffer ready for current fd
* if new fd: malloc & lst_add_front.
* if existing fd: lst_move_front.
*/
static bool	find_or_malloc_buffer(t_buffer_list **list, int fd)
{
	t_buffer_list	*node;
	t_buffer_list	*prev;

	if (*list && (*list)->fd == fd)
		return (true);
	prev = *list;
	while (prev && prev->next && prev->next->fd != fd)
		prev = prev->next;
	if (prev && prev->next && prev->next->fd == fd)
	{
		node = prev->next->next;
		prev->next->next = *list;
		*list = prev->next;
		prev->next = node;
		return (true);
	}
	node = malloc(sizeof(t_buffer_list));
	if (node == NULL)
		return (false);
	node->fd = fd;
	node->begin = node->data;
	node->end = node->begin;
	node->next = *list;
	*list = node;
	return (true);
}

/*
* ensure buffer has unprocessed data or read(fd) to refill buffer
* always return true unless read() failed
*/
static bool	read_fd_to_buffer(t_buffer_list *buffer)
{
	int	read_length;

	if (buffer->begin == buffer->end)
	{
		read_length = read(buffer->fd, buffer->data, sizeof(buffer->data));
		if (read_length == -1)
			return (false);
		buffer->begin = buffer->data;
		buffer->end = buffer->begin + read_length;
	}
	return (true);
}

/* search buffer, update buffer state, copy buffer to end of output line */
static bool	append_line_from_buffer(
	char **line_str, size_t *line_len, t_buffer_list *buffer)
{
	char	*new_string;
	size_t	copied;
	char	*copy;
	char	*copy_end;

	copy = buffer->begin;
	copy_end = buffer->begin;
	while (copy_end != buffer->end && *(copy_end++) != '\n')
		;
	buffer->begin = copy_end;
	new_string = malloc(*line_len + (copy_end - copy) + 1);
	if (new_string == NULL)
		return (false);
	copied = -1;
	while (++copied < *line_len)
		new_string[copied] = (*line_str)[copied];
	while (copy != copy_end)
		new_string[copied++] = *copy++;
	new_string[copied] = '\0';
	*line_len = copied;
	free(*line_str);
	*line_str = new_string;
	return (true);
}

/* remove first node (current fd buffer) from list & free it */
static void	free_buffer(t_buffer_list **list)
{
	t_buffer_list	*deleting;

	deleting = *list;
	*list = (*list)->next;
	free(deleting);
}
