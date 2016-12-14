
Group replace function for sqlite3.
====================================
This aggregate function works just like group_concat, but replaces key with value in a string.
This can be used to make parameterized text with multiple parameters (key) which will be
replaced with the corresponding value using this aggregation function.

Usage:
-----------------
group_replace(string, key, value)
example:

    SELECT group_replace('just a simple test', 'test', 'succesful test');

Compile the code:
-----------------
    gcc -g -fPIC -shared ./group_replace.c  -o group_replace.so

Load the extension:
-----------------
    .load ./group_replace

Example:
-----------------
    -- in this example we create a table wich contains parameterized texts
    -- a text can contain as much parameters as you want,
    CREATE TABLE log_texts   (
      "log_text_id" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
      "text"       TEXT
    );

    -- parameters are stored in a key/value table.
    -- for each parameter in the text (table examples) we create a key/value row
    CREATE TABLE log_parameters (
     "log_parameter_id" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
     "key"          VARCHAR,
     "value"        VARCHAR,
     "log_text_id"   INTEGER,
     CONSTRAINT "fk_log_text_id"
        FOREIGN KEY("log_text_id")
        REFERENCES "log_texts"("log_text_id")
        ON DELETE CASCADE
        ON UPDATE CASCADE
    );

    CREATE INDEX "log_text_parameter_idx" ON "log_parameters"("example_id");

    -- lets insert some data
    -- first example has two parameters
    INSERT INTO "log_texts" ("text") VALUES ("Settings are changed by NAME, last update was DAY days ago.");
    INSERT INTO "log_parameters" ("log_text_id", "key", "value") VALUES (1,'NAME', 'Anthony');
    INSERT INTO "log_parameters" ("log_text_id", "key", "value") VALUES (1,'DAY', '25');

    -- you can use any name for the parameter, just use this name as key in the key/value table
    INSERT INTO "log_texts" ("text") VALUES ("User added %dog# %*$$ new samples to the database.");
    INSERT INTO "log_parameters" ("log_text_id", "key", "value") VALUES (2,'%dog# %*$$', '38');

    -- this example demonstrates multiple pararameters with the same name (key)
    -- all these parameters will be replaced by the value
    INSERT INTO "log_texts" ("text") VALUES ("%user changed settings. Last time %user updated settings was %days days ago.");
    INSERT INTO "log_parameters" ("log_text_id", "key", "value") VALUES (3,'%user', 'Penny');
    INSERT INTO "log_parameters" ("log_text_id", "key", "value") VALUES (3,'%days', '2');

    -- load the extension, this way sqlite knows the new `group_replace` function
    SELECT load_extension('./sqlite3_group_replace_extension.so');

    -- execute the query below
    SELECT group_replace(lt.`text`, lp.`key`, lp.`value`) FROM `log_texts` lt
    JOIN log_parameters lp on lp.`log_text_id`=lt.`log_text_id`
    GROUP BY lt.`log_text_id`;

    -- this should result in:
    -- Settings are changed by Anthony, last update was 25 days ago.
    -- User added 38 new samples to the database.
    -- Penny changed settings. Last time Penny updated settings was 2 days ago.


Multi language logdata:
-----------------------
In our project we use this function for translated logdata.
The log texts are stored in multiple languages.
The log parameters (key/value) has multiple columns for values,
- a text column if we want the key to be replaced with a text, just like the example above
- a foreign key column to replace the key with another translated text

We use the sql COALESCE function to get the simple text value or the translated text value.


Performance:
-----------------
I tested above example on 100.000 texts, and the query was executed on my machine in 0.23 seconds.
(tested using sqlite timer)

    .timer on

The same data using group_concat was done in 0.21 seconds.
Of course this is al little bit faster, because replacing texts is more expensive than simple concatination.

    SELECT group_concat(lp.`key`, lp.`value`) FROM `log_texts` lt
    JOIN log_parameters lp on lp.`log_text_id`=lt.`log_text_id`
    GROUP BY lt.`log_text_id`;


Licence / inprovements
-----------------
This software is licensed under LGPL.
(use it "as is", and send improvements by making a pull request.)

