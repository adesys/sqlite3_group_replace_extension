/**
 * Group replace function.
 * This aggregate function works just like group_concat, but replaces key with value in a string.
 * This can be used to make parameterized text with multiple parameters (key) which will be
 * replaced with the corresponding value using this aggregation function.
 *
 * Compile the code:
 *     gcc -g -fPIC -shared ./sqlite3_group_replace_extension.c  -o sqlite3_group_replace_extension.so
 *
 * Load the extension:
 *     select load_extension('./sqlite3_group_replace_extension.so');
 *
 * Usage: see README.md
 *
 * Licence: LGPL
 * Author: Anthony Lansbergen
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3ext.h>
SQLITE_EXTENSION_INIT1
typedef struct SCtx SCtx;
struct SCtx {
    int rowCnt;
    char *result;
};
// ------------------------------------------------------------------------------------------------
// recursive function to replace one match
static char* replace_one(char* input, char* startPointSearch, char* key, char* value){
    char* match = strstr(startPointSearch, key);
    if(match == NULL){ // no new match found
        return input;
    }
    else{// found a match, lets replace key with value
        int offset = 0;
        char* newResult  = sqlite3_malloc(strlen(input) - strlen(key) + strlen(value) +1); // calculate new string size
        newResult[0]='\0'; // initialize to empty string
        offset = (int)(match-input);
        strncat(newResult, input, (int)offset);  // append all text from left side of the key
        strcat (newResult, value);                 // append the value
        strcat(newResult, match+strlen(key));      // append remaining text on right side of the key
        sqlite3_free(input);                               // free the original data
        input = newResult; // add the new data to the result
        return replace_one(input, input+offset+strlen(value),key,value);
    }
}
// ------------------------------------------------------------------------------------------------
// function called for each row during the aggreagation
static void group_replace_step(sqlite3_context* ctx, int argc, sqlite3_value**argv) {
    SCtx *p = (SCtx *) sqlite3_aggregate_context(ctx, sizeof(*p));
    if(sqlite3_value_type(argv[0]) == SQLITE_TEXT &&
       sqlite3_value_type(argv[1]) == SQLITE_TEXT &&
       sqlite3_value_type(argv[2]) == SQLITE_TEXT  ){

        char *startString = sqlite3_value_text(argv[0]);
        char *key         = sqlite3_value_text(argv[1]);
        char *value       = sqlite3_value_text(argv[2]);

        // start with the init string
        if (!p->result) {
            p->result = sqlite3_malloc(strlen(startString) + 1);
            strcpy(p->result, startString);
        }
        // lets replace all keys with value
        if(p->result != NULL && key != NULL && value != NULL){
            p->result = replace_one(p->result, p->result, key, value);
        }
    }
    else{
        char* message = "invalid parameter types, all three paramameters should be of type TEXT";
        sqlite3_result_error(ctx, message, strlen(message));
    }
    p->rowCnt++;  // next row
}
// ------------------------------------------------------------------------------------------------
// finalize the aggregation
static void group_replace_final(sqlite3_context* ctx) {
    SCtx *p = (SCtx *) sqlite3_aggregate_context(ctx, sizeof(*p));
    if(p->result){
        sqlite3_result_text(ctx,  p->result, strlen(p->result), sqlite3_free);
    }
}
// register the function to sqlite
int sqlite3_extension_init(
    sqlite3 *db,
    char **pzErrMsg,
    const sqlite3_api_routines *pApi
){
    SQLITE_EXTENSION_INIT2(pApi)
    sqlite3_create_function(db, "group_replace", 3, SQLITE_UTF8, NULL, NULL, group_replace_step, group_replace_final);
    return 0;
}
// ------------------------------------------------------------------------------------------------
