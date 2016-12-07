#include <dirent.h>
#include <fcntl.h>
#include <gtk/gtk.h>
#include <signal.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unistd.h>
#define n 1024

int fd, len;
#define NAMEDPIPE_NAME "/tmp/cd"
#define BUF_SIZE 4096
#define Message1 "Blanking disk...\n"
#define ISO ".iso"
#define WAV ".wav"
#define MP3 ".mp3"

char buffer[BUF_SIZE];
int ChooseFileFlag = 0;
int ChooseFolderFlag = 0;
int OpenFlag = 0;

static void ChooseFile(GtkWidget *button, gpointer *data) {
  GtkWidget *dialog;
  dialog = gtk_file_chooser_dialog_new(
      "Chosse a file", GTK_WINDOW(data[0]), GTK_FILE_CHOOSER_ACTION_OPEN,
      GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      NULL);
  gtk_widget_show_all(dialog);
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                      g_get_home_dir());
  gint resp = gtk_dialog_run(GTK_DIALOG(dialog));
  if (resp == GTK_RESPONSE_OK) {
    gchar *text = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    gtk_label_set_text(GTK_LABEL(data[1]), text);
    gtk_widget_destroy(dialog);
    ChooseFileFlag = 1;
  } else {
    gchar *text = "You pressed Cancel\n";
    gtk_label_set_text(GTK_LABEL(data[1]), text);
    gtk_widget_destroy(dialog);
    ChooseFileFlag = 0;
  }
}

static void ChooseFolder(GtkWidget *button, gpointer *data) {
  GtkWidget *dialog;
  dialog = gtk_file_chooser_dialog_new(
      "Chosse a file", GTK_WINDOW(data[0]), GTK_FILE_CHOOSER_ACTION_OPEN,
      GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      NULL);
  gtk_widget_show_all(dialog);
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                      g_get_home_dir());
  gint resp = gtk_dialog_run(GTK_DIALOG(dialog));
  if (resp == GTK_RESPONSE_OK) {
    gchar *text = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    size_t str_length = strlen(text);
    for (str_length; str_length > 0; str_length--) {
      if (text[str_length] == '/') {
        text[str_length] = '\0';
        break;
      }
    }
    gtk_label_set_text(GTK_LABEL(data[1]), text);
    gtk_widget_destroy(dialog);
    ChooseFolderFlag = 1;
  } else {
    gchar *text = "You pressed Cancel\n";
    gtk_label_set_text(GTK_LABEL(data[1]), text);
    gtk_widget_destroy(dialog);
    ChooseFolderFlag = 0;
  }
}

void *thread_function1(void *arg) {
  if (OpenFlag == 0) {
    system("eject sr0");
    OpenFlag = 1;
  } else {
    system("eject -t sr0");
    OpenFlag = 0;
  }
}

static void getInfo(GtkWidget *button, gpointer *data) { system("./conf"); }

static void open_close(GtkWidget *button, gpointer *data) {
  int res;
  pthread_t a_thread;
  void *thread_result;
  res = pthread_create(&a_thread, NULL, thread_function1, NULL);
  if (res != 0) {
    perror("Thread creation failed");
    return;
  }
}

static void do_iso(GtkWidget *button, gpointer *data) {
  gchar *text = gtk_label_get_label(data[1]);
  if ((!ChooseFileFlag) && (!ChooseFolderFlag)) {
    text = "Choose file!\n";
    gtk_label_set_text(GTK_LABEL(data[1]), text);
    return;
  }
  int i = 0;
  size_t str_length = strlen(text);
  char *command = (char *)malloc(n);
  char *name = (char *)malloc(n);
  char *path = (char *)malloc(n);
  strcpy(command, "mkisofs -o ");
  for (str_length; str_length > 0; str_length--) {
    if (text[str_length] == '/')

    {
      str_length += 1;
      while ((text[str_length] != '.') && (text[str_length] != '\0')) {
        name[i] = text[str_length];
        str_length++;
        i++;
      }
      name[i] = '\0';
      break;
    }
  }
  strcat(name, ".iso ");
  strcpy(path, text);
  strcat(command, name);
  strcat(command, path);
  system(command);
  free(command);
  free(name);
  free(path);
}

void *thread_function3(char *data) {
  printf("%s\n\n", data);
  system(data);
}

static float percent = 0.0;
static gboolean inc_progress(gpointer *data) {
  while (percent < 1) {
    percent += 0.01;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(data[2]), percent);
  }
}

void *thread_function4(gpointer *data) {
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(data[2]), 0);
  gchar *text = gtk_label_get_label(data[1]);
  char *extension = (char *)malloc(n);
  char *command = (char *)malloc(n);
  int i = 0;
  size_t str_length = strlen(text);
  printf("%s\n", text);
  for (str_length; str_length > 0; str_length--) {
    if (text[str_length] == '.') {
      while (text[str_length] != '\0') {
        extension[i] = text[str_length];
        str_length++;
        i++;
      }
      extension[i] = '\0';
      break;
    }
  }
  if (strcmp(extension, ISO) == 0) {
    strcpy(command, "./iso ");
    strcat(command, text);
    system(command);
  }
  if (strcmp(extension, WAV) == 0) {
    strcpy(command, "./au ");
    strcat(command, text);
    system(command);
  }
  if (strcmp(extension, MP3) == 0) {
    int i = 0;
    int res;
    pthread_t a_thread;
    void *thread_result;
    char *comm = (char *)malloc(n);
    char *mp = (char *)malloc(n);
    char *wav = (char *)malloc(n);
    size_t str_Length = strlen(text);
    while (1) {
      if (text[i] == '.') {
        mp[i] = '\0';
        wav[i] = '\0';
        break;
      } else {
        mp[i] = text[i];
        wav[i] = text[i];
        i++;
      }
    }
    memset(comm, '\0', n);
    strcat(mp, ".mp3 ");
    strcat(wav, ".wav ");
    strcat(comm, "mpg123 -w ");
    strcat(comm, wav);
    strcat(comm, " ");
    strcat(comm, mp);
    system(comm);
    memset(comm, '\0', n);
    strcpy(comm, "./au ");
    strcat(comm, wav);
    system(comm);
    free(comm);
    free(mp);
    free(wav);
  }
  free(extension);
  free(command);
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(data[2]), 1);
}

static void do_write(GtkWidget *button, gpointer *data) {
  int res;
  pthread_t a_thread;
  void *thread_result;
  res = pthread_create(&a_thread, NULL, thread_function4, data);
  if (res != 0) {
    perror("Thread creation failed");
    return;
  }
}

void *thread_function2(gpointer *data) { system("./blank"); }

static void do_blank(GtkWidget *button, gpointer *data) {
  gtk_label_set_text(GTK_LABEL(data[1]), Message1);
  int res;
  pthread_t a_thread;
  void *thread_result;
  res = pthread_create(&a_thread, NULL, thread_function2, data);
  if (res != 0) {
    perror("Thread creation failed");
    return;
  }
  return;
}

static void open_pipe() {
  if (mkfifo(NAMEDPIPE_NAME, 0777)) {
    perror("Mkfifo");
  }
  if ((fd = open(NAMEDPIPE_NAME, O_RDWR)) <= 0) {
    perror("openfile");
    return;
  }
}

gpointer thread_func(gpointer *data) {
  while (1) {
    memset(buffer, '\0', BUF_SIZE);
    if ((len = read(fd, buffer, BUF_SIZE - 1)) > 0) {
      if ((buffer[0] >= '0') && (buffer[0] <= '10')) {
        int i = atoi(buffer);
        gdouble j = 0;
        int count = 0;
        while (count < i) {
          j += 0.1;
          gtk_progress_bar_set_fraction(data[2], j);
          count++;
        }
      } else {
        gtk_label_set_text(GTK_LABEL(data[1]), buffer);
      }
    }
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  int pid;
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *file;
  GtkWidget *folder;
  GtkWidget *iso;
  GtkWidget *delete;
  GtkWidget *diskinfo;
  GtkWidget *open;
  GtkWidget *info;
  GtkWidget *write;
  GtkWidget *pbar;

  int res;
  pthread_t a_thread;
  void *thread_result;

  gtk_init(&argc, &argv);

  open_pipe();

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size(GTK_WINDOW(window), 400, 500);
  gtk_window_set_title(GTK_WINDOW(window), "CD");
  gtk_container_set_border_width(GTK_CONTAINER(window), 5);

  vbox = gtk_vbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  file = gtk_button_new_with_label("Выбрать файл");
  folder = gtk_button_new_with_label("Выбрать папку");
  write = gtk_button_new_with_label("Записать файл");
  iso = gtk_button_new_with_label("Сделать ISO");
  delete = gtk_button_new_with_label("Очистить диск");
  diskinfo = gtk_button_new_with_label("Информация о диске");
  open = gtk_button_new_with_label("Открыть/закрыть привод");
  info = gtk_label_new("");
  pbar = gtk_progress_bar_new();
  label = gtk_label_new("Bobrovsky © 2016");

  gtk_box_pack_start(GTK_BOX(vbox), file, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), folder, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), write, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), iso, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), delete, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), diskinfo, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), open, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), info, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), pbar, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  gpointer *data[3];
  data[0] = window;
  data[1] = info;
  data[2] = pbar;

  res = pthread_create(&a_thread, NULL, thread_func, data);
  if (res != 0) {
    perror("Thread creation failed");
    return;
  }

  g_signal_connect(file, "clicked", G_CALLBACK(ChooseFile), data);
  g_signal_connect(folder, "clicked", G_CALLBACK(ChooseFolder), data);
  g_signal_connect(iso, "clicked", G_CALLBACK(do_iso), data);
  g_signal_connect(write, "clicked", G_CALLBACK(do_write), data);
  g_signal_connect(delete, "clicked", G_CALLBACK(do_blank), data);
  g_signal_connect(open, "clicked", G_CALLBACK(open_close), data);
  g_signal_connect(diskinfo, "clicked", G_CALLBACK(getInfo), data);
  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit),
                   G_OBJECT(window));

  gtk_widget_show_all(window);

  gtk_main();
  close(fd);
  remove(NAMEDPIPE_NAME);
  return 0;
}
