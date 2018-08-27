/* ************************************************************************** */
/*                                                                            */
/*                                                                            */
/*   io.c                                                                     */
/*                                                                            */
/*   By: phrytsenko                                                           */
/*                                                                            */
/*   Created: 2018/08/23 17:00:56 by phrytsenko                               */
/*   Updated: 2018/08/27 18:43:58 by phrytsenko                               */
/*                                                                            */
/* ************************************************************************** */

#include "client.h"

int		g_symb = 0;
bool	g_input_avb = false;
bool	g_prevent_update = false;

static int		readline_input_avail(void)
{
	return (g_input_avb);
}

static int		readline_getc(FILE *dummy)
{
	(void)dummy;
	g_prevent_update = false;
	if (!good_connection(g_env.sockfd))
	{
		g_env.connection_lost = 1;
		display_chat();
		while (try_reconnect() < 0);
		g_env.connection_lost = 0;
	}
	else if (g_env.connection_lost)
	{
		beep();
		return (0);
	}
	g_input_avb = false;
	return (g_symb);
}

static void		proceed_nickname(char *nickname)
{
	if (ft_cinustr(nickname) > MAX_NICKNAME_LEN)
	{
		g_prevent_update = true;
		werase(g_env.ws.input);
		mvwprintw(g_env.ws.input, 0, 0, "* Nickname is too long *");
		wrefresh(g_env.ws.input);
	}
	else if (!nickname_is_valid(nickname))
	{
		g_prevent_update = true;
		werase(g_env.ws.input);
		mvwprintw(g_env.ws.input, 0, 0, "* Invalid nickname *");
		wrefresh(g_env.ws.input);
	}
	else
	{
		send_data(g_env.sockfd, nickname, ft_strlen(nickname) + 1, 0);
		recieve_data(g_env.sockfd, (void **)&g_env.nickname, 0);
		display_chat();
		ungetch(ERR);
	}
}

static void		got_command(char *line)
{
	g_prevent_update = false;
	if (line && *line && !g_env.nickname)
		proceed_nickname(line);
	else if (line && *line)
	{
		char	tag[128];
		char	* trimmed;

		if (!(trimmed = ft_strsub(line, 0, ft_cinustrcn(line, MSG_MAX_LEN))))
			return ;
		sprintf(tag, "["SELF_POINT"%s]: ", g_env.nickname);
		ft_dlstpush(&g_env.chat_history.lines, ft_dlstnew(ft_strjoin(tag, trimmed), sizeof(void *)));
		g_env.chat_history.size++;
		g_env.layot.chat_offset = 0;
		if (send_data(g_env.sockfd, trimmed, ft_strlen(trimmed) + 1, 0) < 0)
		{
			g_env.connection_lost = 1;
			display_chat();
			while (try_reconnect() < 0);
			g_env.connection_lost = 0;
		}
		display_chat();
	}
	free(line);
}

static void		readline_redisplay(void)
{
	if (g_prevent_update)
		return ;
	size_t	cursor_col = ft_cinustrn(rl_line_buffer, rl_point);
	size_t	offset_x = cursor_col % (g_env.ws.input->_maxx + 1);
	size_t	mult_x = cursor_col / (g_env.ws.input->_maxx + 1) * (g_env.ws.input->_maxx + 1);
	size_t	rl_line_buffer_len = ft_cinustr(rl_line_buffer);
	char	* msg = ft_strsub(rl_line_buffer, ft_cinustrcn(rl_line_buffer, mult_x),
							rl_point + ft_cinustrcn(rl_line_buffer + rl_point, g_env.ws.input->_maxx - offset_x + 1));

	werase(g_env.ws.input);
	werase(g_env.ws.input_b);
	if ((g_env.nickname && rl_line_buffer_len > MSG_MAX_LEN) || (!g_env.nickname && rl_line_buffer_len > MAX_NICKNAME_LEN))
		wattron(g_env.ws.input, COLOR_PAIR(C_COLOR_RED));
	wprintw(g_env.ws.input, "%s", msg);
	if ((g_env.nickname && rl_line_buffer_len > MSG_MAX_LEN) || (!g_env.nickname && rl_line_buffer_len > MAX_NICKNAME_LEN))
		wattroff(g_env.ws.input, COLOR_PAIR(C_COLOR_RED));
	free(msg);
	box(g_env.ws.input_b, 0, 0);
	if (!g_env.nickname)
		mvwprintw(g_env.ws.input_b, 0, 1, "Enter nickname (%d/%d)", rl_line_buffer_len, MAX_NICKNAME_LEN);
	else if (g_env.connection_lost)
		mvwprintw(g_env.ws.input_b, 0, 1, " Wait, reconnecting... ");
	else
		mvwprintw(g_env.ws.input_b, 0, 1, "%s, %s (%d/%d)", rl_display_prompt, g_env.nickname, rl_line_buffer_len, MSG_MAX_LEN);
	wrefresh(g_env.ws.input_b);
	wrefresh(g_env.ws.input);
	wmove(g_env.ws.input, 0, offset_x);
}

void			handle_input(int fd, short ev, bool block)
{
	(void)fd;
	(void)ev;
	while ((g_symb = wgetch(g_env.ws.input)) != ERR)
		if (g_symb == 0x1b)
		{
			uint64_t	utf = 0;

			((char *)&utf)[0] = g_symb;
			for (size_t it = 1; it < sizeof(uint64_t); it++)
			{
				if ((g_symb = wgetch(g_env.ws.input)) == 0x1b)
				{
					ungetch(g_symb);
					break ;
				}
				((char *)&utf)[it] = (g_symb == ERR ? 0 : g_symb);
			}
			switch (utf)
			{
				case RL_KEY_UP:
					g_env.layot.chat_offset + g_env.ws.chat->_maxy + 1 < g_env.chat_history.size
						? g_env.layot.chat_offset++ : (uint)beep();
					display_chat();
					break ;
				case RL_KEY_DOWN:
					g_env.layot.chat_offset ? g_env.layot.chat_offset-- : (uint)beep();
					display_chat();
					break ;
				case RL_KEY_PAGEUP:
					g_env.layot.u_online_offset + g_env.ws.sidebar->_maxy + 1 < g_env.users_online.size
						? g_env.layot.u_online_offset++ : (uint)beep();
					// display_chat();
					break ;
				case RL_KEY_PAGEDOWN:
					g_env.layot.u_online_offset ? g_env.layot.u_online_offset-- : (uint)beep();
					// display_chat();
					break ;
				case RL_KEY_ESC:
					return ;
				default:
					// For debug
					// werase(g_env.ws.input);
					// mvwprintw(g_env.ws.input, 0, 0, "%ld", utf);
					// wrefresh(g_env.ws.input);
					g_input_avb = true;
					for (size_t it = 0; it < sizeof(uint64_t) && (g_symb = ((char *)&utf)[it]); it++)
						rl_callback_read_char();
					break ;
			}
		}
		else if (g_symb == '\f')
		{
			g_env.layot.chat_offset = 0;
			display_chat();
		}
		else if (g_symb == KEY_RESIZE)
			continue ;
		else
		{
			g_input_avb = true;
			rl_callback_read_char();
			if (!block)
				break ;
		}
}

void			init_readline(void)
{
	rl_bind_key('\t', rl_insert);
	rl_catch_signals = 0;
	rl_catch_sigwinch = 0;
	rl_deprep_term_function = NULL;
	rl_prep_term_function = NULL;
	rl_change_environment = 0;

	rl_getc_function = readline_getc;
	rl_input_available_hook = readline_input_avail;
	rl_redisplay_function = readline_redisplay;

	rl_callback_handler_install("Your message", got_command);
}
