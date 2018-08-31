/* ************************************************************************** */
/*                                                                            */
/*                                                                            */
/*   msg_helpers_2.c                                                          */
/*                                                                            */
/*   By: phrytsenko                                                           */
/*                                                                            */
/*   Created: 2018/08/31 19:15:46 by phrytsenko                               */
/*   Updated: 2018/08/31 19:16:52 by phrytsenko                               */
/*                                                                            */
/* ************************************************************************** */

#include "server.h"

char	*get_room_data(t_chat_room * room)
{
	char	*ret;
	t_dlist	*user;

	if (!room)
		return (NULL);
	user = room->users;
	ret = malloc(MAX_NICKNAME_LEN * 8 + 16);
	sprintf(ret, "[%s]: ", room->name);
	while (user && (user = user->next) != room->users)
	{
		ret = ft_strjoin(ret, ((t_client *)user->content)->nickname)
													- _clean(ret);
		ret = ft_strjoin(ret, " ") - _clean(ret);
	}
	return (ret);
}

char	*get_room_list(void)
{
	char		*ret;
	t_dlist		*room_node;
	t_chat_room	*room;

	ret = ft_strnew(0);
	room_node = g_env.all_rooms;
	while (room_node && (room_node = room_node->next) != g_env.all_rooms)
	{
		room = (t_chat_room *)room_node->content;
		room->passwd ? (ret = ft_strjoin(ret, ROOM_LOCKED) - _clean(ret)) : 0;
		ret = ft_strjoin(ret, room->name) - _clean(ret);
		ret = ft_strjoin(ret, " ") - _clean(ret);
	}
	return (ret);
}

void	update_clients_data(t_chat_room *room)
{
	size_t		data_len;
	char		* data;
	t_dlist		* user;

	data = get_room_data(room);
	data_len = ft_strlen(data);
	user = room->users;
	while (user && (user = user->next) != room->users)
		send_data(((t_client *)user->content)->sockfd,
					data, data_len + 1, UPDATE_USERS);
	free(data);
}

void	update_room_list(t_client *client)
{
	t_dlist	*client_node;
	char	*r_list;

	r_list = get_room_list();
	if (client)
		send_data(client->sockfd, r_list, ft_strlen(r_list) + 1, UPDATE_ROOMS);
	else if ((client_node = g_env.all_clients))
		while ((client_node = client_node->next) != g_env.all_clients)
			send_data(((t_client *)client_node->content)->sockfd, r_list,
				ft_strlen(r_list) + 1, UPDATE_ROOMS);
	free(r_list);
}

void	sync_chat_history(t_client * c)
{
	int					fd;
	char				buffer[1024];
	ssize_t				buf_len;
	char				*data;
	static t_chat_room	*c_room;

	c_room = (t_chat_room *)c->chat_room_node->content;
	if (!c_room || (fd = open(c_room->log_name,	O_RDONLY)) < 0)
		return ((void)send_data(c->sockfd, HISTORY_ERR,
					ft_strlen(HISTORY_ERR) + 1, UPDATE_HISTORY));
	data = ft_strnew(0);
	while ((buf_len = read(fd, buffer, sizeof(buffer) - 1)) > 0)
	{
		buffer[buf_len] = 0;
		data = ft_strjoin(data, buffer) - _clean(data);
	}
	buf_len < 0
		? send_data(c->sockfd, HISTORY_ERR,
							ft_strlen(HISTORY_ERR) + 1, UPDATE_HISTORY)
		: send_data(c->sockfd, data, ft_strlen(data) + 1, UPDATE_HISTORY);
	free(data);
}
