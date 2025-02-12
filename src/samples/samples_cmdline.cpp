#include <stdio.h>
#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "base/base_core.h"
#include "base/base_string.h"
#include "base/base_arena.h"
#include "base/base_os.h"

#include "base/base_string.cpp"
#include "base/base_arena.cpp"
#include "base/base_os.cpp"



void str8list_push(Str8List* list, Str8 string) {
    Str8ListNode* node = 0;
    if(list->count == 0) 
    {
        Str8ListNode* node = (Str8ListNode*) malloc(sizeof(Str8ListNode));
        memset(node, 0, sizeof(Str8ListNode));
        node->str = string;
        list->first = node;
        list->last = node;
    }
    else
    {
        for (node = list->first; node != 0; node = node->next) {};
            node = (Str8ListNode*) malloc(sizeof(Str8ListNode));
            memset(node, 0, sizeof(Str8ListNode));
            node->str = string;
            list->last->next = node;
            list->last = node;

    }
    list->count++;
}

int main(int argc, char** argv) {
    Str8List string_list = {0};
    for(i32 i = 1; i < argc; i++) {
        printf("%s\n", argv[i]);
        str8list_push(&string_list, str8(argv[i]));
    }

    printf("--------------------\n\n");
    for(Str8ListNode* node = string_list.first; node != 0; node = node->next) {
        printf("%s\n", (char*)(node->str.str));
    }
}