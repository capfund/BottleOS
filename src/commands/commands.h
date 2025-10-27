#ifndef COMMANDS_H
#define COMMANDS_H

void cmd_hello(void);
void cmd_clear(void);
void cmd_echo(int argc, char *argv[]);
int cmd_bye(int argc, char *argv[]);
void cmd_ls(void);
void cmd_touch(int argc, char *argv[]);
void cmd_cat(int argc, char *argv[]);
void cmd_write(int argc, char **argv);

#endif
