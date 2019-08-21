#ifndef _CONFIG_FILE_H_
#define _CONFIG_FILE_H_

namespace zf {

enum {
  CONFIG_OK = 0,
  CONFIG_ERROR = 1
};

typedef char flag_t;

typedef struct command_s {
  const char *name;
  int (*handler)(int argc, char **argv, struct command_s *cmd, void *conf);
  unsigned int offset;                  // offset of the slot
} command_t;

#define null_command { "", NULL, 0 }

#define is_null_command(command) (strlen((command).name) == 0)

int load_conf_file(const char *filename, command_t *cmd_table, void *conf);

int conf_set_flag_slot(int argc, char **argv, command_t *cmd, void *conf);

int conf_set_usec_slot(int argc, char **argv, command_t *cmd, void *conf);

int conf_set_str_slot(int argc, char **argv, command_t *cmd, void *conf);

int conf_set_num_slot(int argc, char **argv, command_t *cmd, void *conf);

int conf_set_mem_slot(int argc, char **argv, command_t *cmd, void *conf);

} // namespace zf

#endif


