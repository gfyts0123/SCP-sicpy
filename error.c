#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "MEM.h"
#include "DBG.h"
#include "sicpy.h"

extern char *yytext;


/* ���뱨����Ϣ */
char* scp_compile_error_message_format[] = {
    "��($(token))���������﷨����",
    "����ȷ���ַ�($(bad_char))",
    "�������ظ�($(name))",
};

/* ����ʱ������Ϣ */
char* scp_runtime_error_message_format[] = {
    "�Ҳ�������($(name))��",
    "�Ҳ�������($(name))��",
    "����Ĳ����������ں������塣",
    "����Ĳ����������ں������塣",
    "�������ʽ��ֵ������boolean�͡�",
    "��������Ĳ�������������ֵ���͡�",
    "˫Ŀ������$(operator)�Ĳ��������Ͳ���ȷ��",
    "$(operator)��������������boolean�͡�",
    "�Ҳ�����Ӧ�ļ�����Ϊfopen()���������ļ���·���ʹ򿪷�ʽ�����߶����ַ������͵ģ���",
    "��Ϊfclose()���������ļ�ָ�롣",
    "��Ϊfread()���������ļ�ָ�롣",
    "��Ϊfwrite()���������ļ�ָ����ַ�����",
    "nullֻ����������� == �� !=(���ܽ���$(operator)����)��",
    "��0��������",
    "ȫ�ֱ���$(name)�����ڡ�",
    "�����ں�����ʹ��global��䡣",
    "�����$(operator)���������ַ������͡�",
};

/* �ַ�����ָ�붨��Ϊ�ִ� */
typedef struct {
    char        *string;
} VString;

/* ͳ���ִ����� */
int my_strlen(char *str)
{
    /* ������������ָ�룬��ı�Ϊ0��strlen������ָ�벻�ܴ��� */
    if (str == NULL) {
        return 0;
    }
    return strlen(str);
}

/* ����ִ� */
static void add_string(VString *message, char *str)
{

    int old_len = my_strlen(message->string);
    int new_size = old_len + strlen(str) + 1;
    message->string = MEM_realloc(message->string, new_size);
    strcpy(&message->string[old_len], str);
}

/* ����ַ� */
static void add_character(VString *message, int ch)
{
    int current_len = my_strlen(message->string);
    message->string = MEM_realloc(message->string, current_len + 2);
    message->string[current_len] = ch;
    message->string[current_len+1] = '\0';
}

/* ����䳤ʵ�Σ�ȫ������args������ */
static void create_message_args(MessageArgument *arg_list, va_list ap)
{
    int index = 0;
    MessageArgumentType type;
    
    /* ���������б�ֱ���趨�Ĳ���ĩβMESSAGE_ARGUMENT_END��va_arg��ȡָ�����͵ĵ�ǰ���� */
    while ((type = va_arg(ap, MessageArgumentType)) != MESSAGE_ARGUMENT_END) {
        arg_list[index].type = type;
        arg_list[index].name = va_arg(ap, char*);
        switch (type) {
        case INT_MESSAGE_ARGUMENT:
            arg_list[index].u.int_val = va_arg(ap, int);
            break;
        case DOUBLE_MESSAGE_ARGUMENT:
            arg_list[index].u.double_val = va_arg(ap, double);
            break;
        case STRING_MESSAGE_ARGUMENT:
            arg_list[index].u.string_val = va_arg(ap, char*);
            break;
        case POINTER_MESSAGE_ARGUMENT:
            arg_list[index].u.pointer_val = va_arg(ap, void*);
            break;
        case CHARACTER_MESSAGE_ARGUMENT:
            arg_list[index].u.character_val = va_arg(ap, int);
            break;
        /* �����ǲ��ó��ֵ������ʹ��assert(0)���� */
        case MESSAGE_ARGUMENT_END:
        default:
            assert(0);
        }
        index++;
        assert(index < MESSAGE_ARGUMENT_MAX);
    }
}

/* ��ʵ���б����ҵ�ָ����������������ǰʵ�� */
static MessageArgument search_argument(MessageArgument *arg_list, char *arg_name)
{
    int i;

    for (i = 0; arg_list[i].type != MESSAGE_ARGUMENT_END; i++) {
        /* ���������ͬ */
        if (!strcmp(arg_list[i].name, arg_name)) {
            return arg_list[i];
        }
    }
    assert(0);
}

/* ��ʽ��������Ϣ */
static void format_message(char *format, VString *message, va_list ap)
{
    int i;
    char        buf[LINE_BUF_SIZE];
    int         arg_name_index;
    char        arg_name[LINE_BUF_SIZE];
    MessageArgument arg_list[MESSAGE_ARGUMENT_MAX];     /* �䳤ʵ����Ϣ�б� */

    /* �䳤ʵ����Ϣ����arg_list�� */
    create_message_args(arg_list, ap);

    /* �����ִ�������$ */
    for (i = 0; format[i] != '\0'; i++) {
        if (format[i] != '$') {
            add_character(message, format[i]);
            continue;
        }
        assert(format[i+1] == '(');
        i += 2;
        /* ��ʼ����$�Ĳ��� */
        for (arg_name_index = 0; format[i] != ')'; arg_name_index++, i++) {
            arg_name[arg_name_index] = format[i];
        }
        arg_name[arg_name_index] = '\0';
        assert(format[i] == ')');

        /* ����ʵ���б��ҵ���ǰʵ�� */
        MessageArgument cur_arg = search_argument(arg_list, arg_name);

        /* ����ʵ�����ͣ�д��message�ִ� */
        switch (cur_arg.type) {
        case INT_MESSAGE_ARGUMENT:
            sprintf(buf, "%d", cur_arg.u.int_val);
            add_string(message, buf);
            break;
        case DOUBLE_MESSAGE_ARGUMENT:
            sprintf(buf, "%f", cur_arg.u.double_val);
            add_string(message, buf);
            break;
        case STRING_MESSAGE_ARGUMENT:
            strcpy(buf, cur_arg.u.string_val);
            add_string(message, cur_arg.u.string_val);
            break;
        case POINTER_MESSAGE_ARGUMENT:
            sprintf(buf, "%p", cur_arg.u.pointer_val);
            add_string(message, buf);
            break;
        case CHARACTER_MESSAGE_ARGUMENT:
            sprintf(buf, "%c", cur_arg.u.character_val);
            add_string(message, buf);
            break;
        case MESSAGE_ARGUMENT_END:
        default:
            assert(0);
        }
    }
}


/* �����ʹ��� */
void scp_compile_error(CompileError id, ...)
{
    /* ��ȡ��ȷ���������б� */
    va_list     ap;
    VString     message;

    va_start(ap, id);   /* ���ݵ�һ��������ַ�ҵ��������б� */
    int line_number = scp_get_interpreter()->current_line_number;
    message.string = NULL;
    format_message(scp_compile_error_message_format[id], &message, ap);
    fprintf(stderr, "%3d:%s\n", line_number, message.string);
    va_end(ap);         /* �ͷű䳤ʵ���б� */

    exit(1);
}

/* ����ʱ���� */
void scp_runtime_error(int line_number, RuntimeError id, ...)
{
    va_list ap;
    VString message;

    va_start(ap, id);
    message.string = NULL;
    format_message(scp_runtime_error_message_format[id], &message, ap);
    fprintf(stderr, "%3d:%s\n", line_number, message.string);
    va_end(ap);

    exit(1);
}

/* �﷨�������� */
int yyerror(char const *str)
{
    char *near_token;

    if (yytext[0] == '\0') {
        near_token = "EOF";
    } else {
        near_token = yytext;
    }
    scp_compile_error(PARSE_ERR, STRING_MESSAGE_ARGUMENT, "token", near_token, MESSAGE_ARGUMENT_END);
    return 0;
}
