
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <memory.h>

int append_write(char* filename, char* data) {
    FILE *fp = fopen(filename, "w+");
    if (fp == NULL) {
        printf("打开文件失败:%s\n", filename);
        exit(EXIT_FAILURE);
    }
    int num = fputs(data, fp);
    
    if (num != EOF) {
        int code = fclose(fp);
        if (code != EOF) {
            return num;
        }
    }
    fclose(fp);
    return EOF;
}

void format_read(FILE *fp, char* mode, char* format, ...) {

    // %[*][width][length]type

    va_list ap;
    va_start(ap, format);
    vfscanf(fp, format, ap);
    va_end(ap);
}

char* read_all(FILE* fp) {

    int code = 0;
    struct stat fi;
    code = fstat(fp->_file, &fi);
    // fseek(fp, 0, SEEK_END);
    // ftell(fp);
    char* buf = NULL;
    buf = malloc(fi.st_size);
    memset(buf, 0, fi.st_size);
    fread(buf, fi.st_size, 1, fp);
    return buf;
}

int main(char** args) {
    char* filename = "d:/ztestfiles/ztest.txt";
    FILE *fp = fopen(filename, "r");

    char word[6];
    char ignore[1];
    char* format = "%[^\n]%c";
    format_read(fp, "r", format, word, ignore);
    printf("read: %s\n", word);
    format_read(fp, "r", format, word, ignore);
    printf("read: %s\n", word);
    rewind(fp);
    char* contents = read_all(fp);
    printf("read all contents:\n%s", contents);
    free(contents);

    fclose(fp);
    return EXIT_SUCCESS;
}