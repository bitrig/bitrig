/*	$OpenBSD: gui.pro,v 1.1 1996/09/07 21:40:29 downsj Exp $	*/
/* gui.c */
void gui_start __PARMS((void));
void gui_prepare __PARMS((int *argc, char **argv));
void gui_init __PARMS((void));
void gui_exit __PARMS((void));
int gui_init_font __PARMS((void));
void gui_set_cursor __PARMS((int row, int col));
void gui_update_cursor __PARMS((void));
void gui_resize_window __PARMS((int pixel_width, int pixel_height));
void gui_reset_scroll_region __PARMS((void));
void gui_start_highlight __PARMS((long_u mask));
void gui_stop_highlight __PARMS((long_u mask));
void gui_write __PARMS((char_u *s, int len));
void gui_undraw_cursor __PARMS((void));
void gui_redraw __PARMS((int x, int y, int w, int h));
void gui_redraw_block __PARMS((int row1, int col1, int row2, int col2));
int check_col __PARMS((int col));
int check_row __PARMS((int row));
void gui_send_mouse_event __PARMS((int button, int x, int y, int repeated_click, int_u modifiers));
void gui_own_selection __PARMS((void));
void gui_lose_selection __PARMS((void));
void gui_copy_selection __PARMS((void));
void gui_auto_select __PARMS((void));
void gui_menu_cb __PARMS((GuiMenu *menu));
int gui_get_menu_index __PARMS((GuiMenu *menu, int state));
void gui_do_menu __PARMS((char_u *cmd, char_u *arg, int force));
char_u *gui_set_context_in_menu_cmd __PARMS((char_u *cmd, char_u *arg, int force));
int gui_ExpandMenuNames __PARMS((regexp *prog, int *num_file, char_u ***file));
void gui_init_which_components __PARMS((char_u *oldval));
int gui_do_scroll __PARMS((void));
int gui_get_max_horiz_scroll __PARMS((void));
int gui_do_horiz_scroll __PARMS((void));
