#include "../include/t.h"
#include "../include/compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Function declarations
void error(char * text, int line);
void arrlstrip(char * line[]);
int stringIsInt(char * s);
int stringHasChar(char * s, char c);
int stringInList(char * arr[], char * key);

char * compileline(char * line[], int num, int lineLen, int isInline);
char * compile_tokens(char ** tokens);
char * compile_raw(char * rawtext, int maxtokensize);
char * quokka_compile(char * filename);

// Bools
int verbose = 0;

// Ints
int scope = 0;

// Strings
char * current_file;
char * file_declarations;
char * object_def;
char * class_constructor;
char * class_declarations;
char * cur_class_def;

// String arrays
char * pointers[512];
char * values[512];
char * arrptrs[512];

void error(char * text, int line)
{
    int MAXLINE = 256;

    char * fullfile = readfile(current_file);
    char * linepreview = (char *)malloc(MAXLINE);
    strcpy(linepreview, "");

    int c = 0;
    for (int i = 0; i < strlen(fullfile) - 1; i++)
    {
        if (fullfile[i] == '\n')
            c++;
        else if (c == line && strlen(linepreview) < MAXLINE && fullfile[i] != 10 && fullfile[i] != 13)
            strcat(linepreview, String(fullfile[i]).value);
        if (c > line)
            break;
    }

    line++;

    println("\nProgram compilation terminated:");
    printf("At %s:%d\n", current_file, line);
    printf("--> %s\n", text);
    printf("  %d | %s\n\n", line, linepreview);
    free(linepreview);
    exit(EXIT_FAILURE);
}

void arrlstrip(char * line[])
{
    int position, c, n;
    position = 0;
    n = arrsize(line);
    for (c = position - 1; c < n - 1; c++)
        line[c] = line[c + 1];
}

int stringIsInt(char * s)
{
    for (int i = 0; i < strlen(s); ++i)
    {
        if (s[i] != '0' &&
            s[i] != '1' &&
            s[i] != '2' &&
            s[i] != '3' &&
            s[i] != '4' &&
            s[i] != '5' &&
            s[i] != '6' &&
            s[i] != '7' &&
            s[i] != '8' &&
            s[i] != '9')
            return 0;
    }
    return 1;
}

int stringHasChar(char * s, char c)
{
    for (int i = 0; i < strlen(s); ++i)
    {
        if (s[i] == c)
            return 1;
    }
    return 0;
}

int stringInList(char * arr[], char * key)
{
    for (int n = 0; n < arrsize(arr); ++n)
    {
        if (strcmp(arr[n], key) == 0)
            return 1;
    }
    return 0;
}

char * compileline(char * line[], int num, int lineLen, int isInline)
{
    // num is current line number

    char r[512] = {0};
    char * r_ptr = r;
    int len;
    if (lineLen == -1) len = arrsize(line);
    else len = lineLen;

    if (startswith(line[0], "//"))
        return r_ptr;

    if (len < 1)
        return r_ptr;

    if (line[1] == NULL && len <= 1)
        line[1] = "";

    for (int p = 0; p < len; p++)
    {
        if (startswith(line[p], "'") && endswith(line[p], "'"))
        {
            char * temp = (char *)malloc(strlen(line[p]));
            strcpy(temp, "\"");
            string sliced = __slice_string__(String(line[p]), 1, 1);
            string escaped = __replace_string__(sliced, String("\""), String("\\\""));
            strcat(temp, escaped.value);
            strcat(temp, "\"");
            strcpy(line[p], temp);
            free(temp);
        }
    }

    if (scope == -1)
    {
        if (strcmp(line[0], "end") == 0)
        {
            if (len > 1) error("end action received too many arguments", num);
            scope++;
        }
        else if (len > 1)
            error("invalid syntax, headers must be included on separate lines", num);
        else
        {
            char * header_name;
            if (stringHasChar(line[0], '.'))
            {
                header_name = (char *)malloc(strlen(line[0]) + 1);
                strcpy(header_name, line[0]);
            }
            else
            {
                header_name = (char *)malloc(strlen(line[0]) + 3);
                strcpy(header_name, line[0]);
                strcat(header_name, ".q");
            }
            if (strcmp(current_file, header_name) == 0)
            {
                char * err = (char *)malloc(strlen(line[0]) + strlen(header_name) + 92); // 1 + 18 + 72 + 1 (\0 ending)
                strcpy(err, "'");
                strcat(err, line[0]);
                strcat(err, "' (at local path '");
                strcat(err, header_name);
                strcat(err, "') is being imported from within itself, this should not happen");
                error(err, num);
            }
            char * oldfile = current_file;
            current_file = header_name;
            scope = 0;
            char * header_data = quokka_compile(header_name);
            current_file = oldfile;
            scope = -1;
            if (strlen(header_data) <= 0)
            {
                char * err = (char *)malloc(strlen(line[0]) + strlen(header_name) + 92); // 1 + 18 + 72 + 1 (\0 ending)
                strcpy(err, "'");
                strcat(err, line[0]);
                strcat(err, "' (at local path '");
                strcat(err, header_name);
                strcat(err, "') is an invalid header file, make sure the file exists and is not empty");
                error(err, num);
            }
            strcat(r, header_data);
        }

        return r_ptr;
    }

    // if (strcmp(line[0], "push") == 0)
    // {
    //     if (isInline) error("push action must be at start of line", num);
    //     if (len < 2) error("push action missing argument", num);
    //     arrlstrip(line);
    //     len--;
    //     strcat(r, "push(pt,");
    //     strcat(r, compileline(line, num, len, 1));
    //     strcat(r, ");\n");
    // }
    // else if (strcmp(line[0], "pop") == 0)
    // {
    //     if (!isInline) error("pop action must not be at start of line", num);
    //     if (len > 1) error("pop action received too many arguments", num);
    //     strcat(r, "pop(pt);\n");
    // }
    if (strcmp(line[0], "print") == 0)
    {
        if (len < 2) strcat(r, "print(pop(pt));\n");
        else
        {
            arrlstrip(line);
            len--;
            strcat(r, "print(");
            strcat(r, compileline(line, num, len, 1));
            strcat(r, ");\n");
        }
    }
    else if (strcmp(line[0], "println") == 0)
    {
        if (len < 2) strcat(r, "println(pop(pt));\n");
        else
        {
            arrlstrip(line);
            len--;
            strcat(r, "println(");
            strcat(r, compileline(line, num, len, 1));
            strcat(r, ");\n");
        }
    }
    else if (strcmp(line[0], "if") == 0)
    {
        if (isInline) error("if statement must be at start of line", num);
        if (len < 2) error("if statement missing condition", num);
        else if (len > 2) error("if statement received too many conditions", num);
        strcat(r, "if(");
        strcat(r, line[1]);
        strcat(r, "){\n");
        scope++;
    }
    else if (strcmp(line[0], "elif") == 0)
    {
        if (isInline) error("elif statement must be at start of line", num);
        if (len < 2) error("elif statement missing condition", num);
        else if (len > 2) error("elif statement received too many conditions", num);
        strcat(r, "else if(");
        strcat(r, line[1]);
        strcat(r, "){\n");
        scope++;
    }
    else if (strcmp(line[0], "else") == 0)
    {
        if (isInline) error("else statement must be at start of line", num);
        else if (len > 1) error("else statement received too many conditions", num);
        strcat(r, "else{\n");
        scope++;
    }
    else if (strcmp(line[0], "while") == 0)
    {
        if (isInline) error("while loop must be at start of line", num);
        if (len < 2) error("while loop missing condition", num);
        else if (len > 2) error("while loop received too many conditions", num);
        strcat(r, "while(");
        strcat(r, line[1]);
        strcat(r, "){\n");
        scope++;
    }
    else if (strcmp(line[0], "for") == 0)
    {
        if (isInline) error("for loop must be at start of line", num);
        if (len < 2) error("for loop missing variable type", num);
        if (len < 3) error("for loop missing variable name", num);
        if (len < 4) error("for loop missing variable value", num);
        if (len < 5) error("for loop missing condition", num);
        if (len < 6) error("for loop missing end-of-line action (usually i++ or similar)", num);
        if (len > 6) error("for loop received too many arguments", num);
        strcat(r, "for(");
        strcat(r, line[1]);
        strcat(r, " ");
        strcat(r, line[2]);
        strcat(r, "=");
        strcat(r, line[3]);
        strcat(r, ";");
        strcat(r, line[4]);
        strcat(r, ";");
        strcat(r, line[5]);
        strcat(r, "){\n");
        values[arrsize(values)] = line[2];
        scope++;
    }
    else if (strcmp(line[0], "foreach") == 0)
    {
        if (isInline) error("foreach loop must be at start of line", num);
        if (len < 2) error("foreach loop missing variable type", num);
        if (len < 3) error("foreach loop missing variable name", num);
        if (len < 4) error("foreach loop missing iterable", num);
        if (len > 4) error("foreach loop received too many arguments", num);
        strcat(r, "for(_FORVARNAME;");
        strcat(r, "_FORVARNAME<arrsize(");
        strcat(r, line[3]);
        strcat(r, ");_FORVARNAME++){\n");
        strcat(r, line[1]);
        strcat(r, " ");
        strcat(r, line[2]);
        strcat(r, "=");
        strcat(r, line[3]);
        strcat(r, "[_FORVARNAME];\n");
        values[arrsize(values)] = line[2];
        scope++;
    }
    else if (strcmp(line[0], "rangefor") == 0)
    {
        char * start;
        char * stop;
        char * step;
        if (isInline) error("rangefor loop must be at start of line", num);
        else if (len < 2) error("rangefor loop missing variable name", num);
        else if (len < 3) error("rangefor loop missing range stop number", num);
        else if (len == 3)
        {
            start = "0";
            stop = line[2];
            step = "1";
        }
        else if (len == 4)
        {
            start = line[2];
            stop = line[3];
            step = "1";
        }
        else if (len == 5)
        {
            start = line[2];
            stop = line[3];
            step = line[4];
        }
        else if (len > 5) error("rangefor loop received too many arguments", num);
        strcat(r, "for(int ");
        strcat(r, line[1]);
        strcat(r, "=");
        strcat(r, start);
        strcat(r, ";");
        strcat(r, line[1]);
        strcat(r, "<");
        strcat(r, stop);
        strcat(r, ";");
        strcat(r, line[1]);
        strcat(r, "+=");
        strcat(r, step);
        strcat(r, "){\n");
        values[arrsize(values)] = line[2];
        scope++;
    }
    else if (strcmp(line[0], "string") == 0)
    {
        if (isInline) error("string declaration must be at start of line", num);
        else if (len < 2) error("string declaration missing variable name", num);
        else if (len < 3)
        {
            strcat(r, "string ");
            strcat(r, line[1]);
            strcat(r, ";\n");
            //pointers[arrsize(pointers)] = line[1];
        }
        else
        {
            strcat(r, "string ");
            strcat(r, line[1]);
            strcat(r, "=");
            //pointers[arrsize(pointers)] = line[1];
            arrlstrip(line);
            arrlstrip(line);
            len -= 2;
            char * stringvalue = compileline(line, num, len, 1);
            if (endswith(stringvalue, ","))
                strcpy(stringvalue, __slice_string__(String(stringvalue), 0, 1).value);
            if (!startswith(stringvalue, "String("))
            {
                char newstring[1024] = {0};
                strcpy(newstring, "String(");
                strcat(newstring, stringvalue);
                strcat(newstring, ")");
                strcat(r, newstring);
            } else strcat(r, stringvalue);
            strcat(r, ";\n");
        }
    }
    else if (strcmp(line[0], "string[]") == 0)
    {
        if (isInline) error("string[] declaration must be at start of line", num);
        else if (len < 2) error("string[] declaration missing variable name", num);
        else if (len < 3)
        {
            arrptrs[arrsize(arrptrs)] = line[1];
            strcat(r, "string*");
            strcat(r, line[1]);
            strcat(r, ";\n");
        }
        else
        {
            arrptrs[arrsize(arrptrs)] = line[1];
            strcat(r, "string*");
            strcat(r, line[1]);
            strcat(r, "=(string*)malloc(2*sizeof(string);");
            strcat(r, line[1]);
            strcat(r, "[0]=String(\"\\0\")");
            arrlstrip(line);
            arrlstrip(line);
            len -= 2;
            char * stringvalue = compileline(line, num, len, 1);
            strcat(r, stringvalue);
            strcat(r, ";\n");
        }
    }
    else if (strcmp(line[0], "int") == 0)
    {
        if (isInline) error("int declaration must be at start of line", num);
        else if (len < 2) error("int declaration missing arguments", num);
        else if (len < 3)
        {
            strcat(r, "int ");
            strcat(r, line[1]);
            strcat(r, ";\n");
            values[arrsize(values)] = line[1];
        }
        else
        {
            strcat(r, "int ");
            strcat(r, line[1]);
            strcat(r, "=");
            values[arrsize(values)] = line[1];
            arrlstrip(line);
            arrlstrip(line);
            len -= 2;
            strcat(r, compileline(line, num, len, 1));
            strcat(r, ";\n");
        }
    }
    // else if (strcmp(line[0], "pointer") == 0)
    // {
    //     if (isInline) error("pointer declaration must be at start of line", num);
    //     if (len < 2) error("pointer declaration missing arguments", num);
    //     if (len < 3)
    //     {
    //         strcat(r, "pointer ");
    //         strcat(r, line[1]);
    //         strcat(r, ";\n");
    //     }
    //     else
    //     {
    //         strcat(r, "pointer ");
    //         strcat(r, line[1]);
    //         strcat(r, ";\n");
    //         strcat(r, line[1]);
    //         strcat(r, ".value=");
    //         pointers[arrsize(pointers)] = line[1];
    //         arrlstrip(line);
    //         arrlstrip(line);
    //         len -= 2;
    //         strcat(r, compileline(line, num, len, 1));
    //         strcat(r, ";\n");
    //     }
    // }
    else if (strcmp(line[0], "set") == 0)
    {
        if (isInline) error("set action must be at start of line", num);
        if (len < 2) error("set action missing variable name", num);
        if (len < 3) error("set action missing new variable value", num);
        strcat(r, line[1]);
        strcat(r, "=");
        arrlstrip(line);
        arrlstrip(line);
        len -= 2;
        strcat(r, compileline(line, num, len, 1));
        strcat(r, ";\n");
    }
    // else if (strcmp(line[0], "var") == 0)
    // {
    //     if (isInline) error("var declaration must be at start of line", num);
    //     if (len < 2) error("var declaration missing variable type", num);
    //     if (len < 3) error("var declaration missing variable name", num);
    //     if (len < 4)
    //     {
    //         strcat(r, line[1]);
    //         strcat(r, " ");
    //         strcat(r, line[2]);
    //         strcat(r, ";\n");
    //     }
    //     else
    //     {
    //         strcat(r, line[1]);
    //         strcat(r, " ");
    //         strcat(r, line[2]);
    //         strcat(r, "=");
    //         strcat(r, line[3]);
    //         strcat(r, ";\n");
    //     }
    // }
    else if (startswith(line[1], "[") && endswith(line[1], "]"))
    {
        if (len > 3) error("index can not take arguments", num);
        strcat(r, line[0]);
        strcat(r, "[");
        strcat(r, __slice_string__(String(line[1]), 1, 1).value);
        strcat(r, "]");
        if (stringInList(arrptrs, line[0]))
        {
            strcat(r, ".value");
        }
        
        if (!isInline)
            strcat(r, ";\n");
    }
    else if (endswith(line[0], ":"))
    {
        char * name = __slice_string__(String(line[0]), 0, 1).value;
        if (isInline) error("proc `:` action must be at start of line", num);
        else if (scope > 0 && !strlen(class_constructor)) error("proc `:` action can not be in a scope greater than 1", num);
        else if (len < 1) error("proc `:` action missing arguments", num);
        else if (len > 1) error("proc `:` action received too many arguments", num);
        else if (len == 1 && strcmp(name, "include") != 0)
        {
            strcat(r, "int ");
            strcat(r, name);
            strcat(r, "(");
            if (strcmp(name, "main") == 0)
            {
                strcat(r, "int argc,char**argv");
                values[arrsize(values)] = "argc";
                pointers[arrsize(pointers)] = "argv";
            }
            else
            {
                file_declarations = (char *)realloc(file_declarations,
                    strlen(file_declarations) + 4 + strlen(name) + 4 + 1);
                strcat(file_declarations, "int ");
                strcat(file_declarations, name);
                strcat(file_declarations, "();\n");
            }
            strcat(r, "){\n");
        }
        if (strcmp(name, "include") == 0)
            scope = -1;
        else
            scope++;
    }
    else if (strcmp(line[0], "def") == 0)
    {
        if (isInline) error("def action must be at start of line", num);
        if (scope == 1 && strlen(object_def))
        {
            if (len < 2) error("def action missing type", num);
            else if (len < 3) error("def action missing name", num);
            else if (len == 3)
            {
                strcat(r, line[1]);
                strcat(r, " (*");
                strcat(r, line[2]);
                strcat(r, ")");
                strcat(r, "(");
                strcat(r, object_def);
                strcat(r, " self);\n");
            }
            else if (len == 4)
            {
                strcat(r, line[1]);
                strcat(r, " (*");
                strcat(r, line[2]);
                strcat(r, ")");
                strcat(r, "(");
                strcat(r, object_def);
                strcat(r, " self,");
                strcat(r, line[3]);
                strcat(r, ");\n");
            }
            else if (len > 4) error("def action received too many arguments", num);
        }
        else
        {
            if (scope > 0 && !strlen(class_constructor))
                error("def action can not be in a scope greater than 1 (excluding class definitions)", num);
            else if (len < 2) error("def action missing type", num);
            else if (len < 3) error("def action missing name", num);
            else if (len == 3)
            {
                if (strlen(class_constructor))
                {
                    class_declarations = (char *)realloc(class_declarations,
                        strlen(class_declarations) + strlen(line[1]) + 3 +
                        strlen(line[2]) + 12 + strlen(class_constructor) + 8 + 1);
                    strcat(class_declarations, line[1]);
                    strcat(class_declarations, " __");
                    strcat(class_declarations, line[2]);
                    strcat(class_declarations, "_");
                    strcat(class_declarations, class_constructor);
                    strcat(class_declarations, "__(");
                    strcat(class_declarations, class_constructor);
                    strcat(class_declarations, " self){\n");

                    cur_class_def = (char *)realloc(cur_class_def,
                        strlen(cur_class_def) + strlen(line[2]) + 1);
                    strcat(cur_class_def, line[2]);
                }
                else
                {
                    strcat(r, line[1]);
                    strcat(r, " ");
                    strcat(r, line[2]);
                    strcat(r, "(){\n");
                    file_declarations = (char *)realloc(file_declarations,
                        strlen(file_declarations) + strlen(line[1]) + 1 + strlen(line[2]) + 4 + 1);
                    strcat(file_declarations, line[1]);
                    strcat(file_declarations, " ");
                    strcat(file_declarations, line[2]);
                    strcat(file_declarations, "();\n");
                }
            }
            else if (len > 4) error("def action received too many arguments", num);
            // if (len == 4)
            else
            {
                if (strlen(class_constructor))
                {
                    class_declarations = (char *)realloc(class_declarations,
                        strlen(class_declarations) + strlen(line[1]) + 3 + strlen(line[2]) +
                        1 + strlen(class_constructor) + 3 + strlen(class_constructor) + 6 +
                        strlen(line[3]) + 3 + 1);
                    strcat(class_declarations, line[1]);
                    strcat(class_declarations, " __");
                    strcat(class_declarations, line[2]);
                    strcat(class_declarations, "_");
                    strcat(class_declarations, class_constructor);
                    strcat(class_declarations, "__(");
                    strcat(class_declarations, class_constructor);
                    strcat(class_declarations, " self,");
                    strcat(class_declarations, line[3]);
                    strcat(class_declarations, "){\n");

                    cur_class_def = (char *)realloc(cur_class_def,
                        strlen(cur_class_def) + strlen(line[2]) + 1);
                    strcat(cur_class_def, line[2]);
                }
                else
                {
                    strcat(r, line[1]);
                    strcat(r, " ");
                    strcat(r, line[2]);
                    strcat(r, "(");
                    strcat(r, line[3]);
                    strcat(r, "){\n");
                    file_declarations = (char *)realloc(
                        file_declarations, strlen(line[1]) + 1 + strlen(line[2]) + 1 + strlen(line[3]) + 4 + 1);
                    strcat(file_declarations, line[1]);
                    strcat(file_declarations, " ");
                    strcat(file_declarations, line[2]);
                    strcat(file_declarations, "(");
                    strcat(file_declarations, line[3]);
                    strcat(file_declarations, ");\n");
                }
            }
            scope++;
        }
    }
    else if (strcmp(line[0], "object") == 0)
    {
        char * name;
        if (strlen(class_constructor)) error("object definition must not be within class constructor", num);
        if (isInline) error("object definition must be at start of line", num);
        if (len > 2) error("object definition received too many arguments", num);
        if (len < 2)
        {
            int ext = strlen(strchr(current_file, '.'));
            name = (char *)malloc(strlen(current_file) - ext);
            strcpy(name, __slice_string__(String(current_file), 0, ext).value);
        }
        else
        {
            name = (char *)malloc(strlen(line[1]));
            strcpy(name, line[1]);
        }
        // Define type
        file_declarations = (char *)realloc(
            file_declarations, strlen(file_declarations) + 17 + strlen(name) + 10 + strlen(name) + 2 + 1);
        strcat(file_declarations, "typedef struct __");
        strcat(file_declarations, name);
        strcat(file_declarations, "_Struct__ ");
        strcat(file_declarations, name);
        strcat(file_declarations, ";\n");
        // Define struct
        file_declarations = (char *)realloc(
            file_declarations, strlen(file_declarations) + 9 + strlen(name) + 12 + 1);
        strcat(file_declarations, "struct __");
        strcat(file_declarations, name);
        strcat(file_declarations, "_Struct__ {\n");
        object_def = (char *)realloc(object_def, strlen(object_def) + strlen(name) + 1);
        strcpy(object_def, name);
        free(name);
        scope++;
    }
    else if (strcmp(line[0], "class") == 0)
    {
        char * name;
        char * args = 0;
        if (strlen(class_constructor)) error("class constructor can not be within class constructor", num);
        if (scope > 0) error("class constructor scope must not be higher than minimum scope", num);
        if (isInline) error("class constructor definition must be at start of line", num);
        if (len > 3) error("class constructor definition received too many arguments", num);
        // if `class | classname | (int arg1, int arg2)`
        if (len == 3)
        {
            name = (char *)malloc(strlen(line[1]));
            strcpy(name, line[1]);

            args = line[2];
        }
        else if (len == 2)
        {
            // if `class | (int arg1, int arg2)`
            if (startswith(line[1], "(") && endswith(line[1], ")"))
            {
                int ext = strlen(strchr(current_file, '.'));
                name = (char *)malloc(strlen(current_file) - ext);
                strcpy(name, __slice_string__(String(current_file), 0, ext).value);

                args = line[1];
            }
            // if `class | classname`
            else
            {
                name = (char *)malloc(strlen(line[1]));
                strcpy(name, line[1]);
            }
        }
        else if (len == 1)
        {
            int ext = strlen(strchr(current_file, '.'));
            name = (char *)malloc(strlen(current_file) - ext);
            strcpy(name, __slice_string__(String(current_file), 0, ext).value);
        }
        strcat(r, name);
        strcat(r, " __");
        strcat(r, name);
        strcat(r, "_Constructor__(");
        if (args) println("YUP");
        else println("NUPE");
        if (args && strlen(args) > 2)
        {
            char * tempstr[512];
            tokenise(tempstr, __slice_string__(String(args), 1, 1).value, ",");
            for (int p = 0; p < arrsize(tempstr); p++)
            {
                tempstr[p] = cpstrip(tempstr[p]);
                if (verbose) println(tempstr[p]);
            }
            strcat(r, compileline(tempstr, num, arrsize(tempstr), 1));
        }
        strcat(r, "){\n");
        strcat(r, name);
        strcat(r, " self;\n");
        class_constructor = (char *)realloc(class_constructor,
            strlen(class_constructor) + strlen(name) + 1);
        strcpy(class_constructor, name);
        free(name);
        scope++;
    }
    else if (strcmp(line[0], "new") == 0)
    {
        char * args = 0;
        if (isInline) error("new object creation must be at start of line", num);
        if (scope < 1) error("new object creation must not be at minimum scope", num);
        if (len > 4) error("new object creation received too many arguments", num);
        if (len < 2) error("new object creation missing object type", num);
        if (len < 3) error("new object creation missing object name", num);
        if (len == 4)
            args = line[3];
        if (!(startswith(args, "(") && endswith(args, ")")) || !args)
            error("invalid constructor arguments in new object creation, arguments must have braces", num);
        strcat(r, line[1]);
        strcat(r, " ");
        strcat(r, line[2]);
        strcat(r, "=__");
        strcat(r, line[1]);
        strcat(r, "_Constructor__(");
        if (args && strlen(args) > 2)
        {
            char * tempstr[512];
            tokenise(tempstr, __slice_string__(String(args), 1, 1).value, ",");
            for (int p = 0; p < arrsize(tempstr); p++)
            {
                tempstr[p] = cpstrip(tempstr[p]);
                if (verbose) println(tempstr[p]);
            }
            strcat(r, compileline(tempstr, num, arrsize(tempstr), 1));
        }
        strcat(r, ");\n");
    }
    else if (startswith(line[1], "(") && endswith(line[1], ")"))
    {
        if (len > 3) error("function call can not take arguments after itself", num);
        char * tempstr[512];
        tokenise(tempstr, __slice_string__(String(line[1]), 1, 1).value, ",");
        if (verbose) println("START");
        for (int p = 0; p < arrsize(tempstr); p++)
        {
            tempstr[p] = cpstrip(tempstr[p]);
            if (verbose) println(tempstr[p]);
        }
        if (verbose) println("END");
        int tempsize = arrsize(tempstr);
        strcat(r, line[0]);
        strcat(r, "(");
        if (stringHasChar(line[0], '.'))
        {
            char ** temp = (char **)malloc(charCount(line[0], '.') + 1 * sizeof(char *));
            tokenise(temp, line[0], ".");
            strcat(r, cpstrip(temp[0]));
            if (arrsize(tempstr))
                strcat(r, ",");
        }
        if (tempsize)
            strcat(r, compileline(tempstr, num, tempsize, 1));
        strcat(r, ")");
        if (!isInline)
            strcat(r, ";\n");
    }
    else if (strcmp(line[0], "end") == 0)
    {
        if (isInline) error("end action must be at start of line", num);
        else if (scope < 1) error("end action has nothing to end, program is already at minimum scope", num);
        else if (len > 1) error("end action received too many arguments", num);
        else if (strlen(class_constructor) && scope == 1)
        {
            strcat(r, "return self;}\n");
            class_constructor = (char *)realloc(class_constructor, 1);
            //strcpy(class_constructor, "");
            class_constructor[0] = '\0';
        }
        else if (strlen(object_def) && scope == 1)
        {
            file_declarations = (char *)realloc(file_declarations,
                strlen(file_declarations) + 3 + 1);
            strcat(file_declarations, "};\n");
            object_def = (char *)realloc(object_def, 1);
            //strcpy(object_def, "");
            object_def[0] = '\0';
        }
        else if (strlen(class_constructor) && strlen(cur_class_def))
        {
            class_declarations = (char *)realloc(class_declarations,
                strlen(class_declarations) + 2 + 1);
            strcat(class_declarations, "}\n");
            strcat(r, "self.");
            strcat(r, cur_class_def);
            strcat(r, "=__");
            strcat(r, cur_class_def);
            strcat(r, "_");
            strcat(r, class_constructor);
            strcat(r, "__;\n");
            cur_class_def = (char *)realloc(cur_class_def, 1);
            //strcpy(cur_class_def, "");
            cur_class_def[0] = '\0';
        }
        else strcat(r, "}\n");
        scope--;
    }
    else if (strcmp(line[0], "continue") == 0)
    {
        if (isInline) error("continue action must be at start of line", num);
        if (scope < 1) error("continue action has nothing to end, program is already at minimum scope", num);
        if (len > 1) error("continue action received too many arguments", num);
        strcat(r, "continue;\n");
    }
    else if (strcmp(line[0], "ret") == 0)
    {
        if (isInline) error("ret action must be at start of line", num);
        if (scope < 1) error("ret action can not return a value at minimum scope", num);
        if (len < 2) error("ret action missing argument", num);
        else if (len > 2) error("ret action received too many arguments", num);
        else
        {
            strcat(r, "return ");
            arrlstrip(line);
            len--;
            strcat(r, compileline(line, num, len, 1));
            strcat(r, ";\n");
        }
    }
    else if (strcmp(line[1], "++") == 0)
    {
        if (isInline) error("++ action must be at start of line, after variable name", num);
        if (scope < 1) error("++ action can not be at minimum scope", num);
        if (len > 3) error("++ action received too many arguments", num);
        else if (len > 2)
        {
            strcat(r, line[0]);
            strcat(r, "+=");
            strcat(r, line[2]);
            strcat(r, ";\n");
        }
        else
        {
            strcat(r, "++");
            strcat(r, line[0]);
            strcat(r, ";\n");
        }
    }
    else if (strcmp(line[1], "--") == 0)
    {
        if (isInline) error("-- action must be at start of line, after variable name", num);
        if (scope < 1) error("-- action can not be at minimum scope", num);
        if (len > 3) error("-- action received too many arguments", num);
        else if (len > 2)
        {
            strcat(r, line[0]);
            strcat(r, "-=");
            strcat(r, line[2]);
            strcat(r, ";\n");
        }
        else
        {
            strcat(r, "--");
            strcat(r, line[0]);
            strcat(r, ";\n");
        }
    }
    else if (startswith(line[0], "{") && endswith(line[0], "}"))
    {
        char * tempstr[512];
        tokenise(tempstr, __slice_string__(String(line[0]), 1, 1).value, ",");
        if (verbose) println("START");
        for (int p = 0; p < arrsize(tempstr); p++)
        {
            tempstr[p] = cpstrip(tempstr[p]);
            if (verbose) println(tempstr[p]);
        }
        if (verbose) println("END");
        strcat(r, "{");
        strcat(r, compileline(tempstr, num, arrsize(tempstr), 1));
        strcat(r, "}");
        if (!isInline)
            strcat(r, ";\n");
    }
    else if (stringInList(pointers, line[0]))
    {
        strcat(r, line[0]);
        strcat(r, ".value");
        if (!isInline)
            strcat(r, ";\n");
    }
    else if (stringInList(values, line[0]))
    {
        strcat(r, line[0]);
        if (!isInline)
            strcat(r, ";\n");
    }
    else
    {
        for (int p = 0; p < len; p++)
        {
            if (startswith(line[p], "\"") && endswith(line[p], "\""))
            {
                strcat(r, "String(");
                strcat(r, line[p]);
                strcat(r, ")");
                if (!isInline)
                    strcat(r, ";\n");
                else strcat(r, ",");
            }
            else if (stringHasChar(line[0], ','))
            {
                strcat(r, line[p]);
                if (!isInline)
                    strcat(r, ";\n");
            }
            else
            {
                strcat(r, line[p]);
                if (isInline)
                    strcat(r, ",");
                else strcat(r, ";\n");
                while (endswith(r, ",,"))
                    strcpy(r, __slice_string__(String(r), 0, 1).value);
            }
        }
        while (endswith(r, ","))
            strcpy(r, __slice_string__(String(r), 0, 1).value);
    }

    return r_ptr;
}

char * compile_tokens(char ** tokens)
{
    // Keep watch of this in case the files start to get bigger than 6000 characters
    char * compiled = (char *)malloc(6000);
    strcpy(compiled, "");

    for (int i = 0; i < arrsize(tokens); i++)
    {
        if (tokens[i] == NULL)
            continue;

        char * line[512];
        tokenise(line, tokens[i], "|");
        for (int p = 0; line[p] != NULL; p++)
        {
            line[p] = cpstrip(line[p]);
        }

        if (arrsize(line) < 1) continue;
        if (verbose) printf("SIZE %d\n", arrsize(line));

        char * result = compileline(line, i, -1, 0);
        if (strlen(object_def))
        {
            file_declarations = (char *)realloc(file_declarations,
                strlen(file_declarations) + strlen(result) + 1);
            strcat(file_declarations, result);
        }
        else if (strlen(class_constructor) && strlen(cur_class_def))
        {
            class_declarations = (char *)realloc(class_declarations,
                strlen(class_declarations) + strlen(result) + 1);
            strcat(class_declarations, result);
        }
        else strcat(compiled, result);

        if (verbose) printf("-%s-\n", tokens[i]);
        for (int p = 0; line[p] != NULL; p++)
        {
            if (verbose) printf("<%s>\n", line[p]);
        }
    }

    return compiled;
}

char * compile_raw(char * rawtext, int maxtokensize)
{
    if (maxtokensize == -1)
        maxtokensize = 1024;
    char * tokens[maxtokensize];
    ntokenise(tokens, rawtext, "\n");

    return compile_tokens(tokens);
}

char * quokka_compile(char * filename)
{
    char * buffer = readfile(filename);

    if (!buffer)
        return 0;

    return compile_raw(buffer, -1);
}

int main(int argc, char ** argv)
{
    if (argc > 3)
    {
        if (strcmp(argv[3], "-v") == 0)
            verbose = 1;
    }

    if (verbose) println("--START--");

    // Set up a line-by-line interpreter like the Python one possibly?
    if (argc < 2)
    {
        println("Input file path not given, no data to compile.");
        return 1;
    }
    else if (argc < 3)
    {
        println("Output file path not given.");
        return 1;
    }

    // Clear file_declarations
    file_declarations = (char *)malloc(1);
    strcpy(file_declarations, "");

    // Clear object_def
    object_def = (char *)malloc(1);
    strcpy(object_def, "");

    // Clear class_constructor
    class_constructor = (char *)malloc(1);
    strcpy(class_constructor, "");

    // Clear class_declarations
    class_declarations = (char *)malloc(1);
    strcpy(class_declarations, "");

    // Clear cur_class_def
    cur_class_def = (char *)malloc(1);
    strcpy(cur_class_def, "");

    // Full path directing to first file to compile
    char * fullname = getrealpath(argv[1]);
    if (!fullname)
    {
        println("Input file path not found or not accessible.");
        return 1;
    }

    // Full path directing to output file
    char * outputpath = getrealpath(argv[2]);

    // Dirname of first file to compile
    char * dirname = strndup(fullname, strlen(fullname) - strlen(strrchr(fullname, '/')));

    // File name of first file to compile (no dirname)
    char * fname = strrchr(fullname, '/') + 1;

    current_file = fname;

    // Move CWD to dirname
    chdir(dirname);

    char * compiled = quokka_compile(fname);

    if (!compiled)
        return 0;

    FILE * fp = fopen(outputpath, "w");
    fprintf(fp, "%s", C_HEADERS);
    fprintf(fp, "%s", file_declarations);
    fprintf(fp, "%s", class_declarations);
    fprintf(fp, "%s", compiled);
    fclose(fp);

    if (verbose) printf("--END--\n");

    return 0;
}
