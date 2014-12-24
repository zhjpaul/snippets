#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAXARGS 3

struct cmd {
    int type;
};

struct execcmd {
    int type;
    char *argv[MAXARGS];
};

struct redircmd {
    int  type;
    struct cmd *cmd;
    char *file;
    int mode;
    int fd;
};

struct pipecmd {
    int type;
    struct cmd *left;
    struct cmd *right;
};

int fork1(void);
struct cmd *parsecmd(char*);

void runcmd(struct cmd *cmd) {
    int p[2], r;
    struct execcmd *ecmd;
    struct pipecmd *pcmd;
    struct redircmd *rcmd;

    if (cmd == 0) {
        exit(0);
    }

    switch (cmd->type) {
    default:
        fprintf(stderr, "unknown runcmd\n");
        exit(-1);
    case ' ':
        ecmd = (struct execcmd*) cmd;
        if (ecmd->argv[0] == 0) {
            exit(0);
        }
        //fprintf(stderr, "exec not implemented\n");
        char *path = "/bin/";
        strcpy(path+5, ecmd->argv[0]);
        fprintf(stderr, path);
        execl(path, ecmd->argv[0], ecmd->argv[1], ecmd->argv[2], NULL);
        break;
    case '>':
    case '<':
        rcmd = (struct redircmd*) cmd;
        fprintf(stderr, "redir not implemented\n");
        runcmd(rcmd->cmd);
        break;
    case '|':
        pcmd = (struct pipecmd*) cmd;
        fprintf(stderr, "pipe not implemented\n");
        break;
    }
    exit(0);
}

int getcmd(char *buf, int nbuf) {
    if (isatty(fileno(stdin))) {
        fprintf(stdout, "6.828$");
    }
    memset(buf, 0, nbuf);
    fgets(buf, nbuf, stdin);
    if (buf[0] == 0) {
        return -1;
    }
    return 0;
}

int main(void) {
    static char buf[100];
    int fd, r;

    while(getcmd(buf, sizeof(buf)) >= 0) {
        if (buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' ') {
            buf[strlen(buf)-1] = 0;
            if (chdir(buf+3) < 0) {
                fprintf(stderr, "cannot cd %s\n", buf+3);
            }
            continue;
        }
        if (fork1() == 0) {
            runcmd(parsecmd(buf));
        }
        wait(&r);
    }
    exit(0);
}

int fork1(void) {
    int pid;

    pid = fork();
    if (pid == -1) {
        perror("fork");
    }

    return pid;
}

struct cmd *execcmd(void) {
    struct execcmd *cmd;

    cmd = malloc(sizeof(*cmd));
    memset(cmd, 0 ,sizeof(*cmd));
    cmd->type = ' ';
    return (struct cmd*) cmd;
}

struct cmd *redircmd(struct cmd *subcmd, char *file, int type) {
    struct redircmd *cmd;

    cmd = malloc(sizeof(*cmd));
    memset(cmd, 0, sizeof(*cmd));
    cmd->type = type;
    cmd->cmd = subcmd;
    cmd->file = file;
    cmd->mode = (type == '<') ? O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC;
    cmd->fd = (type == '<') ? 0 : 1;
    return (struct cmd*) cmd;
}

struct cmd* pipecmd(struct cmd *left, struct cmd *right) {
    struct pipecmd *cmd;

    cmd = malloc(sizeof(*cmd));
    memset(cmd, 0, sizeof(*cmd));
    cmd->type = '|';
    cmd->left = left;
    cmd->right = right;
    return (struct cmd*)cmd;
}

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>";

int gettoken(char **ps, char *es, char **q, char **eq) {
    char *s;
    int ret;

    s = *ps;
    while (s < es && strchr(whitespace, *s)) s++;

    if (q) {
        *q = s;
    }

    ret = *s;
    switch (*s) {
    case 0:
        break;
    case '|':
    case '<':
    case '>':
        s++;
        break;
    default: 
        ret = 'a';
        while (s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
            s++;
        break;
    }
    if (eq) {
        *eq = s;
    }

    while (s < es && strchr(whitespace, *s)) s++;
    *ps = s;
    return ret;
}

int peek(char **ps, char *es, char *toks) {
    char *s;

    s = *ps;
    while (s < es && strchr(whitespace, *s)) s++;
    *ps = s;
    return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);

char *mkcopy(char *s, char *es) {
    int n = es -s; 
    char *c = malloc(n+1);
    assert(c);
    strncpy(c, s, n);
    c[n] = 0;
    return c;
}

struct cmd* parsecmd(char *s) {
    char *es;
    struct cmd *cmd;

    es = s + strlen(s);
    cmd = parseline(&s, es);
    peek(&s, es, "");
    if (s != es) {
        fprintf(stderr, "leftovers :%s\n", s);
        exit(-1);
    }
    return cmd;
}

struct cmd *parseline(char **ps, char *es) {
    struct cmd *cmd;
    cmd = parsepipe(ps, es);
    return cmd;
}

struct cmd *parsepipe(char **ps, char *es) {
    struct cmd *cmd;

    cmd = parseexec(ps, es);
    if (peek(ps, es, "|")) {
        gettoken(ps, es, 0, 0);
        cmd = pipecmd(cmd, parsepipe(ps, es));
    }
    return cmd;
}

struct cmd* parseredirs(struct cmd *cmd, char **ps, char *es) {
    int tok;
    char *q, *eq;

    while (peek(ps, es, "<>")) {
        tok = gettoken(ps, es, 0, 0);
        if (gettoken(ps, es, &q, &eq) != 'a') {
            fprintf(stderr, "missing file for redirection\n");
            exit(-1);
        }
        switch (tok) {
        case '<':
            cmd = redircmd(cmd, mkcopy(q, eq), '<');
            break;
        case '>':
            cmd = redircmd(cmd, mkcopy(q, eq), '>');
            break;
        }
    }
    return cmd;
}

struct cmd *parseexec(char **ps, char *es) {
    char *q, *eq;
    int tok, argc;
    struct execcmd *cmd;
    struct cmd *ret;

    ret = execcmd();
    cmd = (struct execcmd*)ret;

    argc = 0;
    ret = parseredirs(ret, ps, es);
    while (!peek(ps, es, "|")) {
        if ((tok=gettoken(ps, es, &q, &eq)) == 0) break;
        if (tok != 'a') {
            fprintf(stderr, "synax error\n");
            exit(-1);
        }
        cmd->argv[argc] = mkcopy(q, eq);
        argc++;
        if (argc >= MAXARGS) {
            fprintf(stderr, "too many args\n");
            exit(-1);
        }
        ret = parseredirs(ret, ps, es);
    }
    cmd->argv[argc] = 0;
    return ret;
}
