/******************************************************************************
*   TinTin++                                                                  *
*   Copyright (C) 2005 (See CREDITS file)                                     *
*                                                                             *
*   This program is protected under the GNU GPL (See COPYING)                 *
*                                                                             *
*   This program is free software; you can redistribute it and/or modify      *
*   it under the terms of the GNU General Public License as published by      *
*   the Free Software Foundation; either version 2 of the License, or         *
*   (at your option) any later version.                                       *
*                                                                             *
*   This program is distributed in the hope that it will be useful,           *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
*   GNU General Public License for more details.                              *
*                                                                             *
*   You should have received a copy of the GNU General Public License         *
*   along with this program; if not, write to the Free Software               *
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
*******************************************************************************/

/******************************************************************************
*   file: class.c - funtions related to the scroll back buffer                *
*           (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t ++ 2.00              *
*                     coded by Igor van den Hoven 2004                        *
******************************************************************************/


#include "tintin.h"


DO_COMMAND(do_class)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE], pr[BUFFER_SIZE];
	struct listroot *root;
	struct listnode *node, *class;
	int cnt;

	root = ses->list[LIST_CLASS];

	arg = get_arg_in_braces(arg, left, FALSE);
	arg = get_arg_in_braces(arg, right, FALSE);
	arg = get_arg_in_braces(arg, pr, FALSE);

	if (*left == 0)
	{
		tintin_header(ses, " CLASSES ");

		for (node = root->f_node ; node ; node = node->next)
		{
			tintin_printf2(ses, "%-20s %3d %s",
			node->left,
			count_class(ses, node),
			HAS_BIT(node->flags, NODE_FLAG_CLASS) ? "OPEN" : "CLOSED");
		}
	}
	else if (*right == 0)
	{
		class = search_node_with_wild(root, left);

		if (class)
		{
			tintin_header(ses, " %s ", left);

			for (cnt = 0 ; cnt < LIST_MAX ; cnt++)
			{
				for (node = ses->list[cnt]->f_node ; node ; node = node->next)
				{
					if (node->class == class)
					{
						shownode_list(ses, node, cnt);
					}
				}
			}
		}
		else
		{
			tintin_printf2(ses, "#CLASS {%s} DOES NOT EXIST.", left);
		}
	}
	else
	{
			
		for (cnt = 0 ; cnt < CLASS_MAX ; cnt++)
		{
			if (is_abbrev(right, class_table[cnt].name))
			{
				break;
			}
		}

		if (cnt == CLASS_MAX)
		{
			tintin_printf2(ses, "#SYNTAX: CLASS {name} {OPEN|CLOSE|READ|WRITE|KILL}.", left, capitalize(right));
		}
		else
		{
			if (!searchnode_list(root, left))
			{
				updatenode_list(ses, left, right, pr, LIST_CLASS);
			}
			class_table[cnt].class(ses, left, pr);
		}
	}
	return ses;
}

/*
	Not needed with #class <name> kill

DO_COMMAND(do_unclass)
{
	char left[BUFFER_SIZE];
	struct listroot *root;
	struct listnode *node;
	int found = FALSE;

	root = ses->list[LIST_CLASS];

	arg = get_arg_in_braces(arg, left, TRUE);

	while ((node = search_node_with_wild(root, left)) != NULL)
	{
		show_message(ses, LIST_CLASS, "#OK. {%s} IS NO LONGER A CLASS.", left);

		deletenode_list(ses, node, LIST_CLASS);

		found = TRUE;
	}

	if (found == FALSE)
	{
		show_message(ses, LIST_CLASS, "#UNCLASS: NO MATCH(ES) FOUND FOR {%s}.", left);
	}
	return ses;
}

*/

int count_class(struct session *ses, struct listnode *class)
{
	struct listnode *node;
	int list, cnt;

	for (cnt = list = 0 ; list < LIST_MAX ; list++)
	{
		for (node = ses->list[list]->f_node ; node ; node = node->next)
		{
			if (node->class == class)
			{
				cnt++;
			}
		}
	}
	return cnt;
}

void kill_classes(struct session *ses)
{
	int list;
	struct listnode *node, *node_next;

	for (list = 0 ; list < LIST_ALL ; list++)
	{
		for (node = ses->list[list]->f_node ; node ; node = node_next)
		{
			node_next = node->next;

			if (node->class || list == LIST_CLASS)
			{
				deletenode_list(ses, node, list);
			}
		}
	}
}

DO_CLASS(class_open)
{
	ses->class = search_node_with_wild(ses->list[LIST_CLASS], left);

	SET_BIT(ses->class->flags, NODE_FLAG_CLASS);

	show_message(ses, LIST_CLASS, "#CLASS {%s} HAS BEEN OPENED.", left);

	return ses;
}

DO_CLASS(class_close)
{
	struct listnode *node;

	node = search_node_with_wild(ses->list[LIST_CLASS], left);

	show_message(ses, LIST_CLASS, "#CLASS {%s} HAS BEEN CLOSED.", node->left);

	DEL_BIT(node->flags, NODE_FLAG_CLASS);

	if (node == ses->class)
	{
		ses->class = NULL;

		for (node = ses->list[LIST_CLASS]->l_node ; node ; node = node->prev)
		{
			if (HAS_BIT(node->flags, NODE_FLAG_CLASS))
			{
				class_open(ses, node->left, "");

				break;
			}
		}
	}
	return ses;
}

DO_CLASS(class_read)
{
	class_open(ses, left, "");

	do_read(ses, right);

	class_close(ses, left, "");

	return ses;
}

DO_CLASS(class_write)
{
	FILE *file;
	char temp[BUFFER_SIZE];
	struct listnode *node, *class;
	int cnt;

	class = search_node_with_wild(ses->list[LIST_CLASS], left);

	if (*right == 0 || (file = fopen(right, "w")) == NULL)
	{
		tintin_printf(ses, "#ERROR: #CLASS WRITE {%s} - COULDN'T OPEN FILE TO WRITE.", right);

		return ses;
	}

	fprintf(file, "%cCLASS {%s} OPEN\n\n", gtd->tintin_char, left);

	for (cnt = 0 ; cnt < LIST_MAX ; cnt++)
	{
		for (node = ses->list[cnt]->f_node ; node ; node = node->next)
		{
			if (node->class == class)
			{
				prepare_for_write(cnt, node, temp);

				fputs(temp, file);
			}
		}
	}

	fprintf(file, "\n%cCLASS {%s} CLOSE\n", gtd->tintin_char, left);

	fclose(file);

	show_message(ses, LIST_CLASS, "#CLASS {%s} HAS BEEN WRITTEN TO FILE.", left);

	return ses;
}

DO_CLASS(class_kill)
{
	struct listnode *node, *node_next, *class;
	int cnt;

	class = search_node_with_wild(ses->list[LIST_CLASS], left);

	for (cnt = 0 ; cnt < LIST_MAX ; cnt++)
	{
		if (cnt == LIST_CLASS)
		{
			continue;
		}
		for (node = ses->list[cnt]->f_node ; node ; node = node_next)
		{
			node_next = node->next;

			if (node->class == class)
			{
				deletenode_list(ses, node, cnt);
			}
		}
	}

	show_message(ses, LIST_CLASS, "#CLASS {%s} HAS BEEN CLEARED.", left);

	deletenode_list(ses, class, LIST_CLASS);

	return ses;
}
