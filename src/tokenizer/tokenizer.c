#include "tokenizer.h"

#include "stdio.h"
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_WIDTH 1024


void print_token(Token tok) {
    printf("idx: %d, len: %d, type: %d, val: \'%s\'\n", tok.index, tok.len, tok.type, tok.val);
}

Tokenizer load_tokens(const char* file_name) {
    Tokenizer tz = { 0 };
    
    FILE* file = fopen(file_name, "r");
    
    char line[MAX_LINE_WIDTH];
    int idx = 0;
    int tok_start = 0;
    int tok_idx = 0;
    tz.tokens[tok_idx++].type = TOK_NONE;
    char cur; 
    char last = '\0';
    while(fgets(line, MAX_LINE_WIDTH, file)) {
        idx = 0;
        tok_start = 0;
        //printf("line: %s", line);
        cur = line[idx];
        while(cur != '\0') {
            if (last == '{' || last == '=' || last == ',' || tok_start == 0) {
                while (cur == ' ') {
                    cur = line[++idx];
                    tok_start++;
                }
            }
            if (cur == '=') {
                tz.tokens[tok_idx].index = tok_start;
                tz.tokens[tok_idx].len = idx - tok_start;
                tz.tokens[tok_idx].type = TOK_VARIABLE;
                strncpy(tz.tokens[tok_idx].val, line + tz.tokens[tok_idx].index, tz.tokens[tok_idx].len);
                tok_start = idx + 1;
                tok_idx++;
            }
            else if (cur == '{') {
                tz.tokens[tok_idx].index = idx;
                tz.tokens[tok_idx].len = 1;
                tz.tokens[tok_idx].type = TOK_OPEN_BRACKET;
                strncpy(tz.tokens[tok_idx].val, line + tz.tokens[tok_idx].index, tz.tokens[tok_idx].len);
                tok_start = idx + 1;
                tok_idx++;
            }
            else if (cur == '}') {
                tz.tokens[tok_idx].index = idx;
                tz.tokens[tok_idx].len = 1;
                tz.tokens[tok_idx].type = TOK_CLOSE_BRACKET;
                strncpy(tz.tokens[tok_idx].val, line + tz.tokens[tok_idx].index, tz.tokens[tok_idx].len);
                tok_start = idx + 1;
                tok_idx++;
            }
            else if (cur == '\n' || cur == ';' || cur == ',') {
                if (idx - tok_start > 0) {
                    tz.tokens[tok_idx].index = tok_start;
                    tz.tokens[tok_idx].len = idx - tok_start;
                    tz.tokens[tok_idx].type = TOK_VALUE;
                    strncpy(tz.tokens[tok_idx].val, line + tz.tokens[tok_idx].index, tz.tokens[tok_idx].len);
                    tok_start = idx + 1;
                    tok_idx++;
                }
            }
            idx++;
            last = cur;
            cur = line[idx];
        }
    }

    tz.num_tokens = tok_idx;

    //for (int i = 0; i < tz.num_tokens; i++) {
        //print_token(tz.tokens[i]);
    //}

    fclose(file);
    return tz;
}

Values* get_variable(Tokenizer t, const char* var) {
    int found_var = 0;
    Values* values = malloc(sizeof(Values));
    values->num_values = 0;
    for (int i = 0; i < t.num_tokens; i++) {
        if (t.tokens[i].type == TOK_VARIABLE) {
            if (strcmp(var, t.tokens[i].val) == 0) {
                found_var = 1;
            } else {
                found_var = 0;
            }
        }
        else if (found_var) {
            if (t.tokens[i].type == TOK_VALUE) {
                Values* nvalues = realloc(values, sizeof(Values) + (values->num_values + 1) * sizeof(Token));
                values = nvalues;
                memcpy(&values->tokens[values->num_values], &t.tokens[i], sizeof(Token));
                //printf("n: %d, %s\n", values->num_values, values->tokens[values->num_values].val);
                values->num_values++;
            } 
            else if (t.tokens[i].type == TOK_VARIABLE) {
                found_var = 0;
                break;
            }
        }
    }
    return values;
};


/*int main() {
    Tokenizer tz = load_tokens("./config");
    Values* values = get_variable(tz, "ip");
    free(values);
    values = get_variable(tz, "key_binds");
    free(values);
    return 0;
}*/
