/**
 * Group replace function.
 * This aggregate function works just like group_concat, but replaces key with value in a string.
 * This can be used to make parameterized text with multiple parameters (key) which will be
 * replaced with the corresponding value using this aggregation funtion.
 *
 * Compile the code:
 *     gcc -g -fPIC -shared ./sqlite3_group_replace_extension.c  -o sqlite3_group_replace_extension.so
 *
 * Load the extension:
 *     select load_extension('./sqlite3_concat.o');
 *
 * Example:
 *     CREATE TABLE examples   (
 *       "example_id" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
 *       "text"       TEXT
 *     );
 *     CREATE TABLE key_values (
 *      "key_value_id" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
 *      "key"          VARCHAR,
 *      "value"        VARCHAR,
 *      "example_id"   INTEGER,
 *      CONSTRAINT "fk_example_id"
 *         FOREIGN KEY("example_id")
 *         REFERENCES "examples"("example_id")
 *         ON DELETE CASCADE
 *         ON UPDATE CASCADE
 *     );
 *
 *     INSERT INTO "examples" ("text") VALUES ("Hello, my name is NAME, i'm AGE years old");
 *     INSERT INTO key_values ("example_id", "key", "value") VALUES (1,'NAME', 'Anthony');
 *     INSERT INTO key_values ("example_id", "key", "value") VALUES (1,'AGE', '25');
 *     INSERT INTO "examples" ("text") VALUES ("I have a dog named %dog, ... well thats all folks!");
 *     INSERT INTO key_values ("example_id", "key", "value") VALUES (2,'%dog', 'Nolan');
 *     INSERT INTO "examples" ("text") VALUES ("this example contains two keys, %key and %key");
 *     INSERT INTO key_values ("example_id", "key", "value") VALUES (3,'%key', 'value');
 *
 *     SELECT load_extension('./sqlite3_group_replace_extension.so');
 *
 *     -- execute the query below
 *     SELECT group_replace(e.text, kv.key, kv. value) FROM examples e
 *     JOIN key_values kv on kv.example_id=e.example_id
 *     GROUP BY e.example_id;
 *
 *     -- this should result in:
 *     -- Hello, my name is Anthony, i'm 25 years old
 *     -- I have a dog named Nolan, ... well thats all folks!
 *     -- this example contains two keys, value and value
 *
 * Author: Anthony Lansbergen
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <assert.h>

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
typedef struct SCtx SCtx;
struct SCtx {
    int rowCnt;
//    int charCnt;
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
        char* newResult  = malloc(strlen(input) - strlen(key) + strlen(value) +1); // calculate new string size
        newResult[0]='\0'; // initialize to empty string
        offset = (int)(match-input);
        strncat(newResult, input, (int)offset);  // append all text from left side of the key
        strcat (newResult, value);                 // append the value
        strcat(newResult, match+strlen(key));      // append remaining text on right side of the key
        free(input);                               // free the original data
        input = newResult; // add the new data to the result
        return replace_one(input, input+offset+strlen(value),key,value);
    }
}
// ------------------------------------------------------------------------------------------------
// function called for each row during the aggreagation
static void group_replace_step(sqlite3_context* ctx, int argc, sqlite3_value**argv) {
    SCtx *p = (SCtx *) sqlite3_aggregate_context(ctx, sizeof(*p));

    char *startString = sqlite3_value_text(argv[0]);
    char *key         = sqlite3_value_text(argv[1]);
    char *value       = sqlite3_value_text(argv[2]);
    // start with the init string
    if (!p->result) {
        p->result = malloc(strlen(startString) + 1);
        strcpy(p->result, startString);
    }
    // lets replace all keys with value
    if(p->result != NULL && key != NULL && value != NULL){
        p->result = replace_one(p->result, p->result, key, value);
    }
    p->rowCnt++;  // next row
}
// ------------------------------------------------------------------------------------------------
// finalize the aggregation
static void group_replace_final(sqlite3_context* ctx) {
    SCtx *p = (SCtx *) sqlite3_aggregate_context(ctx, sizeof(*p));
    //  printf("Finally: %s\n", p->result);
    sqlite3_result_text(ctx,  p->result, strlen(p->result), NULL);
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
