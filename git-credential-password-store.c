#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ftw.h>
#include <ctype.h>

#define BUF_SIZE 1024
static char buffer[BUF_SIZE];
static char *store_root;

static void die(const char *err)
{
    fprintf(stderr, "%s\n", err);
    exit(1);
}

static void *xstrdup(const char *str)
{
    void *ret = strdup(str);
    if (!ret) die("Out of memory");
    return ret;
}

static void *xstrconcat(const char *str1, const char *str2)
{
    void *ret = malloc(strlen(str1) + strlen(str2));
    if (!ret) die("Out of memory");
    strcpy(ret, str1);
    strcpy(ret + strlen(str1), str2);
    return ret;
}

static char *xstrstrip(char *str)
{
    int len = strlen(str);
    char *ret = str;
    while(*ret && isspace(*ret)) { ++ret; --len; }
    if(ret!=str) memmove(str, ret, len + 1);
    while(len && isspace(str[len-1])) str[--len]='\0';
    return str;
}


//-----------------------------------------------------------------------------
//
// CREDENTIALS
//
// These are passed around as key=value lines in stdin/stdout.
//
//-----------------------------------------------------------------------------

struct Credentials {
    char *protocol;
    char *username;
    char *host;
    char *path;
    char *password;
};

static struct Credentials* git_credentials;

static struct Credentials* credentials_new()
{
    void *ret = calloc(1, sizeof(struct Credentials));
    if(!ret) die("Out of memory.");
    return ret;
}

static void credentials_free(struct Credentials* creds)
{
    if(!creds) return;
    free(creds->protocol);
    free(creds->username);
    free(creds->host);
    free(creds->path);
    free(creds->password);
    free(creds);
}

static void credentials_set(struct Credentials *creds,
                            char *prop,
                            char *value)
{
    if(!strcasecmp("host", prop)) {
        free(creds->host);
        char *sep = strchr(value, ':');
        if(sep) *sep = '\0';
        creds->host = xstrdup(value);
        return;
    }

    char **dest = NULL;
    if(!strcasecmp("protocol",prop)) dest = &(creds->protocol);
    else if(!strcasecmp("password",prop)) dest = &(creds->password);
    else if(!strcasecmp("path",prop)) dest = &(creds->path);
    else if(!strcasecmp("username",prop)
            || !strcasecmp("user",prop)
            || !strcasecmp("login",prop))
        dest = &(creds->username);

    if(dest) {
        free(*dest);
        *dest = xstrstrip(xstrdup(value));
    }
}

static void credentials_line(
    struct Credentials *creds,
    char *line,
    char delim)
{
    //
    // This is not very robust parsing, but it is not any worse than
    // the parsing in the osxkeychain credentials helper.
    //
    if(!line || *line == '\n') return;
    line[strlen(line)-1] = '\0';
    char *sep = strchr(line, delim);
    if(!sep) return;
    *sep++ = '\0';
    credentials_set(creds, line, sep);
}

static void credentials_git_read()
{
    while(fgets(buffer, sizeof(buffer), stdin))
        credentials_line(git_credentials, buffer, '=');
}

static void credentials_pass_read(struct Credentials *creds,
                                  const char *entry)
{
    char *cmd = xstrconcat("pass ", entry);
    FILE* stream = popen(cmd, "r");
    if(!stream) goto finish;

    char line[BUF_SIZE];
    for(int first=1; fgets(line, sizeof(line), stream); first=0) {
        if(first) credentials_set(creds, "password", line);
        else credentials_line(creds, line, ':');
    }

finish:
    if(stream) pclose(stream);
    free(cmd);
}

//-----------------------------------------------------------------------------
//
// GET
//
//-----------------------------------------------------------------------------

static int got_one = 0;

int do_get(const char *filepath,
           const struct stat *info,
           const int ftw_type,
           struct FTW *pathinfo)
{
    if(got_one || ftw_type != FTW_F) return 0;

    int len = strlen(filepath);

    if(len < 5) return 0;
    if(strcmp(filepath + len - 4, ".gpg")) return 0;
    if(strncmp(store_root, filepath, strlen(store_root))) return 0;

    char *key = xstrdup(filepath);
    key[strlen(key)-4] = '\0';
    memmove(key, key+strlen(store_root)+1, strlen(key) - strlen(store_root));

    if(!strcasestr(key, git_credentials->host)) return 0;

    struct Credentials *candidate = credentials_new();
    credentials_pass_read(candidate, key);
    if(candidate->password
       && candidate->username
       && (!git_credentials->username
           || !strcmp(candidate->username, git_credentials->username)))
    {
        printf("password=%s\n", candidate->password);
        if(!git_credentials->username) printf("username=%s\n", candidate->username);
        got_one = 1;
    }

    free(key);
    credentials_free(candidate);

    return 0;
}

int main(int argc, char **argv)
{
    if(!argv[1])
        die("usage: git credential-password-store get");

    char *env;
    if((env = getenv("PASSWORD_STORE_DIR"))) store_root = xstrdup(env);
    else store_root = xstrconcat(getenv("HOME"), "/.password-store");

    git_credentials = credentials_new();

    if(!strcmp(argv[1],"get")) {
        credentials_git_read();
        if(git_credentials->host)
            nftw(store_root, do_get, 1, FTW_PHYS);
    }

    credentials_free(git_credentials);
    free(store_root);
}
