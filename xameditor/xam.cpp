#include <gtk/gtk.h>
#include <memory>
#include <string>
#include <vector>

class XamEditor {
  private:
    GtkWidget *window;
    GtkWidget *notebook;
    GtkWidget *menubar;
    GtkWidget *vbox;
    GtkAccelGroup *accel_group;

    struct Tab {
        GtkWidget *scrolled_window;
        GtkWidget *text_view;
        GtkTextBuffer *text_buffer;
        std::string filename;
        bool is_modified;

        Tab()
            : scrolled_window(nullptr), text_view(nullptr), text_buffer(nullptr),
              filename("Untitled"), is_modified(false) {}

        ~Tab() = default;
    };

    std::vector<std::unique_ptr<Tab>> tabs;
    int tab_counter = 1;

  public:
    XamEditor() {
        // Initialize GTK
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "Text Editor");
        gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

        // Create accelerator group
        accel_group = gtk_accel_group_new();
        gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

        // Create main container
        vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(window), vbox);

        // Create menu bar
        create_menubar();

        // Create notebook for tabs
        notebook = gtk_notebook_new();
        gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
        gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

        // Connect signals
        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

        // Create first tab
        new_file();

        gtk_widget_show_all(window);
    }

    void create_menubar() {
        menubar = gtk_menu_bar_new();
        gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

        // File menu
        GtkWidget *file_menu = gtk_menu_new();
        GtkWidget *file_item = gtk_menu_item_new_with_label("File");
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);
        gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file_item);

        create_menu_item(file_menu, "New", G_CALLBACK(on_new_activate), GDK_KEY_n,
                         GDK_CONTROL_MASK);
        create_menu_item(file_menu, "Open", G_CALLBACK(on_open_activate), GDK_KEY_o,
                         GDK_CONTROL_MASK);
        gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), gtk_separator_menu_item_new());
        create_menu_item(file_menu, "Save", G_CALLBACK(on_save_activate), GDK_KEY_s,
                         GDK_CONTROL_MASK);
        create_menu_item(file_menu, "Save As", G_CALLBACK(on_save_as_activate), GDK_KEY_s,
                         static_cast<GdkModifierType>(GDK_CONTROL_MASK | GDK_SHIFT_MASK));

        // Edit menu
        GtkWidget *edit_menu = gtk_menu_new();
        GtkWidget *edit_item = gtk_menu_item_new_with_label("Edit");
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(edit_item), edit_menu);
        gtk_menu_shell_append(GTK_MENU_SHELL(menubar), edit_item);

        create_menu_item(edit_menu, "Cut", G_CALLBACK(on_cut_activate), GDK_KEY_x,
                         GDK_CONTROL_MASK);
        create_menu_item(edit_menu, "Copy", G_CALLBACK(on_copy_activate), GDK_KEY_c,
                         GDK_CONTROL_MASK);
        create_menu_item(edit_menu, "Paste", G_CALLBACK(on_paste_activate), GDK_KEY_v,
                         GDK_CONTROL_MASK);

        // Search menu
        GtkWidget *search_menu = gtk_menu_new();
        GtkWidget *search_item = gtk_menu_item_new_with_label("Search");
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(search_item), search_menu);
        gtk_menu_shell_append(GTK_MENU_SHELL(menubar), search_item);

        create_menu_item(search_menu, "Find", G_CALLBACK(on_find_activate), GDK_KEY_f,
                         GDK_CONTROL_MASK);
        create_menu_item(search_menu, "Replace", G_CALLBACK(on_replace_activate), GDK_KEY_h,
                         GDK_CONTROL_MASK);

        // Help menu
        GtkWidget *help_menu = gtk_menu_new();
        GtkWidget *help_item = gtk_menu_item_new_with_label("Help");
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_item), help_menu);
        gtk_menu_shell_append(GTK_MENU_SHELL(menubar), help_item);

        create_menu_item(help_menu, "About", G_CALLBACK(on_about_activate), 0, (GdkModifierType)0);
    }

    void create_menu_item(GtkWidget *menu, const char *label, GCallback callback, guint key,
                          GdkModifierType modifier) {
        GtkWidget *item = gtk_menu_item_new_with_label(label);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(item, "activate", callback, this);

        if (key != 0) {
            gtk_widget_add_accelerator(item, "activate", accel_group, key, modifier,
                                       GTK_ACCEL_VISIBLE);
        }
    }

    void new_file() {
        auto tab = std::make_unique<Tab>();

        // Create text buffer and view
        tab->text_buffer = gtk_text_buffer_new(NULL);
        tab->text_view = gtk_text_view_new_with_buffer(tab->text_buffer);
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tab->text_view), GTK_WRAP_WORD);

        // CSS for font
        GtkCssProvider *css_provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(
            css_provider, "textview { font-family: Monospace; font-size: 11pt; }", -1, NULL);
        gtk_style_context_add_provider(gtk_widget_get_style_context(tab->text_view),
                                       GTK_STYLE_PROVIDER(css_provider),
                                       GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(css_provider);

        // Scrolled window and container add
        tab->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tab->scrolled_window),
                                       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(tab->scrolled_window), tab->text_view);

        // Tab label and notebook page
        std::string tab_title = "Untitled " + std::to_string(tab_counter++);
        tab->filename = tab_title;
        GtkWidget *tab_label = gtk_label_new(tab_title.c_str());

        int page_num =
            gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tab->scrolled_window, tab_label);
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page_num);

        // Connect buffer changed
        g_signal_connect(tab->text_buffer, "changed", G_CALLBACK(on_text_changed), this);

        // push the tab and then show the widget via tabs.back()
        tabs.push_back(std::move(tab));
        gtk_widget_show_all(tabs.back()->scrolled_window);
    }

    Tab *get_current_tab() {
        int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
        if (current_page >= 0 && current_page < tabs.size()) {
            return tabs[current_page].get();
        }
        return nullptr;
    }

    void open_file() {
        GtkWidget *dialog = gtk_file_chooser_dialog_new(
            "Open File", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel",
            GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);

        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

            // Read file content
            GError *error = NULL;
            gchar *content = NULL;
            gsize length = 0;

            if (g_file_get_contents(filename, &content, &length, &error)) {
                new_file();
                Tab *tab = get_current_tab();
                if (tab) {
                    gtk_text_buffer_set_text(tab->text_buffer, content, length);
                    tab->filename = filename;
                    tab->is_modified = false;

                    // Update tab label
                    gchar *basename = g_path_get_basename(filename);
                    GtkWidget *tab_label = gtk_label_new(basename);
                    gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), tab->scrolled_window,
                                               tab_label);
                    g_free(basename);
                }
                g_free(content);
            } else {
                show_error_dialog("Error opening file", error->message);
                g_error_free(error);
            }

            g_free(filename);
        }

        gtk_widget_destroy(dialog);
    }

    void save_file() {
        Tab *tab = get_current_tab();
        if (!tab)
            return;

        if (tab->filename.find("Untitled") == 0) {
            save_file_as();
            return;
        }

        GtkTextIter start, end;
        gtk_text_buffer_get_bounds(tab->text_buffer, &start, &end);
        gchar *content = gtk_text_buffer_get_text(tab->text_buffer, &start, &end, FALSE);

        GError *error = NULL;
        if (g_file_set_contents(tab->filename.c_str(), content, -1, &error)) {
            tab->is_modified = false;
        } else {
            show_error_dialog("Error saving file", error->message);
            g_error_free(error);
        }

        g_free(content);
    }

    void save_file_as() {
        GtkWidget *dialog = gtk_file_chooser_dialog_new(
            "Save File As", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_SAVE, "_Cancel",
            GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, NULL);

        gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

            Tab *tab = get_current_tab();
            if (tab) {
                tab->filename = filename;
                save_file();

                // Update tab label
                gchar *basename = g_path_get_basename(filename);
                GtkWidget *tab_label = gtk_label_new(basename);
                gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), tab->scrolled_window, tab_label);
                g_free(basename);
            }

            g_free(filename);
        }

        gtk_widget_destroy(dialog);
    }

    void show_find_dialog() {
        GtkWidget *dialog =
            gtk_dialog_new_with_buttons("Find", GTK_WINDOW(window), GTK_DIALOG_MODAL, "_Cancel",
                                        GTK_RESPONSE_CANCEL, "_Find", GTK_RESPONSE_ACCEPT, NULL);

        GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        GtkWidget *entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter search text...");
        gtk_box_pack_start(GTK_BOX(content_area), entry, TRUE, TRUE, 5);

        gtk_widget_show_all(dialog);

        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            const gchar *search_text = gtk_entry_get_text(GTK_ENTRY(entry));
            find_text(search_text);
        }

        gtk_widget_destroy(dialog);
    }

    void find_text(const gchar *search_text) {
        Tab *tab = get_current_tab();
        if (!tab || !search_text || strlen(search_text) == 0)
            return;

        GtkTextIter start, match_start, match_end;
        gtk_text_buffer_get_start_iter(tab->text_buffer, &start);

        if (gtk_text_iter_forward_search(&start, search_text, GTK_TEXT_SEARCH_CASE_INSENSITIVE,
                                         &match_start, &match_end, NULL)) {
            gtk_text_buffer_select_range(tab->text_buffer, &match_start, &match_end);
            gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(tab->text_view), &match_start, 0.0, FALSE,
                                         0.0, 0.0);
        }
    }

    void show_replace_dialog() {
        GtkWidget *dialog = gtk_dialog_new_with_buttons(
            "Replace", GTK_WINDOW(window), GTK_DIALOG_MODAL, "_Cancel", GTK_RESPONSE_CANCEL,
            "_Replace All", GTK_RESPONSE_ACCEPT, NULL);

        GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

        GtkWidget *find_entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(find_entry), "Find...");

        GtkWidget *replace_entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(replace_entry), "Replace with...");

        gtk_box_pack_start(GTK_BOX(content_area), find_entry, TRUE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(content_area), replace_entry, TRUE, TRUE, 5);

        gtk_widget_show_all(dialog);

        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            const gchar *find_text = gtk_entry_get_text(GTK_ENTRY(find_entry));
            const gchar *replace_text = gtk_entry_get_text(GTK_ENTRY(replace_entry));
            replace_all_text(find_text, replace_text);
        }

        gtk_widget_destroy(dialog);
    }

    void replace_all_text(const gchar *find_text, const gchar *replace_text) {
        Tab *tab = get_current_tab();
        if (!tab || !find_text || strlen(find_text) == 0)
            return;

        GtkTextIter start, end;
        gtk_text_buffer_get_bounds(tab->text_buffer, &start, &end);
        gchar *content = gtk_text_buffer_get_text(tab->text_buffer, &start, &end, FALSE);

        std::string text_content(content);
        std::string find_str(find_text);
        std::string replace_str(replace_text);

        size_t pos = 0;
        while ((pos = text_content.find(find_str, pos)) != std::string::npos) {
            text_content.replace(pos, find_str.length(), replace_str);
            pos += replace_str.length();
        }

        gtk_text_buffer_set_text(tab->text_buffer, text_content.c_str(), -1);
        g_free(content);
    }

    void show_about_dialog() {
        gtk_show_about_dialog(GTK_WINDOW(window), "program-name", "Text Editor", "version", "1.0",
                              "comments", "A simple text editor with tabs", "copyright", "© 2025",
                              "website", "https://example.com", NULL);
    }

    void show_error_dialog(const char *title, const char *message) {
        GtkWidget *dialog =
            gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", title);
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", message);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }

    // Static callback functions
    static void on_new_activate(GtkWidget *widget, gpointer data) {
        XamEditor *editor = static_cast<XamEditor *>(data);
        editor->new_file();
    }

    static void on_open_activate(GtkWidget *widget, gpointer data) {
        XamEditor *editor = static_cast<XamEditor *>(data);
        editor->open_file();
    }

    static void on_save_activate(GtkWidget *widget, gpointer data) {
        XamEditor *editor = static_cast<XamEditor *>(data);
        editor->save_file();
    }

    static void on_save_as_activate(GtkWidget *widget, gpointer data) {
        XamEditor *editor = static_cast<XamEditor *>(data);
        editor->save_file_as();
    }

    static void on_cut_activate(GtkWidget *widget, gpointer data) {
        XamEditor *editor = static_cast<XamEditor *>(data);
        Tab *tab = editor->get_current_tab();
        if (tab) {
            GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
            gtk_text_buffer_cut_clipboard(tab->text_buffer, clipboard, TRUE);
        }
    }

    static void on_copy_activate(GtkWidget *widget, gpointer data) {
        XamEditor *editor = static_cast<XamEditor *>(data);
        Tab *tab = editor->get_current_tab();
        if (tab) {
            GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
            gtk_text_buffer_copy_clipboard(tab->text_buffer, clipboard);
        }
    }

    static void on_paste_activate(GtkWidget *widget, gpointer data) {
        XamEditor *editor = static_cast<XamEditor *>(data);
        Tab *tab = editor->get_current_tab();
        if (tab) {
            GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
            gtk_text_buffer_paste_clipboard(tab->text_buffer, clipboard, NULL, TRUE);
        }
    }

    static void on_find_activate(GtkWidget *widget, gpointer data) {
        XamEditor *editor = static_cast<XamEditor *>(data);
        editor->show_find_dialog();
    }

    static void on_replace_activate(GtkWidget *widget, gpointer data) {
        XamEditor *editor = static_cast<XamEditor *>(data);
        editor->show_replace_dialog();
    }

    static void on_about_activate(GtkWidget *widget, gpointer data) {
        XamEditor *editor = static_cast<XamEditor *>(data);
        editor->show_about_dialog();
    }

    static void on_text_changed(GtkTextBuffer *buffer, gpointer data) {
        XamEditor *editor = static_cast<XamEditor *>(data);
        Tab *tab = editor->get_current_tab();
        if (tab && tab->text_buffer == buffer) {
            tab->is_modified = true;
        }
    }
};

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    XamEditor editor;
    gtk_main();
    return 0;
}
