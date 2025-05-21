#include <stdio.h>
#include "base/base_core.h"
#include "base/base_arena.h"
#include "os/os_core.h"

#include "base/base_arena.cpp"
#include "os/os_core.cpp"

// jajaja
int       suma          (       int a              , int        b            );

/*
POOL(type name, handle)
or i could do:
POOL(type, name, handle) and then: POOL(Foo, foosPool, TextureHandle) POOL(IGNORED1, IGNORED2, IGNORED3) IGNORED1 IGNORED2
// TODO probar esto

struct #handle;
struct Pool_#type
{
    struct Pool_#type_Entry
    {
        u32 gen = 1;
        #type data;
    };
    Pool_#type_Entry entries[15];
};

#handle pool_create(#type data)
#type pool_get(#handle)

struct #handle
{
    ...
};
b32 handle_is_valid(#handle)
b32 handle_is_empty(#handle)

*/

struct Buffer
{
    u8 *data;
    size_t size;
};

typedef Buffer String;

String str(const char *str, size_t len)
{
    return {.data = (u8*)str, .size = len};
}

static Arena g_arena;
String str_concat(String a, String b)
{
    String result = {0};
    result.size = a.size + b.size;
    // no arena alternative 
    //char *buf = (char*)malloc(200);
    // arena alternative 
    char buf[200];
    char *at = buf;
    for(u32 i = 0; i < a.size; i++)
    {
        *at++ = a.data[i];
    }
    for(u32 i = 0; i < b.size; i++)
    {
        *at++ = b.data[i];
    }
    // no arena alternative 
    // result.data = (u8*)buf;
    // arena alternative 
    result.data = (u8*)arena_push_copy(&g_arena, result.size, (u8*)buf);
    return result;
}

struct MetaPool
{
    String type; 
    String name; 
    String pool_name; 
    String handle; 
    MetaPool *next;
};

static MetaPool *g_meta_pool;


Buffer read_file_contents(const char *filename)
{
    Buffer result = {0};
    FILE* file = fopen(filename, "rb");
    if (file)  
    {
        fseek(file, 0, SEEK_END);
        size_t filesize = ftell(file);
        fseek(file, 0,SEEK_SET);

        u8 *buffer = (u8*) malloc(filesize + 1);
        fread(buffer, 1, filesize, file);

        buffer[filesize] = '\0';

        result.data = buffer;
        result.size = filesize;
        fclose(file);
    }
    return result;
}


enum TTokenType
{
    TokenType_Unknown,
    TokenType_Identifier,
    TokenType_Number,
    TokenType_String,
    TokenType_Space,
    TokenType_EndOfLine,
    TokenType_LessThan,
    TokenType_Equals,
    TokenType_Comma,
    TokenType_Semicolon,
    TokenType_GreaterThan,
    TokenType_Asterisk,
    TokenType_Blank,
    TokenType_OpenParen,
    TokenType_CloseParen,
    TokenType_OpenBrace,
    TokenType_CloseBrace,
    TokenType_OpenBracket,
    TokenType_CloseBracket,
    TokenType_EOF,
    TokenType_Count
};

static const char *tokentype_to_str[TokenType_Count] = 
{
    "TokenType_Unknown",
    "TokenType_Identifier",
    "TokenType_Number",
    "TokenType_String",
    "TokenType_Space",
    "TokenType_EndOfLine",
    "TokenType_LessThan",
    "TokenType_Equals",
    "TokenType_Comma",
    "TokenType_Semicolon",
    "TokenType_GreaterThan",
    "TokenType_Asterisk",
    "TokenType_Blank",
    "TokenType_OpenParen",
    "TokenType_CloseParen",
    "TokenType_OpenBrace",
    "TokenType_CloseBrace",
    "TokenType_OpenBracket",
    "TokenType_CloseBracket",
    "TokenType_EOF"
};

struct Token
{
    const char* filename;
    // TODO how do i populate this text because if its a single token then its just a single char
    // but if its like an identifier i need to wait until its no longer one and that may be tricky. And yes probably is the trickiest
    // part of all!
    String text;
    u32 line_number;
    u32 column_number;
    TTokenType type;
};

struct Tokenizer
{
    const char* filename;
    String input;
    u32 line_number;
    u32 column_number;
    u8 *at;
};

b32 char_is_alpha(char c)
{
    b32 result = ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
    return result;
}

b32 char_is_number(char c)
{
    b32 result = c >= '0' && c <= '9';
    return result;
}

b32 char_is_whitespace(char c)
{
    b32 result = (c == ' ' || c == '\t');
    return result;
}

b32 char_is_endofline(Tokenizer *tokenizer)
{
    char *curr_char = (char*)tokenizer->at;
    char *next_char = (char*)(curr_char + 1);
    b32 result =  (*curr_char == '\n' && *next_char == '\r') || (*curr_char == '\r' && *next_char == '\n');
    return result;
}


void tokenizer_advance(Tokenizer *tokenizer, u32 advancement)
{
    tokenizer->at+=advancement;
    tokenizer->column_number += advancement;
}

Token get_next_token(Tokenizer *tokenizer)
{
    Token token = {0};
    token.column_number = tokenizer->column_number;
    token.line_number = tokenizer->line_number;

    char ch = *tokenizer->at;
    switch(ch)
    {
        case '(':
        {
            token.type = TokenType_OpenParen; token.text = str("(", 1);tokenizer_advance(tokenizer, 1);
        } break;
        case ')':
        {
            token.type = TokenType_CloseParen;token.text = str(")", 1);tokenizer_advance(tokenizer, 1);
        } break;
        case '[':
        {
            token.type = TokenType_OpenBracket;token.text = str("[", 1);tokenizer_advance(tokenizer, 1);
        } break;
        case ']':
        {
            token.type = TokenType_CloseBracket;token.text = str("]", 1);tokenizer_advance(tokenizer, 1);
        } break;
        case '{':
        {
            token.type = TokenType_OpenBrace;token.text = str("{",1);tokenizer_advance(tokenizer, 1);
        } break;
        case '}':
        {
            token.type = TokenType_CloseBrace;token.text = str("}", 1);tokenizer_advance(tokenizer, 1);
        } break;
        case '<':
        {
            token.type = TokenType_LessThan;token.text = str("<",1);tokenizer_advance(tokenizer, 1);
        } break;
        case '>':
        {
            token.type = TokenType_GreaterThan;token.text = str(">", 1);tokenizer_advance(tokenizer, 1);
        } break;
        case '=':
        {
            token.type = TokenType_Equals;token.text = str("=", 1);tokenizer_advance(tokenizer, 1);
        } break;
        case ',':
        {
            token.type = TokenType_Comma;token.text = str(",", 1);tokenizer_advance(tokenizer, 1);
        } break;
        case ';':
        {
            token.type = TokenType_Semicolon;token.text = str(";", 1);tokenizer_advance(tokenizer, 1);
        } break;
        case '*':
        {
            token.type = TokenType_Asterisk;token.text = str("*",1);tokenizer_advance(tokenizer, 1);
        } break;
        case '#':
        {
            token.type = TokenType_Blank;token.text = str("#", 1);tokenizer_advance(tokenizer, 1);
        } break;
        case '\0': 
        {
            token.type = TokenType_EOF;
        } break;
        default:
        {
            if(char_is_whitespace(ch))
            {
                token.type = TokenType_Space;
                while(char_is_whitespace(*tokenizer->at))
                {
                    tokenizer_advance(tokenizer, 1);
                }
            }
            else if(char_is_endofline(tokenizer))
            {
                token.type = TokenType_EndOfLine;
                while(char_is_endofline(tokenizer))
                {
                    tokenizer_advance(tokenizer, 2);
                    tokenizer->column_number = 1;
                    tokenizer->line_number++;
                }
            }
            else if(char_is_alpha(ch))
            {
                token.type = TokenType_Identifier;
                u8* start = tokenizer->at;
                u8* end = start;
                char ch = *tokenizer->at;
                token.column_number = tokenizer->column_number;
                token.line_number = tokenizer->line_number;
                while(char_is_alpha(ch) || char_is_number(ch) || ch == '-' || ch == '_')
                {
                    tokenizer_advance(tokenizer, 1);
                    ch = *tokenizer->at;
                    end++;
                }
                size_t len = end - start;
                String identifier = {.data = start, .size = len};
                token.text = identifier;
            }
            else
            {
                tokenizer_advance(tokenizer, 1);
            }
        } break;
    }
    return token;
}

b32 required_identifier(String a, String b)
{
    b32 result = true;
    if(a.size != b.size)
    {
        result = false;
    }

    for(u32 i = 0; i < a.size; i++)
    {
        if(a.data[i] != b.data[i])
        {
            result = false;
            break;
        }
    }
    return result;
}

b32 parse_pool(Tokenizer *tokenizer, Token input_token)
{
    MetaPool meta_pool = {0};
    b32 result = false;
    Token token = {0};
    if(required_identifier(str("POOL", 4), input_token.text))
    {
        token = get_next_token(tokenizer);
        while(token.type == TokenType_Space)
        {
            //tokenizer_advance(tokenizer, 1);
            token = get_next_token(tokenizer);
        }
        if(token.type == TokenType_OpenParen)
        {
            token = get_next_token(tokenizer);
            while(token.type == TokenType_Space)
            {
                //tokenizer_advance(tokenizer, 1);
                token = get_next_token(tokenizer);
            }
            if(token.type == TokenType_Identifier) // first arg
            {
                meta_pool.type = token.text;
                token = get_next_token(tokenizer);
                while(token.type == TokenType_Space)
                {
                    //tokenizer_advance(tokenizer, 1);
                    token = get_next_token(tokenizer);
                }
                if(token.type == TokenType_Identifier) // second arg
                {
                    meta_pool.name = token.text;
                    token = get_next_token(tokenizer);
                    while(token.type == TokenType_Space)
                    {
                        //tokenizer_advance(tokenizer, 1);
                        token = get_next_token(tokenizer);
                    }
                    if(token.type == TokenType_Comma)
                    {
                        token = get_next_token(tokenizer);
                        while(token.type == TokenType_Space)
                        {
                            //tokenizer_advance(tokenizer, 1);
                            token = get_next_token(tokenizer);
                        }
                        if(token.type == TokenType_Identifier) // third arg
                        {
                            meta_pool.handle = token.text;
                            token = get_next_token(tokenizer);
                            while(token.type == TokenType_Space)
                            {
                                //tokenizer_advance(tokenizer, 1);
                                token = get_next_token(tokenizer);
                            }
                            if(token.type == TokenType_CloseParen)
                            {
                                MetaPool *new_meta_pool = (MetaPool*)malloc(sizeof(MetaPool));
                                new_meta_pool->type = meta_pool.type;
                                new_meta_pool->name = meta_pool.name;
                                new_meta_pool->pool_name = meta_pool.handle;
                                new_meta_pool->handle = str_concat(meta_pool.handle, str("Handle", 6));
                                new_meta_pool->next = g_meta_pool;
                                g_meta_pool = new_meta_pool;
                                //g_meta_pool->next = new_meta_pool;
                                result = true;
                            }
                        }
                    }
                }
            }
        }
    }
    //POOL(type name, handle_name)
    //POOL(type name,handle_name)
    return result;
}

char peek(Tokenizer *tokenizer)
{
    return *tokenizer->at;
    //token.filename = tokenizer->filename;
    //token.text;
    //token.line_number = tokenizer->line_number;
    //token.column_number = tokenizer->column_number;
    //token.type = tokenizer->;
}

int main()
{
    arena_init(&g_arena, mb(2));
    const char *filename = "src\\samples\\metagen\\program.cpp";
    Buffer file_contents = read_file_contents(filename);

    if (file_contents.data)
    {
        Tokenizer tokenizer = {0};
        tokenizer.at = file_contents.data;
        tokenizer.column_number = 1;
        tokenizer.line_number = 1;
        tokenizer.input = file_contents;
        tokenizer.filename = filename;
        for(;;)
        {
            Token token = get_next_token(&tokenizer);
            u32 line_number = token.line_number;
            u32 column_number = token.column_number;
            TTokenType type = token.type;
            if(type != TokenType_Space && type != TokenType_EndOfLine && type != TokenType_EOF)
            {
                //printf("Token: (%s value: %.*s), line: %d, col: %d\n", tokentype_to_str[type], u32(token.text.size), token.text.data, line_number, column_number);
                if(type == TokenType_Identifier)
                {
                    if(parse_pool(&tokenizer, token))
                    {
                        //printf("Pool parsed successfully! line: %d col: %d\n", tokenizer.line_number, tokenizer.column_number);
                    }
                }
            }
            if(type == TokenType_EOF)
            {
                break;
            }
            #if 0
            if(token_type == TokenType_OpenParen)
            {
                b32 is_valid = parse_pool(tokenizer);
                if (is_valid)
                {

                }
            }
            #endif
        }

        for(MetaPool *ptr = g_meta_pool; ptr; ptr = ptr->next)
        {
            String type = ptr->type;
            String name = ptr->name;
            String pool_name = ptr->pool_name;
            String handle = ptr->handle;
            printf(""
            "struct %.*s\n"
            "{\n"
            "    u32 idx;\n"
            "    u32 gen;\n"
            "};\n\n"
            "b32 handle_is_valid(%.*s handle)\n"
            "{\n"
            "    b32 result = false;\n"
            "    if (handle.gen != 0)\n"
            "    {\n"
            "        result = true;\n"
            "    }\n"
            "    return result;\n"
            "}\n\n"
            "b32 handle_is_empty(%.*s handle)\n"
            "{\n"
            "    b32 result = false;\n"
            "    if (handle.gen == 0)\n"
            "    {\n"
            "        result = true;\n"
            "    }\n"
            "    return result;\n"
            "}\n\n"
            "struct Pool_%.*s\n"
            "{\n"
            "    struct Pool_%.*s_Entry\n"
            "    {\n"
            "        u32 gen = 1;\n"
            "        %.*s data;\n"
            "    };\n"
            "    u32 count = 1;\n"
            "    Pool_%.*s_Entry entries[15];\n"
            "};\n\n"

            "%.*s pool_create(Pool_%.*s *pool, %.*s data)\n"
            "{\n"
            "    %.*s handle = {0};\n"
            "    Assert(pool->count < array_count(pool->entries));\n"
            "    handle.idx = pool->count++;\n"
            "    handle.gen = 1;\n"
            "    Pool_%.*s::Pool_%.*s_Entry entry = {.gen = 1, .data = data};\n"
            "    pool->entries[handle.idx] = entry;\n"
            "    return handle;\n"
            "}\n\n"
            "%.*s *pool_get(Pool_%.*s *pool, %.*s handle)\n"
            "{\n"
            "    %.*s *result = 0;\n"
            "    if(handle_is_valid(handle));\n"
            "    {\n"
            "        result = &pool->entries[handle.idx].data;\n"
            "    }\n"
            "    return result;\n"
            "}\n",

            (u32)handle.size, handle.data,
            (u32)handle.size, handle.data,
            (u32)handle.size, handle.data,

            (u32)pool_name.size, pool_name.data,
            (u32)pool_name.size, pool_name.data,
            (u32)type.size, type.data,
            (u32)pool_name.size, pool_name.data,

            // pool_create
            (u32)handle.size, handle.data, (u32)pool_name.size, pool_name.data,
            (u32)type.size, type.data, (u32)handle.size, handle.data, 
            (u32)pool_name.size, pool_name.data,
            (u32)pool_name.size, pool_name.data,

            // pool_get
            (u32)type.size, type.data, (u32)pool_name.size, pool_name.data, (u32)handle.size, handle.data,
            (u32)type.size, type.data
            );
        }
    }
}